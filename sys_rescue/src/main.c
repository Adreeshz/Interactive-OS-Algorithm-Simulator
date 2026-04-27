#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <poll.h>
#include <semaphore.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>
#include <termios.h>
#include "sync_engine.h"
#include "scheduler.h"
#include "game_infrastructure.h"
#include "user_management.h"
#include "question_pool.h"
#include "login_system.h"
#include "algorithms.h"

#define GAME_TITLE "Interactive OS Algorithm Simulator"
#define GAME_VERSION "2.0.0"
#define LEVEL_TIME_LIMIT 1800  // 30 minutes per level

/* ============================================
   GLOBAL GAME STATE
   ============================================ */

/* ============================================
   GAME STATE STRUCTURE
   ============================================ */

typedef struct {
    GameSession* session;
    QuestionPool* question_pool;
    int current_level;
    int score;
    int levels_completed;
    int time_remaining;
    int time_up;
    pthread_mutex_t timer_lock;
    pthread_t timer_thread;
    
    // Algorithms for demonstration
    BankersAlgorithm* banker;
    Scheduler* scheduler;
    MemoryManager* memory;
    PageReplacementSystem* paging;
    DiskScheduler* disk_sched;
    
    // Background simulation
    int sim_active;
    pthread_t sim_thread;
    int cpu_operations_count;
    int page_faults_count;
    int disk_operations_count;
    int memory_allocations_count;
    
    // Producer-Consumer: Event Logging
    EventQueue* event_queue;
} GameEngine;

GameEngine engine;

/* ============================================
   NOTE: Event logging using EventQueue from game_infrastructure.h
   EventQueue is initialized and managed by game_infrastructure module
   ============================================ */

/* ============================================
   TIMER FUNCTIONS
   ============================================ */

void* global_timer_func(void* arg) {
    (void)arg;
    
    while (engine.time_remaining > 0 && !engine.time_up) {
        sleep(1);
        pthread_mutex_lock(&engine.timer_lock);
        if (engine.time_remaining > 0) {
            engine.time_remaining--;
        }
        if (engine.time_remaining == 0) {
            engine.time_up = 1;
        }
        pthread_mutex_unlock(&engine.timer_lock);
    }
    return NULL;
}

int get_time_remaining(void) {
    pthread_mutex_lock(&engine.timer_lock);
    int time = engine.time_remaining;
    pthread_mutex_unlock(&engine.timer_lock);
    return time;
}

int is_time_up(void) {
    pthread_mutex_lock(&engine.timer_lock);
    int up = engine.time_up;
    pthread_mutex_unlock(&engine.timer_lock);
    return up;
}

/* ============================================
   BACKGROUND OS SIMULATION THREAD
   ============================================ */

void* background_simulation_thread(void* arg) {
    (void)arg;
    
    int cycle = 0;
    while (engine.sim_active) {
        cycle++;
        
        // Simulate CPU scheduling
        if (engine.scheduler && cycle % 2 == 0) {
            engine.cpu_operations_count++;
            int page_num = rand() % 256;
            
            // Simulate page access
            if (engine.paging) {
                page_system_access(engine.paging, page_num);
                engine.page_faults_count += engine.paging->page_faults;
            }
        }
        
        // Simulate memory allocation
        if (engine.memory && cycle % 3 == 0) {
            int size = 64 + (rand() % 256);
            int pid = 100 + (rand() % 10);
            memory_allocate(engine.memory, pid, size);
            engine.memory_allocations_count++;
        }
        
        // Simulate disk I/O
        if (engine.disk_sched && cycle % 4 == 0) {
            int track = rand() % 200;
            disk_scheduler_add_request(engine.disk_sched, track);
            engine.disk_operations_count++;
        }
        
        // Simulate banker's algorithm resource requests
        if (engine.banker && cycle % 5 == 0) {
            int pid = 1 + (rand() % 3);
            int resource = rand() % 3;
            int amount = 1 + (rand() % 3);
            banker_request_resource(engine.banker, pid, resource, amount);
        }
        
        // Run silently - only display metrics when user chooses System Monitor
        usleep(500000);  // 500ms cycle
    }
    
    return NULL;
}

void start_background_simulation(void) {
    engine.sim_active = 1;
    engine.cpu_operations_count = 0;
    engine.page_faults_count = 0;
    engine.disk_operations_count = 0;
    engine.memory_allocations_count = 0;
    
    if (pthread_create(&engine.sim_thread, NULL, background_simulation_thread, NULL) != 0) {
        printf("❌ Failed to start background simulation\n");
        engine.sim_active = 0;
    }
}

void stop_background_simulation(void) {
    if (engine.sim_active) {
        engine.sim_active = 0;
        if (engine.sim_thread != 0) {
            pthread_join(engine.sim_thread, NULL);
        }
    }
}

/* ============================================
   UTILITY FUNCTIONS
   ============================================ */

void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void display_header(const char* title) {
    clear_screen();
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║  %-50s║\n", GAME_TITLE);
    printf("║  %-50s║\n", GAME_VERSION);
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    if (title) {
        printf("╔════════════════════════════════════════════════════╗\n");
        printf("║  %-50s║\n", title);
        printf("╚════════════════════════════════════════════════════╝\n\n");
    }
}

void display_timer(void) {
    int remaining = get_time_remaining();
    int minutes = remaining / 60;
    int seconds = remaining % 60;
    
    if (remaining > 300) {
        printf("⏱️  %02d:%02d remaining\n", minutes, seconds);
    } else if (remaining > 60) {
        printf("⚠️  %02d:%02d remaining\n", minutes, seconds);
    } else if (remaining > 0) {
        printf("🔴 %02d:%02d remaining - HURRY!\n", minutes, seconds);
    }
    fflush(stdout);
}

void press_any_key(void) {
    printf("\n>>> Press ENTER to continue <<<\n");
    getchar();
}

/* ============================================
   LOGIN SCREENS
   ============================================ */

void display_login_menu(void) {
    display_header("🔐 LOGIN SYSTEM");
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  1. Login                                         ║\n");
    printf("║  2. Register New Account                          ║\n");
    printf("║  3. Exit                                          ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

int login_flow(void) {
    while (1) {
        display_login_menu();
        printf("Select option [1-3]: ");
        
        char choice[10];
        if (fgets(choice, sizeof(choice), stdin) == NULL) continue;
        
        int option = atoi(choice);
        
        switch (option) {
            case 1: {
                display_header("🔐 USER LOGIN");
                printf("Username: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) == NULL) break;
                username[strcspn(username, "\n")] = 0;
                
                printf("Password: ");
                char password[MAX_PASSWORD_LEN];
                if (fgets(password, sizeof(password), stdin) == NULL) break;
                password[strcspn(password, "\n")] = 0;
                
                int result = login_system_login(engine.session, username, password);
                if (result == 0) {
                    printf("✅ Login successful! Welcome, %s!\n", username);
                    sleep(2);
                    return 1;  // Success
                } else {
                    printf("❌ Login failed (error code: %d)\n", result);
                    printf("   0: User banned\n   -2: User not found\n   -4: Wrong password\n");
                    sleep(2);
                }
                break;
            }
            
            case 2: {
                display_header("📝 USER REGISTRATION");
                printf("Username: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) == NULL) break;
                username[strcspn(username, "\n")] = 0;
                
                printf("Password: ");
                char password[MAX_PASSWORD_LEN];
                if (fgets(password, sizeof(password), stdin) == NULL) break;
                password[strcspn(password, "\n")] = 0;
                
                int result = login_system_register(engine.session, username, password);
                if (result == 0) {
                    printf("✅ Registration successful! Please login.\n");
                    sleep(2);
                } else {
                    printf("❌ Registration failed (error code: %d)\n", result);
                    sleep(2);
                }
                break;
            }
            
            case 3:
                printf("Goodbye!\n");
                return 0;  // Exit
                
            default:
                printf("❌ Invalid option.\n");
        }
    }
    return 0;
}

/* ============================================
   LEVEL CONFIGURATION
   ============================================ */

typedef struct {
    int level_id;
    const char* level_name;
    int time_limit;           // in seconds
    int questions_required;   // 5 questions per level
} LevelConfig;

// Level configurations: time increases with difficulty
LevelConfig level_configs[] = {
    {0, "Linux Commands & Shell Scripting", 15*60, 5},      // 15 mins
    {1, "System Calls", 20*60, 5},                          // 20 mins
    {2, "Synchronization", 25*60, 5},                       // 25 mins
    {3, "CPU Scheduling", 30*60, 5},                        // 30 mins
    {4, "Banker's Algorithm & Deadlock", 35*60, 5},         // 35 mins
    {5, "Memory Management & Disk Scheduling", 40*60, 5},   // 40 mins
    {6, "Page Replacement & Virtual Memory", 45*60, 5}      // 45 mins
};

#define NUM_LEVELS 7

/* ============================================
   LEVEL QUESTION SYSTEM
   ============================================ */

void run_level_with_questions(int level_id) {
    if (level_id < 0 || level_id >= NUM_LEVELS) return;
    
    LevelConfig config = level_configs[level_id];
    
    char header[100];
    snprintf(header, sizeof(header), "LEVEL %d: %s", level_id, config.level_name);
    display_header(header);
    
    printf("📚 TIME LIMIT: %d minutes\n", config.time_limit / 60);
    printf("📝 QUESTIONS: %d\n", config.questions_required);
    printf("🎯 OBJECTIVE: Answer all %d questions correctly before time runs out!\n\n", config.questions_required);
    printf("Press ENTER to start...");
    getchar();
    
    // Initialize level timer
    engine.time_remaining = config.time_limit;
    engine.time_up = 0;
    
    // Start timer thread
    pthread_create(&engine.timer_thread, NULL, global_timer_func, NULL);
    
    DifficultyLevel current_difficulty = BEGINNER;
    int questions_answered = 0;
    int correct_answers = 0;
    int level_score = 0;
    
    // Track asked questions to prevent repetition
    int asked_question_ids[100];
    int asked_count = 0;
    memset(asked_question_ids, 0, sizeof(asked_question_ids));
    
    clear_screen();
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║  LEVEL %d: %s\n", level_id, config.level_name);
    printf("║  Answer %d questions to complete this level\n", config.questions_required);
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    // Ask questions until either completed or time runs out
    while (questions_answered < config.questions_required && !is_time_up()) {
        printf("📋 Question %d/%d\n", questions_answered + 1, config.questions_required);
        printf("Difficulty: %s | Time: %d:%02d remaining\n\n",
               current_difficulty == BEGINNER ? "🔹 BEGINNER" :
               current_difficulty == INTERMEDIATE ? "🟡 INTERMEDIATE" :
               current_difficulty == ADVANCED ? "🔶 ADVANCED" : "🔴 PROFICIENT",
               get_time_remaining() / 60, get_time_remaining() % 60);
        
        // Get question (with repetition prevention)
        Question* q = NULL;
        int attempts = 0;
        while (q == NULL && attempts < 20) {
            q = question_get_random(engine.question_pool, level_id, current_difficulty);
            if (q == NULL) {
                printf("❌ No questions available at this difficulty!\n");
                break;
            }
            
            // Check if this question was already asked
            int already_asked = 0;
            for (int i = 0; i < asked_count; i++) {
                if (asked_question_ids[i] == q->id) {
                    already_asked = 1;
                    break;
                }
            }
            
            if (already_asked) {
                q = NULL;  // Force getting another question
                attempts++;
            }
        }
        
        if (!q) {
            printf("❌ No new questions available at this difficulty!\n");
            break;
        }
        
        // Add this question ID to the asked list
        if (asked_count < 100) {
            asked_question_ids[asked_count++] = q->id;
        }
        
        printf("%s\n", q->question);
        if (strlen(q->hint) > 0) {
            printf("💡 Hint: %s\n", q->hint);
        }
        printf("\nYour answer: ");
        fflush(stdout);
        
        char answer[MAX_ANSWER_LEN];
        if (fgets(answer, sizeof(answer), stdin) == NULL) {
            printf("❌ Error reading input.\n");
            continue;
        }
        
        // Check if time ran out during answer
        if (is_time_up()) {
            printf("\n⏰ TIME'S UP! Level failed - you ran out of time!\n");
            break;
        }
        
        answer[strcspn(answer, "\n")] = 0;
        
        // Check answer
        int is_correct = (strstr(q->answer, answer) != NULL || strstr(answer, q->answer) != NULL);
        
        if (is_correct) {
            printf("\n✅ CORRECT! (+%d points)\n\n", q->points);
            correct_answers++;
            level_score += q->points;
            
            // Increase difficulty if doing very well
            if (correct_answers > 0 && correct_answers % 2 == 0 && current_difficulty < PROFICIENT) {
                printf("🎉 Excellent progress! Moving to next difficulty level!\n\n");
                current_difficulty = difficulty_get_next(current_difficulty);
            }
        } else {
            printf("\n❌ INCORRECT. Correct answer: %s\n", q->answer);
            printf("⚠️  You need to answer correctly to proceed.\n\n");
            
            // Decrease difficulty if making mistakes
            if (questions_answered > 2 && current_difficulty > BEGINNER) {
                printf("📚 Let's review. Dropping to previous difficulty level.\n\n");
                current_difficulty = difficulty_get_previous(current_difficulty);
            }
            continue;  // Don't count this as a completed question
        }
        
        questions_answered++;
        sleep(1);  // Brief pause before next question
    }
    
    // Cancel timer thread
    pthread_cancel(engine.timer_thread);
    pthread_join(engine.timer_thread, NULL);
    
    // Check if level completed
    clear_screen();
    if (questions_answered == config.questions_required && correct_answers == config.questions_required) {
        printf("\n╔════════════════════════════════════════════════════╗\n");
        printf("║  🎉 LEVEL %d COMPLETE! 🎉\n", level_id);
        printf("║  You answered all questions correctly!\n");
        printf("║  Level Score: +%d points\n", level_score);
        printf("╚════════════════════════════════════════════════════╝\n\n");
        
        engine.score += level_score;
        engine.levels_completed++;
        
        // PRODUCER: Log level completion event
        char event_msg[EVENT_MESSAGE_SIZE];
        snprintf(event_msg, EVENT_MESSAGE_SIZE, "LEVEL_COMPLETE: Level %d completed with score +%d", level_id, level_score);
        event_produce(engine.event_queue, event_msg, level_id, 0);
    } else if (is_time_up()) {
        printf("\n╔════════════════════════════════════════════════════╗\n");
        printf("║  ⏰ TIME'S UP!\n");
        printf("║  Questions answered: %d/%d\n", questions_answered, config.questions_required);
        printf("║  Correct answers: %d/%d\n", correct_answers, questions_answered);
        printf("║  You must complete this level to proceed.\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");
        printf("Press ENTER to retry this level...");
        getchar();
        return;  // Level not completed, will retry
    } else {
        printf("\n╔════════════════════════════════════════════════════╗\n");
        printf("║  ❌ LEVEL FAILED\n");
        printf("║  Questions answered: %d/%d\n", questions_answered, config.questions_required);
        printf("║  You need to answer all questions correctly.\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");
        printf("Press ENTER to retry this level...");
        getchar();
        return;
    }
    
    printf("Press ENTER to continue...");
    getchar();
}

/* ============================================
   LEVEL 0: LINUX COMMANDS & SHELL SCRIPTING
   ============================================ */

void level_0_linux_commands(void) {
    run_level_with_questions(0);
}

/* ============================================
   LEVEL 1: SYSTEM CALLS
   ============================================ */

void level_1_system_calls(void) {
    run_level_with_questions(1);
}

/* ============================================
   LEVEL 2: SYNCHRONIZATION
   ============================================ */

void level_2_synchronization(void) {
    run_level_with_questions(2);
}

/* ============================================
   LEVEL 3: CPU SCHEDULING
   ============================================ */

void level_3_scheduling(void) {
    run_level_with_questions(3);
}

/* ============================================
   LEVEL 4: BANKER'S ALGORITHM
   ============================================ */

void level_4_bankers_algorithm(void) {
    run_level_with_questions(4);
}

/* ============================================
   LEVEL 5: MEMORY & DISK MANAGEMENT
   ============================================ */

void level_5_memory_disk(void) {
    run_level_with_questions(5);
}

/* ============================================
   LEVEL 6: PAGE REPLACEMENT & VIRTUAL MEMORY
   ============================================ */

void level_6_paging(void) {
    run_level_with_questions(6);
}

/* ============================================
   MAIN MENU
   ============================================ */

void display_main_menu(void) {
    display_header(NULL);
    
    printf("📊 USER: %s  |  🏆 SCORE: %d  |  ✅ LEVELS: %d/%d\n\n",
           engine.session->current_user->username,
           engine.score,
           engine.levels_completed,
           NUM_LEVELS);
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  1. Start Next Level                              ║\n");
    printf("║  2. View Statistics                               ║\n");
    printf("║  3. View Help                                     ║\n");
    printf("║  4. System Monitor (Background OS Simulation)     ║\n");
    printf("║  5. Admin Panel (if authorized)                   ║\n");
    printf("║  6. Logout                                        ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void display_help(void) {
    display_header("📖 HELP & GUIDE");
    
    printf("GAME OVERVIEW:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Interactive OS Algorithm Simulator is an interactive OS learning game.\n");
    printf("Answer questions to progress through 7 levels!\n\n");
    
    printf("LEVELS:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Level 0: Linux Commands & Shell Scripting (15 mins)\n");
    printf("Level 1: System Calls (20 mins)\n");
    printf("Level 2: Synchronization (25 mins)\n");
    printf("Level 3: CPU Scheduling (30 mins)\n");
    printf("Level 4: Banker's Algorithm & Deadlock (35 mins)\n");
    printf("Level 5: Memory Management & Disk Scheduling (40 mins)\n");
    printf("Level 6: Page Replacement & Virtual Memory (45 mins)\n\n");
    
    printf("DIFFICULTY SYSTEM:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🔹 Beginner  - Start here, basic concepts\n");
    printf("🟡 Intermediate - Applied knowledge\n");
    printf("🔶 Advanced - Complex scenarios\n");
    printf("🔴 Proficient - Expert level\n\n");
    
    printf("TIMER:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Each level has a strict time limit.\n");
    printf("If time runs out, you must restart the level.\n");
    printf("Time increases for more difficult levels!\n\n");
    
    press_any_key();
}

void display_game_statistics(void) {
    display_header("📊 YOUR STATISTICS");
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  OVERALL PROGRESS                                 ║\n");
    printf("║  Total Score: %d points                            ║\n", engine.score);
    printf("║  Levels Completed: %d/%d                           ║\n", engine.levels_completed, NUM_LEVELS);
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    press_any_key();
}

void display_system_monitor(void) {
    int monitoring = 1;
    int update_count = 0;

    // Use non-canonical mode for immediate key reads
    struct termios old_settings, new_settings;
    tcgetattr(STDIN_FILENO, &old_settings);
    new_settings = old_settings;
    new_settings.c_lflag &= ~(ICANON | ECHO);
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);

    // use poll + read for reliable non-blocking input
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    while (monitoring) {
        system("clear");
        display_header("SYSTEM MONITOR - LIVE OS SIMULATION");

        printf("Real-time OS Algorithm Activity (Press 'q' to exit):\n\n");

        printf("╔════════════════════════════════════════════════════╗\n");
        printf("║         LIVE BACKGROUND SIMULATION                 ║\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");

        printf("CPU SCHEDULING:\n");
        printf("   - Operations Executed: %d\n", engine.cpu_operations_count);
        printf("   - Algorithm: Round-Robin (RR)\n");
        printf("   - Time Quantum: 4ms\n\n");

        printf("MEMORY MANAGEMENT:\n");
        printf("   - Allocations Performed: %d\n", engine.memory_allocations_count);
        printf("   - Algorithm: Buddy System (First-Fit)\n");
        printf("   - Total Memory: 1024 KB\n");
        printf("   - Fragmentation: %.1f%%\n\n", (float)memory_get_fragmentation(engine.memory));

        printf("PAGE REPLACEMENT:\n");
        printf("   - Page Faults: %d\n", engine.page_faults_count);
        printf("   - Algorithm: LRU (Least Recently Used)\n");
        printf("   - Page Frames: 4\n");
        printf("   - Page Size: 4096 bytes\n\n");

        printf("DISK SCHEDULING:\n");
        printf("   - I/O Operations: %d\n", engine.disk_operations_count);
        printf("   - Algorithm: C-SCAN (Circular SCAN)\n");
        printf("   - Disk Tracks: 200\n\n");

        printf("SYNCHRONIZATION:\n");
        printf("   - Active Mutexes: 4 (users, banker, scheduler, memory)\n");
        printf("   - Synchronization Model: POSIX Threads\n");
        printf("   - Deadlock Prevention: Banker's Algorithm\n\n");

        printf("═══════════════════════════════════════════════════════\n");
        printf("Update #%d (Next update in 2 seconds... Press 'q' to exit)\n", ++update_count);
        printf("═══════════════════════════════════════════════════════\n");
        fflush(stdout);

        // Wait up to ~2 seconds but check input frequently
        for (int i = 0; i < 20; i++) {
            usleep(100000);

            int poll_count = poll(&pfd, 1, 0);
            if (poll_count > 0 && (pfd.revents & POLLIN)) {
                char buf[16];
                ssize_t r = read(STDIN_FILENO, buf, sizeof(buf));
                if (r > 0) {
                    for (ssize_t k = 0; k < r; k++) {
                        if (buf[k] == 'q' || buf[k] == 'Q') {
                            monitoring = 0;
                            break;
                        }
                    }
                }
            }

            if (!monitoring) break;
        }
    }

    // Restore terminal settings and clear input buffer
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
    tcflush(STDIN_FILENO, TCIFLUSH);
    fflush(stdout);
}

/* ============================================
   MAIN GAME LOOP
   ============================================ */

void game_loop(void) {
    while (engine.levels_completed < NUM_LEVELS) {
        display_main_menu();
        
        printf("Select option [1-6]: ");
        char choice[10];
        if (fgets(choice, sizeof(choice), stdin) == NULL) continue;
        
        int option = atoi(choice);
        
        switch (option) {
            case 1:
                switch (engine.levels_completed) {
                    case 0: level_0_linux_commands(); break;
                    case 1: level_1_system_calls(); break;
                    case 2: level_2_synchronization(); break;
                    case 3: level_3_scheduling(); break;
                    case 4: level_4_bankers_algorithm(); break;
                    case 5: level_5_memory_disk(); break;
                    case 6: level_6_paging(); break;
                }
                break;
                
            case 2:
                display_game_statistics();
                break;
                
            case 3:
                display_help();
                break;
                
            case 4:
                display_system_monitor();
                break;
                
            case 5:
                if (login_system_is_admin(engine.session)) {
                    login_system_admin_panel(engine.session);
                } else {
                    printf("❌ Admin access denied.\n");
                    sleep(2);
                }
                break;
                
            case 6:
                printf("Logging out...\n");
                fflush(stdout);
                tcflush(STDIN_FILENO, TCIFLUSH);  // Clear input buffer before logout
                login_system_logout(engine.session);
                return;

                
            default:
                printf("❌ Invalid option.\n");
        }
    }
    
    display_header("🎉 VICTORY!");
    printf("You have completed all %d levels!\n", NUM_LEVELS);
    printf("Final Score: %d points\n\n", engine.score);
    printf("Congratulations on mastering OS concepts!\n");
    
    press_any_key();
}

/* ============================================
   MAIN ENTRY POINT
   ============================================ */

int main(void) {
    srand(time(NULL));
    
    // Initialize game engine
    engine.session = login_system_init();
    engine.question_pool = question_pool_init();
    engine.current_level = 0;
    engine.score = 0;
    engine.levels_completed = 0;
    engine.time_remaining = 0;
    engine.time_up = 0;
    
    pthread_mutex_init(&engine.timer_lock, NULL);
    
    // Initialize Producer-Consumer: Event Queue
    engine.event_queue = event_queue_init();
    if (engine.event_queue) {
        if (pthread_create(&engine.event_queue->consumer_thread, NULL, event_consumer_worker, (void*)engine.event_queue) != 0) {
            printf("❌ Failed to start event consumer thread\n");
        }
    }
    
    // Load questions from file
    question_pool_load_from_file(engine.question_pool, "data/questions.txt");
    
    // Initialize algorithms
    engine.banker = banker_init(3);  // 3 resources
    engine.scheduler = scheduler_init(SCHED_RR_ALG, 4);  // Round-robin with quantum 4
    engine.memory = memory_init(1024, PLACE_BEST_FIT);  // 1KB memory, best-fit
    engine.paging = page_system_init(4, PAGE_LRU);  // 4 frames, LRU
    engine.disk_sched = disk_scheduler_init(200, DISK_CSCAN);  // 200 tracks, C-SCAN
    
    // Start interactive engine: show login screen and run sessions until user exits
    while (login_flow()) {
        // Start background simulation for the authenticated session
        start_background_simulation();

        // Main game loop
        game_loop();

        // Stop background simulation after logout or exit from game loop
        stop_background_simulation();
    }
    
    // Cleanup
    login_system_destroy(engine.session);
    question_pool_destroy(engine.question_pool);
    
    // Destroy Producer-Consumer: Event Queue
    if (engine.event_queue) {
        event_queue_destroy(engine.event_queue);
    }
    
    banker_destroy(engine.banker);
    scheduler_destroy(engine.scheduler);
    memory_destroy(engine.memory);
    page_system_destroy(engine.paging);
    disk_scheduler_destroy(engine.disk_sched);
    pthread_mutex_destroy(&engine.timer_lock);
    
    printf("Thanks for playing Interactive OS Algorithm Simulator!\n");
    
    return 0;
}
