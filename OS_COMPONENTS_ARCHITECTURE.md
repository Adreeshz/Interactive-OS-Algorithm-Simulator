# SYS_RESCUE: OS Components Architecture

## Overview
This document describes the 5 core Operating Systems concepts implemented in the SYS_RESCUE Interactive OS Algorithm Simulator and their roles in the project infrastructure (outside of gameplay).

---

## 1. System Calls & Thread Management

### What It Does
System calls provide the interface between user-level application code and kernel-level services. In this project, thread management enables concurrent execution of multiple game components and background simulations.

### Where It's Used

#### 1.1 Active Sessions Simulation Thread
**Location:** `src/login_system.c:50`, `src/active_sessions.c:374-380`

```c
// Initialization in login_system_init()
active_sessions_start_simulation(global_active_sessions);

// Thread creation in active_sessions.c
pthread_create(&manager->sim_thread, NULL, active_sessions_simulation_thread, manager);
```

**Purpose:**
- Simulates concurrent player activity (alice_gamer, bob_player, etc.)
- Continuously updates player states, scores, and activities
- Allows admin to view live session data without affecting the current player

**How It Works:**
```c
void* active_sessions_simulation_thread(void* arg) {
    ActiveSessionsManager* manager = (ActiveSessionsManager*)arg;
    
    while (manager->simulation_running) {
        pthread_mutex_lock(&manager->sessions_lock);
        
        // For each active player:
        // - Randomly advance questions answered (40% chance)
        // - Move to different levels (20% chance)
        // - Record violations (10% chance)
        // - Update last activity time
        
        pthread_mutex_unlock(&manager->sessions_lock);
        sleep(2);  // Update every 2 seconds
    }
    return NULL;
}
```

#### 1.2 Background Simulation Thread
**Location:** `src/main.c:157-175`

```c
void start_background_simulation(void) {
    engine.sim_active = 1;
    pthread_create(&engine.sim_thread, NULL, background_simulation_thread, NULL);
}
```

**Purpose:**
- Simulates OS operations running in the background
- Tracks CPU operations, page faults, disk operations, memory allocations
- Provides data for system monitor display

**How It Works:**
- Runs independently from main game loop
- Increments operation counters periodically
- Stopped cleanly when player logs out or game ends

#### 1.3 Event Consumer Thread
**Location:** `src/game_infrastructure.c`, `src/main.c:797-801`

```c
pthread_create(&engine.event_queue->consumer_thread, NULL, 
               event_consumer_worker, (void*)engine.event_queue);
```

**Purpose:**
- Consumes events from the producer-consumer event queue
- Logs events to `/tmp/game_events.log`
- Runs independently without blocking game flow

### Key System Call Functions Used

| Function | Purpose |
|----------|---------|
| `pthread_create()` | Create new threads for concurrent operations |
| `pthread_join()` | Wait for thread completion (cleanup) |
| `pthread_mutex_init/destroy()` | Initialize/destroy mutual exclusion locks |
| `sem_init/destroy()` | Initialize/destroy semaphores |
| `sleep()` | System call to pause thread execution |
| `pthread_mutex_lock/unlock()` | Acquire/release mutex for critical sections |

### Cleanup & Thread Lifecycle

**Proper Thread Termination:**
```c
// In login_system_destroy():
if (global_active_sessions) {
    active_sessions_stop_simulation(global_active_sessions);  // Signal thread to stop
    
    if (global_active_sessions->sim_thread != 0) {
        pthread_join(global_active_sessions->sim_thread, NULL);  // Wait for exit
    }
    
    active_sessions_destroy(global_active_sessions);
    global_active_sessions = NULL;
}

if (engine.sim_active) {
    engine.sim_active = 0;
    if (engine.sim_thread != 0) {
        pthread_join(engine.sim_thread, NULL);
    }
}

if (engine.event_queue) {
    event_queue_destroy(engine.event_queue);  // Joins consumer thread
}
```

---

## 2. Synchronization: Producer-Consumer Pattern

### What It Does
The Producer-Consumer pattern is a classic synchronization mechanism that allows safe data exchange between threads. One thread produces data while another consumes it, with synchronization primitives ensuring no race conditions occur.

### Where It's Used

#### 2.1 Event Logging System
**Location:** `src/game_infrastructure.c`, `include/game_infrastructure.h`

**Purpose:**
- Log all game events (level completion, score updates, timer events, answers)
- Producer: Game logic (main game loop)
- Consumer: Background event consumer thread

#### 2.2 Data Structure
```c
typedef struct {
    GameEvent events[MAX_GAME_EVENTS];  // 50 events max
    int head, tail, count;              // Buffer pointers
    
    // Synchronization primitives
    pthread_mutex_t event_mutex;        // Exclusive access to buffer
    sem_t empty_slots;                  // Initially = MAX_GAME_EVENTS
    sem_t full_slots;                   // Initially = 0
    
    pthread_t consumer_thread;
    int active;
} EventQueue;
```

### How It Works

#### Producer Side (Game Logic)
**Location:** Various places in `src/main.c`

```c
// Example: When player completes level
char event_msg[EVENT_MESSAGE_SIZE];
snprintf(event_msg, EVENT_MESSAGE_SIZE, 
         "LEVEL_COMPLETE: Level %d completed with score +%d", 
         level_id, level_score);

// PRODUCER: Add event to queue
event_produce(engine.event_queue, event_msg, level_id, 0);
```

**Producer Implementation:**
```c
void event_produce(EventQueue* eq, const char* msg, int level, int severity) {
    if (!eq) return;
    
    // Step 1: Wait for empty slot (blocking if buffer full)
    sem_wait(&eq->empty_slots);
    
    // Step 2: Lock critical section
    pthread_mutex_lock(&eq->event_mutex);
    
    // Step 3: Add event to buffer
    strncpy(eq->buffer[eq->tail].message, msg, EVENT_MESSAGE_SIZE - 1);
    eq->buffer[eq->tail].level = level;
    eq->buffer[eq->tail].severity = severity;
    eq->tail = (eq->tail + 1) % MAX_GAME_EVENTS;
    eq->count++;
    
    // Step 4: Unlock critical section
    pthread_mutex_unlock(&eq->event_mutex);
    
    // Step 5: Signal that slot is now full
    sem_post(&eq->full_slots);
}
```

#### Consumer Side (Background Thread)
**Location:** `src/game_infrastructure.c`

```c
void* event_consumer_worker(void* arg) {
    EventQueue* eq = (EventQueue*)arg;
    
    while (eq->active) {
        // Step 1: Wait for full slot (blocking if buffer empty)
        sem_wait(&eq->full_slots);
        
        if (!eq->active) break;
        
        // Step 2: Lock critical section
        pthread_mutex_lock(&eq->event_mutex);
        
        if (eq->count > 0) {
            // Step 3: Extract event
            GameEvent event = eq->buffer[eq->head];
            eq->head = (eq->head + 1) % MAX_GAME_EVENTS;
            eq->count--;
            
            pthread_mutex_unlock(&eq->event_mutex);
            
            // Step 4: Process outside critical section
            FILE* log_file = fopen("/tmp/game_events.log", "a");
            if (log_file) {
                fprintf(log_file, "[EVENT] %s\n", event.message);
                fclose(log_file);
            }
        } else {
            pthread_mutex_unlock(&eq->event_mutex);
        }
        
        // Step 5: Signal that slot is now empty
        sem_post(&eq->empty_slots);
    }
    
    return NULL;
}
```

### Synchronization Guarantees

| Scenario | Behavior |
|----------|----------|
| Producer faster than consumer | Producer blocks on `sem_wait(&empty_slots)` when buffer full |
| Consumer faster than producer | Consumer blocks on `sem_wait(&full_slots)` when buffer empty |
| Concurrent access | Mutex ensures only one thread accesses buffer at a time |

### Events Being Logged

- **Level Completion:** "LEVEL_COMPLETE: Level X completed with score +Y"
- **Score Updates:** Score changes when questions answered correctly
- **Timer Events:** Time updates during gameplay
- **Answer Events:** Player submissions tracked
- **Violations:** Rule violations recorded

### Log Location
All events written to: `/tmp/game_events.log`

---

## 3. CPU Scheduling: Round-Robin (RR)

### What It Does
CPU scheduling determines which process gets CPU time and for how long. Round-Robin allocates equal time slices (quantum) to each process in a queue, implementing fair, preemptive scheduling.

### Where It's Used

#### 3.1 Scheduler Initialization
**Location:** `src/main.c:815`

```c
engine.scheduler = scheduler_init(SCHED_RR_ALG, 4);  // Round-robin with quantum 4
```

**Purpose:**
- Simulates OS-level process scheduling
- Demonstrates how multiple processes share CPU time fairly
- Used in game levels and background demonstrations

#### 3.2 Data Structures
**Location:** `include/scheduler.h`, `src/scheduler.c`

```c
typedef struct {
    int pid;                    // Process ID
    char name[50];              // Process name
    int burst_time;             // CPU time needed
    int time_remaining;         // Remaining CPU time
    int arrival_time;           // When process arrived
    int completion_time;        // When process finished
    int waiting_time;           // Total time waiting
    int turnaround_time;        // Total time in system
    SchedulerState state;       // READY, RUNNING, BLOCKED, COMPLETED
} Process;

typedef struct {
    Process processes[MAX_PROCESSES];
    int process_count;
    int current_index;
    int quantum;                // Time slice for RR
    int total_time;
    SchedulerAlgorithm algorithm;
    Queue ready_queue;          // Queue of ready processes
} Scheduler;
```

### How It Works

#### Round-Robin Algorithm
**Location:** `src/scheduler.c`

```c
void scheduler_run_timeslice(Scheduler* s) {
    if (!s || s->process_count == 0) return;
    
    for (int i = 0; i < s->process_count; i++) {
        Process* p = &s->processes[i];
        
        if (p->state == COMPLETED) continue;
        
        // Execute for quantum time slice
        int executed = MIN(s->quantum, p->time_remaining);
        p->time_remaining -= executed;
        s->total_time += executed;
        
        // Update state
        if (p->time_remaining <= 0) {
            p->state = COMPLETED;
            p->completion_time = s->total_time;
            p->turnaround_time = p->completion_time - p->arrival_time;
        } else {
            // If time slice used up, go to back of queue
            p->state = READY;
        }
    }
}
```

#### Key Properties of RR Scheduling

| Property | Value/Behavior |
|----------|----------------|
| **Quantum** | 4 time units |
| **Fairness** | Every process gets equal CPU time |
| **Preemption** | Yes (forcibly switches after quantum) |
| **Context Switching** | High (every quantum expires) |
| **Starvation** | No (all processes get CPU time) |
| **Average Waiting Time** | Moderate (depends on process count) |

### Where Scheduling is Applied

1. **Level 3: Scheduling Algorithms**
   - Students solve scheduling problems
   - Calculate: Waiting Time, Turnaround Time, Average metrics
   
2. **Background Simulation**
   - `background_simulation_thread` simulates CPU operations
   - Increments operation counters like CPU operations, page faults
   
3. **Admin Monitor**
   - `display_system_monitor()` shows scheduling statistics
   - Displays CPU operations count, utilization metrics

### Scheduling Metrics Calculated

```c
// After all processes complete:
Average Waiting Time = Σ(waiting_time) / process_count
Average Turnaround Time = Σ(turnaround_time) / process_count
CPU Utilization = (total_execution_time / total_elapsed_time) * 100
```

---

## 4. Banker's Algorithm: Deadlock Prevention

### What It Does
Banker's Algorithm prevents deadlock by checking if resource allocation is safe before granting it. It ensures the system never enters a deadlocked state by analyzing resource availability and process requests.

### Where It's Used

#### 4.1 Banker Initialization
**Location:** `src/main.c:814`

```c
engine.banker = banker_init(3);  // 3 types of resources
```

**Purpose:**
- Prevent deadlock during resource allocation
- Simulate real OS resource management
- Demonstrate safe state analysis

#### 4.2 Data Structures
**Location:** `include/algorithms.h`, `src/algorithms.c`

```c
typedef struct {
    int pid;                          // Process ID
    int max_resources[MAX_RESOURCES]; // Maximum need
    int allocated[MAX_RESOURCES];     // Currently allocated
    int need[MAX_RESOURCES];          // Still need
} BankerProcess;

typedef struct {
    BankerProcess processes[MAX_PROCESSES];
    int available[MAX_RESOURCES];     // Available resources
    int resource_count;
    int process_count;
    int total_resources[MAX_RESOURCES];
} BankerAlgorithm;
```

### How It Works

#### Safe State Check
**Location:** `src/algorithms.c:90-140`

```c
int banker_is_safe_state(BankerAlgorithm* banker) {
    if (!banker) return 0;
    
    int available[MAX_RESOURCES];
    memcpy(available, banker->available, 
           sizeof(int) * banker->resource_count);
    
    int finished[MAX_PROCESSES] = {0};
    int completed = 0;
    
    // Try to find safe sequence
    while (completed < banker->process_count) {
        int found = 0;
        
        for (int i = 0; i < banker->process_count; i++) {
            if (finished[i]) continue;
            
            BankerProcess* p = &banker->processes[i];
            int can_finish = 1;
            
            // Check if process can finish with available resources
            for (int j = 0; j < banker->resource_count; j++) {
                if (p->need[j] > available[j]) {
                    can_finish = 0;
                    break;
                }
            }
            
            if (can_finish) {
                // Process can finish - simulate it and release resources
                for (int j = 0; j < banker->resource_count; j++) {
                    available[j] += p->allocated[j];
                }
                finished[i] = 1;
                completed++;
                found = 1;
                break;
            }
        }
        
        if (!found) {
            // No process can finish - UNSAFE STATE
            return 0;
        }
    }
    
    // All processes can finish - SAFE STATE
    return 1;
}
```

#### Resource Request Algorithm
**Location:** `src/algorithms.c`

```c
int banker_request_resources(BankerAlgorithm* banker, int pid, 
                            const int* request) {
    if (!banker || !request) return -1;
    
    BankerProcess* p = &banker->processes[pid];
    
    // Step 1: Check if request <= need
    for (int i = 0; i < banker->resource_count; i++) {
        if (request[i] > p->need[i]) {
            return -1;  // Request exceeds need
        }
    }
    
    // Step 2: Check if request <= available
    for (int i = 0; i < banker->resource_count; i++) {
        if (request[i] > banker->available[i]) {
            return 0;   // Must wait
        }
    }
    
    // Step 3: Simulate allocation
    for (int i = 0; i < banker->resource_count; i++) {
        banker->available[i] -= request[i];
        p->allocated[i] += request[i];
        p->need[i] -= request[i];
    }
    
    // Step 4: Check if state is safe
    if (banker_is_safe_state(banker)) {
        return 1;  // Grant request
    }
    
    // Step 5: Rollback if unsafe
    for (int i = 0; i < banker->resource_count; i++) {
        banker->available[i] += request[i];
        p->allocated[i] -= request[i];
        p->need[i] += request[i];
    }
    
    return 0;  // Deny request
}
```

### Deadlock Prevention Guarantees

| Condition | Guarantee |
|-----------|-----------|
| **Safe State** | System can never reach deadlock |
| **Process Completion** | All processes can eventually finish |
| **Resource Availability** | Resources sufficient for at least one process to finish |
| **Circular Wait** | Prevented by ordered resource allocation |

### Where Banker's Algorithm is Applied

1. **Level 4: Banker's Algorithm**
   - Students solve resource allocation problems
   - Determine safe/unsafe states
   - Calculate maximum available resources

2. **Background Resource Management**
   - Simulates OS preventing deadlock
   - Allocates resources to simulated processes

3. **Admin Monitor**
   - Display resource allocation status
   - Show available resources

### Example Scenario

```
Resources: 3 types (A=10, B=5, C=7)
Processes: 3

Process 0: Max=[10,5,7], Allocated=[5,2,3], Need=[5,3,4]
Process 1: Max=[4,3,3],  Allocated=[2,0,1], Need=[2,3,2]
Process 2: Max=[7,5,5],  Allocated=[0,1,0], Need=[7,4,5]

Available: [3,2,3]

Safe Sequence: P1 → P0 → P2
Result: SAFE STATE ✓
```

---

## 5. Page Replacement & Memory Management & Disk Scheduling

### What It Does

These three components work together to simulate modern virtual memory and disk I/O management:

- **Memory Management:** Allocates and deallocates memory using placement algorithms
- **Page Replacement:** Determines which memory page to evict when memory is full
- **Disk Scheduling:** Orders disk I/O requests to minimize seek time

### Where It's Used

#### 5.1 Initialization
**Location:** `src/main.c:816-818`

```c
engine.memory = memory_init(1024, PLACE_BEST_FIT);      // 1KB memory, best-fit
engine.paging = page_system_init(4, PAGE_LRU);          // 4 frames, LRU
engine.disk_sched = disk_scheduler_init(200, DISK_CSCAN); // 200 tracks, C-SCAN
```

---

## 5.1 Memory Management: Best-Fit Allocation

### What It Does
Memory management allocates physical memory to processes efficiently, minimizing fragmentation and wasted space.

### Data Structures
**Location:** `include/algorithms.h`, `src/algorithms.c`

```c
typedef struct {
    int start_address;      // Starting memory address
    int size;               // Block size
    int process_id;         // Owner process ID
    int allocated;          // 1 if allocated, 0 if free
} MemoryBlock;

typedef struct {
    MemoryBlock* blocks;
    int block_count;
    int total_memory;
    MemoryPlacementAlgorithm algorithm;
} MemoryManager;
```

### How It Works: Best-Fit Algorithm

**Location:** `src/algorithms.c`

```c
int memory_allocate(MemoryManager* mem, int process_id, int size) {
    if (!mem || size <= 0) return -1;
    
    int best_fit_index = -1;
    int best_fit_size = INT_MAX;
    
    // Step 1: Find smallest free block that fits
    for (int i = 0; i < mem->block_count; i++) {
        MemoryBlock* block = &mem->blocks[i];
        
        if (!block->allocated && block->size >= size) {
            // Found a block that fits
            if (block->size < best_fit_size) {
                best_fit_index = i;
                best_fit_size = block->size;
            }
        }
    }
    
    if (best_fit_index == -1) {
        return -1;  // No suitable block found
    }
    
    // Step 2: Allocate memory
    MemoryBlock* block = &mem->blocks[best_fit_index];
    block->process_id = process_id;
    block->allocated = 1;
    
    // Step 3: Create new free block if there's leftover space
    if (block->size > size) {
        // Shift blocks and insert new free block
        mem->blocks[mem->block_count].start_address = 
            block->start_address + size;
        mem->blocks[mem->block_count].size = 
            block->size - size;
        mem->blocks[mem->block_count].allocated = 0;
        mem->block_count++;
        
        block->size = size;
    }
    
    return block->start_address;
}
```

### Best-Fit vs Other Algorithms

| Algorithm | Behavior | Fragmentation |
|-----------|----------|----------------|
| **Best-Fit** | Smallest block that fits | Low |
| **First-Fit** | First block that fits | Medium |
| **Worst-Fit** | Largest available block | High |

### Where Applied

1. **Level 3: Memory Management**
   - Allocate memory to processes
   - Calculate fragmentation
   - Optimize memory usage

2. **Background Simulation**
   - Track memory allocations count
   - `engine.memory_allocations_count` incremented during simulation

---

## 5.2 Page Replacement: Least Recently Used (LRU)

### What It Does
Page replacement determines which page to remove from physical memory (page frame) when new pages need to be loaded. LRU removes the page that hasn't been used for the longest time.

### Data Structures
**Location:** `include/algorithms.h`, `src/algorithms.c`

```c
typedef struct {
    int page_number;        // Virtual page number
    int frame_number;       // Physical frame it occupies (-1 if not in memory)
    int last_used_time;     // Timestamp of last access
    int valid;              // 1 if page in memory
} PageTableEntry;

typedef struct {
    PageTableEntry* page_table;
    int* frames;            // Physical frames
    int frame_count;        // Number of frames
    int page_count;         // Number of pages
    int page_faults;        // Total page faults
    int time_counter;       // Current time
    PageReplacementAlgorithm algorithm;
} PageSystem;
```

### How It Works: LRU Algorithm

**Location:** `src/algorithms.c`

```c
int page_system_access_page(PageSystem* ps, int page_num) {
    if (!ps || page_num < 0 || page_num >= ps->page_count) {
        return -1;
    }
    
    PageTableEntry* entry = &ps->page_table[page_num];
    
    if (entry->valid) {
        // Page hit - page is already in memory
        entry->last_used_time = ps->time_counter++;
        return entry->frame_number;
    }
    
    // Page miss - need to load page
    ps->page_faults++;
    
    // Step 1: Find free frame
    int frame = -1;
    for (int i = 0; i < ps->frame_count; i++) {
        if (ps->frames[i] == -1) {
            frame = i;
            break;
        }
    }
    
    // Step 2: If no free frame, evict LRU page
    if (frame == -1) {
        int lru_page = -1;
        int lru_time = INT_MAX;
        
        for (int i = 0; i < ps->page_count; i++) {
            if (ps->page_table[i].valid) {
                if (ps->page_table[i].last_used_time < lru_time) {
                    lru_time = ps->page_table[i].last_used_time;
                    lru_page = i;
                }
            }
        }
        
        if (lru_page != -1) {
            // Evict LRU page
            frame = ps->page_table[lru_page].frame_number;
            ps->page_table[lru_page].valid = 0;
            ps->page_table[lru_page].frame_number = -1;
        }
    }
    
    // Step 3: Load new page into frame
    ps->frames[frame] = page_num;
    entry->valid = 1;
    entry->frame_number = frame;
    entry->last_used_time = ps->time_counter++;
    
    return frame;
}
```

### Page Replacement Algorithm Comparison

| Algorithm | Selection Criteria | Fault Rate |
|-----------|-------------------|------------|
| **LRU** | Least recently used | Optimal (theoretically) |
| **FIFO** | Oldest page | Higher than LRU |
| **LFU** | Least frequently used | Medium |
| **Optimal** | Future reference | Best (requires future knowledge) |

### Where Applied

1. **Level 6: Paging**
   - Students analyze page references
   - Calculate page faults
   - Compare algorithms

2. **Background Simulation**
   - `engine.page_faults_count` incremented when pages swapped
   - Simulates realistic page fault rates

3. **System Monitor**
   - Display page fault statistics
   - Show memory utilization

---

## 5.3 Disk Scheduling: C-SCAN

### What It Does
Disk scheduling orders I/O requests to minimize head movement and disk arm seek time. C-SCAN (Circular SCAN) moves the disk head in one direction until the end, then jumps to the beginning and sweeps again.

### Data Structures
**Location:** `include/algorithms.h`, `src/algorithms.c`

```c
typedef struct {
    int track;              // Disk track number (0-199)
    int arrival_time;       // When request arrived
    int service_time;       // Time to serve request
    int wait_time;          // Time waiting in queue
    int served;             // 1 if already served
} DiskRequest;

typedef struct {
    DiskRequest requests[MAX_DISK_REQUESTS];
    int request_count;
    int total_tracks;       // Total tracks (0-199)
    int current_position;   // Current head position
    int direction;          // 0=towards 0, 1=towards max
    int total_seek_time;
    int total_wait_time;
    DiskSchedulingAlgorithm algorithm;
} DiskScheduler;
```

### How It Works: C-SCAN Algorithm

**Location:** `src/algorithms.c`

```c
int disk_scheduler_schedule(DiskScheduler* ds) {
    if (!ds || ds->request_count == 0) return 0;
    
    int total_seek = 0;
    int current_pos = ds->current_position;
    
    // Direction: 1 = towards max tracks, 0 = towards 0
    int direction = 1;
    
    for (int sweep = 0; sweep < 2; sweep++) {
        if (direction == 1) {
            // Sweep from current position to max
            for (int track = current_pos; track < ds->total_tracks; track++) {
                for (int i = 0; i < ds->request_count; i++) {
                    if (!ds->requests[i].served && ds->requests[i].track == track) {
                        // Seek to this track
                        int seek_distance = abs(track - current_pos);
                        total_seek += seek_distance;
                        current_pos = track;
                        
                        ds->requests[i].served = 1;
                        ds->requests[i].wait_time = total_seek - ds->requests[i].arrival_time;
                    }
                }
            }
            
            // Jump to beginning
            total_seek += current_pos;  // Cost to go from end to 0
            current_pos = 0;
            direction = 0;
        } else {
            // Sweep from 0 to current position
            for (int track = 0; track < current_pos; track++) {
                for (int i = 0; i < ds->request_count; i++) {
                    if (!ds->requests[i].served && ds->requests[i].track == track) {
                        int seek_distance = abs(track - current_pos);
                        total_seek += seek_distance;
                        current_pos = track;
                        
                        ds->requests[i].served = 1;
                        ds->requests[i].wait_time = total_seek - ds->requests[i].arrival_time;
                    }
                }
            }
        }
    }
    
    ds->total_seek_time = total_seek;
    return total_seek;
}
```

### Disk Scheduling Algorithm Comparison

| Algorithm | Head Movement | Fairness | Starvation |
|-----------|---------------|----------|-----------|
| **C-SCAN** | Circular sweeps | High | No |
| **SCAN** | Back and forth | High | No |
| **FCFS** | Random | Low | No |
| **SSTF** | Nearest next | Low | Possible |

### Where Applied

1. **Level 6: Disk Scheduling**
   - Order I/O requests
   - Calculate total seek time
   - Compare scheduling algorithms

2. **Background Simulation**
   - `engine.disk_operations_count` incremented
   - Simulates realistic disk I/O patterns

3. **System Monitor**
   - Display disk activity
   - Show queue length

### Example C-SCAN Scenario

```
Disk Tracks: 0-199
Requests at: [50, 100, 150, 25, 175]
Current Position: 75

Sequence:
1. 75 → 100 (seek 25)
2. 100 → 150 (seek 50)
3. 150 → 175 (seek 25)
4. 175 → 0 (jump 175)
5. 0 → 25 (seek 25)
6. 25 → 50 (seek 25)

Total Seek Time: 25+50+25+175+25+25 = 325
```

---

## Integration: How It All Works Together

### Startup Sequence
```
main()
  ├─ login_system_init()
  │  └─ active_sessions_init()
  │     └─ active_sessions_start_simulation() ──► Thread 1: Simulate players
  │
  ├─ event_queue_init()
  │  └─ pthread_create(event_consumer_worker) ──► Thread 2: Log events
  │
  ├─ banker_init(3)           ──► Deadlock prevention
  ├─ scheduler_init(RR, 4)    ──► Process scheduling
  ├─ memory_init(1024, BEST)  ──► Memory allocation
  ├─ page_system_init(4, LRU) ──► Page replacement
  ├─ disk_scheduler_init(200) ──► Disk I/O ordering
  │
  ├─ start_background_simulation() ──► Thread 3: OS operations
  │
  └─ game_loop()
     ├─ Thread 1: Updates player sessions every 2 seconds
     ├─ Thread 2: Logs events to /tmp/game_events.log
     ├─ Thread 3: Tracks CPU/disk/memory operations
     └─ Main: Runs gameplay, levels, admin panel
```

### Shutdown Sequence
```
game_loop() returns (player logs out)
  ├─ stop_background_simulation()
  │  └─ Wait for Thread 3 to finish
  │
  ├─ login_system_destroy()
  │  ├─ active_sessions_stop_simulation()
  │  │  └─ Wait for Thread 1 to finish
  │  │
  │  ├─ event_queue_destroy()
  │  │  └─ Wait for Thread 2 to finish
  │  │
  │  └─ user_db_destroy()
  │
  ├─ banker_destroy()
  ├─ scheduler_destroy()
  ├─ memory_destroy()
  ├─ page_system_destroy()
  ├─ disk_scheduler_destroy()
  │
  └─ Program exits cleanly
```

---

## Data Flow: Event Production to Logging

```
Game Level Complete Event
       ↓
event_produce()
  ├─ Wait for empty slot in buffer
  ├─ Lock mutex
  ├─ Add event: "LEVEL_COMPLETE: Level X ..."
  ├─ Update tail pointer
  ├─ Unlock mutex
  └─ Signal full slot
       ↓
event_consumer_worker() (Background Thread)
  ├─ Wait for full slot
  ├─ Lock mutex
  ├─ Extract event from head
  ├─ Update head pointer
  ├─ Unlock mutex
  ├─ Signal empty slot
  └─ Write to /tmp/game_events.log
```

---

## Key Synchronization Points

### Critical Sections Protected by Mutex

| Location | Resource | Mutex |
|----------|----------|-------|
| Active Sessions | Player data | `sessions_lock` |
| Event Queue | Event buffer | `event_mutex` |
| Game Engine | Score, level data | `timer_lock` |

### Semaphore Usage

| Semaphore | Initial Value | Purpose |
|-----------|---------------|---------|
| `empty_slots` | MAX_GAME_EVENTS | Count of empty slots in event buffer |
| `full_slots` | 0 | Count of full slots in event buffer |

---

## Performance Considerations

### Active Sessions Simulation
- Update Interval: 2 seconds
- Operations per cycle: Scan all players, random state changes
- Memory: Fixed (10 players max)
- Overhead: Low (simple operations, infrequent updates)

### Event Logging
- Buffer Size: 50 events
- Consumer: Runs independently, doesn't block producer
- I/O: Asynchronous file writes
- Performance: Minimal impact on gameplay

### Background Simulation
- Update Interval: Varies (increments counters)
- Operations: CPU, page faults, disk, memory
- Runs: Entire game session
- Stopped: Cleanly when logout

### Scheduler
- Quantum: 4 time units
- Processes: Up to 16 processes
- Algorithm: Round-Robin (fair, preemptive)

### Memory Management
- Total: 1KB (1024 bytes)
- Algorithm: Best-Fit (reduces fragmentation)
- Blocks: Dynamic (created/destroyed)

### Paging
- Frames: 4 physical frames
- Pages: Simulated virtual memory
- Algorithm: LRU (optimal theoretical performance)
- Page Faults: Tracked and displayed

### Disk Scheduling
- Tracks: 0-199 (200 tracks total)
- Algorithm: C-SCAN (fair, predictable)
- Requests: Up to 100

---

## Conclusion

The SYS_RESCUE simulator integrates 5 fundamental OS concepts to create a realistic educational platform:

1. **Thread Management** enables concurrent simulations
2. **Synchronization** ensures safe data access across threads
3. **Scheduling** demonstrates fair CPU resource allocation
4. **Deadlock Prevention** shows safe resource management
5. **Virtual Memory & I/O** completes the OS foundation

Each component works independently yet cohesively, allowing students to understand how modern operating systems manage resources and coordinate multiple activities simultaneously.
