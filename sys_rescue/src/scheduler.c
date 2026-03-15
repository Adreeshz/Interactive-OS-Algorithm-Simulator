#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

/* ============================================
   QUEUE MANAGEMENT
   ============================================ */

ReadyQueue* queue_init(void) {
    ReadyQueue* rq = (ReadyQueue*)malloc(sizeof(ReadyQueue));
    if (!rq) return NULL;
    rq->count = 0;
    return rq;
}

void queue_destroy(ReadyQueue* rq) {
    if (rq) free(rq);
}

void queue_add_task(ReadyQueue* rq, Task task) {
    if (rq && rq->count < MAX_TASKS) {
        rq->tasks[rq->count] = task;
        rq->count++;
    }
}

void queue_remove_task(ReadyQueue* rq, int index) {
    if (rq && index >= 0 && index < rq->count) {
        for (int i = index; i < rq->count - 1; i++) {
            rq->tasks[i] = rq->tasks[i + 1];
        }
        rq->count--;
    }
}

void queue_display(ReadyQueue* rq) {
    if (!rq || rq->count == 0) {
        printf("Ready Queue is empty.\n");
        return;
    }
    
    printf("\n=== READY QUEUE ===\n");
    printf("%-6s %-20s %-8s %-10s %-10s\n", "ID", "Task Name", "Burst", "Priority", "Arrival");
    printf("----------------------------------------------\n");
    
    for (int i = 0; i < rq->count; i++) {
        printf("%-6d %-20s %-8d %-10d %-10d\n",
               rq->tasks[i].id,
               rq->tasks[i].task_name,
               rq->tasks[i].burst_time,
               rq->tasks[i].priority,
               rq->tasks[i].arrival_time);
    }
    printf("\n");
}

/* ============================================
   COMPARISON FUNCTIONS FOR SORTING
   ============================================ */

int compare_burst_time(const void* a, const void* b) {
    Task* task_a = (Task*)a;
    Task* task_b = (Task*)b;
    return task_a->burst_time - task_b->burst_time;
}

int compare_priority(const void* a, const void* b) {
    Task* task_a = (Task*)a;
    Task* task_b = (Task*)b;
    return task_a->priority - task_b->priority;
}

int compare_arrival_time(const void* a, const void* b) {
    Task* task_a = (Task*)a;
    Task* task_b = (Task*)b;
    return task_a->arrival_time - task_b->arrival_time;
}

/* ============================================
   CALCULATION FUNCTIONS
   ============================================ */

void calculate_times(ReadyQueue* rq) {
    if (!rq || rq->count == 0) return;
    
    int current_time = 0;
    
    for (int i = 0; i < rq->count; i++) {
        if (current_time < rq->tasks[i].arrival_time) {
            current_time = rq->tasks[i].arrival_time;
        }
        
        current_time += rq->tasks[i].burst_time;
        rq->tasks[i].completion_time = current_time;
        rq->tasks[i].turnaround_time = rq->tasks[i].completion_time - rq->tasks[i].arrival_time;
        rq->tasks[i].waiting_time = rq->tasks[i].turnaround_time - rq->tasks[i].burst_time;
    }
}

void calculate_statistics(ReadyQueue* rq, SchedulerStats* stats) {
    if (!rq || !stats || rq->count == 0) return;
    
    float total_waiting = 0;
    float total_turnaround = 0;
    int total_time = 0;
    
    for (int i = 0; i < rq->count; i++) {
        total_waiting += rq->tasks[i].waiting_time;
        total_turnaround += rq->tasks[i].turnaround_time;
        total_time += rq->tasks[i].burst_time;
    }
    
    stats->avg_waiting_time = total_waiting / rq->count;
    stats->avg_turnaround_time = total_turnaround / rq->count;
    stats->total_cpu_time = total_time;
    stats->cpu_utilization = (total_time * 100.0) / (stats->total_cpu_time + 10);  // +10 idle time
}

void display_statistics(SchedulerStats* stats) {
    printf("\n=== SCHEDULER STATISTICS ===\n");
    printf("Average Waiting Time:     %.2f ms\n", stats->avg_waiting_time);
    printf("Average Turnaround Time:  %.2f ms\n", stats->avg_turnaround_time);
    printf("Total CPU Time:           %d ms\n", stats->total_cpu_time);
    printf("CPU Utilization:          %.2f%%\n", stats->cpu_utilization);
    printf("\n");
}

/* ============================================
   SCHEDULING ALGORITHMS
   ============================================ */

void schedule_fcfs(ReadyQueue* rq, SchedulerStats* stats) {
    printf("\n=== FCFS (FIRST COME FIRST SERVE) ===\n");
    printf("Executing tasks in arrival order...\n");
    
    // Sort by arrival time
    qsort(rq->tasks, rq->count, sizeof(Task), compare_arrival_time);
    
    int current_time = 0;
    printf("\n%-8s %-20s %-20s %-15s\n", "Time", "Task", "Execution", "Status");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    for (int i = 0; i < rq->count; i++) {
        Task* task = &rq->tasks[i];
        
        if (current_time < task->arrival_time) {
            printf("%-8d IDLE\n", current_time);
            current_time = task->arrival_time;
        }
        
        printf("%-8d %-20s [%d - %d]          Executing\n",
               current_time,
               task->task_name,
               current_time,
               current_time + task->burst_time);
        
        current_time += task->burst_time;
        task->completion_time = current_time;
        task->turnaround_time = task->completion_time - task->arrival_time;
        task->waiting_time = task->turnaround_time - task->burst_time;
    }
    
    calculate_statistics(rq, stats);
    display_statistics(stats);
}

void schedule_sjf(ReadyQueue* rq, SchedulerStats* stats) {
    printf("\n=== SJF (SHORTEST JOB FIRST) ===\n");
    printf("Sorting by burst time (shortest first)...\n");
    
    // Sort by burst time
    qsort(rq->tasks, rq->count, sizeof(Task), compare_burst_time);
    
    int current_time = 0;
    printf("\n%-8s %-20s %-20s %-15s\n", "Time", "Task", "Execution", "Status");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    for (int i = 0; i < rq->count; i++) {
        Task* task = &rq->tasks[i];
        
        current_time += task->burst_time;
        task->completion_time = current_time;
        task->turnaround_time = task->completion_time - task->arrival_time;
        task->waiting_time = task->turnaround_time - task->burst_time;
        
        printf("%-8d %-20s [%d - %d]          SJF Selected\n",
               current_time - task->burst_time,
               task->task_name,
               current_time - task->burst_time,
               current_time);
    }
    
    calculate_statistics(rq, stats);
    display_statistics(stats);
}

void schedule_priority(ReadyQueue* rq, SchedulerStats* stats) {
    printf("\n=== PRIORITY SCHEDULING ===\n");
    printf("Sorting by priority (lower value = higher priority)...\n");
    
    // Sort by priority
    qsort(rq->tasks, rq->count, sizeof(Task), compare_priority);
    
    int current_time = 0;
    printf("\n%-8s %-20s %-12s %-20s %-15s\n", "Time", "Task", "Priority", "Execution", "Status");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    for (int i = 0; i < rq->count; i++) {
        Task* task = &rq->tasks[i];
        
        current_time += task->burst_time;
        task->completion_time = current_time;
        task->turnaround_time = task->completion_time - task->arrival_time;
        task->waiting_time = task->turnaround_time - task->burst_time;
        
        printf("%-8d %-20s %-12d [%d - %d]          P:%d\n",
               current_time - task->burst_time,
               task->task_name,
               task->priority,
               current_time - task->burst_time,
               current_time,
               task->priority);
    }
    
    calculate_statistics(rq, stats);
    display_statistics(stats);
}

void schedule_round_robin(ReadyQueue* rq, int time_quantum, SchedulerStats* stats) {
    printf("\n=== ROUND ROBIN (TIME QUANTUM: %d ms) ===\n", time_quantum);
    printf("Simulating RR execution with mathematical analysis...\n");
    
    int current_time = 0;
    int total_remaining = 0;
    
    printf("\n%-8s %-20s %-15s %-12s\n", "Quantum", "Task", "Remaining", "Status");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    // Calculate totals
    for (int i = 0; i < rq->count; i++) {
        total_remaining += rq->tasks[i].burst_time;
    }
    
    // Simulate RR (simplified - in reality would need true time-sharing)
    int remaining[MAX_TASKS];
    for (int i = 0; i < rq->count; i++) {
        remaining[i] = rq->tasks[i].burst_time;
    }
    
    int completed = 0;
    while (completed < rq->count) {
        for (int i = 0; i < rq->count; i++) {
            if (remaining[i] > 0) {
                int execute_time = (remaining[i] < time_quantum) ? remaining[i] : time_quantum;
                remaining[i] -= execute_time;
                printf("%-8d %-20s %-15d", current_time, rq->tasks[i].task_name, remaining[i]);
                
                if (remaining[i] == 0) {
                    rq->tasks[i].completion_time = current_time + execute_time;
                    printf(" COMPLETED\n");
                    completed++;
                } else {
                    printf(" CONTEXT_SWITCH\n");
                }
                
                current_time += execute_time;
            }
        }
    }
    
    calculate_times(rq);
    calculate_statistics(rq, stats);
    display_statistics(stats);
}

/* ============================================
   INTERACTIVE GAME FUNCTIONS
   ============================================ */

void scheduling_level_demo(void) {
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║      OS SCHEDULER ENGINE DEMONSTRATION             ║\n");
    printf("║                                                    ║\n");
    printf("║  Level 2: The CPU Bottleneck (Practical 4)        ║\n");
    printf("║  Task Scheduling & CPU Optimization               ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    
    // Create sample tasks
    ReadyQueue* rq = queue_init();
    
    Task tasks[] = {
        {1, 5, 1, 0, 0, 0, 0, "ReadDisk", NULL},
        {2, 2, 2, 0, 0, 0, 0, "ProcessData", NULL},
        {3, 8, 3, 2, 0, 0, 0, "CompileCode", NULL},
        {4, 1, 1, 4, 0, 0, 0, "PrintStatus", NULL}
    };
    
    // Display available algorithms
    printf("\n📊 READY QUEUE ANALYSIS\n");
    printf("Number of pending tasks: 4\n");
    printf("Total CPU burst time: 16 ms\n");
    
    // Add tasks to queue
    for (int i = 0; i < 4; i++) {
        queue_add_task(rq, tasks[i]);
    }
    
    queue_display(rq);
    
    // Test different algorithms
    SchedulerStats stats;
    
    // FCFS
    ReadyQueue* rq_fcfs = queue_init();
    for (int i = 0; i < 4; i++) queue_add_task(rq_fcfs, tasks[i]);
    schedule_fcfs(rq_fcfs, &stats);
    float fcfs_waiting = stats.avg_waiting_time;
    queue_destroy(rq_fcfs);
    
    // SJF
    ReadyQueue* rq_sjf = queue_init();
    for (int i = 0; i < 4; i++) queue_add_task(rq_sjf, tasks[i]);
    schedule_sjf(rq_sjf, &stats);
    float sjf_waiting = stats.avg_waiting_time;
    queue_destroy(rq_sjf);
    
    // PRIORITY
    ReadyQueue* rq_prio = queue_init();
    for (int i = 0; i < 4; i++) queue_add_task(rq_prio, tasks[i]);
    schedule_priority(rq_prio, &stats);
    queue_destroy(rq_prio);
    
    // ROUND ROBIN
    ReadyQueue* rq_rr = queue_init();
    for (int i = 0; i < 4; i++) queue_add_task(rq_rr, tasks[i]);
    schedule_round_robin(rq_rr, 4, &stats);
    queue_destroy(rq_rr);
    
    // Recommendation
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║  🎯 ALGORITHM COMPARISON & RECOMMENDATION          ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("FCFS Average Waiting Time:     %.2f ms ⚠️  SLOW\n", fcfs_waiting);
    printf("SJF Average Waiting Time:      %.2f ms ✅ OPTIMAL\n", sjf_waiting);
    printf("\n✓ SJF is the optimal choice for this workload!\n");
    
    queue_destroy(rq);
}

void interactive_scheduler_puzzle(int difficulty) {
    printf("\n🎮 INTERACTIVE SCHEDULER PUZZLE (Difficulty: %d)\n", difficulty);
    printf("Analyze the task queue and select the best scheduling algorithm!\n");
}
