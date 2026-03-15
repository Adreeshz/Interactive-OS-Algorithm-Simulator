#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <time.h>

#define MAX_TASKS 100

/* ============================================
   TASK DEFINITION
   ============================================ */

typedef struct {
    int id;
    int burst_time;
    int priority;
    int arrival_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    char task_name[50];
    void (*function_pointer)(void);
} Task;

/* ============================================
   READY QUEUE
   ============================================ */

typedef struct {
    Task tasks[MAX_TASKS];
    int count;
} ReadyQueue;

/* ============================================
   SCHEDULER ALGORITHMS
   ============================================ */

typedef enum {
    FCFS,           // First Come First Serve
    SJF,            // Shortest Job First (Non-preemptive)
    PRIORITY,       // Priority Based
    ROUND_ROBIN     // Round Robin with Time Quantum
} SchedulingAlgorithm;

/* ============================================
   SCHEDULER STATISTICS
   ============================================ */

typedef struct {
    float avg_waiting_time;
    float avg_turnaround_time;
    int total_cpu_time;
    float cpu_utilization;
} SchedulerStats;

/* Function Prototypes */

// Queue Management
ReadyQueue* queue_init(void);
void queue_destroy(ReadyQueue* rq);
void queue_add_task(ReadyQueue* rq, Task task);
void queue_remove_task(ReadyQueue* rq, int index);
void queue_display(ReadyQueue* rq);

// Scheduling Algorithms
void schedule_fcfs(ReadyQueue* rq, SchedulerStats* stats);
void schedule_sjf(ReadyQueue* rq, SchedulerStats* stats);
void schedule_priority(ReadyQueue* rq, SchedulerStats* stats);
void schedule_round_robin(ReadyQueue* rq, int time_quantum, SchedulerStats* stats);

// Utility Functions
void calculate_times(ReadyQueue* rq);
void calculate_statistics(ReadyQueue* rq, SchedulerStats* stats);
int compare_burst_time(const void* a, const void* b);
int compare_priority(const void* a, const void* b);
int compare_arrival_time(const void* a, const void* b);
void display_statistics(SchedulerStats* stats);

// Game Interaction
void scheduling_level_demo(void);
void interactive_scheduler_puzzle(int difficulty);

#endif // SCHEDULER_H
