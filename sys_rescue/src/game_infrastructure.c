/*
 * game_infrastructure.c
 * 
 * Implementation of the synchronization-based game infrastructure.
 * Uses Producer-Consumer, Readers-Writers, and Dining Philosophers
 * patterns for system management.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "game_infrastructure.h"

/* ============================================
   PRODUCER-CONSUMER: Event Logging
   ============================================ */

EventQueue* event_queue_init(void) {
    EventQueue* eq = (EventQueue*)malloc(sizeof(EventQueue));
    if (!eq) return NULL;
    
    eq->head = 0;
    eq->tail = 0;
    eq->count = 0;
    eq->event_counter = 0;
    eq->active = 1;
    
    // Initialize synchronization primitives
    pthread_mutex_init(&eq->event_mutex, NULL);
    sem_init(&eq->empty_slots, 0, MAX_GAME_EVENTS);  // Buffer capacity
    sem_init(&eq->full_slots, 0, 0);                  // No events initially
    
    return eq;
}

void event_queue_destroy(EventQueue* eq) {
    if (!eq) return;
    
    eq->active = 0;
    
    // Wait for consumer to finish
    pthread_join(eq->consumer_thread, NULL);
    
    pthread_mutex_destroy(&eq->event_mutex);
    sem_destroy(&eq->empty_slots);
    sem_destroy(&eq->full_slots);
    
    free(eq);
}

/*
 * Producer function: Add event to queue
 * 
 * PATTERN:
 * 1. Wait for empty slot (sem_wait on empty_slots)
 * 2. Lock critical section (mutex_lock)
 * 3. Add event to buffer
 * 4. Unlock critical section (mutex_unlock)
 * 5. Signal event available (sem_post on full_slots)
 */
void event_produce(EventQueue* eq, const char* msg, int level, int severity) {
    if (!eq) return;
    
    // Wait for empty slot in buffer
    sem_wait(&eq->empty_slots);
    
    // Enter critical section
    pthread_mutex_lock(&eq->event_mutex);
    
    // Add event to circular buffer
    GameEvent* event = &eq->events[eq->tail];
    strncpy(event->message, msg, EVENT_MESSAGE_SIZE - 1);
    event->message[EVENT_MESSAGE_SIZE - 1] = '\0';
    event->timestamp = time(NULL);
    event->event_id = eq->event_counter++;
    event->level = level;
    event->severity = severity;
    
    // Update tail and count
    eq->tail = (eq->tail + 1) % MAX_GAME_EVENTS;
    eq->count++;
    
    // Exit critical section
    pthread_mutex_unlock(&eq->event_mutex);
    
    // Signal that an event is available
    sem_post(&eq->full_slots);
}

/*
 * Consumer thread: Process events from queue
 * 
 * PATTERN:
 * 1. Wait for full slot (sem_wait on full_slots)
 * 2. Lock critical section (mutex_lock)
 * 3. Remove event from buffer
 * 4. Unlock critical section (mutex_unlock)
 * 5. Signal empty slot available (sem_post on empty_slots)
 * 6. Process event (outside critical section)
 */
void* event_consumer_worker(void* arg) {
    EventQueue* eq = (EventQueue*)arg;
    if (!eq) return NULL;
    
    FILE* event_log = fopen("/tmp/sys_rescue_events.log", "a");
    if (!event_log) {
        perror("Failed to open event log");
        return NULL;
    }
    
    while (eq->active) {
        // Wait for event to be available
        if (sem_wait(&eq->full_slots) == -1) break;
        
        if (!eq->active) break;
        
        // Enter critical section
        pthread_mutex_lock(&eq->event_mutex);
        
        // Get event from circular buffer
        GameEvent event = eq->events[eq->head];
        eq->head = (eq->head + 1) % MAX_GAME_EVENTS;
        eq->count--;
        
        // Exit critical section
        pthread_mutex_unlock(&eq->event_mutex);
        
        // Signal that a slot is now empty
        sem_post(&eq->empty_slots);
        
        // Process event (outside critical section - no blocking)
        const char* severity_str[] = {"[INFO]", "[WARN]", "[CRIT]"};
        fprintf(event_log, "%s %s Level %d: %s (ID:%d)\n",
                severity_str[event.severity],
                ctime(&event.timestamp),
                event.level,
                event.message,
                event.event_id);
        fflush(event_log);
        
        usleep(10000);  // Small delay to allow other threads to work
    }
    
    fclose(event_log);
    return NULL;
}

/* ============================================
   READERS-WRITERS: Game State Management
   ============================================ */

SharedGameState* game_state_create(void) {
    SharedGameState* gs = (SharedGameState*)malloc(sizeof(SharedGameState));
    if (!gs) return NULL;
    
    gs->current_level = 0;
    gs->total_score = 0;
    gs->time_remaining = 300;
    gs->levels_completed = 0;
    gs->total_attempts = 0;
    gs->total_correct = 0;
    gs->last_update_time = time(NULL);
    
    pthread_rwlock_init(&gs->state_lock, NULL);
    
    return gs;
}

void shared_game_state_destroy(SharedGameState* gs) {
    if (!gs) return;
    pthread_rwlock_destroy(&gs->state_lock);
    free(gs);
}

/*
 * Read operations: Multiple threads can read simultaneously
 * Uses: pthread_rwlock_rdlock / pthread_rwlock_unlock
 */

int game_state_read_score(SharedGameState* gs) {
    if (!gs) return 0;
    
    pthread_rwlock_rdlock(&gs->state_lock);
    int score = gs->total_score;
    pthread_rwlock_unlock(&gs->state_lock);
    
    return score;
}

int game_state_read_level(SharedGameState* gs) {
    if (!gs) return 0;
    
    pthread_rwlock_rdlock(&gs->state_lock);
    int level = gs->current_level;
    pthread_rwlock_unlock(&gs->state_lock);
    
    return level;
}

int game_state_read_time(SharedGameState* gs) {
    if (!gs) return 0;
    
    pthread_rwlock_rdlock(&gs->state_lock);
    int time = gs->time_remaining;
    pthread_rwlock_unlock(&gs->state_lock);
    
    return time;
}

void game_state_read_all(SharedGameState* gs, int* score, int* level, int* time, int* completed) {
    if (!gs) return;
    
    pthread_rwlock_rdlock(&gs->state_lock);
    
    if (score) *score = gs->total_score;
    if (level) *level = gs->current_level;
    if (time) *time = gs->time_remaining;
    if (completed) *completed = gs->levels_completed;
    
    pthread_rwlock_unlock(&gs->state_lock);
}

/*
 * Write operations: Only one writer at a time (exclusive access)
 * Uses: pthread_rwlock_wrlock / pthread_rwlock_unlock
 */

void game_state_write_score(SharedGameState* gs, int score) {
    if (!gs) return;
    
    pthread_rwlock_wrlock(&gs->state_lock);
    gs->total_score = score;
    gs->last_update_time = time(NULL);
    pthread_rwlock_unlock(&gs->state_lock);
}

void game_state_write_level(SharedGameState* gs, int level) {
    if (!gs) return;
    
    pthread_rwlock_wrlock(&gs->state_lock);
    gs->current_level = level;
    gs->last_update_time = time(NULL);
    pthread_rwlock_unlock(&gs->state_lock);
}

void game_state_write_time(SharedGameState* gs, int time_left) {
    if (!gs) return;
    
    pthread_rwlock_wrlock(&gs->state_lock);
    gs->time_remaining = time_left;
    pthread_rwlock_unlock(&gs->state_lock);
}

void game_state_write_all(SharedGameState* gs, int score, int level, int time_left) {
    if (!gs) return;
    
    pthread_rwlock_wrlock(&gs->state_lock);
    
    gs->total_score = score;
    gs->current_level = level;
    gs->time_remaining = time_left;
    gs->last_update_time = time(NULL);
    
    pthread_rwlock_unlock(&gs->state_lock);
}

void game_state_increment_attempts(SharedGameState* gs) {
    if (!gs) return;
    
    pthread_rwlock_wrlock(&gs->state_lock);
    gs->total_attempts++;
    pthread_rwlock_unlock(&gs->state_lock);
}

void game_state_increment_correct(SharedGameState* gs) {
    if (!gs) return;
    
    pthread_rwlock_wrlock(&gs->state_lock);
    gs->total_correct++;
    pthread_rwlock_unlock(&gs->state_lock);
}

/* ============================================
   DINING PHILOSOPHERS: Resource Pool Management
   ============================================ */

ResourcePool* resource_pool_init(void) {
    ResourcePool* rp = (ResourcePool*)malloc(sizeof(ResourcePool));
    if (!rp) return NULL;
    
    // Initialize all resource mutexes (the "chopsticks")
    for (int i = 0; i < MAX_RESOURCES; i++) {
        pthread_mutex_init(&rp->resources[i], NULL);
        rp->resource_in_use[i] = 0;
    }
    
    // Resource names
    rp->resource_names[0] = "Event Queue";
    rp->resource_names[1] = "Score Update";
    rp->resource_names[2] = "Timer Access";
    rp->resource_names[3] = "Display Buffer";
    rp->resource_names[4] = "File I/O";
    
    pthread_mutex_init(&rp->status_lock, NULL);
    rp->total_deadlocks_prevented = 0;
    rp->total_resource_acquisitions = 0;
    
    return rp;
}

void resource_pool_destroy(ResourcePool* rp) {
    if (!rp) return;
    
    for (int i = 0; i < MAX_RESOURCES; i++) {
        pthread_mutex_destroy(&rp->resources[i]);
    }
    
    pthread_mutex_destroy(&rp->status_lock);
    free(rp);
}

/*
 * Asymmetric solution to prevent deadlock:
 * 
 * Philosopher 0,1,2,3: Acquire LEFT (resource[id]) then RIGHT (resource[(id+1)%5])
 * Philosopher 4:       Acquire RIGHT first, then LEFT (breaks circular wait)
 * 
 * This ensures that not all philosophers can be in "waiting for right" state simultaneously.
 */
bool resource_pool_acquire(ResourcePool* rp, int philosopher_id) {
    if (!rp || philosopher_id >= MAX_RESOURCES) return false;
    
    int left = philosopher_id;
    int right = (philosopher_id + 1) % MAX_RESOURCES;
    
    // Asymmetric solution
    if (philosopher_id < MAX_RESOURCES - 1) {
        // Standard order: left first, then right
        pthread_mutex_lock(&rp->resources[left]);
        pthread_mutex_lock(&rp->resources[right]);
    } else {
        // Philosopher 4: right first, then left (breaks circular wait)
        pthread_mutex_lock(&rp->resources[right]);
        pthread_mutex_lock(&rp->resources[left]);
    }
    
    // Update statistics
    pthread_mutex_lock(&rp->status_lock);
    rp->total_resource_acquisitions++;
    rp->resource_in_use[left] = philosopher_id;
    rp->resource_in_use[right] = philosopher_id;
    pthread_mutex_unlock(&rp->status_lock);
    
    return true;
}

void resource_pool_release(ResourcePool* rp, int philosopher_id) {
    if (!rp || philosopher_id >= MAX_RESOURCES) return;
    
    int left = philosopher_id;
    int right = (philosopher_id + 1) % MAX_RESOURCES;
    
    // Asymmetric release (opposite order of acquisition)
    if (philosopher_id < MAX_RESOURCES - 1) {
        pthread_mutex_unlock(&rp->resources[right]);
        pthread_mutex_unlock(&rp->resources[left]);
    } else {
        pthread_mutex_unlock(&rp->resources[left]);
        pthread_mutex_unlock(&rp->resources[right]);
    }
    
    // Update statistics
    pthread_mutex_lock(&rp->status_lock);
    rp->resource_in_use[left] = -1;
    rp->resource_in_use[right] = -1;
    pthread_mutex_unlock(&rp->status_lock);
}

int resource_pool_get_deadlocks_prevented(ResourcePool* rp) {
    if (!rp) return 0;
    
    pthread_mutex_lock(&rp->status_lock);
    int count = rp->total_deadlocks_prevented;
    pthread_mutex_unlock(&rp->status_lock);
    
    return count;
}

int resource_pool_get_acquisitions(ResourcePool* rp) {
    if (!rp) return 0;
    
    pthread_mutex_lock(&rp->status_lock);
    int count = rp->total_resource_acquisitions;
    pthread_mutex_unlock(&rp->status_lock);
    
    return count;
}

/* ============================================
   GLOBAL INFRASTRUCTURE MANAGER
   ============================================ */

GameInfrastructure* infrastructure_init(void) {
    GameInfrastructure* infra = (GameInfrastructure*)malloc(sizeof(GameInfrastructure));
    if (!infra) return NULL;
    
    // Initialize all three synchronization components
    infra->event_queue = event_queue_init();
    infra->game_state = game_state_create();
    infra->resource_pool = resource_pool_init();
    infra->infrastructure_active = 1;
    
    // Start event consumer thread
    if (infra->event_queue) {
        pthread_create(&infra->event_consumer_thread, NULL, 
                      event_consumer_worker, infra->event_queue);
    }
    
    return infra;
}

void infrastructure_destroy(GameInfrastructure* infra) {
    if (!infra) return;
    
    infra->infrastructure_active = 0;
    
    // Destroy all components
    if (infra->event_queue) {
        event_queue_destroy(infra->event_queue);
    }
    
    if (infra->game_state) {
        shared_game_state_destroy(infra->game_state);
    }
    
    if (infra->resource_pool) {
        resource_pool_destroy(infra->resource_pool);
    }
    
    free(infra);
}

/*
 * Convenience functions that abstract infrastructure usage
 */

void infrastructure_log_event(GameInfrastructure* infra, const char* msg, int level) {
    if (!infra || !infra->event_queue) return;
    event_produce(infra->event_queue, msg, level, 0);  // 0 = info severity
}

void infrastructure_update_score(GameInfrastructure* infra, int new_score) {
    if (!infra || !infra->game_state) return;
    
    // Acquire resource (deadlock-free)
    if (infra->resource_pool) {
        resource_pool_acquire(infra->resource_pool, 1);  // Resource 1 = Score Update
    }
    
    game_state_write_score(infra->game_state, new_score);
    
    // Release resource
    if (infra->resource_pool) {
        resource_pool_release(infra->resource_pool, 1);
    }
}

void infrastructure_update_level(GameInfrastructure* infra, int new_level) {
    if (!infra || !infra->game_state) return;
    
    // Acquire resource (deadlock-free)
    if (infra->resource_pool) {
        resource_pool_acquire(infra->resource_pool, 2);  // Resource 2 = Level Access
    }
    
    game_state_write_level(infra->game_state, new_level);
    
    // Release resource
    if (infra->resource_pool) {
        resource_pool_release(infra->resource_pool, 2);
    }
}
