#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <pthread.h>
#include <semaphore.h>

#define MAX_PROCESSES 10
#define MAX_RESOURCES 5
#define MAX_DISK_REQUESTS 20

/* ============================================
   BANKER'S ALGORITHM - DEADLOCK AVOIDANCE
   ============================================ */

typedef struct {
    int pid;
    int allocated[MAX_RESOURCES];
    int max_needed[MAX_RESOURCES];
    int claimed[MAX_RESOURCES];
} ProcessControl;

typedef struct {
    ProcessControl processes[MAX_PROCESSES];
    int available[MAX_RESOURCES];
    int total[MAX_RESOURCES];
    int process_count;
    int resource_count;
    pthread_mutex_t banker_lock;
} BankersAlgorithm;

BankersAlgorithm* banker_init(int num_resources);
void banker_destroy(BankersAlgorithm* banker);
int banker_request_resource(BankersAlgorithm* banker, int pid, int resource_id, int amount);
int banker_release_resource(BankersAlgorithm* banker, int pid, int resource_id, int amount);
int banker_is_safe_state(BankersAlgorithm* banker);
int banker_detect_deadlock(BankersAlgorithm* banker, int* deadlocked_pids, int* count);
int banker_kill_process(BankersAlgorithm* banker, int pid);
void banker_display_state(BankersAlgorithm* banker);

/* ============================================
   SCHEDULING ALGORITHMS
   ============================================ */

typedef enum {
    SCHED_FCFS_ALG,      // First Come First Served
    SCHED_RR_ALG,        // Round Robin
    SCHED_PRIORITY_ALG   // Priority Scheduling
} SchedulingType;

typedef struct {
    int pid;
    int burst_time;
    int arrival_time;
    int priority;
    int remaining_time;
    int wait_time;
    int turnaround_time;
} ProcessSchedule;

typedef struct {
    ProcessSchedule processes[MAX_PROCESSES];
    int process_count;
    int time_quantum;
    SchedulingType type;
    float avg_wait_time;
    float avg_turnaround_time;
} Scheduler;

Scheduler* scheduler_init(SchedulingType type, int quantum);
void scheduler_destroy(Scheduler* sched);
void scheduler_add_process(Scheduler* sched, int pid, int burst, int arrival, int priority);
void scheduler_fcfs(Scheduler* sched);
void scheduler_round_robin(Scheduler* sched);
void scheduler_priority(Scheduler* sched);
void scheduler_display_gantt_chart(Scheduler* sched);
float scheduler_get_avg_wait_time(Scheduler* sched);
float scheduler_get_avg_turnaround_time(Scheduler* sched);

/* ============================================
   MEMORY MANAGEMENT - BUDDY SYSTEM & PLACEMENT
   ============================================ */

typedef enum {
    PLACE_FIRST_FIT,
    PLACE_BEST_FIT,
    PLACE_WORST_FIT,
    PLACE_NEXT_FIT
} PlacementStrategy;

typedef struct {
    int block_id;
    int size;
    int is_free;
    int process_id;
} MemoryBlock;

typedef struct {
    MemoryBlock blocks[MAX_PROCESSES * 2];
    int block_count;
    int total_memory;
    PlacementStrategy strategy;
    int next_fit_index;
} MemoryManager;

MemoryManager* memory_init(int total_memory, PlacementStrategy strategy);
void memory_destroy(MemoryManager* mem);
int memory_allocate(MemoryManager* mem, int process_id, int size);
int memory_deallocate(MemoryManager* mem, int process_id);
void memory_buddy_split(MemoryManager* mem, int block_index);
void memory_buddy_merge(MemoryManager* mem);
void memory_display_layout(MemoryManager* mem);
int memory_get_fragmentation(MemoryManager* mem);

/* ============================================
   PAGE REPLACEMENT ALGORITHMS
   ============================================ */

typedef enum {
    PAGE_FIFO,
    PAGE_LRU,
    PAGE_OPTIMAL
} PageReplacementType;

typedef struct {
    int page_number;
    int arrival_time;
    int last_used_time;
} PageFrame;

typedef struct {
    PageFrame frames[MAX_PROCESSES];
    int frame_count;
    int page_faults;
    int page_hits;
    PageReplacementType type;
} PageReplacementSystem;

PageReplacementSystem* page_system_init(int num_frames, PageReplacementType type);
void page_system_destroy(PageReplacementSystem* sys);
int page_system_access(PageReplacementSystem* sys, int page_num);
void page_system_fifo(PageReplacementSystem* sys, int page_num);
void page_system_lru(PageReplacementSystem* sys, int page_num);
void page_system_optimal(PageReplacementSystem* sys, int page_num);
float page_system_hit_ratio(PageReplacementSystem* sys);
void page_system_display_stats(PageReplacementSystem* sys);

/* ============================================
   DISK SCHEDULING ALGORITHMS
   ============================================ */

typedef enum {
    DISK_FCFS,
    DISK_SSTF,      // Shortest Seek Time First
    DISK_SCAN,
    DISK_CSCAN      // Circular SCAN
} DiskSchedulingType;

typedef struct {
    int request_id;
    int track;
    int arrival_time;
    int completion_time;
    int wait_time;
} DiskRequest;

typedef struct {
    DiskRequest requests[MAX_DISK_REQUESTS];
    int request_count;
    int current_head_position;
    int total_seek_time;
    int num_tracks;
    DiskSchedulingType type;
} DiskScheduler;

DiskScheduler* disk_scheduler_init(int num_tracks, DiskSchedulingType type);
void disk_scheduler_destroy(DiskScheduler* disk);
void disk_scheduler_add_request(DiskScheduler* disk, int track);
void disk_scheduler_fcfs(DiskScheduler* disk);
void disk_scheduler_sstf(DiskScheduler* disk);
void disk_scheduler_scan(DiskScheduler* disk);
void disk_scheduler_cscan(DiskScheduler* disk);
void disk_scheduler_display_sequence(DiskScheduler* disk);
int disk_scheduler_get_total_seek_time(DiskScheduler* disk);

#endif // ALGORITHMS_H
