#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>
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
} GameEngine;

GameEngine engine;

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
   ADAPTIVE QUESTION SYSTEM
   ============================================ */

int ask_adaptive_question(int level_id, DifficultyLevel* current_difficulty, ProficiencyMetrics* metrics) {
    if (!engine.question_pool || !metrics) return -1;
    
    // Get random question at current difficulty
    Question* q = question_get_random(engine.question_pool, level_id, *current_difficulty);
    if (!q) {
        printf("No questions available at this difficulty level.\n");
        return -2;
    }
    
    display_timer();
    printf("\n📋 Question (%s):\n", 
        *current_difficulty == BEGINNER ? "🔹 BEGINNER" :
        *current_difficulty == INTERMEDIATE ? "🟡 INTERMEDIATE" :
        *current_difficulty == ADVANCED ? "🔶 ADVANCED" : "🔴 PROFICIENT");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("%s\n\n", q->question);
    
    if (strlen(q->hint) > 0) {
        printf("💡 Hint: %s\n\n", q->hint);
    }
    
    printf("Your answer: ");
    fflush(stdout);
    
    char answer[MAX_ANSWER_LEN];
    if (fgets(answer, sizeof(answer), stdin) == NULL) return -3;
    
    if (is_time_up()) return -4;
    
    answer[strcspn(answer, "\n")] = 0;
    
    // Check answer (simple substring match)
    int is_correct = (strstr(q->answer, answer) != NULL || strstr(answer, q->answer) != NULL);
    
    if (is_correct) {
        printf("✅ CORRECT! (%d points)\n", q->points);
        engine.score += q->points;
        proficiency_update(metrics, 1, *current_difficulty);
        
        // Increase difficulty if doing well
        if (metrics->proficiency_percentage >= 80.0 && *current_difficulty < PROFICIENT) {
            printf("🎉 Excellent! Moving to next difficulty level!\n");
            *current_difficulty = difficulty_get_next(*current_difficulty);
        }
    } else {
        printf("❌ Incorrect. The answer was: %s\n", q->answer);
        proficiency_update(metrics, 0, *current_difficulty);
        
        // Decrease difficulty if struggling
        if (metrics->proficiency_percentage < 50.0 && *current_difficulty > BEGINNER) {
            printf("📚 Let's review. Dropping to previous difficulty level.\n");
            *current_difficulty = difficulty_get_previous(*current_difficulty);
        }
    }
    
    sleep(1);
    return is_correct ? 1 : 0;
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
   LEVEL 0: FILE SYSTEM & BANKER'S ALGORITHM
   ============================================ */

void level_0_banker_algorithm(void) {
    display_header("LEVEL 0: BANKER'S ALGORITHM & DEADLOCK DETECTION");
    
    printf("🔴 CRITICAL: System facing potential deadlock!\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("Learn about banker's algorithm for deadlock avoidance.\n");
    printf("5 questions, adaptive difficulty, 30 minutes.\n\n");
    
    engine.time_remaining = LEVEL_TIME_LIMIT;
    engine.time_up = 0;
    pthread_create(&engine.timer_thread, NULL, global_timer_func, NULL);
    
    DifficultyLevel difficulty = BEGINNER;
    ProficiencyMetrics* metrics = engine.session->proficiency[0];
    int correct = 0;
    
    for (int q = 0; q < 5 && !is_time_up(); q++) {
        printf("\n┌─ Question %d/5 ─┐\n", q + 1);
        int result = ask_adaptive_question(0, &difficulty, metrics);
        if (result > 0) correct++;
        if (result == -4) break;  // Time's up
    }
    
    pthread_join(engine.timer_thread, NULL);
    
    display_header("LEVEL 0 COMPLETE");
    printf("Correct Answers: %d/5\n", correct);
    printf("Points Earned: %d\n\n", correct * 20);
    proficiency_display(metrics);
    
    engine.score += correct * 20;
    engine.levels_completed++;
    press_any_key();
}

/* ============================================
   LEVEL 1: SCHEDULING ALGORITHMS
   ============================================ */

void level_1_scheduling(void) {
    display_header("LEVEL 1: CPU SCHEDULING ALGORITHMS");
    
    printf("🔴 CRITICAL: CPU scheduling is inefficient!\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("Master FCFS, Round-Robin, and Priority scheduling.\n");
    printf("5 questions, adaptive difficulty, 30 minutes.\n\n");
    
    engine.time_remaining = LEVEL_TIME_LIMIT;
    engine.time_up = 0;
    pthread_create(&engine.timer_thread, NULL, global_timer_func, NULL);
    
    DifficultyLevel difficulty = BEGINNER;
    ProficiencyMetrics* metrics = engine.session->proficiency[1];
    int correct = 0;
    
    for (int q = 0; q < 5 && !is_time_up(); q++) {
        printf("\n┌─ Question %d/5 ─┐\n", q + 1);
        int result = ask_adaptive_question(3, &difficulty, metrics);
        if (result > 0) correct++;
        if (result == -4) break;
    }
    
    pthread_join(engine.timer_thread, NULL);
    
    display_header("LEVEL 1 COMPLETE");
    printf("Correct Answers: %d/5\n", correct);
    printf("Points Earned: %d\n\n", correct * 20);
    proficiency_display(metrics);
    
    engine.score += correct * 20;
    engine.levels_completed++;
    press_any_key();
}

/* ============================================
   LEVEL 2: SYNCHRONIZATION
   ============================================ */

void level_2_synchronization(void) {
    display_header("LEVEL 2: SYNCHRONIZATION PRIMITIVES");
    
    printf("🔴 CRITICAL: Race conditions everywhere!\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("Learn mutexes, semaphores, and reader-writer locks.\n");
    printf("5 questions, adaptive difficulty, 30 minutes.\n\n");
    
    engine.time_remaining = LEVEL_TIME_LIMIT;
    engine.time_up = 0;
    pthread_create(&engine.timer_thread, NULL, global_timer_func, NULL);
    
    DifficultyLevel difficulty = BEGINNER;
    ProficiencyMetrics* metrics = engine.session->proficiency[2];
    int correct = 0;
    
    for (int q = 0; q < 5 && !is_time_up(); q++) {
        printf("\n┌─ Question %d/5 ─┐\n", q + 1);
        int result = ask_adaptive_question(2, &difficulty, metrics);
        if (result > 0) correct++;
        if (result == -4) break;
    }
    
    pthread_join(engine.timer_thread, NULL);
    
    display_header("LEVEL 2 COMPLETE");
    printf("Correct Answers: %d/5\n", correct);
    printf("Points Earned: %d\n\n", correct * 20);
    proficiency_display(metrics);
    
    engine.score += correct * 20;
    engine.levels_completed++;
    press_any_key();
}

/* ============================================
   LEVEL 3: MEMORY MANAGEMENT
   ============================================ */

void level_3_memory_management(void) {
    display_header("LEVEL 3: MEMORY MANAGEMENT");
    
    printf("🔴 CRITICAL: Memory fragmentation and page faults!\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    printf("Master buddy system, paging, and disk scheduling.\n");
    printf("5 questions, adaptive difficulty, 30 minutes.\n\n");
    
    engine.time_remaining = LEVEL_TIME_LIMIT;
    engine.time_up = 0;
    pthread_create(&engine.timer_thread, NULL, global_timer_func, NULL);
    
    DifficultyLevel difficulty = BEGINNER;
    ProficiencyMetrics* metrics = engine.session->proficiency[3];
    int correct = 0;
    
    for (int q = 0; q < 5 && !is_time_up(); q++) {
        printf("\n┌─ Question %d/5 ─┐\n", q + 1);
        int result = ask_adaptive_question(2, &difficulty, metrics);
        if (result > 0) correct++;
        if (result == -4) break;
    }
    
    pthread_join(engine.timer_thread, NULL);
    
    display_header("LEVEL 3 COMPLETE");
    printf("Correct Answers: %d/5\n", correct);
    printf("Points Earned: %d\n\n", correct * 20);
    proficiency_display(metrics);
    
    engine.score += correct * 20;
    engine.levels_completed++;
    press_any_key();
}

/* ============================================
   MAIN MENU
   ============================================ */

void display_main_menu(void) {
    display_header(NULL);
    
    printf("📊 USER: %s  |  🏆 SCORE: %d  |  ✅ LEVELS: %d/4\n\n",
           engine.session->current_user->username,
           engine.score,
           engine.levels_completed);
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  1. Start Next Level                              ║\n");
    printf("║  2. View Statistics                               ║\n");
    printf("║  3. View Help                                     ║\n");
    printf("║  4. Admin Panel (if authorized)                   ║\n");
    printf("║  5. Logout                                        ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void display_help(void) {
    display_header("📖 HELP & GUIDE");
    
    printf("GAME OVERVIEW:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Interactive OS Algorithm Simulator is an interactive OS algorithm simulator.\n");
    printf("Learn and master operating system concepts through\n");
    printf("practical questions with adaptive difficulty.\n\n");
    
    printf("LEVELS:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("Level 0: Banker's Algorithm & Deadlock Detection\n");
    printf("Level 1: CPU Scheduling (FCFS, RR, Priority)\n");
    printf("Level 2: Synchronization Primitives\n");
    printf("Level 3: Memory Management\n\n");
    
    printf("DIFFICULTY SYSTEM:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🔹 Beginner  - Fundamental concepts\n");
    printf("🟡 Intermediate - Applied knowledge\n");
    printf("🔶 Advanced - Complex scenarios\n");
    printf("🔴 Proficient - Expert level\n\n");
    
    printf("Questions adjust based on your performance!\n\n");
    
    press_any_key();
}

void display_statistics(void) {
    display_header("📊 YOUR STATISTICS");
    
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║  OVERALL PROGRESS                                 ║\n");
    printf("║  Total Score: %d points                            ║\n", engine.score);
    printf("║  Levels Completed: %d/4                            ║\n", engine.levels_completed);
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("PROFICIENCY BY LEVEL:\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    const char* level_names[] = {"Banker's Algorithm", "Scheduling", "Synchronization", "Memory Management"};
    
    for (int i = 0; i < 4; i++) {
        if (engine.session->proficiency[i]) {
            ProficiencyMetrics* m = engine.session->proficiency[i];
            printf("%s:\n", level_names[i]);
            printf("  Proficiency: %.1f%% %s\n", m->proficiency_percentage, proficiency_get_grade(m));
            printf("  Questions Answered: %d\n", m->questions_answered);
            printf("  Correct: %d (%d%%)\n\n", m->questions_correct,
                   m->questions_answered > 0 ? (m->questions_correct * 100) / m->questions_answered : 0);
        }
    }
    
    press_any_key();
}

/* ============================================
   MAIN GAME LOOP
   ============================================ */

void game_loop(void) {
    while (engine.levels_completed < 4) {
        display_main_menu();
        
        printf("Select option [1-5]: ");
        char choice[10];
        if (fgets(choice, sizeof(choice), stdin) == NULL) continue;
        
        int option = atoi(choice);
        
        switch (option) {
            case 1:
                switch (engine.levels_completed) {
                    case 0: level_0_banker_algorithm(); break;
                    case 1: level_1_scheduling(); break;
                    case 2: level_2_synchronization(); break;
                    case 3: level_3_memory_management(); break;
                }
                break;
                
            case 2:
                display_statistics();
                break;
                
            case 3:
                display_help();
                break;
                
            case 4:
                if (login_system_is_admin(engine.session)) {
                    login_system_admin_panel(engine.session);
                } else {
                    printf("❌ Admin access denied.\n");
                    sleep(2);
                }
                break;
                
            case 5:
                printf("Logging out...\n");
                login_system_logout(engine.session);
                return;
                
            default:
                printf("❌ Invalid option.\n");
        }
    }
    
    display_header("🎉 VICTORY!");
    printf("You have completed all 4 levels!\n");
    printf("Final Score: %d points\n\n", engine.score);
    printf("Proficiency Summary:\n");
    for (int i = 0; i < 4; i++) {
        if (engine.session->proficiency[i]) {
            printf("  Level %d: %.1f%%\n", i, engine.session->proficiency[i]->proficiency_percentage);
        }
    }
    
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
    
    // Load questions from file
    question_pool_load_from_file(engine.question_pool, "data/questions.txt");
    
    // Initialize algorithms
    engine.banker = banker_init(3);  // 3 resources
    engine.scheduler = scheduler_init(SCHED_RR_ALG, 4);  // Round-robin with quantum 4
    engine.memory = memory_init(1024, PLACE_BEST_FIT);  // 1KB memory, best-fit
    engine.paging = page_system_init(4, PAGE_LRU);  // 4 frames, LRU
    engine.disk_sched = disk_scheduler_init(200, DISK_CSCAN);  // 200 tracks, C-SCAN
    
    // Display boot sequence
    display_header(NULL);
    printf("Initializing Interactive OS Algorithm Simulator...\n");
    sleep(1);
    printf("✓ Question pool loaded\n");
    sleep(1);
    printf("✓ Algorithms initialized\n");
    sleep(1);
    printf("✓ Ready to start\n\n");
    press_any_key();
    
    // Login flow
    if (!login_flow()) {
        printf("Exiting Interactive OS Algorithm Simulator.\n");
        return 0;
    }
    
    // Main game loop
    game_loop();
    
    // Cleanup
    login_system_destroy(engine.session);
    question_pool_destroy(engine.question_pool);
    banker_destroy(engine.banker);
    scheduler_destroy(engine.scheduler);
    memory_destroy(engine.memory);
    page_system_destroy(engine.paging);
    disk_scheduler_destroy(engine.disk_sched);
    pthread_mutex_destroy(&engine.timer_lock);
    
    printf("Thanks for playing Interactive OS Algorithm Simulator!\n");
    
    return 0;
}
