#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include "sync_engine.h"
#include "scheduler.h"
#include "game_infrastructure.h"
#include "user_management.h"
#include "question_pool.h"
#include "login_system.h"
#include "algorithms.h"

#define GAME_TITLE "SYS_RESCUE: Interactive OS Algorithm Simulator"
#define GAME_VERSION "1.0.0"

/* ============================================
   GAME STATE
   ============================================ */

typedef struct {
    int current_level;
    int score;
    int time_remaining;
    int levels_completed;
    GameState* game_state;
    LogBuffer* log;
    pthread_t timer_thread;
    int time_up;
    pthread_mutex_t timer_lock;
    // Level-specific timer
    int level_time_remaining;
    int level_time_up;
    pthread_t level_timer_thread;
    int show_level_timer;
    
    // Synchronization-based infrastructure
    GameInfrastructure* infrastructure;
    
    // Login and user management
    GameSession* session;
    QuestionPool* question_pool;
    
    // Algorithms
    BankersAlgorithm* banker;
    Scheduler* scheduler;
    MemoryManager* memory;
    PageReplacementSystem* paging;
    DiskScheduler* disk_sched;
} GameEngine;

GameEngine engine;

/* ============================================
   TIMER THREAD
   ============================================ */

void* timer_thread_func(void* arg) {
    (void)arg;  // Unused parameter
    
    while (engine.time_remaining > 0 && !engine.time_up) {
        sleep(1);  // Decrement every second
        
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
   LEVEL TIMER THREAD (1 minute per level)
   ============================================ */

void* level_timer_thread_func(void* arg) {
    (void)arg;
    
    while (engine.level_time_remaining > 0 && !engine.level_time_up && engine.show_level_timer) {
        sleep(1);
        
        pthread_mutex_lock(&engine.timer_lock);
        if (engine.level_time_remaining > 0) {
            engine.level_time_remaining--;
        }
        
        if (engine.level_time_remaining == 0) {
            engine.level_time_up = 1;
        }
        pthread_mutex_unlock(&engine.timer_lock);
    }
    
    return NULL;
}

void display_level_timer(void) {
    pthread_mutex_lock(&engine.timer_lock);
    int time_left = engine.level_time_remaining;
    pthread_mutex_unlock(&engine.timer_lock);
    
    int seconds = time_left % 60;
    
    if (time_left > 20) {
        printf("                                                 ⏱️  %02d:%02d\n", time_left / 60, seconds);
    } else if (time_left > 0) {
        printf("                                                 ⚠️  %02d:%02d\n", time_left / 60, seconds);
    } else {
        printf("                                                 💥 TIME UP!\n");
    }
    fflush(stdout);
}

void start_level_timer(int seconds) {
    pthread_mutex_lock(&engine.timer_lock);
    engine.level_time_remaining = seconds;
    engine.level_time_up = 0;
    engine.show_level_timer = 1;
    pthread_mutex_unlock(&engine.timer_lock);
    
    pthread_create(&engine.level_timer_thread, NULL, level_timer_thread_func, NULL);
}

void stop_level_timer(void) {
    pthread_mutex_lock(&engine.timer_lock);
    engine.show_level_timer = 0;
    pthread_mutex_unlock(&engine.timer_lock);
    
    pthread_join(engine.level_timer_thread, NULL);
}

int is_level_time_up(void) {
    pthread_mutex_lock(&engine.timer_lock);
    int up = engine.level_time_up;
    pthread_mutex_unlock(&engine.timer_lock);
    return up;
}

int get_level_time_remaining(void) {
    pthread_mutex_lock(&engine.timer_lock);
    int time = engine.level_time_remaining;
    pthread_mutex_unlock(&engine.timer_lock);
    return time;
}

/* ============================================
   UTILITY FUNCTIONS
   ============================================ */

void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void print_separator(char c, int width) {
    for (int i = 0; i < width; i++) printf("%c", c);
    printf("\n");
}

void press_any_key(void) {
    printf("\n>>> Press ENTER to continue <<<\n");
    getchar();
}

int get_user_choice(int min, int max) {
    int choice;
    char input[100];
    
    while (1) {
        // Check if time is up
        if (engine.show_level_timer && is_level_time_up()) {
            return -1;  // Return -1 to signal time's up
        }
        
        // Display timer
        if (engine.show_level_timer) {
            display_level_timer();
        }
        
        printf("\nEnter your choice [%d-%d]: ", min, max);
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) != NULL) {
            choice = atoi(input);
            if (choice >= min && choice <= max) {
                return choice;
            }
        }
        printf("❌ Invalid choice. Please try again.\n");
    }
}

void print_game_header(void) {
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║  %-50s║\n", GAME_TITLE);
    printf("║  %-50s║\n", GAME_VERSION);
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void print_game_status(void) {
    printf("\n📊 MAINFRAME STATUS\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🎮 Current Level:     %d\n", engine.current_level);
    printf("🏆 Score:            %d\n", engine.score);
    printf("✅ Levels Completed: %d/4\n", engine.levels_completed);
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

/* ============================================
   LEVEL IMPLEMENTATIONS
   ============================================ */

void level_0_terminal_boot(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  LEVEL 0: FILE SYSTEM RECOVERY                    ║\n");
    printf("║  (File Permissions & Ownership)                   ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("🔴 CRITICAL: You accidentally deleted audit logs!\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("Fortunately, you have a backup draft copy.\n");
    printf("But the system owner controls the actual log file.\n\n");
    
    start_level_timer(60);  // 60 seconds for this level
    
    char user_input[256];
    int task = 1;
    int correct_answers = 0;
    
    // TASK 1: Protect the draft file from deletion
    while (task == 1 && !is_level_time_up()) {
        display_level_timer();
        printf("\n📋 TASK 1: Protect the Draft\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Your draft file (draft_audit.log) contains important data.\n");
        printf("You need to REMOVE write permissions so it cannot be deleted.\n\n");
        printf("What command protects the file from deletion?\n");
        printf("(Hint: chmod command to remove write permissions)\n");
        printf(">>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            
            user_input[strcspn(user_input, "\n")] = 0;
            
            // Accept variations: chmod a-w, chmod 444, chmod -w
            if (strstr(user_input, "chmod") && (strstr(user_input, "a-w") || 
                strstr(user_input, "444") || strstr(user_input, "-w") || 
                strstr(user_input, "u-w"))) {
                printf("✅ CORRECT! You've protected the draft file.\n");
                printf("   The draft is now read-only and safe from deletion.\n\n");
                correct_answers++;
                task = 2;
            } else {
                printf("❌ Incorrect. Use chmod to remove write permissions.\n");
                printf("   Example: chmod 444 draft_audit.log\n\n");
            }
        }
    }
    
    // TASK 2: Try to read the audit log (will fail with permission denied)
    while (task == 2 && !is_level_time_up()) {
        display_level_timer();
        printf("\n📋 TASK 2: Read the Audit Logs\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Now you need to understand what caused the crash.\n");
        printf("You must read the audit log file (audit.log).\n\n");
        printf("What command reads file contents?\n");
        printf(">>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            
            user_input[strcspn(user_input, "\n")] = 0;
            
            if (strstr(user_input, "cat audit.log")) {
                printf("⚠️  You executed: cat audit.log\n");
                printf("   ERROR: Permission denied!\n");
                printf("   The file is owned by root and you don't have read permission.\n\n");
                task = 3;
            } else {
                printf("❌ Try: cat audit.log\n\n");
            }
        }
    }
    
    // TASK 3: Change ownership of the file
    while (task == 3 && !is_level_time_up()) {
        display_level_timer();
        printf("\n📋 TASK 3: Claim File Ownership\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("The system owner hasn't given you read permission.\n");
        printf("You need to change file ownership to yourself.\n");
        printf("Your username is: 'username'\n\n");
        printf("What command changes file ownership?\n");
        printf("(Hint: chown command to change owner)\n");
        printf(">>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            
            user_input[strcspn(user_input, "\n")] = 0;
            
            // Accept variations
            if (strstr(user_input, "chown") && (strstr(user_input, "username") || 
                strstr(user_input, "$USER") || strstr(user_input, "audit.log"))) {
                printf("✅ CORRECT! You've changed file ownership.\n");
                printf("   audit.log is now owned by: username\n");
                printf("   You now have read permissions!\n\n");
                correct_answers++;
                task = 4;
            } else {
                printf("❌ Incorrect. Use: sudo chown username audit.log\n");
                printf("   Remember, your username is 'username'\n\n");
            }
        }
    }
    
    // TASK 4: Read and analyze the logs - Thread apocalypse discovery
    while (task == 4 && !is_level_time_up()) {
        display_level_timer();
        printf("\n📋 TASK 4: Analyze the Crash Logs\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Now that you have access, read the audit logs.\n");
        printf("You discover a THREAD APOCALYPSE:\n\n");
        printf("  [14:32:01] Thread 1 created with pthread_create()\n");
        printf("  [14:32:05] Thread 1 NEVER called pthread_join()\n");
        printf("  [14:32:10] Thread 1 completes execution\n");
        printf("  [14:32:11] ⚠️  ZOMBIE THREAD: Parent never terminated!\n");
        printf("  [14:32:15] Thread 2 created (Thread 1 orphaned it)\n");
        printf("  [14:32:20] Parent process SUDDENLY TERMINATED!\n");
        printf("  [14:32:21] ⚠️  ORPHAN THREADS: No parent to manage!\n");
        printf("  [14:32:30] Resource leaks accumulate...\n");
        printf("  [14:32:31] ❌ SYSTEM CRASH FROM THREAD CHAOS!\n\n");
        printf("What is the PRIMARY problem here?\n");
        printf("  a) Too many threads were created\n");
        printf("  b) Threads not properly joined/terminated (zombies) & orphaned\n");
        printf("  c) Threads used incorrect synchronization\n");
        printf("  Enter choice (a/b/c): ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            
            user_input[strcspn(user_input, "\n")] = 0;
            
            if (strcmp(user_input, "b") == 0) {
                printf("✅ CORRECT! Zombie threads (not joined) + orphaned threads!\n");
                printf("   This is why the system crashed.\n");
                printf("   LESSON: Always pthread_join() your threads!\n");
                printf("   NEXT: Level 1 will teach proper thread creation!\n\n");
                correct_answers++;
                task = 5;
            } else {
                printf("❌ Incorrect. The answer is 'b' - Thread management failure!\n");
                printf("   Zombie & orphan threads caused the crash.\n\n");
            }
        }
    }
    
    stop_level_timer();
    
    if (is_level_time_up()) {
        printf("\n╔════════════════════════════════════════════════════╗\n");
        printf("║                   💥 TIME'S UP! 💥                ║\n");
        printf("║                                                  ║\n");
        printf("║  You have run out of time!                       ║\n");
        printf("║  The system has CRASHED!                         ║\n");
        printf("║                                                  ║\n");
        printf("║  Restarting game from the beginning...           ║\n");
        printf("╚════════════════════════════════════════════════════╝\n");
        sleep(2);
        engine.current_level = 0;
        engine.score = 0;
        engine.levels_completed = 0;
        return;
    }
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║  ✅ LEVEL 0 COMPLETE!                                ║\n");
    printf("║  Tasks Completed: %d/4                               ║\n", correct_answers);
    printf("║  File system recovered. Thread apocalypse logged.    ║\n");
    printf("║  Points Earned: %d                                   ║\n", correct_answers * 60);
    printf("║                                                      ║\n");
    printf("║  ⚠️  CRITICAL FINDING: Thread management failure!    ║\n");
    printf("║  Level 1: Restore the system with proper threads     ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    
    engine.score += correct_answers * 60;
    engine.levels_completed++;
}

void level_1_reactor_core(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  LEVEL 1: THREAD RESTORATION                       ║\n");
    printf("║  (Proper Thread Creation & Management)             ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("🔴 CRITICAL: System full of zombie & orphan threads!\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("The previous admin left a mess of improperly managed threads.\n");
    printf("You must write CORRECT threading code to restore the system.\n");
    printf("Each question requires a CODE SNIPPET answer.\n\n");
    
    printf("⏱️  TIME LIMIT: 180 seconds (3 minutes) for 3 questions\n");
    printf("📝 Write complete, compilable C code for each question.\n\n");
    
    start_level_timer(180);  // 180 seconds (3 minutes) for this level
    
    char user_input[512];  // Larger buffer for code snippets
    int correct_answers = 0;
    int question = 1;
    
    // QUESTION 1: Thread creation with proper structure
    while (question == 1 && !is_level_time_up()) {
        display_level_timer();
        printf("\n� QUESTION 1: Thread Creation Foundation\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Write the C code to CREATE a thread that executes a\n");
        printf("simple function that prints 'Thread started'.\n\n");
        printf("Include:\n");
        printf("  1. Thread variable declaration (pthread_t)\n");
        printf("  2. pthread_create() call with correct parameters\n");
        printf("  3. pthread_join() to properly wait for thread\n\n");
        printf("Paste your code snippet below (max 5 lines):\n");
        printf(">>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            
            user_input[strcspn(user_input, "\n")] = 0;
            
            // Check for key components of proper thread creation
            int has_pthread_t = strstr(user_input, "pthread_t") != NULL;
            int has_pthread_create = strstr(user_input, "pthread_create") != NULL;
            int has_pthread_join = strstr(user_input, "pthread_join") != NULL;
            int has_null_return = strstr(user_input, "NULL") != NULL;
            
            if (has_pthread_t && has_pthread_create && has_pthread_join && has_null_return) {
                printf("✅ CORRECT! You have all required components:\n");
                printf("   ✓ pthread_t variable declared\n");
                printf("   ✓ pthread_create() with proper parameters\n");
                printf("   ✓ pthread_join() to reap the thread\n");
                printf("   ✓ NULL for thread attributes\n");
                printf("   This PREVENTS zombie threads!\n\n");
                correct_answers++;
                question = 2;
            } else {
                printf("❌ Incomplete. Your code is missing:\n");
                if (!has_pthread_t) printf("   ✗ pthread_t variable declaration\n");
                if (!has_pthread_create) printf("   ✗ pthread_create() call\n");
                if (!has_pthread_join) printf("   ✗ pthread_join() call\n");
                if (!has_null_return) printf("   ✗ NULL for thread attributes\n");
                printf("   Try again!\n\n");
            }
        }
    }
    
    // QUESTION 2: Thread function with return value
    while (question == 2 && !is_level_time_up()) {
        display_level_timer();
        printf("\n🔴 QUESTION 2: Thread Function with Return Value\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Write a THREAD FUNCTION that:\n");
        printf("  1. Takes an integer parameter (count = 5)\n");
        printf("  2. Prints numbers from 1 to 'count'\n");
        printf("  3. Returns a pointer to the result (malloc'd)\n");
        printf("  4. Uses void* for pthread compatibility\n\n");
        printf("Example signature: void* thread_function(void* arg)\n");
        printf("Paste your code snippet (max 10 lines):\n");
        printf(">>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            
            user_input[strcspn(user_input, "\n")] = 0;
            
            // Check for thread function components
            int has_void_ptr_sig = strstr(user_input, "void*") != NULL && 
                                   (strstr(user_input, "thread_function") != NULL || 
                                    strstr(user_input, "thread_func") != NULL);
            int has_int_cast = strstr(user_input, "(int)") != NULL || 
                               strstr(user_input, "(intptr_t)") != NULL;
            int has_loop = strstr(user_input, "for") != NULL || 
                           strstr(user_input, "while") != NULL;
            int has_malloc = strstr(user_input, "malloc") != NULL;
            int has_return = strstr(user_input, "return") != NULL;
            
            if (has_void_ptr_sig && has_int_cast && has_loop && has_malloc && has_return) {
                printf("✅ CORRECT! Thread function has all components:\n");
                printf("   ✓ void* return type (pthread standard)\n");
                printf("   ✓ Proper parameter casting from void*\n");
                printf("   ✓ Loop to process data (for/while)\n");
                printf("   ✓ malloc() to allocate return value\n");
                printf("   ✓ return statement with proper type\n");
                printf("   This is a proper pthread-compatible function!\n\n");
                correct_answers++;
                question = 3;
            } else {
                printf("❌ Incomplete. Your code is missing:\n");
                if (!has_void_ptr_sig) printf("   ✗ void* return type and function signature\n");
                if (!has_int_cast) printf("   ✗ Proper cast from void* to int\n");
                if (!has_loop) printf("   ✗ Loop to process data (for/while)\n");
                if (!has_malloc) printf("   ✗ malloc() for return value\n");
                if (!has_return) printf("   ✗ return statement\n");
                printf("   Try again!\n\n");
            }
        }
    }
    
    // QUESTION 3: Multiple threads with proper cleanup
    while (question == 3 && !is_level_time_up()) {
        display_level_timer();
        printf("\n🔴 QUESTION 3: Multiple Threads & Cleanup\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Create 3 threads, each with different IDs (1,2,3).\n");
        printf("After all threads complete:\n");
        printf("  1. Collect return values from all 3 threads\n");
        printf("  2. Print the results\n");
        printf("  3. Free allocated memory\n");
        printf("  4. NO orphan or zombie threads!\n\n");
        printf("Paste your code snippet here:\n");
        printf(">>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            
            user_input[strcspn(user_input, "\n")] = 0;
            
            // Check for proper multi-threading with cleanup
            int has_array = strstr(user_input, "[3]") != NULL || 
                            strstr(user_input, "[NUM_THREADS]") != NULL;
            int has_loop_create = (strstr(user_input, "for") != NULL && 
                                   strstr(user_input, "pthread_create") != NULL) ||
                                  (strstr(user_input, "pthread_create") != NULL);
            int has_join_loop = (strstr(user_input, "for") != NULL && 
                                 strstr(user_input, "pthread_join") != NULL) ||
                                strstr(user_input, "pthread_join") != NULL;
            int has_free = strstr(user_input, "free") != NULL;
            int has_void_ptr_collect = strstr(user_input, "void*") != NULL && 
                                       strstr(user_input, "result") != NULL;
            
            if (has_array && has_loop_create && has_join_loop && has_free && has_void_ptr_collect) {
                printf("✅ CORRECT! Complete multi-threading solution:\n");
                printf("   ✓ Thread array for 3 threads\n");
                printf("   ✓ Loop to create all threads\n");
                printf("   ✓ Loop to join all threads (proper reaping!)\n");
                printf("   ✓ Collection of return values\n");
                printf("   ✓ free() to deallocate memory\n");
                printf("   This is PROPER THREAD MANAGEMENT!\n");
                printf("   ✅ NO ZOMBIE THREADS! NO MORE ORPHANS! ✅\n\n");
                correct_answers++;
                question = 4;
            } else {
                printf("❌ Incomplete. Your code is missing:\n");
                if (!has_array) printf("   ✗ Thread array (pthread_t array[3])\n");
                if (!has_loop_create) printf("   ✗ Loop to create multiple threads\n");
                if (!has_join_loop) printf("   ✗ Loop to join and reap threads\n");
                if (!has_free) printf("   ✗ free() calls for cleanup\n");
                if (!has_void_ptr_collect) printf("   ✗ Collection of void* return values\n");
                printf("   Try again!\n\n");
            }
        }
    }
    
    stop_level_timer();
    
    if (is_level_time_up()) {
        printf("\n╔════════════════════════════════════════════════════╗\n");
        printf("║                   💥 TIME'S UP! 💥                   ║\n");
        printf("║  The system has CRASHED!                             ║\n");
        printf("║  Restarting game from the beginning...               ║\n");
        printf("╚══════════════════════════════════════════════════════╝\n");
        sleep(2);
        engine.current_level = 0;
        engine.score = 0;
        engine.levels_completed = 0;
        return;
    }
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║  ✅ LEVEL 1 COMPLETE!                                ║\n");
    printf("║  Code Snippets Accepted: %d/3                        ║\n", correct_answers);
    printf("║  Threading system restored to stability!             ║\n");
    printf("║  Points Earned: %d                                   ║\n", correct_answers * 80);
    printf("║                                                      ║\n");
    printf("║  ✅ NO MORE ZOMBIES! NO MORE ORPHANS! ✅             ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    
    engine.score += correct_answers * 80;
    engine.levels_completed++;
    press_any_key();
}

void level_2_synchronization_mastery(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  LEVEL 2: SYNCHRONIZATION MASTERY                 ║\n");
    printf("║  (Readers-Writers, Producer-Consumer, Dining Phil) ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("🔴 CRITICAL: Multiple synchronization failures!\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("3 problems to fix:\n");
    printf("  1. READERS-WRITERS DEADLOCK\n");
    printf("  2. PRODUCER-CONSUMER STARVATION\n");
    printf("  3. DINING PHILOSOPHERS DEADLOCK\n\n");
    
    printf("⏱️  TIME: 600 seconds (10 minutes)\n\n");
    
    start_level_timer(600);
    char user_input[1024];
    int correct_answers = 0;
    int problem = 1;
    
    // PROBLEM 1
    while (problem == 1 && !is_level_time_up()) {
        display_level_timer();
        printf("\n🔴 PROBLEM 1: READERS-WRITERS DEADLOCK\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("You read while admin waits to write - DEADLOCK!\n\n");
        printf("BROKEN: Uses mutex (exclusive lock)\n");
        printf("FIX: Use pthread_rwlock_t (readers-writers lock)\n\n");
        printf("Your code:\n>>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            user_input[strcspn(user_input, "\n")] = 0;
            
            if (strstr(user_input, "rwlock") && strstr(user_input, "rdlock")) {
                printf("✅ CORRECT!\n✓ Concurrent readers allowed\n💚 DEADLOCK PREVENTED!\n\n");
                correct_answers++;
                problem = 2;
            } else {
                printf("❌ Missing: rwlock or rdlock\n\n");
            }
        }
    }
    
    // PROBLEM 2
    while (problem == 2 && !is_level_time_up()) {
        display_level_timer();
        printf("\n🔴 PROBLEM 2: PRODUCER-CONSUMER STARVATION\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("Consumer busy-waiting, wastes CPU!\n\n");
        printf("BROKEN: Uses sleep polling\n");
        printf("FIX: Use sem_t (semaphores)\n\n");
        printf("Your code:\n>>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            user_input[strcspn(user_input, "\n")] = 0;
            
            if (strstr(user_input, "sem_wait") && strstr(user_input, "sem_post")) {
                printf("✅ CORRECT!\n✓ Efficient blocking\n💚 STARVATION ELIMINATED!\n\n");
                correct_answers++;
                problem = 3;
            } else {
                printf("❌ Missing: sem_wait or sem_post\n\n");
            }
        }
    }
    
    // PROBLEM 3
    while (problem == 3 && !is_level_time_up()) {
        display_level_timer();
        printf("\n🔴 PROBLEM 3: DINING PHILOSOPHERS\n");
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        printf("All philosophers deadlocked!\n\n");
        printf("BROKEN: All use same lock order\n");
        printf("FIX: Use asymmetric ordering\n\n");
        printf("Your code:\n>>> ");
        fflush(stdout);
        
        if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
            if (is_level_time_up()) break;
            user_input[strcspn(user_input, "\n")] = 0;
            
            if (strstr(user_input, "if") && (strstr(user_input, ">") || strstr(user_input, "<"))) {
                printf("✅ CORRECT!\n✓ Asymmetric ordering\n💚 DEADLOCK-FREE!\n\n");
                correct_answers++;
                problem = 4;
            } else {
                printf("❌ Missing: if or comparison\n\n");
            }
        }
    }
    
    stop_level_timer();
    
    if (is_level_time_up()) {
        printf("\n💥 TIME'S UP!\n");
        sleep(2);
        engine.current_level = 0;
        engine.score = 0;
        engine.levels_completed = 0;
        return;
    }
    
    printf("\n✅ LEVEL 2 COMPLETE!\n");
    printf("Fixed: %d/3 problems\n", correct_answers);
    printf("Points: %d\n\n", correct_answers * 100);
    
    engine.score += correct_answers * 100;
    engine.levels_completed++;
    press_any_key();
}

void level_3_deadlock_avoidance(void) {
    clear_screen();
    print_game_header();
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  LEVEL 3: MULTI-ALGORITHM MASTERY                 ║\n");
    printf("║  (Round-Robin & Priority Scheduling)              ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    start_level_timer(60);
    
    char user_input[100];
    int correct_answers = 0;
    
    display_level_timer();
    printf("Q1: In Round-Robin with quantum=3ms, what happens if burst > quantum?\n");
    printf("  b) Task goes to back of queue\n");
    printf(">>> ");
    fflush(stdout);
    
    if (fgets(user_input, sizeof(user_input), stdin) != NULL) {
        if (!is_level_time_up()) {
            user_input[strcspn(user_input, "\n")] = 0;
            if (strcmp(user_input, "b") == 0) {
                printf("✅ CORRECT!\n");
                correct_answers++;
            }
        }
    }
    
    stop_level_timer();
    
    printf("\n✅ LEVEL 3 COMPLETE!\n");
    printf("Points: %d\n\n", correct_answers * 75);
    
    engine.score += correct_answers * 75;
    engine.levels_completed++;
    press_any_key();
}

/* ============================================
   AUTHENTICATION SYSTEM
   ============================================ */

int authenticate_user(void) {
    GameSession* session = login_system_init();
    if (!session) return -1;
    
    int authenticated = 0;
    
    while (!authenticated) {
        login_system_display_login_screen();
        
        printf("\n1. Login\n");
        printf("2. Register\n");
        printf("3. Exit\n");
        printf("Select [1-3]: ");
        
        char choice[10];
        if (fgets(choice, sizeof(choice), stdin) == NULL) continue;
        
        int option = atoi(choice);
        
        switch (option) {
            case 1: {
                printf("Username: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) == NULL) continue;
                username[strcspn(username, "\n")] = 0;
                
                printf("Password: ");
                char password[MAX_PASSWORD_LEN];
                if (fgets(password, sizeof(password), stdin) == NULL) continue;
                password[strcspn(password, "\n")] = 0;
                
                int result = login_system_login(session, username, password);
                if (result == 0) {
                    printf("\n✅ Login successful!\n");
                    printf("Welcome, %s!\n\n", session->current_user->username);
                    
                    if (login_system_is_admin(session)) {
                        printf("🔑 Admin user detected. Access to admin panel available.\n\n");
                    }
                    
                    authenticated = 1;
                } else {
                    switch (result) {
                        case -2: printf("❌ User not found.\n"); break;
                        case -3: printf("❌ User is banned.\n"); break;
                        case -4: printf("❌ Wrong password.\n"); break;
                        default: printf("❌ Login failed.\n");
                    }
                }
                sleep(2);
                break;
            }
            
            case 2: {
                login_system_display_register_screen();
                
                printf("Username: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) == NULL) continue;
                username[strcspn(username, "\n")] = 0;
                
                printf("Password: ");
                char password[MAX_PASSWORD_LEN];
                if (fgets(password, sizeof(password), stdin) == NULL) continue;
                password[strcspn(password, "\n")] = 0;
                
                int result = login_system_register(session, username, password);
                if (result == 0) {
                    printf("\n✅ Registration successful!\n");
                    printf("Please login with your credentials.\n\n");
                } else {
                    switch (result) {
                        case -2: printf("❌ Database full.\n"); break;
                        case -3: printf("❌ User already exists.\n"); break;
                        case -4: printf("❌ Username too long.\n"); break;
                        case -5: printf("❌ Password too long.\n"); break;
                        default: printf("❌ Registration failed.\n");
                    }
                }
                sleep(2);
                break;
            }
            
            case 3:
                login_system_destroy(session);
                return -1;
                
            default:
                printf("❌ Invalid choice.\n");
                sleep(1);
        }
    }
    
    // Store session in engine
    engine.session = session;
    return 0;
}

/* ============================================
   ADAPTIVE DIFFICULTY SYSTEM
   ============================================ */

int ask_adaptive_question(int level_id, ProficiencyMetrics* metrics) {
    if (!metrics || !engine.question_pool) return 0;
    
    DifficultyLevel difficulty = metrics->current_level;
    Question* question = question_get_random(engine.question_pool, level_id, difficulty);
    
    if (!question) return 0;
    
    printf("\n");
    print_separator('─', 50);
    printf("Difficulty: ");
    switch (difficulty) {
        case BEGINNER: printf("🔹 BEGINNER"); break;
        case INTERMEDIATE: printf("🟡 INTERMEDIATE"); break;
        case ADVANCED: printf("🔶 ADVANCED"); break;
        case PROFICIENT: printf("🔴 PROFICIENT"); break;
    }
    printf(" | Points: %d\n", question->points);
    print_separator('─', 50);
    printf("\n%s\n\n", question->question);
    
    if (strlen(question->hint) > 0) {
        printf("💡 Hint: %s\n\n", question->hint);
    }
    
    printf("Your answer: ");
    char answer[MAX_ANSWER_LEN];
    if (fgets(answer, sizeof(answer), stdin) == NULL) return 0;
    answer[strcspn(answer, "\n")] = 0;
    
    // Simple answer checking - can be enhanced with fuzzy matching
    int is_correct = (strstr(answer, question->answer) != NULL) || 
                     (strcasestr(answer, question->answer) != NULL);
    
    if (is_correct) {
        printf("✅ CORRECT!\n");
        engine.score += question->points;
        proficiency_update(metrics, 1, difficulty);
        return 1;
    } else {
        printf("❌ Incorrect. Expected: %s\n", question->answer);
        proficiency_update(metrics, 0, difficulty);
        return 0;
    }
}

/* ============================================
   ALGORITHMS DEMONSTRATION LEVELS
   ============================================ */

void level_banker_algorithm(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  BONUS: BANKER'S ALGORITHM - DEADLOCK AVOIDANCE   ║\n");
    printf("║  (Resource Allocation & Safety State)             ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    engine.banker = banker_init(3);  // 3 resources
    
    // Set up initial state
    for (int i = 0; i < 3; i++) {
        engine.banker->available[i] = 10;
        engine.banker->total[i] = 10;
    }
    
    // Add some processes
    for (int i = 0; i < 3; i++) {
        engine.banker->processes[i].pid = i + 1;
        engine.banker->processes[i].max_needed[0] = 7;
        engine.banker->processes[i].max_needed[1] = 5;
        engine.banker->processes[i].max_needed[2] = 3;
    }
    engine.banker->process_count = 3;
    
    printf("Testing Banker's Algorithm with 3 processes and 3 resources...\n\n");
    
    banker_display_state(engine.banker);
    
    printf("\nAttempting resource allocation...\n");
    int result = banker_request_resource(engine.banker, 1, 0, 3);
    
    if (result == 0) {
        printf("✅ Resource allocation successful! System in safe state.\n");
        engine.score += 50;
    } else {
        printf("❌ Allocation would cause deadlock. Request denied.\n");
    }
    
    banker_display_state(engine.banker);
    press_any_key();
    
    banker_destroy(engine.banker);
}

void level_scheduling_algorithms(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  BONUS: CPU SCHEDULING ALGORITHMS                 ║\n");
    printf("║  (FCFS, Round-Robin, Priority)                    ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Which scheduling algorithm would you like to simulate?\n");
    printf("1. FCFS (First Come First Served)\n");
    printf("2. Round Robin (RR)\n");
    printf("3. Priority Scheduling\n");
    printf("Select [1-3]: ");
    
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin) == NULL) return;
    
    engine.scheduler = scheduler_init(SCHED_FCFS_ALG, 4);
    
    // Add some processes
    scheduler_add_process(engine.scheduler, 1, 8, 0, 3);
    scheduler_add_process(engine.scheduler, 2, 4, 1, 2);
    scheduler_add_process(engine.scheduler, 3, 2, 2, 1);
    
    int option = atoi(choice);
    
    switch (option) {
        case 1:
            printf("\n📊 FCFS Scheduling:\n");
            scheduler_fcfs(engine.scheduler);
            break;
        case 2:
            printf("\n📊 Round-Robin Scheduling (quantum=4):\n");
            scheduler_round_robin(engine.scheduler);
            break;
        case 3:
            printf("\n📊 Priority Scheduling:\n");
            scheduler_priority(engine.scheduler);
            break;
        default:
            printf("Invalid option\n");
            return;
    }
    
    scheduler_display_gantt_chart(engine.scheduler);
    engine.score += 75;
    
    press_any_key();
    scheduler_destroy(engine.scheduler);
}

void level_memory_management(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  BONUS: MEMORY MANAGEMENT                         ║\n");
    printf("║  (Buddy System & Placement Strategies)            ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Which placement strategy?\n");
    printf("1. First Fit\n");
    printf("2. Best Fit\n");
    printf("3. Worst Fit\n");
    printf("Select [1-3]: ");
    
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin) == NULL) return;
    
    PlacementStrategy strategy = PLACE_FIRST_FIT;
    int option = atoi(choice);
    
    switch (option) {
        case 1: strategy = PLACE_FIRST_FIT; break;
        case 2: strategy = PLACE_BEST_FIT; break;
        case 3: strategy = PLACE_WORST_FIT; break;
    }
    
    engine.memory = memory_init(1024, strategy);
    
    // Allocate some memory
    memory_allocate(engine.memory, 1, 256);
    memory_allocate(engine.memory, 2, 128);
    memory_allocate(engine.memory, 3, 512);
    
    memory_display_layout(engine.memory);
    
    engine.score += 75;
    press_any_key();
    
    memory_destroy(engine.memory);
}

void level_paging_system(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  BONUS: PAGE REPLACEMENT ALGORITHMS               ║\n");
    printf("║  (FIFO, LRU, Optimal)                             ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Which page replacement algorithm?\n");
    printf("1. FIFO\n");
    printf("2. LRU (Least Recently Used)\n");
    printf("3. Optimal\n");
    printf("Select [1-3]: ");
    
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin) == NULL) return;
    
    PageReplacementType type = PAGE_FIFO;
    int option = atoi(choice);
    
    switch (option) {
        case 1: type = PAGE_FIFO; break;
        case 2: type = PAGE_LRU; break;
        case 3: type = PAGE_OPTIMAL; break;
    }
    
    engine.paging = page_system_init(4, type);  // 4 page frames
    
    printf("\nAccessing pages: 0, 1, 2, 3, 0, 1, 4, 0, 1, 2, 3, 4\n\n");
    
    int page_sequence[] = {0, 1, 2, 3, 0, 1, 4, 0, 1, 2, 3, 4};
    for (int i = 0; i < 12; i++) {
        page_system_access(engine.paging, page_sequence[i]);
    }
    
    page_system_display_stats(engine.paging);
    
    engine.score += 75;
    press_any_key();
    
    page_system_destroy(engine.paging);
}

void level_disk_scheduling(void) {
    clear_screen();
    print_game_header();
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  BONUS: DISK SCHEDULING ALGORITHMS                ║\n");
    printf("║  (FCFS, SSTF, SCAN, CSCAN)                        ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Which disk scheduling algorithm?\n");
    printf("1. FCFS\n");
    printf("2. SSTF (Shortest Seek Time First)\n");
    printf("3. SCAN\n");
    printf("4. CSCAN (Circular SCAN)\n");
    printf("Select [1-4]: ");
    
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin) == NULL) return;
    
    DiskSchedulingType type = DISK_FCFS;
    int option = atoi(choice);
    
    switch (option) {
        case 1: type = DISK_FCFS; break;
        case 2: type = DISK_SSTF; break;
        case 3: type = DISK_SCAN; break;
        case 4: type = DISK_CSCAN; break;
    }
    
    engine.disk_sched = disk_scheduler_init(200, type);
    
    // Add disk requests
    int tracks[] = {82, 170, 43, 140, 24, 16, 190};
    for (int i = 0; i < 7; i++) {
        disk_scheduler_add_request(engine.disk_sched, tracks[i]);
    }
    
    printf("\nDisk requests at tracks: 82, 170, 43, 140, 24, 16, 190\n");
    printf("Head position: 50\n\n");
    
    switch (option) {
        case 1: disk_scheduler_fcfs(engine.disk_sched); break;
        case 2: disk_scheduler_sstf(engine.disk_sched); break;
        case 3: disk_scheduler_scan(engine.disk_sched); break;
        case 4: disk_scheduler_cscan(engine.disk_sched); break;
    }
    
    disk_scheduler_display_sequence(engine.disk_sched);
    
    engine.score += 75;
    press_any_key();
    
    disk_scheduler_destroy(engine.disk_sched);
}

/* ============================================
   MAIN ENTRY POINT
   ============================================ */


/* ============================================
   GAME LOOP
   ============================================ */

void game_loop(void) {
    int restart_game = 1;
    
    while (restart_game) {
        engine.current_level = 0;
        engine.score = 0;
        engine.time_remaining = 300;
        engine.levels_completed = 0;
        engine.time_up = 0;
        engine.show_level_timer = 0;
        engine.game_state = game_state_init();
        engine.log = log_buffer_init();
        
        pthread_mutex_init(&engine.timer_lock, NULL);
        engine.infrastructure = infrastructure_init();
        
        if (engine.infrastructure) {
            infrastructure_log_event(engine.infrastructure, "=== GAME SESSION STARTED ===", 0);
        }
        
        pthread_create(&engine.timer_thread, NULL, timer_thread_func, NULL);
        
        while (engine.levels_completed < 4 && !is_time_up()) {
            display_main_menu();
            print_game_status();
            
            if (is_time_up()) {
                printf("\n💥 TIME'S UP!\n");
                break;
            }
            
            int choice = get_user_choice(1, 4);
            
            switch (choice) {
                case 1:
                    switch (engine.levels_completed) {
                        case 0:
                            level_0_terminal_boot();
                            break;
                        case 1:
                            level_1_reactor_core();
                            break;
                        case 2:
                            level_2_synchronization_mastery();
                            break;
                        case 3:
                            level_3_deadlock_avoidance();
                            break;
                    }
                    break;
                case 2:
                    print_game_status();
                    press_any_key();
                    break;
                case 3:
                    display_help();
                    break;
                case 4:
                    printf("\nThanks for playing!\n");
                    engine.time_up = 1;
                    pthread_join(engine.timer_thread, NULL);
                    pthread_mutex_destroy(&engine.timer_lock);
                    game_state_destroy(engine.game_state);
                    log_buffer_destroy(engine.log);
                    
                    if (engine.infrastructure) {
                        infrastructure_log_event(engine.infrastructure, "=== GAME SESSION ENDED ===", 0);
                        sleep(1);
                        infrastructure_destroy(engine.infrastructure);
                    }
                    
                    return;
            }
            
            if (is_time_up()) {
                printf("\n💥 TIME'S UP!\n");
                sleep(3);
                break;
            }
        }
        
        engine.time_up = 1;
        pthread_join(engine.timer_thread, NULL);
        pthread_mutex_destroy(&engine.timer_lock);
        
        if (engine.infrastructure) {
            infrastructure_log_event(engine.infrastructure, "=== SESSION RESTART ===", 0);
            sleep(1);
            infrastructure_destroy(engine.infrastructure);
        }
        
        if (!is_time_up()) {
            printf("\n🎉 VICTORY!\n");
            restart_game = 0;
        } else {
            printf("\nTry again? (YES/NO): ");
            char restart_input[10];
            if (fgets(restart_input, sizeof(restart_input), stdin) != NULL) {
                if (strcasecmp(restart_input, "YES") == 0 || strcmp(restart_input, "1") == 0) {
                    restart_game = 1;
                } else {
                    restart_game = 0;
                }
            }
        }
        
        game_state_destroy(engine.game_state);
        log_buffer_destroy(engine.log);
    }
}


int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    // Parse command-line arguments
    char* scheduler_type = NULL;
    int time_quantum = 4;
    
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--scheduler=", 12) == 0) {
            scheduler_type = argv[i] + 12;
        } else if (strncmp(argv[i], "--quantum=", 10) == 0) {
            time_quantum = atoi(argv[i] + 10);
        }
    }
    
    // Initialize question pool
    engine.question_pool = question_pool_init();
    if (engine.question_pool) {
        question_pool_load_from_file(engine.question_pool, "data/questions.txt");
    }
    
    // Display boot sequence
    clear_screen();
    FILE* logo_file = fopen("assets/boot_logo.txt", "r");
    if (logo_file) {
        char line[256];
        while (fgets(line, sizeof(line), logo_file)) {
            printf("%s", line);
        }
        fclose(logo_file);
    } else {
        print_game_header();
        printf("Beginning recovery sequence...\n");
    }
    
    press_any_key();
    
    // Authenticate user
    if (authenticate_user() != 0) {
        printf("Authentication failed. Exiting.\n");
        return 1;
    }
    
    // Start game
    game_loop();
    
    // Cleanup
    if (engine.session) {
        login_system_destroy(engine.session);
    }
    if (engine.question_pool) {
        question_pool_destroy(engine.question_pool);
    }
    
    return 0;
}

void display_main_menu(void) {
    clear_screen();
    print_game_header();
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║            🎮 MAIN MENU 🎮                        ║\n");
    printf("║  1. Continue Game                                 ║\n");
    printf("║  2. View Game Status                              ║\n");
    printf("║  3. View Help & Controls                          ║\n");
    printf("║  4. Quit Game                                     ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void display_help(void) {
    clear_screen();
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║              📖 HELP & CONTROLS 📖                ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    printf("GAME OBJECTIVE:\n");
    printf("Solve 4 challenging OS-based puzzles.\n\n");
    printf("LEVELS:\n");
    printf("  Level 0: File System Recovery\n");
    printf("  Level 1: Thread Restoration\n");
    printf("  Level 2: Synchronization Mastery\n");
    printf("  Level 3: Scheduling\n\n");
    printf("USERNAME: 'username'\n\n");
    press_any_key();
}
