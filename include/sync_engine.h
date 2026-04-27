#ifndef SYNC_ENGINE_H
#define SYNC_ENGINE_H

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define LOG_BUFFER_SIZE 10
#define LOG_MESSAGE_SIZE 256
#define NUM_PHILOSOPHERS 5
#define NUM_CHANNELS 5

/* ============================================
   PRODUCER-CONSUMER: Event Logger
   ============================================ */

typedef struct {
    char log_buffer[LOG_BUFFER_SIZE][LOG_MESSAGE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t log_mutex;
    sem_t empty;
    sem_t full;
} LogBuffer;

/* ============================================
   READERS-WRITERS: Game State
   ============================================ */

typedef struct {
    int score;
    int time_left;
    int current_room;
    pthread_rwlock_t state_lock;
} GameState;

/* ============================================
   DINING PHILOSOPHERS: Audio/Task Mixer
   ============================================ */

typedef struct {
    pthread_mutex_t channels[NUM_CHANNELS];
    int channel_status[NUM_CHANNELS];  // 0 = free, thread_id = busy
    pthread_mutex_t status_lock;
} ChannelPool;

/* Function Prototypes */

// Producer-Consumer Functions
LogBuffer* log_buffer_init(void);
void log_buffer_destroy(LogBuffer* lb);
void log_produce(LogBuffer* lb, const char* message);
void* log_consumer_thread(void* arg);

// Readers-Writers Functions
GameState* game_state_init(void);
void game_state_destroy(GameState* gs);
void read_game_state(GameState* gs, int* score, int* time, int* room);
void write_game_state(GameState* gs, int score, int time, int room);

// Dining Philosophers Functions
ChannelPool* channel_pool_init(void);
void channel_pool_destroy(ChannelPool* cp);
int philosopher_acquire_channels(ChannelPool* cp, int philosopher_id);
void philosopher_release_channels(ChannelPool* cp, int philosopher_id);
void* philosopher_worker_thread(void* arg);

// Utility Functions
void synchronization_demo(void);
void test_producer_consumer(void);
void test_readers_writers(void);
void test_dining_philosophers(void);

#endif // SYNC_ENGINE_H
