/*
 * ════════════════════════════════════════════════════════════════════════════
 * CONCEPT 1: SYSTEM CALLS & THREADING IMPLEMENTATION
 * ════════════════════════════════════════════════════════════════════════════
 * 
 * This file consolidates the actual implementation of all System Calls and
 * Threading concepts from the SYS_RESCUE Interactive OS Algorithm Simulator.
 * 
 * Topics Covered:
 * 1. Active Sessions Simulation Thread (pthread_create, pthread_mutex)
 * 2. Background Simulation Thread (concurrent OS operations)
 * 3. Event Consumer Thread (Producer-Consumer with semaphores)
 * 4. Admin Panel: View Active Sessions with Process Management (getpid, fork, kill)
 * 
 * For presentation to professor - shows actual working code from the project.
 */

/* ════════════════════════════════════════════════════════════════════════════
   TOPIC 1: ACTIVE SESSIONS SIMULATION THREAD
   ════════════════════════════════════════════════════════════════════════════
   
   Demonstrates:
   - pthread_create() - creating threads
   - pthread_mutex_lock/unlock - protecting shared data
   - Concurrent execution and synchronization
   - Real-time player state simulation
*/

/*
 * THREAD FUNCTION: active_sessions_simulation_thread()
 * 
 * Purpose: Runs in a background thread to simulate concurrent player activities
 * Parameters: arg - pointer to ActiveSessionsManager structure
 * Returns: NULL (thread return value)
 * 
 * Key Concepts:
 * - Runs independently from main thread
 * - Uses mutex to protect shared player data
 * - Updates player states randomly (answers questions, changes levels, records violations)
 * - Updates every 2 seconds (sleep(2))
 */

void* active_sessions_simulation_thread(void* arg) {
    ActiveSessionsManager* manager = (ActiveSessionsManager*)arg;
    
    // Simulate activities for each player
    while (manager->simulation_running) {
        // SYNCHRONIZATION: Lock before accessing shared data
        pthread_mutex_lock(&manager->sessions_lock);
        
        for (int i = 0; i < manager->active_count; i++) {
            if (!manager->sessions[i].is_active) continue;
            
            ActiveGameSession* session = &manager->sessions[i];
            
            // Randomly update player state and score
            int action = rand() % 100;
            
            if (action < 40) {
                // Player answered a question (40% chance)
                session->questions_answered++;
                if (rand() % 100 < 75) {  // 75% accuracy rate
                    session->correct_answers++;
                    session->score += 10;
                }
                session->last_activity_time = time(NULL);
            } 
            else if (action < 60) {
                // Player moved to a different level (20% chance)
                int new_level = rand() % 6;
                session->current_level = new_level;
                
                switch (new_level) {
                    case 0:
                        session->state = PLAYING_LEVEL_0;
                        break;
                    case 1:
                        session->state = PLAYING_LEVEL_1;
                        break;
                    case 2:
                        session->state = PLAYING_LEVEL_2;
                        break;
                    case 3:
                        session->state = PLAYING_LEVEL_3;
                        break;
                    case 4:
                        session->state = PLAYING_LEVEL_4;
                        break;
                    case 5:
                        session->state = PLAYING_LEVEL_5;
                        break;
                }
                session->last_activity_time = time(NULL);
            } 
            else if (action < 75) {
                // Player is in menu (idle)
                session->state = IDLE;
            } 
            else if (action < 85) {
                // Violation recorded (rarely - 10% chance)
                session->violations++;
                session->last_activity_time = time(NULL);
            }
        }
        
        // SYNCHRONIZATION: Unlock after modifying shared data
        pthread_mutex_unlock(&manager->sessions_lock);
        
        // Sleep for 2 seconds before next update cycle
        sleep(2);
    }
    
    return NULL;
}

/*
 * START SIMULATION: active_sessions_start_simulation()
 * 
 * Creates the background thread for session simulation.
 * Called during game initialization to start concurrent session tracking.
 */

void active_sessions_start_simulation(ActiveSessionsManager* manager) {
    if (!manager || manager->simulation_running) return;
    
    manager->simulation_running = 1;
    
    // SYSTEM CALL: pthread_create()
    // Creates new thread running active_sessions_simulation_thread function
    // Thread ID stored in manager->sim_thread for later joining
    pthread_create(&manager->sim_thread, NULL, active_sessions_simulation_thread, manager);
}


/* ════════════════════════════════════════════════════════════════════════════
   TOPIC 2: BACKGROUND SIMULATION THREAD
   ════════════════════════════════════════════════════════════════════════════
   
   Demonstrates:
   - Independent background thread execution
   - Concurrent OS operation simulation
   - CPU scheduling, memory allocation, page replacement, disk I/O, deadlock prevention
*/

/*
 * THREAD FUNCTION: background_simulation_thread()
 * 
 * Purpose: Simulates OS operations in background while game runs
 * - CPU scheduling operations
 * - Memory allocation/deallocation
 * - Page faults and page replacement
 * - Disk I/O scheduling
 * - Banker's algorithm resource requests
 * 
 * Runs continuously until engine.sim_active is set to 0
 * Updates every 500ms
 */

void* background_simulation_thread(void* arg) {
    (void)arg;  // Unused parameter
    
    int cycle = 0;
    while (engine.sim_active) {
        cycle++;
        
        // Simulate CPU scheduling (every 2 cycles)
        if (engine.scheduler && cycle % 2 == 0) {
            engine.cpu_operations_count++;
            int page_num = rand() % 256;
            
            // Simulate page access
            if (engine.paging) {
                page_system_access(engine.paging, page_num);
                engine.page_faults_count += engine.paging->page_faults;
            }
        }
        
        // Simulate memory allocation (every 3 cycles)
        if (engine.memory && cycle % 3 == 0) {
            int size = 64 + (rand() % 256);
            int pid = 100 + (rand() % 10);
            memory_allocate(engine.memory, pid, size);
            engine.memory_allocations_count++;
        }
        
        // Simulate disk I/O (every 4 cycles)
        if (engine.disk_sched && cycle % 4 == 0) {
            int track = rand() % 200;
            disk_scheduler_add_request(engine.disk_sched, track);
            engine.disk_operations_count++;
        }
        
        // Simulate banker's algorithm resource requests (every 5 cycles)
        if (engine.banker && cycle % 5 == 0) {
            int pid = 1 + (rand() % 3);
            int resource = rand() % 3;
            int amount = 1 + (rand() % 3);
            banker_request_resource(engine.banker, pid, resource, amount);
        }
        
        // Run silently - only display metrics when user chooses System Monitor
        usleep(500000);  // 500ms cycle time
    }
    
    return NULL;
}

/*
 * START BACKGROUND SIMULATION: start_background_simulation()
 * 
 * Initializes and starts the background OS operations thread.
 * Called during game engine initialization.
 */

void start_background_simulation(void) {
    engine.sim_active = 1;
    engine.cpu_operations_count = 0;
    engine.page_faults_count = 0;
    
    // SYSTEM CALL: pthread_create()
    // Creates background simulation thread
    if (pthread_create(&engine.sim_thread, NULL, background_simulation_thread, NULL) != 0) {
        perror("Failed to create background simulation thread");
    }
}


/* ════════════════════════════════════════════════════════════════════════════
   TOPIC 3: EVENT CONSUMER THREAD (PRODUCER-CONSUMER PATTERN)
   ════════════════════════════════════════════════════════════════════════════
   
   Demonstrates:
   - Synchronization with semaphores (sem_wait, sem_post)
   - Mutex locks for critical sections
   - Producer-Consumer pattern
   - Thread-safe circular buffer
   - Asynchronous event logging
*/

/*
 * THREAD FUNCTION: event_consumer_worker()
 * 
 * Purpose: Consumer thread in producer-consumer pattern
 * Consumes events from shared queue and writes them to log file
 * 
 * Synchronization Pattern:
 * 1. sem_wait(&full_slots) - Wait if buffer empty
 * 2. pthread_mutex_lock() - Enter critical section
 * 3. Extract event from buffer, update head/count
 * 4. pthread_mutex_unlock() - Exit critical section
 * 5. sem_post(&empty_slots) - Signal slot is now empty
 * 6. Process event outside critical section
 * 
 * This pattern prevents:
 * - Race conditions (mutex)
 * - Producer overflow (empty_slots semaphore)
 * - Consumer starvation (full_slots semaphore)
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
        // SEMAPHORE: Wait for event to be available
        // Blocks if buffer is empty (full_slots = 0)
        // Unblocks when producer signals (full_slots++)
        if (sem_wait(&eq->full_slots) == -1) break;
        
        if (!eq->active) break;
        
        // MUTEX: Enter critical section for buffer access
        pthread_mutex_lock(&eq->event_mutex);
        
        // Get event from circular buffer
        GameEvent event = eq->events[eq->head];
        eq->head = (eq->head + 1) % MAX_GAME_EVENTS;
        eq->count--;
        
        // MUTEX: Exit critical section
        pthread_mutex_unlock(&eq->event_mutex);
        
        // SEMAPHORE: Signal that a slot is now empty
        // Producer can now use this slot if waiting
        sem_post(&eq->empty_slots);
        
        // Process event OUTSIDE critical section (no blocking)
        const char* severity_str[] = {"[INFO]", "[WARN]", "[CRIT]"};
        fprintf(event_log, "%s %s Level %d: %s (ID:%d)\n",
                severity_str[event.severity],
                ctime(&event.timestamp),
                event.level,
                event.message,
                event.event_id);
        fflush(event_log);
        
        // Small delay to allow other threads to work
        usleep(10000);  // 10ms
    }
    
    fclose(event_log);
    return NULL;
}

/*
 * PRODUCER FUNCTION: event_produce()
 * 
 * Called by main game loop to add events to queue
 * 
 * Synchronization Pattern:
 * 1. sem_wait(&empty_slots) - Wait if buffer full
 * 2. pthread_mutex_lock() - Enter critical section
 * 3. Add event to buffer, update tail/count
 * 4. pthread_mutex_unlock() - Exit critical section
 * 5. sem_post(&full_slots) - Signal new event available
 */

void event_produce(EventQueue* eq, const char* msg, int level, int severity) {
    if (!eq) return;
    
    // SEMAPHORE: Wait for empty slot in buffer
    // Blocks if buffer is full (empty_slots = 0)
    sem_wait(&eq->empty_slots);
    
    // MUTEX: Enter critical section
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
    
    // MUTEX: Exit critical section
    pthread_mutex_unlock(&eq->event_mutex);
    
    // SEMAPHORE: Signal that an event is available
    // Consumer wakes up if waiting on full_slots
    sem_post(&eq->full_slots);
}


/* ════════════════════════════════════════════════════════════════════════════
   TOPIC 4: ADMIN PANEL - VIEW ACTIVE SESSIONS WITH PROCESS MANAGEMENT
   ════════════════════════════════════════════════════════════════════════════
   
   Demonstrates System Calls:
   - getpid() - Get current process ID
   - fork() - Create child processes (simulated in player spawning)
   - kill() - Send signals to processes (SIGTERM, SIGKILL, SIGUSR1, SIGUSR2)
   
   This is an interactive menu accessed by admin users.
   Shows 8 sub-options for process management and session monitoring.
*/

/*
 * ADMIN PANEL: Case 5 - View Active Sessions & Process Management
 * 
 * The code below is the actual implementation from login_system.c
 * Shows how system calls are integrated into the game interface.
 */

// case 5: {
//     printf("\n╔════════════════════════════════════════════════════╗\n");
//     printf("║       VIEW ACTIVE SESSIONS & PROCESS MANAGEMENT     ║\n");
//     printf("║            (System Calls Demo)                      ║\n");
//     printf("╚════════════════════════════════════════════════════╝\n\n");
//     
//     if (!global_active_sessions) {
//         printf("❌ No active sessions manager available.\n");
//         break;
//     }
//     
//     int sessions_menu = 1;
//     while (sessions_menu) {
//         printf("\n╔════════════════════════════════════════════════════╗\n");
//         printf("║         ACTIVE SESSIONS MENU                       ║\n");
//         printf("╚════════════════════════════════════════════════════╝\n\n");
//         printf("  1. View Active Processes (getpid demo)\n");
//         printf("  2. Send SIGTERM to User (graceful shutdown)\n");
//         printf("  3. Send SIGKILL to User (forced termination)\n");
//         printf("  4. Send SIGUSR1 to User (custom signal)\n");
//         printf("  5. Send SIGUSR2 to User (custom signal)\n");
//         printf("  6. View Player Details\n");
//         printf("  7. Watch Live Updates (10 cycles)\n");
//         printf("  8. Back to Admin Menu\n\n");
//         printf("Select [1-8]: ");
//         
//         char sessions_choice[10];
//         if (fgets(sessions_choice, sizeof(sessions_choice), stdin) == NULL) continue;
//         int sessions_option = atoi(sessions_choice);
//         
//         switch (sessions_option) {
//             
//             // ═══════════════════════════════════════════════════════════════
//             // OPTION 1: View Active Processes (getpid demo)
//             // ═══════════════════════════════════════════════════════════════
//             case 1: {
//                 printf("\n[SYSTEM CALL: getpid()] Retrieving Active User Processes:\n");
//                 printf("────────────────────────────────────────────────────\n");
//                 
//                 pthread_mutex_lock(&global_active_sessions->sessions_lock);
//                 
//                 // SYSTEM CALL: getpid()
//                 // Returns the process ID of the calling process (admin process)
//                 printf("Admin Process PID: %d\n", getpid());
//                 
//                 printf("\nActive User Processes:\n");
//                 printf("%-5s | %-20s | %-10s | %-15s\n", "ID", "Username", "PID*", "Status");
//                 printf("─────┼──────────────────────┼────────────┼─────────────────\n");
//                 
//                 for (int i = 0; i < global_active_sessions->active_count; i++) {
//                     if (global_active_sessions->sessions[i].is_active) {
//                         printf("%-5d | %-20s | %-10d | %-15s\n",
//                             global_active_sessions->sessions[i].player_id,
//                             global_active_sessions->sessions[i].username,
//                             1000 + i,  // Simulated PID from fork()
//                             "Running");
//                     }
//                 }
//                 printf("────────────────────────────────────────────────────\n");
//                 printf("* PIDs are from fork() system calls during process spawning\n\n");
//                 
//                 pthread_mutex_unlock(&global_active_sessions->sessions_lock);
//                 break;
//             }
//             
//             // ═══════════════════════════════════════════════════════════════
//             // OPTIONS 2-5: Send Signals (SIGTERM, SIGKILL, SIGUSR1, SIGUSR2)
//             // ═══════════════════════════════════════════════════════════════
//             case 2:
//             case 3:
//             case 4:
//             case 5: {
//                 printf("Enter username to send signal: ");
//                 char username[MAX_USERNAME_LEN];
//                 if (fgets(username, sizeof(username), stdin) != NULL) {
//                     username[strcspn(username, "\n")] = 0;
//                     
//                     pthread_mutex_lock(&global_active_sessions->sessions_lock);
//                     int player_id = -1;
//                     int pid_to_signal = -1;
//                     
//                     for (int i = 0; i < global_active_sessions->active_count; i++) {
//                         if (global_active_sessions->sessions[i].is_active &&
//                             strcmp(global_active_sessions->sessions[i].username, username) == 0) {
//                             player_id = global_active_sessions->sessions[i].player_id;
//                             pid_to_signal = 1000 + i;  // Simulated PID from fork()
//                             break;
//                         }
//                     }
//                     pthread_mutex_unlock(&global_active_sessions->sessions_lock);
//                     
//                     if (player_id != -1) {
//                         int signal_num = 0;
//                         const char* signal_name = "";
//                         
//                         switch (sessions_option) {
//                             case 2:
//                                 signal_num = SIGTERM;
//                                 signal_name = "SIGTERM (15)";
//                                 break;
//                             case 3:
//                                 signal_num = SIGKILL;
//                                 signal_name = "SIGKILL (9)";
//                                 break;
//                             case 4:
//                                 signal_num = SIGUSR1;
//                                 signal_name = "SIGUSR1 (10)";
//                                 break;
//                             case 5:
//                                 signal_num = SIGUSR2;
//                                 signal_name = "SIGUSR2 (12)";
//                                 break;
//                         }
//                         
//                         printf("\n[SYSTEM CALL: kill()] Sending Signal\n");
//                         printf("Target User: %s\n", username);
//                         printf("Target PID: %d (from fork())\n", pid_to_signal);
//                         printf("Signal: %s\n", signal_name);
//                         
//                         // SYSTEM CALL: kill(pid, signal)
//                         // Sends specified signal to process with given PID
//                         // SIGTERM: Graceful termination (can be caught)
//                         // SIGKILL: Forced termination (cannot be caught)
//                         // SIGUSR1: User-defined signal 1
//                         // SIGUSR2: User-defined signal 2
//                         if (kill(pid_to_signal, signal_num) == 0) {
//                             printf("✓ Signal %s sent successfully to PID %d\n\n", signal_name, pid_to_signal);
//                         } else {
//                             printf("⚠ Process %d may not exist (expected in simulation)\n\n", pid_to_signal);
//                         }
//                         
//                         // If SIGKILL, remove from active sessions
//                         if (sessions_option == 3) {
//                             active_sessions_remove_player(global_active_sessions, player_id);
//                             printf("Process removed from active sessions.\n");
//                         }
//                     } else {
//                         printf("❌ User '%s' not found in active sessions.\n", username);
//                     }
//                 }
//                 break;
//             }
//             
//             // ═══════════════════════════════════════════════════════════════
//             // OPTION 6: View Player Details
//             // ═══════════════════════════════════════════════════════════════
//             case 6: {
//                 printf("Enter player ID to view details: ");
//                 char pid_str[10];
//                 if (fgets(pid_str, sizeof(pid_str), stdin) != NULL) {
//                     int player_id = atoi(pid_str);
//                     active_sessions_print_player_details(global_active_sessions, player_id);
//                 }
//                 break;
//             }
//             
//             // ═══════════════════════════════════════════════════════════════
//             // OPTION 7: Watch Live Updates
//             // ═══════════════════════════════════════════════════════════════
//             case 7: {
//                 printf("\n");
//                 active_sessions_print_table(global_active_sessions);
//                 printf("Watching live updates (10 cycles, 2 seconds apart):\n\n");
//                 active_sessions_display_live(global_active_sessions);
//                 break;
//             }
//             
//             // ═══════════════════════════════════════════════════════════════
//             // OPTION 8: Back to Admin Menu
//             // ═══════════════════════════════════════════════════════════════
//             case 8:
//                 sessions_menu = 0;
//                 break;
//                 
//             default:
//                 printf("❌ Invalid option.\n");
//         }
//     }
//     break;
// }


/* ════════════════════════════════════════════════════════════════════════════
   SUMMARY & INTEGRATION
   ════════════════════════════════════════════════════════════════════════════
   
   How These 4 Concepts Work Together:
   
   1. ACTIVE SESSIONS THREAD:
      - Runs in background thread (pthread_create)
      - Updates player states every 2 seconds
      - Protected by mutex for thread safety
      - Simulates real player activity
   
   2. BACKGROUND SIMULATION THREAD:
      - Another independent background thread
      - Simulates OS operations (CPU, memory, disk, etc.)
      - Runs with 500ms cycle time
      - Generates metrics for admin monitoring
   
   3. EVENT CONSUMER THREAD:
      - Third background thread for logging
      - Producer-Consumer pattern prevents blocking
      - Uses semaphores to coordinate buffer access
      - Writes all game events to log file
   
   4. ADMIN PANEL PROCESS MANAGEMENT:
      - Main game thread runs admin menu when admin logs in
      - Displays process information using getpid()
      - Sends signals to simulated processes using kill()
      - Shows live session data from Thread #1
      - All while Threads #1, #2, #3 continue running
   
   Result: Concurrent multithreaded application with proper synchronization
*/
