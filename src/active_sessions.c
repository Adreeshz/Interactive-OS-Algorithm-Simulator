#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "active_sessions.h"

/* ============================================
   INITIALIZATION & CLEANUP
   ============================================ */

ActiveSessionsManager* active_sessions_init(void) {
    ActiveSessionsManager* manager = malloc(sizeof(ActiveSessionsManager));
    if (!manager) return NULL;
    
    memset(manager, 0, sizeof(ActiveSessionsManager));
    pthread_mutex_init(&manager->sessions_lock, NULL);
    manager->simulation_running = 0;
    manager->active_count = 0;
    
    return manager;
}

void active_sessions_destroy(ActiveSessionsManager* manager) {
    if (!manager) return;
    
    pthread_mutex_destroy(&manager->sessions_lock);
    free(manager);
}

/* ============================================
   SESSION MANAGEMENT
   ============================================ */

int active_sessions_add_player(ActiveSessionsManager* manager, const char* username) {
    if (!manager || !username || manager->active_count >= MAX_ACTIVE_PLAYERS) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->sessions_lock);
    
    // Create new session
    ActiveGameSession* session = &manager->sessions[manager->active_count];
    session->player_id = manager->active_count + 1;
    strncpy(session->username, username, sizeof(session->username) - 1);
    session->state = IDLE;
    session->current_level = 0;
    session->score = 0;
    session->questions_answered = 0;
    session->correct_answers = 0;
    session->violations = 0;
    session->login_time = time(NULL);
    session->last_activity_time = time(NULL);
    session->is_active = 1;
    
    manager->active_count++;
    
    pthread_mutex_unlock(&manager->sessions_lock);
    return session->player_id;
}

int active_sessions_remove_player(ActiveSessionsManager* manager, int player_id) {
    if (!manager || player_id < 1 || player_id > MAX_ACTIVE_PLAYERS) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->sessions_lock);
    
    for (int i = 0; i < manager->active_count; i++) {
        if (manager->sessions[i].player_id == player_id) {
            manager->sessions[i].is_active = 0;
            manager->sessions[i].state = DISCONNECTED;
            pthread_mutex_unlock(&manager->sessions_lock);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&manager->sessions_lock);
    return -1;
}

void active_sessions_update_score(ActiveSessionsManager* manager, int player_id, int points) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->sessions_lock);
    
    for (int i = 0; i < manager->active_count; i++) {
        if (manager->sessions[i].player_id == player_id && manager->sessions[i].is_active) {
            manager->sessions[i].score += points;
            manager->sessions[i].last_activity_time = time(NULL);
            break;
        }
    }
    
    pthread_mutex_unlock(&manager->sessions_lock);
}

void active_sessions_move_to_level(ActiveSessionsManager* manager, int player_id, int level) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->sessions_lock);
    
    for (int i = 0; i < manager->active_count; i++) {
        if (manager->sessions[i].player_id == player_id && manager->sessions[i].is_active) {
            manager->sessions[i].current_level = level;
            
            // Update state based on level
            if (level == -1) {
                manager->sessions[i].state = IN_ADMIN_PANEL;
            } else {
                switch (level) {
                    case 0:
                        manager->sessions[i].state = PLAYING_LEVEL_0;
                        break;
                    case 1:
                        manager->sessions[i].state = PLAYING_LEVEL_1;
                        break;
                    case 2:
                        manager->sessions[i].state = PLAYING_LEVEL_2;
                        break;
                    case 3:
                        manager->sessions[i].state = PLAYING_LEVEL_3;
                        break;
                    case 4:
                        manager->sessions[i].state = PLAYING_LEVEL_4;
                        break;
                    case 5:
                        manager->sessions[i].state = PLAYING_LEVEL_5;
                        break;
                    default:
                        manager->sessions[i].state = IDLE;
                }
            }
            manager->sessions[i].last_activity_time = time(NULL);
            break;
        }
    }
    
    pthread_mutex_unlock(&manager->sessions_lock);
}

void active_sessions_record_violation(ActiveSessionsManager* manager, int player_id) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->sessions_lock);
    
    for (int i = 0; i < manager->active_count; i++) {
        if (manager->sessions[i].player_id == player_id && manager->sessions[i].is_active) {
            manager->sessions[i].violations++;
            manager->sessions[i].last_activity_time = time(NULL);
            break;
        }
    }
    
    pthread_mutex_unlock(&manager->sessions_lock);
}

/* ============================================
   DISPLAY FUNCTIONS
   ============================================ */

const char* session_state_to_string(SessionState state) {
    switch (state) {
        case PLAYING_LEVEL_0:
            return "🎮 Level 0: File System Recovery";
        case PLAYING_LEVEL_1:
            return "🎮 Level 1: Thread Restoration";
        case PLAYING_LEVEL_2:
            return "🎮 Level 2: Synchronization";
        case PLAYING_LEVEL_3:
            return "🎮 Level 3: Memory Management";
        case PLAYING_LEVEL_4:
            return "🎮 Level 4: Page Replacement";
        case PLAYING_LEVEL_5:
            return "🎮 Level 5: Disk Scheduling";
        case IN_ADMIN_PANEL:
            return "👨‍💼 Admin Panel";
        case IDLE:
            return "⏸️  Idle (In Menu)";
        case DISCONNECTED:
            return "❌ Disconnected";
        default:
            return "❓ Unknown";
    }
}

void active_sessions_print_table(ActiveSessionsManager* manager) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->sessions_lock);
    
    printf("\n╔═══════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          🎮 ACTIVE GAME SESSIONS 🎮                                   ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════════════════════╝\n\n");
    
    if (manager->active_count == 0) {
        printf("No active sessions at this time.\n\n");
        pthread_mutex_unlock(&manager->sessions_lock);
        return;
    }
    
    // Header
    printf("┌────┬──────────────────┬────────────────────────────────┬───────┬────────┬────────────┐\n");
    printf("│ ID │ Username         │ Current Activity               │ Score │ Q/C    │ Violations │\n");
    printf("├────┼──────────────────┼────────────────────────────────┼───────┼────────┼────────────┤\n");
    
    // Rows
    for (int i = 0; i < manager->active_count; i++) {
        ActiveGameSession* session = &manager->sessions[i];
        
        if (session->is_active) {
            int questions = session->questions_answered;
            int correct = session->correct_answers;
            
            printf("│ %2d │ %-16s │ %-30s │ %5d │ %2d/%-2d │ %10d     │\n",
                   session->player_id,
                   session->username,
                   session_state_to_string(session->state),
                   session->score,
                   questions, correct,
                   session->violations);
        }
    }
    
    printf("└────┴──────────────────┴────────────────────────────────┴───────┴────────┴────────────┘\n\n");
    
    // Summary statistics
    int total_players = 0;
    int total_score = 0;
    int total_questions = 0;
    int total_correct = 0;
    
    for (int i = 0; i < manager->active_count; i++) {
        if (manager->sessions[i].is_active) {
            total_players++;
            total_score += manager->sessions[i].score;
            total_questions += manager->sessions[i].questions_answered;
            total_correct += manager->sessions[i].correct_answers;
        }
    }
    
    printf("📊 SESSION STATISTICS:\n");
    printf("   • Active Players: %d\n", total_players);
    printf("   • Combined Score: %d\n", total_score);
    printf("   • Total Questions: %d\n", total_questions);
    printf("   • Correct Answers: %d\n", total_correct);
    
    if (total_questions > 0) {
        float accuracy = (float)total_correct / total_questions * 100.0f;
        printf("   • Overall Accuracy: %.1f%%\n", accuracy);
    }
    printf("\n");
    
    pthread_mutex_unlock(&manager->sessions_lock);
}

void active_sessions_print_player_details(ActiveSessionsManager* manager, int player_id) {
    if (!manager) return;
    
    pthread_mutex_lock(&manager->sessions_lock);
    
    for (int i = 0; i < manager->active_count; i++) {
        if (manager->sessions[i].player_id == player_id && manager->sessions[i].is_active) {
            ActiveGameSession* session = &manager->sessions[i];
            
            printf("\n╔═══════════════════════════════════════════════════════════╗\n");
            printf("║          📋 PLAYER DETAILS - %s\n", session->username);
            printf("╚═══════════════════════════════════════════════════════════╝\n\n");
            
            printf("🆔 Player ID: %d\n", session->player_id);
            printf("👤 Username: %s\n", session->username);
            printf("🎮 Current Activity: %s\n", session_state_to_string(session->state));
            printf("📊 Score: %d points\n", session->score);
            printf("❓ Questions Answered: %d\n", session->questions_answered);
            printf("✅ Correct Answers: %d\n", session->correct_answers);
            
            if (session->questions_answered > 0) {
                float accuracy = (float)session->correct_answers / session->questions_answered * 100.0f;
                printf("📈 Accuracy: %.1f%%\n", accuracy);
            }
            
            printf("⚠️  Violations: %d\n", session->violations);
            
            // Time info
            time_t now = time(NULL);
            int session_duration = (int)(now - session->login_time);
            int last_activity = (int)(now - session->last_activity_time);
            
            printf("⏱️  Session Duration: %d minutes\n", session_duration / 60);
            printf("🔄 Last Activity: %d seconds ago\n\n", last_activity);
            
            pthread_mutex_unlock(&manager->sessions_lock);
            return;
        }
    }
    
    printf("❌ Player with ID %d not found.\n\n", player_id);
    pthread_mutex_unlock(&manager->sessions_lock);
}

/* ============================================
   SIMULATION THREAD FUNCTION
   ============================================ */

void* active_sessions_simulation_thread(void* arg) {
    ActiveSessionsManager* manager = (ActiveSessionsManager*)arg;
    
    // Simulate activities for each player
    while (manager->simulation_running) {
        pthread_mutex_lock(&manager->sessions_lock);
        
        for (int i = 0; i < manager->active_count; i++) {
            if (!manager->sessions[i].is_active) continue;
            
            ActiveGameSession* session = &manager->sessions[i];
            
            // Randomly update player state and score
            int action = rand() % 100;
            
            if (action < 40) {
                // Player answered a question
                session->questions_answered++;
                if (rand() % 100 < 75) {  // 75% accuracy
                    session->correct_answers++;
                    session->score += 10;
                }
                session->last_activity_time = time(NULL);
            } else if (action < 60) {
                // Player moved to a different level
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
            } else if (action < 75) {
                // Player is in menu
                session->state = IDLE;
            } else if (action < 85) {
                // Violation recorded (rarely)
                session->violations++;
                session->last_activity_time = time(NULL);
            }
        }
        
        pthread_mutex_unlock(&manager->sessions_lock);
        
        // Update every 2 seconds
        sleep(2);
    }
    
    return NULL;
}

void active_sessions_start_simulation(ActiveSessionsManager* manager) {
    if (!manager || manager->simulation_running) return;
    
    manager->simulation_running = 1;
    
    // Start simulation thread and store its ID
    pthread_create(&manager->sim_thread, NULL, active_sessions_simulation_thread, manager);
}

void active_sessions_stop_simulation(ActiveSessionsManager* manager) {
    if (!manager) return;
    
    manager->simulation_running = 0;
}

void active_sessions_display_live(ActiveSessionsManager* manager) {
    if (!manager) return;
    
    printf("\n🔴 LIVE SESSION MONITORING (updating every 2 seconds)...\n");
    printf("Press Ctrl+C to stop monitoring.\n\n");
    
    int display_count = 0;
    while (display_count < 10) {  // Show 10 updates
        active_sessions_print_table(manager);
        sleep(2);
        display_count++;
        
        if (display_count < 10) {
            printf("\n⏳ Updating in 2 seconds...\n");
        }
    }
    
    printf("✅ Monitoring complete.\n");
}
