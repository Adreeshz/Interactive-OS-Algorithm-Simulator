#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <pthread.h>
#include <time.h>
#include "user_management.h"
#include "algorithms.h"

#define MAX_SIMULATED_USERS 20
#define SIMULATION_DURATION 300  // 5 minutes per simulation
#define CPU_TIME_SLICE 100       // ms
#define MEMORY_PAGE_SIZE 4096    // bytes
#define DISK_TRACK_COUNT 200

/* ============================================
   USER SIMULATION STATE
   ============================================ */

typedef enum {
    THREAD_STATE_IDLE,
    THREAD_STATE_RUNNING,
    THREAD_STATE_WAITING,
    THREAD_STATE_COMPLETED,
    THREAD_STATE_BLOCKED
} ThreadState;

typedef struct {
    int user_id;
    char username[50];
    pthread_t thread_id;
    ThreadState state;
    int pid;
    int priority;
    int cpu_burst_remaining;
    int memory_required;
    int disk_requests;
    
    // Performance metrics
    int total_cpu_time;
    int wait_time;
    int turnaround_time;
    time_t start_time;
    time_t end_time;
    
    // Algorithm interaction data
    int score;
    int questions_answered;
    int correct_answers;
} SimulatedUser;

/* Forward declare SimulationEnvironment */
typedef struct SimulationEnvironment SimulationEnvironment;

typedef struct {
    SimulatedUser* user;
    SimulationEnvironment* sim;
} ThreadArgs;

typedef struct SimulationEnvironment {
    SimulatedUser users[MAX_SIMULATED_USERS];
    int user_count;
    
    // Shared algorithm instances
    BankersAlgorithm* banker;
    Scheduler* scheduler;
    MemoryManager* memory;
    PageReplacementSystem* paging;
    DiskScheduler* disk_scheduler;
    
    // Synchronization
    pthread_mutex_t users_lock;
    pthread_mutex_t banker_lock;
    pthread_mutex_t scheduler_lock;
    pthread_mutex_t memory_lock;
    pthread_cond_t scheduler_cond;
    
    // Simulation state
    int simulation_active;
    time_t simulation_start;
    int current_time_quantum;
    int total_context_switches;
    int total_page_faults;
    int total_seek_time;
    
    // Statistics
    int completed_users;
    int deadlocked_users;
    int killed_users;
    double avg_wait_time;
    double avg_turnaround_time;
} SimulationEnvironment;

/* ============================================
   FUNCTION DECLARATIONS
   ============================================ */

// Initialization
SimulationEnvironment* simulator_init(int num_users);
void simulator_destroy(SimulationEnvironment* sim);

// User management
int simulator_create_users(SimulationEnvironment* sim, int count);
SimulatedUser* simulator_get_user(SimulationEnvironment* sim, int user_id);

// Thread execution
void* simulator_user_thread(void* arg);
int simulator_start_simulation(SimulationEnvironment* sim);
int simulator_wait_completion(SimulationEnvironment* sim);

// Algorithm demonstrations
void simulator_demonstrate_cpu_scheduling(SimulationEnvironment* sim);
void simulator_demonstrate_banker_algorithm(SimulationEnvironment* sim);
void simulator_demonstrate_memory_management(SimulationEnvironment* sim);
void simulator_demonstrate_page_replacement(SimulationEnvironment* sim);
void simulator_demonstrate_disk_scheduling(SimulationEnvironment* sim);

// Statistics and reporting
void simulator_print_statistics(SimulationEnvironment* sim);
void simulator_print_thread_state(SimulationEnvironment* sim);
void simulator_print_algorithm_results(SimulationEnvironment* sim);

// Utility
int simulator_allocate_resources(SimulationEnvironment* sim, SimulatedUser* user);
int simulator_free_resources(SimulationEnvironment* sim, SimulatedUser* user);
void simulator_update_user_metrics(SimulationEnvironment* sim, SimulatedUser* user);

#endif // SIMULATOR_H
