/*
 * ============================================
 * SYNCHRONIZATION-BASED GAME INFRASTRUCTURE
 * ============================================
 * 
 * This module provides the backend infrastructure for the OS simulation
 * using classic synchronization primitives from Operating Systems theory.
 * 
 * COMPONENTS:
 * 1. Producer-Consumer: Event Logging System
 * 2. Readers-Writers: Game State Management
 * 3. Dining Philosophers: Resource Pool Management
 * 
 * These are NOT gameplay elements but INFRASTRUCTURE used by the game.
 */

#ifndef GAME_INFRASTRUCTURE_H
#define GAME_INFRASTRUCTURE_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define MAX_GAME_EVENTS 50
#define EVENT_MESSAGE_SIZE 256
#define MAX_RESOURCES 5

/* ============================================
   PRODUCER-CONSUMER: Event Logging
   
   Events are produced when:
   - Player starts a level
   - Player submits an answer
   - Player completes a level
   - Player scores points
   - Timer updates
   
   Events are consumed by:
   - Game statistics collector
   - Event logger to file
   - Debug display
   ============================================ */

typedef struct {
    char message[EVENT_MESSAGE_SIZE];
    long timestamp;
    int event_id;
    int level;
    int severity;  // 0=info, 1=warning, 2=critical
} GameEvent;

typedef struct {
    GameEvent events[MAX_GAME_EVENTS];
    int head;
    int tail;
    int count;
    int event_counter;
    
    // Synchronization primitives
    pthread_mutex_t event_mutex;
    sem_t empty_slots;      // Initially = MAX_GAME_EVENTS (buffer capacity)
    sem_t full_slots;       // Initially = 0 (available events)
    
    // Consumer thread
    pthread_t consumer_thread;
    int active;             // Flag to stop consumer
} EventQueue;

// Event Queue Functions
EventQueue* event_queue_init(void);
void event_queue_destroy(EventQueue* eq);
void event_produce(EventQueue* eq, const char* msg, int level, int severity);
void* event_consumer_worker(void* arg);

/* ============================================
   READERS-WRITERS: Game State Management
   
   Multiple threads may need to:
   - READ: Score, time remaining, level progress (non-blocking to each other)
   - WRITE: Update score, level completion (exclusive access)
   
   Example:
   - UI thread: READS current score every frame
   - Level thread: WRITES score when answer is correct
   - Stats thread: READS score to calculate statistics
   ============================================ */

typedef struct {
    // Game state
    int current_level;
    int total_score;
    int time_remaining;
    int levels_completed;
    int total_attempts;
    int total_correct;
    
    // Read-Write lock for thread-safe access
    pthread_rwlock_t state_lock;
    
    // Statistics
    int last_update_time;
} SharedGameState;

// Game State Functions
SharedGameState* game_state_create(void);
void shared_game_state_destroy(SharedGameState* gs);

// Read operations (multiple readers allowed)
int game_state_read_score(SharedGameState* gs);
int game_state_read_level(SharedGameState* gs);
int game_state_read_time(SharedGameState* gs);
void game_state_read_all(SharedGameState* gs, int* score, int* level, int* time, int* completed);

// Write operations (exclusive writer access)
void game_state_write_score(SharedGameState* gs, int score);
void game_state_write_level(SharedGameState* gs, int level);
void game_state_write_time(SharedGameState* gs, int time);
void game_state_write_all(SharedGameState* gs, int score, int level, int time);
void game_state_increment_attempts(SharedGameState* gs);
void game_state_increment_correct(SharedGameState* gs);

/* ============================================
   DINING PHILOSOPHERS: Resource Pool Management
   
   Resources (like event log handles, score updates, etc) are managed
   as shared resources that need synchronization.
   
   5 "Philosophers" represent:
   1. Main game thread
   2. Level 0 thread
   3. Level 1 thread
   4. Level 2 thread
   5. Level 3 thread
   
   Resources ("Chopsticks"):
   1. Event queue access
   2. Score update access
   3. Timer update access
   4. Display buffer access
   5. File I/O access
   
   The asymmetric solution prevents deadlock:
   - Philosophers 0-3 acquire LEFT resource first, then RIGHT
   - Philosopher 4 acquires RIGHT resource first, then LEFT
   ============================================ */

typedef struct {
    // 5 resources represented as mutexes
    pthread_mutex_t resources[MAX_RESOURCES];
    
    // Resource names for debugging
    const char* resource_names[MAX_RESOURCES];
    
    // Status tracking
    int resource_in_use[MAX_RESOURCES];
    pthread_mutex_t status_lock;
    
    // Statistics
    int total_deadlocks_prevented;
    int total_resource_acquisitions;
} ResourcePool;

// Resource Pool Functions
ResourcePool* resource_pool_init(void);
void resource_pool_destroy(ResourcePool* rp);

// Acquire/Release (using asymmetric solution for deadlock prevention)
bool resource_pool_acquire(ResourcePool* rp, int philosopher_id);
void resource_pool_release(ResourcePool* rp, int philosopher_id);

// Statistics
int resource_pool_get_deadlocks_prevented(ResourcePool* rp);
int resource_pool_get_acquisitions(ResourcePool* rp);

/* ============================================
   GLOBAL INFRASTRUCTURE MANAGER
   
   Initializes and manages all three synchronization components.
   ============================================ */

typedef struct {
    EventQueue* event_queue;
    SharedGameState* game_state;
    ResourcePool* resource_pool;
    
    pthread_t event_consumer_thread;
    int infrastructure_active;
} GameInfrastructure;

// Initialization/Cleanup
GameInfrastructure* infrastructure_init(void);
void infrastructure_destroy(GameInfrastructure* infra);

// Convenience functions that use infrastructure
void infrastructure_log_event(GameInfrastructure* infra, const char* msg, int level);
void infrastructure_update_score(GameInfrastructure* infra, int new_score);
void infrastructure_update_level(GameInfrastructure* infra, int new_level);

#endif // GAME_INFRASTRUCTURE_H
