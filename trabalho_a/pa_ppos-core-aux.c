#include "ppos.h"
#include "ppos-core-globals.h"


// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções

// Includes necessários para utilizar o times
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

// Estrutura que define um tratador de sinal
struct sigaction action;

// Estrutura de inicialização to timer
struct itimerval timer;

// Tempo entre interrupções do timer
#define TIMER_MS_TICK 1000

// Definição do fator de envelhecimento da tarefa
#define AGEING_ALPHA 1

// Definição do tempo que a task pode permanecer no processador
#define QUANTUM_VALUE  20

// Definição da escala de prioridades
#define MAX_PRIORITY 20
#define MIN_PRIORITY -20

// Helpers
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define IS_TIME_PREEMPTION_ACTIVE 1


task_t * scheduler () {
    // Implementação do algoritmo de envelhecimento com AGEING_ALPHA 

    task_t* runner = (task_t*)readyQueue->next;
    task_t* priority_task = (task_t*)readyQueue;

    while (runner != (task_t*)readyQueue) {
        if (runner->dynamic_priority < priority_task->dynamic_priority) {
            priority_task->dynamic_priority = MAX(MIN_PRIORITY, MIN(priority_task->dynamic_priority - AGEING_ALPHA, MAX_PRIORITY));
            priority_task = runner;
        } else {
            runner->dynamic_priority = MAX(MIN_PRIORITY, MIN(runner->dynamic_priority - AGEING_ALPHA, MAX_PRIORITY));
        }
        runner = runner->next;
    }

    // Correção das prioridades da tarefa
    task_setprio(priority_task, priority_task->static_priority);

    return priority_task;
}

void task_setprio (task_t *task, int prio) {
    // Ajusta o valor da prioridade dentro do range permitido
    int ranged_priority = MAX(MIN_PRIORITY, MIN(prio, MAX_PRIORITY));

    // Se a task não existir, atualiza a prioridade da task em execução
    if (task == NULL) {
        taskExec->static_priority = taskExec->dynamic_priority = ranged_priority;
        return;
    }

    task->static_priority = task->dynamic_priority = ranged_priority;
}

int task_getprio (task_t *task) {
    // Se a task não existir returna a prioridade estática da task em execução
    if (task == NULL) {
        return taskExec->static_priority;
    }

    return task->static_priority;
}

// Função que lida com cada interrupção
void timer_interrupt_handler(int signum) {
    systemTime++;

    // Ignora a task main
    if (taskExec == taskMain) {
        return;
    }

    // Ignora a task do dispatcher
    if (taskExec == taskDisp) {
        return;
    }

    // Atualiza o contador de tempo que a tarefa está no processador (Quantum)
    taskExec->ramining_quanta--;

    // Preempta a tarefa caso ela exceda o tempo máximo no processdor
    if (taskExec->ramining_quanta <= 0) {
        task_yield();
    }
}

// ****************************************************************************



void before_ppos_init () {
    // Registro do timer que gera interrupção a cada TIMER_MS_TICK 

    if (IS_TIME_PREEMPTION_ACTIVE) {
        // Registra a handler para o sinal de timer SIGALRM
        action.sa_handler = timer_interrupt_handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;

        if (sigaction(SIGALRM, &action, 0) < 0) {
            perror("Erro em sigaction: ");
            exit (1);
        }

        // Primeiro disparo do timer, em micro-segundos
        timer.it_value.tv_usec = TIMER_MS_TICK;

        // Primeiro disparo do timer, em segundos
        timer.it_value.tv_sec  = 0;

        // Disparos subsequentes do timer, em micro-segundos
        timer.it_interval.tv_usec = TIMER_MS_TICK;

        // disparos subsequentes do timer, em segundos
        timer.it_interval.tv_sec  = 0;


        // Arma o temporizador ITIMER_REAL
        if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
            perror ("Erro em setitimer: ");
            exit (1);
        }
    }

#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void after_task_create (task_t *task ) {
    // Definição do quantum da tarefa
    task->ramining_quanta = QUANTUM_VALUE;

    // Definição de algumas propriedades de métrica
    task->created_at = systemTime;
    task->activations = 0;
    
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}

void before_task_switch ( task_t *task ) {
    // Atualização das propriedades de métrica
    taskExec->processor_time += systemTime - task->last_processor_entry_time;
    task->last_processor_entry_time = systemTime;
    task->activations++;

#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

void after_task_resume (task_t *task) {
    // Atualização das propriedades de métrica
    task->last_processor_entry_time = systemTime;
    task->activations++;

#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void after_task_exit () {
    // Contabilização das métricas da tarefa
    taskExec->execution_time = systemTime - taskExec->created_at;
    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", taskExec->id, taskExec->execution_time, taskExec->processor_time, taskExec->activations);

#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}


void after_ppos_init () {
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}

void before_task_suspend ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}
