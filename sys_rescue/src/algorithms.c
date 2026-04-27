#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include "algorithms.h"

/* ============================================
   BANKER'S ALGORITHM IMPLEMENTATION
   ============================================ */

BankersAlgorithm* banker_init(int num_resources) {
    BankersAlgorithm* banker = (BankersAlgorithm*)malloc(sizeof(BankersAlgorithm));
    if (!banker) return NULL;
    
    banker->process_count = 0;
    banker->resource_count = num_resources;
    pthread_mutex_init(&banker->banker_lock, NULL);
    
    memset(banker->processes, 0, sizeof(banker->processes));
    memset(banker->available, 0, sizeof(banker->available));
    memset(banker->total, 0, sizeof(banker->total));
    
    return banker;
}

void banker_destroy(BankersAlgorithm* banker) {
    if (banker) {
        pthread_mutex_destroy(&banker->banker_lock);
        free(banker);
    }
}

int banker_request_resource(BankersAlgorithm* banker, int pid, int resource_id, int amount) {
    if (!banker || resource_id >= banker->resource_count) return -1;
    
    pthread_mutex_lock(&banker->banker_lock);
    
    ProcessControl* proc = NULL;
    for (int i = 0; i < banker->process_count; i++) {
        if (banker->processes[i].pid == pid) {
            proc = &banker->processes[i];
            break;
        }
    }
    
    if (!proc) {
        pthread_mutex_unlock(&banker->banker_lock);
        return -2;  // Process not found
    }
    
    // Check if request exceeds available
    if (amount > banker->available[resource_id]) {
        pthread_mutex_unlock(&banker->banker_lock);
        return -3;  // Not enough resources
    }
    
    // Simulate allocation
    proc->allocated[resource_id] += amount;
    banker->available[resource_id] -= amount;
    
    // Check if system is in safe state
    if (!banker_is_safe_state(banker)) {
        // Rollback
        proc->allocated[resource_id] -= amount;
        banker->available[resource_id] += amount;
        pthread_mutex_unlock(&banker->banker_lock);
        return -4;  // Unsafe state
    }
    
    pthread_mutex_unlock(&banker->banker_lock);
    return 0;  // Success
}

int banker_release_resource(BankersAlgorithm* banker, int pid, int resource_id, int amount) {
    if (!banker || resource_id >= banker->resource_count) return -1;
    
    pthread_mutex_lock(&banker->banker_lock);
    
    ProcessControl* proc = NULL;
    for (int i = 0; i < banker->process_count; i++) {
        if (banker->processes[i].pid == pid) {
            proc = &banker->processes[i];
            break;
        }
    }
    
    if (!proc || proc->allocated[resource_id] < amount) {
        pthread_mutex_unlock(&banker->banker_lock);
        return -2;  // Invalid
    }
    
    proc->allocated[resource_id] -= amount;
    banker->available[resource_id] += amount;
    
    pthread_mutex_unlock(&banker->banker_lock);
    return 0;  // Success
}

int banker_is_safe_state(BankersAlgorithm* banker) {
    if (!banker) return 0;
    
    int available[MAX_RESOURCES];
    memcpy(available, banker->available, sizeof(available));
    
    int finished[MAX_PROCESSES] = {0};
    int safe_sequence[MAX_PROCESSES];
    int count = 0;
    
    for (int i = 0; i < banker->process_count; i++) {
        int found = 0;
        for (int j = 0; j < banker->process_count; j++) {
            if (!finished[j]) {
                int can_allocate = 1;
                for (int k = 0; k < banker->resource_count; k++) {
                    int needed = banker->processes[j].max_needed[k] - 
                                banker->processes[j].allocated[k];
                    if (needed > available[k]) {
                        can_allocate = 0;
                        break;
                    }
                }
                
                if (can_allocate) {
                    finished[j] = 1;
                    for (int k = 0; k < banker->resource_count; k++) {
                        available[k] += banker->processes[j].allocated[k];
                    }
                    safe_sequence[count++] = banker->processes[j].pid;
                    found = 1;
                    break;
                }
            }
        }
        
        if (!found) {
            return 0;  // Deadlock detected
        }
    }
    
    return 1;  // Safe state
}

int banker_detect_deadlock(BankersAlgorithm* banker, int* deadlocked_pids, int* count) {
    if (!banker || !deadlocked_pids || !count) return -1;
    
    pthread_mutex_lock(&banker->banker_lock);
    
    *count = 0;
    if (!banker_is_safe_state(banker)) {
        // Simple deadlock detection: processes that can't make progress
        for (int i = 0; i < banker->process_count; i++) {
            int can_proceed = 0;
            for (int j = 0; j < banker->resource_count; j++) {
                if (banker->processes[i].allocated[j] < banker->processes[i].max_needed[j]) {
                    if (banker->available[j] > 0) {
                        can_proceed = 1;
                        break;
                    }
                }
            }
            
            if (!can_proceed && banker->processes[i].allocated[0] > 0) {
                deadlocked_pids[(*count)++] = banker->processes[i].pid;
            }
        }
    }
    
    pthread_mutex_unlock(&banker->banker_lock);
    return *count > 0 ? 1 : 0;
}

int banker_kill_process(BankersAlgorithm* banker, int pid) {
    if (!banker) return -1;
    
    pthread_mutex_lock(&banker->banker_lock);
    
    int process_idx = -1;
    for (int i = 0; i < banker->process_count; i++) {
        if (banker->processes[i].pid == pid) {
            process_idx = i;
            break;
        }
    }
    
    if (process_idx == -1) {
        pthread_mutex_unlock(&banker->banker_lock);
        return -2;
    }
    
    // Release all resources held by this process
    for (int i = 0; i < banker->resource_count; i++) {
        banker->available[i] += banker->processes[process_idx].allocated[i];
    }
    
    // Remove process
    for (int i = process_idx; i < banker->process_count - 1; i++) {
        memcpy(&banker->processes[i], &banker->processes[i + 1], sizeof(ProcessControl));
    }
    banker->process_count--;
    
    // Kill the actual process with SIGKILL
    kill(pid, SIGKILL);
    
    pthread_mutex_unlock(&banker->banker_lock);
    return 0;
}

void banker_display_state(BankersAlgorithm* banker) {
    if (!banker) return;
    
    pthread_mutex_lock(&banker->banker_lock);
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║          BANKER'S ALGORITHM STATE                 ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Available Resources: ");
    for (int i = 0; i < banker->resource_count; i++) {
        printf("[R%d: %d] ", i, banker->available[i]);
    }
    printf("\n\n");
    
    printf("Process Status:\n");
    printf("PID | ");
    for (int i = 0; i < banker->resource_count; i++) {
        printf("Allocated[R%d] | ", i);
    }
    printf("\n");
    printf("────┼");
    for (int i = 0; i < banker->resource_count; i++) {
        printf("──────────────┼");
    }
    printf("\n");
    
    for (int i = 0; i < banker->process_count; i++) {
        printf("%3d | ", banker->processes[i].pid);
        for (int j = 0; j < banker->resource_count; j++) {
            printf("%13d | ", banker->processes[i].allocated[j]);
        }
        printf("\n");
    }
    
    printf("\nSafe State: %s\n", banker_is_safe_state(banker) ? "YES ✓" : "NO - DEADLOCK!");
    
    pthread_mutex_unlock(&banker->banker_lock);
}

/* ============================================
   SCHEDULING ALGORITHMS IMPLEMENTATION
   ============================================ */

Scheduler* scheduler_init(SchedulingType type, int quantum) {
    Scheduler* sched = (Scheduler*)malloc(sizeof(Scheduler));
    if (!sched) return NULL;
    
    sched->process_count = 0;
    sched->type = type;
    sched->time_quantum = quantum;
    sched->avg_wait_time = 0.0;
    sched->avg_turnaround_time = 0.0;
    
    memset(sched->processes, 0, sizeof(sched->processes));
    
    return sched;
}

void scheduler_destroy(Scheduler* sched) {
    if (sched) {
        free(sched);
    }
}

void scheduler_add_process(Scheduler* sched, int pid, int burst, int arrival, int priority) {
    if (!sched || sched->process_count >= MAX_PROCESSES) return;
    
    ProcessSchedule* proc = &sched->processes[sched->process_count];
    proc->pid = pid;
    proc->burst_time = burst;
    proc->arrival_time = arrival;
    proc->priority = priority;
    proc->remaining_time = burst;
    proc->wait_time = 0;
    proc->turnaround_time = 0;
    
    sched->process_count++;
}

void scheduler_fcfs(Scheduler* sched) {
    if (!sched) return;
    
    // Sort by arrival time
    for (int i = 0; i < sched->process_count - 1; i++) {
        for (int j = 0; j < sched->process_count - i - 1; j++) {
            if (sched->processes[j].arrival_time > sched->processes[j + 1].arrival_time) {
                ProcessSchedule temp = sched->processes[j];
                sched->processes[j] = sched->processes[j + 1];
                sched->processes[j + 1] = temp;
            }
        }
    }
    
    int current_time = 0;
    float total_wait = 0.0;
    float total_turnaround = 0.0;
    
    for (int i = 0; i < sched->process_count; i++) {
        if (current_time < sched->processes[i].arrival_time) {
            current_time = sched->processes[i].arrival_time;
        }
        
        sched->processes[i].wait_time = current_time - sched->processes[i].arrival_time;
        current_time += sched->processes[i].burst_time;
        sched->processes[i].turnaround_time = current_time - sched->processes[i].arrival_time;
        
        total_wait += sched->processes[i].wait_time;
        total_turnaround += sched->processes[i].turnaround_time;
    }
    
    sched->avg_wait_time = total_wait / sched->process_count;
    sched->avg_turnaround_time = total_turnaround / sched->process_count;
}

void scheduler_round_robin(Scheduler* sched) {
    if (!sched || sched->time_quantum <= 0) return;
    
    // Sort by arrival time
    for (int i = 0; i < sched->process_count - 1; i++) {
        for (int j = 0; j < sched->process_count - i - 1; j++) {
            if (sched->processes[j].arrival_time > sched->processes[j + 1].arrival_time) {
                ProcessSchedule temp = sched->processes[j];
                sched->processes[j] = sched->processes[j + 1];
                sched->processes[j + 1] = temp;
            }
        }
    }
    
    int current_time = 0;
    int queue[MAX_PROCESSES];
    int queue_front = 0, queue_rear = 0;
    int completed = 0;
    int* start_time = (int*)malloc(sched->process_count * sizeof(int));
    int* finish_time = (int*)malloc(sched->process_count * sizeof(int));
    
    memset(start_time, -1, sched->process_count * sizeof(int));
    
    // Add initial processes
    for (int i = 0; i < sched->process_count; i++) {
        if (sched->processes[i].arrival_time <= current_time) {
            queue[queue_rear++] = i;
        }
    }
    
    while (completed < sched->process_count) {
        if (queue_front == queue_rear) {
            // Find next arriving process
            int min_arrival = INT_MAX;
            int next_proc = -1;
            for (int i = 0; i < sched->process_count; i++) {
                int proc_running = 0;
                for (int j = 0; j < queue_rear; j++) {
                    if (queue[j] == i) {
                        proc_running = 1;
                        break;
                    }
                }
                if (!proc_running && finish_time[i] == 0 && sched->processes[i].arrival_time < min_arrival) {
                    min_arrival = sched->processes[i].arrival_time;
                    next_proc = i;
                }
            }
            if (next_proc != -1) {
                current_time = sched->processes[next_proc].arrival_time;
                queue[queue_rear++] = next_proc;
            }
        }
        
        if (queue_front < queue_rear) {
            int proc_idx = queue[queue_front++];
            
            if (start_time[proc_idx] == -1) {
                start_time[proc_idx] = current_time;
            }
            
            int execute_time = (sched->processes[proc_idx].remaining_time > sched->time_quantum) ?
                             sched->time_quantum : sched->processes[proc_idx].remaining_time;
            
            sched->processes[proc_idx].remaining_time -= execute_time;
            current_time += execute_time;
            
            if (sched->processes[proc_idx].remaining_time == 0) {
                finish_time[proc_idx] = current_time;
                sched->processes[proc_idx].turnaround_time = finish_time[proc_idx] - sched->processes[proc_idx].arrival_time;
                sched->processes[proc_idx].wait_time = sched->processes[proc_idx].turnaround_time - sched->processes[proc_idx].burst_time;
                completed++;
            } else {
                queue[queue_rear++] = proc_idx;
            }
        }
    }
    
    float total_wait = 0.0, total_turnaround = 0.0;
    for (int i = 0; i < sched->process_count; i++) {
        total_wait += sched->processes[i].wait_time;
        total_turnaround += sched->processes[i].turnaround_time;
    }
    
    sched->avg_wait_time = total_wait / sched->process_count;
    sched->avg_turnaround_time = total_turnaround / sched->process_count;
    
    free(start_time);
    free(finish_time);
}

void scheduler_priority(Scheduler* sched) {
    if (!sched) return;
    
    // Sort by priority (higher priority first)
    for (int i = 0; i < sched->process_count - 1; i++) {
        for (int j = 0; j < sched->process_count - i - 1; j++) {
            if (sched->processes[j].priority < sched->processes[j + 1].priority) {
                ProcessSchedule temp = sched->processes[j];
                sched->processes[j] = sched->processes[j + 1];
                sched->processes[j + 1] = temp;
            }
        }
    }
    
    int current_time = 0;
    float total_wait = 0.0;
    float total_turnaround = 0.0;
    
    for (int i = 0; i < sched->process_count; i++) {
        if (current_time < sched->processes[i].arrival_time) {
            current_time = sched->processes[i].arrival_time;
        }
        
        sched->processes[i].wait_time = current_time - sched->processes[i].arrival_time;
        current_time += sched->processes[i].burst_time;
        sched->processes[i].turnaround_time = current_time - sched->processes[i].arrival_time;
        
        total_wait += sched->processes[i].wait_time;
        total_turnaround += sched->processes[i].turnaround_time;
    }
    
    sched->avg_wait_time = total_wait / sched->process_count;
    sched->avg_turnaround_time = total_turnaround / sched->process_count;
}

void scheduler_display_gantt_chart(Scheduler* sched) {
    if (!sched) return;
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║           SCHEDULING GANTT CHART                 ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Process | Arrival | Burst | Wait | Turnaround\n");
    printf("────────┼─────────┼───────┼──────┼────────────\n");
    
    for (int i = 0; i < sched->process_count; i++) {
        printf("P%-6d |    %3d  |  %3d  | %3d  |    %3d\n",
               sched->processes[i].pid,
               sched->processes[i].arrival_time,
               sched->processes[i].burst_time,
               sched->processes[i].wait_time,
               sched->processes[i].turnaround_time);
    }
    
    printf("\nAverage Wait Time: %.2f\n", sched->avg_wait_time);
    printf("Average Turnaround Time: %.2f\n", sched->avg_turnaround_time);
}

float scheduler_get_avg_wait_time(Scheduler* sched) {
    return sched ? sched->avg_wait_time : 0.0;
}

float scheduler_get_avg_turnaround_time(Scheduler* sched) {
    return sched ? sched->avg_turnaround_time : 0.0;
}

/* ============================================
   MEMORY MANAGEMENT - BUDDY SYSTEM
   ============================================ */

MemoryManager* memory_init(int total_memory, PlacementStrategy strategy) {
    MemoryManager* mem = (MemoryManager*)malloc(sizeof(MemoryManager));
    if (!mem) return NULL;
    
    mem->total_memory = total_memory;
    mem->strategy = strategy;
    mem->next_fit_index = 0;
    mem->block_count = 1;
    
    mem->blocks[0].block_id = 0;
    mem->blocks[0].size = total_memory;
    mem->blocks[0].is_free = 1;
    mem->blocks[0].process_id = -1;
    
    return mem;
}

void memory_destroy(MemoryManager* mem) {
    if (mem) {
        free(mem);
    }
}

int memory_allocate(MemoryManager* mem, int process_id, int size) {
    if (!mem || size <= 0 || mem->block_count >= MAX_PROCESSES * 2) return -1;
    
    int best_block = -1;
    int best_size = INT_MAX;
    int first_free = -1;
    int worst_block = -1;
    int worst_size = -1;
    
    for (int i = 0; i < mem->block_count; i++) {
        if (mem->blocks[i].is_free && mem->blocks[i].size >= size) {
            switch (mem->strategy) {
                case PLACE_FIRST_FIT:
                    mem->blocks[i].is_free = 0;
                    mem->blocks[i].process_id = process_id;
                    
                    if (mem->blocks[i].size > size) {
                        for (int j = mem->block_count; j > i + 1; j--) {
                            mem->blocks[j] = mem->blocks[j - 1];
                        }
                        mem->blocks[i + 1].block_id = mem->block_count;
                        mem->blocks[i + 1].size = mem->blocks[i].size - size;
                        mem->blocks[i + 1].is_free = 1;
                        mem->blocks[i + 1].process_id = -1;
                        mem->blocks[i].size = size;
                        mem->block_count++;
                    }
                    return i;
                    
                case PLACE_BEST_FIT:
                    if (mem->blocks[i].size < best_size) {
                        best_size = mem->blocks[i].size;
                        best_block = i;
                    }
                    break;
                    
                case PLACE_WORST_FIT:
                    if (mem->blocks[i].size > worst_size) {
                        worst_size = mem->blocks[i].size;
                        worst_block = i;
                    }
                    break;
                    
                case PLACE_NEXT_FIT:
                    if (i >= mem->next_fit_index && first_free == -1) {
                        first_free = i;
                    }
                    break;
            }
        }
    }
    
    if (mem->strategy == PLACE_BEST_FIT && best_block != -1) {
        mem->blocks[best_block].is_free = 0;
        mem->blocks[best_block].process_id = process_id;
        
        if (mem->blocks[best_block].size > size) {
            for (int j = mem->block_count; j > best_block + 1; j--) {
                mem->blocks[j] = mem->blocks[j - 1];
            }
            mem->blocks[best_block + 1].block_id = mem->block_count;
            mem->blocks[best_block + 1].size = mem->blocks[best_block].size - size;
            mem->blocks[best_block + 1].is_free = 1;
            mem->blocks[best_block + 1].process_id = -1;
            mem->blocks[best_block].size = size;
            mem->block_count++;
        }
        return best_block;
    }
    
    if (mem->strategy == PLACE_WORST_FIT && worst_block != -1) {
        mem->blocks[worst_block].is_free = 0;
        mem->blocks[worst_block].process_id = process_id;
        
        if (mem->blocks[worst_block].size > size) {
            for (int j = mem->block_count; j > worst_block + 1; j--) {
                mem->blocks[j] = mem->blocks[j - 1];
            }
            mem->blocks[worst_block + 1].block_id = mem->block_count;
            mem->blocks[worst_block + 1].size = mem->blocks[worst_block].size - size;
            mem->blocks[worst_block + 1].is_free = 1;
            mem->blocks[worst_block + 1].process_id = -1;
            mem->blocks[worst_block].size = size;
            mem->block_count++;
        }
        return worst_block;
    }
    
    if (mem->strategy == PLACE_NEXT_FIT && first_free != -1) {
        mem->blocks[first_free].is_free = 0;
        mem->blocks[first_free].process_id = process_id;
        mem->next_fit_index = first_free + 1;
        
        if (mem->blocks[first_free].size > size) {
            for (int j = mem->block_count; j > first_free + 1; j--) {
                mem->blocks[j] = mem->blocks[j - 1];
            }
            mem->blocks[first_free + 1].block_id = mem->block_count;
            mem->blocks[first_free + 1].size = mem->blocks[first_free].size - size;
            mem->blocks[first_free + 1].is_free = 1;
            mem->blocks[first_free + 1].process_id = -1;
            mem->blocks[first_free].size = size;
            mem->block_count++;
        }
        return first_free;
    }
    
    return -2;  // No suitable block found
}

int memory_deallocate(MemoryManager* mem, int process_id) {
    if (!mem) return -1;
    
    for (int i = 0; i < mem->block_count; i++) {
        if (!mem->blocks[i].is_free && mem->blocks[i].process_id == process_id) {
            mem->blocks[i].is_free = 1;
            mem->blocks[i].process_id = -1;
            
            // Merge with adjacent free blocks
            memory_buddy_merge(mem);
            return 0;
        }
    }
    
    return -2;  // Process not found
}

void memory_buddy_split(MemoryManager* mem, int block_index) {
    if (!mem || block_index >= mem->block_count || mem->block_count >= MAX_PROCESSES * 2) return;
    
    if (!mem->blocks[block_index].is_free || mem->blocks[block_index].size < 2) return;
    
    for (int i = mem->block_count; i > block_index + 1; i--) {
        mem->blocks[i] = mem->blocks[i - 1];
    }
    
    mem->blocks[block_index + 1].block_id = mem->block_count;
    mem->blocks[block_index + 1].size = mem->blocks[block_index].size / 2;
    mem->blocks[block_index + 1].is_free = 1;
    mem->blocks[block_index + 1].process_id = -1;
    
    mem->blocks[block_index].size = mem->blocks[block_index].size / 2;
    mem->block_count++;
}

void memory_buddy_merge(MemoryManager* mem) {
    if (!mem) return;
    
    for (int i = 0; i < mem->block_count - 1; i++) {
        if (mem->blocks[i].is_free && mem->blocks[i + 1].is_free &&
            mem->blocks[i].size == mem->blocks[i + 1].size) {
            mem->blocks[i].size *= 2;
            
            for (int j = i + 1; j < mem->block_count - 1; j++) {
                mem->blocks[j] = mem->blocks[j + 1];
            }
            mem->block_count--;
            i--;  // Check again in case we can merge further
        }
    }
}

void memory_display_layout(MemoryManager* mem) {
    if (!mem) return;
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║         MEMORY LAYOUT (%d bytes total)            ║\n", mem->total_memory);
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Block | Start | Size | Status   | PID\n");
    printf("──────┼───────┼──────┼──────────┼─────\n");
    
    int current_start = 0;
    for (int i = 0; i < mem->block_count; i++) {
        printf("%4d  | %5d | %4d | %-8s | %3d\n",
               mem->blocks[i].block_id,
               current_start,
               mem->blocks[i].size,
               mem->blocks[i].is_free ? "FREE" : "USED",
               mem->blocks[i].process_id);
        current_start += mem->blocks[i].size;
    }
    
    printf("\nFragmentation: %.1f%%\n", (float)memory_get_fragmentation(mem));
}

int memory_get_fragmentation(MemoryManager* mem) {
    if (!mem) return 0;
    
    int free_blocks = 0;
    int total_free = 0;
    
    for (int i = 0; i < mem->block_count; i++) {
        if (mem->blocks[i].is_free) {
            free_blocks++;
            total_free += mem->blocks[i].size;
        }
    }
    
    if (free_blocks <= 1) return 0;
    
    return (int)(((float)(free_blocks - 1) / free_blocks) * 100);
}

/* ============================================
   PAGE REPLACEMENT ALGORITHMS
   ============================================ */

PageReplacementSystem* page_system_init(int num_frames, PageReplacementType type) {
    PageReplacementSystem* sys = (PageReplacementSystem*)malloc(sizeof(PageReplacementSystem));
    if (!sys) return NULL;
    
    sys->frame_count = num_frames;
    sys->type = type;
    sys->page_faults = 0;
    sys->page_hits = 0;
    
    memset(sys->frames, 0, sizeof(sys->frames));
    
    for (int i = 0; i < num_frames; i++) {
        sys->frames[i].page_number = -1;
        sys->frames[i].arrival_time = 0;
        sys->frames[i].last_used_time = 0;
    }
    
    return sys;
}

void page_system_destroy(PageReplacementSystem* sys) {
    if (sys) {
        free(sys);
    }
}

int page_system_access(PageReplacementSystem* sys, int page_num) {
    if (!sys) return -1;
    
    // Check if page is already in memory
    for (int i = 0; i < sys->frame_count; i++) {
        if (sys->frames[i].page_number == page_num) {
            sys->frames[i].last_used_time = time(NULL);
            sys->page_hits++;
            return 0;  // Page hit
        }
    }
    
    // Page fault - need to replace
    sys->page_faults++;
    
    // Check for empty frame
    for (int i = 0; i < sys->frame_count; i++) {
        if (sys->frames[i].page_number == -1) {
            sys->frames[i].page_number = page_num;
            sys->frames[i].arrival_time = time(NULL);
            sys->frames[i].last_used_time = time(NULL);
            return 1;  // Page fault, empty frame found
        }
    }
    
    // All frames full - use replacement algorithm
    switch (sys->type) {
        case PAGE_FIFO:
            page_system_fifo(sys, page_num);
            break;
        case PAGE_LRU:
            page_system_lru(sys, page_num);
            break;
        case PAGE_OPTIMAL:
            page_system_optimal(sys, page_num);
            break;
    }
    
    return 1;  // Page fault
}

void page_system_fifo(PageReplacementSystem* sys, int page_num) {
    if (!sys) return;
    
    int oldest_idx = 0;
    for (int i = 1; i < sys->frame_count; i++) {
        if (sys->frames[i].arrival_time < sys->frames[oldest_idx].arrival_time) {
            oldest_idx = i;
        }
    }
    
    sys->frames[oldest_idx].page_number = page_num;
    sys->frames[oldest_idx].arrival_time = time(NULL);
    sys->frames[oldest_idx].last_used_time = time(NULL);
}

void page_system_lru(PageReplacementSystem* sys, int page_num) {
    if (!sys) return;
    
    int lru_idx = 0;
    for (int i = 1; i < sys->frame_count; i++) {
        if (sys->frames[i].last_used_time < sys->frames[lru_idx].last_used_time) {
            lru_idx = i;
        }
    }
    
    sys->frames[lru_idx].page_number = page_num;
    sys->frames[lru_idx].arrival_time = time(NULL);
    sys->frames[lru_idx].last_used_time = time(NULL);
}

void page_system_optimal(PageReplacementSystem* sys, int page_num) {
    if (!sys) return;
    
    // Simplified optimal: replace page with lowest page number (won't be used soon)
    int min_idx = 0;
    for (int i = 1; i < sys->frame_count; i++) {
        if (sys->frames[i].page_number < sys->frames[min_idx].page_number) {
            min_idx = i;
        }
    }
    
    sys->frames[min_idx].page_number = page_num;
    sys->frames[min_idx].arrival_time = time(NULL);
    sys->frames[min_idx].last_used_time = time(NULL);
}

float page_system_hit_ratio(PageReplacementSystem* sys) {
    if (!sys) return 0.0;
    
    int total = sys->page_faults + sys->page_hits;
    if (total == 0) return 0.0;
    
    return (float)sys->page_hits / total;
}

void page_system_display_stats(PageReplacementSystem* sys) {
    if (!sys) return;
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║        PAGE REPLACEMENT STATISTICS                ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Algorithm: ");
    switch (sys->type) {
        case PAGE_FIFO: printf("FIFO\n"); break;
        case PAGE_LRU: printf("LRU\n"); break;
        case PAGE_OPTIMAL: printf("Optimal\n"); break;
    }
    
    printf("Total Frames: %d\n", sys->frame_count);
    printf("Page Hits: %d\n", sys->page_hits);
    printf("Page Faults: %d\n", sys->page_faults);
    printf("Hit Ratio: %.2f%%\n", page_system_hit_ratio(sys) * 100);
    
    printf("\nCurrent Frames:\n");
    for (int i = 0; i < sys->frame_count; i++) {
        if (sys->frames[i].page_number != -1) {
            printf("  Frame %d: Page %d\n", i, sys->frames[i].page_number);
        } else {
            printf("  Frame %d: Empty\n", i);
        }
    }
}

/* ============================================
   DISK SCHEDULING ALGORITHMS
   ============================================ */

DiskScheduler* disk_scheduler_init(int num_tracks, DiskSchedulingType type) {
    DiskScheduler* disk = (DiskScheduler*)malloc(sizeof(DiskScheduler));
    if (!disk) return NULL;
    
    disk->num_tracks = num_tracks;
    disk->type = type;
    disk->current_head_position = 0;
    disk->total_seek_time = 0;
    disk->request_count = 0;
    
    memset(disk->requests, 0, sizeof(disk->requests));
    
    return disk;
}

void disk_scheduler_destroy(DiskScheduler* disk) {
    if (disk) {
        free(disk);
    }
}

void disk_scheduler_add_request(DiskScheduler* disk, int track) {
    if (!disk || disk->request_count >= MAX_DISK_REQUESTS) return;
    if (track < 0 || track >= disk->num_tracks) return;
    
    disk->requests[disk->request_count].request_id = disk->request_count;
    disk->requests[disk->request_count].track = track;
    disk->requests[disk->request_count].arrival_time = time(NULL);
    disk->requests[disk->request_count].completion_time = 0;
    disk->requests[disk->request_count].wait_time = 0;
    
    disk->request_count++;
}

void disk_scheduler_fcfs(DiskScheduler* disk) {
    if (!disk || disk->request_count == 0) return;
    
    disk->total_seek_time = 0;
    disk->current_head_position = 0;
    
    for (int i = 0; i < disk->request_count; i++) {
        int seek_distance = abs(disk->requests[i].track - disk->current_head_position);
        disk->total_seek_time += seek_distance;
        disk->current_head_position = disk->requests[i].track;
        disk->requests[i].completion_time = disk->total_seek_time;
        disk->requests[i].wait_time = disk->total_seek_time;
    }
}

void disk_scheduler_sstf(DiskScheduler* disk) {
    if (!disk || disk->request_count == 0) return;
    
    disk->total_seek_time = 0;
    disk->current_head_position = 0;
    int completed[MAX_DISK_REQUESTS] = {0};
    
    for (int i = 0; i < disk->request_count; i++) {
        int nearest = -1;
        int min_distance = INT_MAX;
        
        for (int j = 0; j < disk->request_count; j++) {
            if (!completed[j]) {
                int distance = abs(disk->requests[j].track - disk->current_head_position);
                if (distance < min_distance) {
                    min_distance = distance;
                    nearest = j;
                }
            }
        }
        
        if (nearest != -1) {
            completed[nearest] = 1;
            disk->total_seek_time += min_distance;
            disk->current_head_position = disk->requests[nearest].track;
            disk->requests[nearest].completion_time = disk->total_seek_time;
            disk->requests[nearest].wait_time = disk->total_seek_time;
        }
    }
}

void disk_scheduler_scan(DiskScheduler* disk) {
    if (!disk || disk->request_count == 0) return;
    
    disk->total_seek_time = 0;
    disk->current_head_position = 0;
    int completed[MAX_DISK_REQUESTS] = {0};
    int direction = 1;  // 1 for increasing, -1 for decreasing
    
    for (int i = 0; i < disk->request_count; i++) {
        int nearest = -1;
        int min_distance = INT_MAX;
        
        for (int j = 0; j < disk->request_count; j++) {
            if (!completed[j]) {
                int distance = disk->requests[j].track - disk->current_head_position;
                if ((direction > 0 && distance >= 0) || (direction < 0 && distance <= 0)) {
                    if (abs(distance) < min_distance) {
                        min_distance = abs(distance);
                        nearest = j;
                    }
                }
            }
        }
        
        if (nearest == -1) {
            direction = -direction;  // Change direction
            i--;
            continue;
        }
        
        completed[nearest] = 1;
        disk->total_seek_time += min_distance;
        disk->current_head_position = disk->requests[nearest].track;
        disk->requests[nearest].completion_time = disk->total_seek_time;
        disk->requests[nearest].wait_time = disk->total_seek_time;
    }
}

void disk_scheduler_cscan(DiskScheduler* disk) {
    if (!disk || disk->request_count == 0) return;
    
    disk->total_seek_time = 0;
    disk->current_head_position = 0;
    int completed[MAX_DISK_REQUESTS] = {0};
    
    for (int i = 0; i < disk->request_count; i++) {
        int nearest = -1;
        int min_distance = INT_MAX;
        
        // First try increasing direction
        for (int j = 0; j < disk->request_count; j++) {
            if (!completed[j]) {
                int distance = disk->requests[j].track - disk->current_head_position;
                if (distance >= 0 && distance < min_distance) {
                    min_distance = distance;
                    nearest = j;
                }
            }
        }
        
        if (nearest == -1) {
            // Wrap around to lowest track
            min_distance = INT_MAX;
            for (int j = 0; j < disk->request_count; j++) {
                if (!completed[j]) {
                    if (disk->requests[j].track < min_distance) {
                        min_distance = disk->requests[j].track;
                        nearest = j;
                    }
                }
            }
        }
        
        if (nearest != -1) {
            completed[nearest] = 1;
            disk->total_seek_time += abs(disk->requests[nearest].track - disk->current_head_position);
            disk->current_head_position = disk->requests[nearest].track;
            disk->requests[nearest].completion_time = disk->total_seek_time;
            disk->requests[nearest].wait_time = disk->total_seek_time;
        }
    }
}

void disk_scheduler_display_sequence(DiskScheduler* disk) {
    if (!disk) return;
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║         DISK SCHEDULING SEQUENCE                  ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Request | Track | Seek Time | Completion Time\n");
    printf("────────┼───────┼───────────┼─────────────────\n");
    
    for (int i = 0; i < disk->request_count; i++) {
        printf("%7d | %5d | %9d | %15d\n",
               disk->requests[i].request_id,
               disk->requests[i].track,
               disk->requests[i].wait_time,
               disk->requests[i].completion_time);
    }
    
    printf("\nTotal Seek Time: %d\n", disk->total_seek_time);
}

int disk_scheduler_get_total_seek_time(DiskScheduler* disk) {
    return disk ? disk->total_seek_time : 0;
}
