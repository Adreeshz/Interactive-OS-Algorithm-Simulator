#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "login_system.h"
#include "active_sessions.h"

// Global active sessions manager (shared across the game)
static ActiveSessionsManager* global_active_sessions = NULL;

GameSession* login_system_init(void) {
    GameSession* session = (GameSession*)malloc(sizeof(GameSession));
    if (!session) return NULL;
    
    session->database = user_db_init();
    session->current_user = NULL;
    session->is_authenticated = 0;
    session->is_admin = 0;
    session->violations = 0;
    
    for (int i = 0; i < 6; i++) {
        session->proficiency[i] = proficiency_init();
    }
    
    // Initialize global active sessions manager on first call
    if (!global_active_sessions) {
        global_active_sessions = active_sessions_init();
        
        // Pre-populate with some simulated active players
        srand(time(NULL));
        const char* sample_usernames[] = {
            "alice_gamer", "bob_player", "charlie_dev", "diana_speedrun",
            "eve_master", "frank_rookie", "grace_pro", "henry_casual"
        };
        int num_samples = sizeof(sample_usernames) / sizeof(sample_usernames[0]);
        
        for (int i = 0; i < num_samples; i++) {
            active_sessions_add_player(global_active_sessions, sample_usernames[i]);
            // Randomize their states
            int player_id = i + 1;
            active_sessions_move_to_level(global_active_sessions, player_id, rand() % 6);
            for (int j = 0; j < 5 + rand() % 15; j++) {
                active_sessions_update_score(global_active_sessions, player_id, rand() % 50);
            }
        }
        
        // Start simulation of active players
        active_sessions_start_simulation(global_active_sessions);
    }
    
    return session;
}

void login_system_destroy(GameSession* session) {
    if (!session) return;
    
    if (session->is_authenticated) {
        login_system_logout(session);
    }
    
    // Stop and properly cleanup active sessions simulation
    if (global_active_sessions) {
        active_sessions_stop_simulation(global_active_sessions);
        
        // Wait for the simulation thread to finish
        if (global_active_sessions->sim_thread != 0) {
            pthread_join(global_active_sessions->sim_thread, NULL);
        }
        
        active_sessions_destroy(global_active_sessions);
        global_active_sessions = NULL;
    }
    
    if (session->database) {
        user_db_destroy(session->database);
    }
    
    for (int i = 0; i < 6; i++) {
        if (session->proficiency[i]) {
            free(session->proficiency[i]);
        }
    }
    
    free(session);
}

int login_system_login(GameSession* session, const char* username, const char* password) {
    if (!session || !session->database || !username || !password) return -1;
    
    int result = user_login(session->database, username, password);
    if (result != 0) return result;
    
    session->current_user = user_get_by_name(session->database, username);
    session->is_authenticated = 1;
    session->is_admin = (session->current_user->role == ROLE_ADMIN);
    session->violations = 0;
    
    return 0;
}

int login_system_register(GameSession* session, const char* username, const char* password) {
    if (!session || !session->database || !username || !password) return -1;
    
    return user_register(session->database, username, password);
}

void login_system_logout(GameSession* session) {
    if (!session || !session->current_user || !session->database) return;
    
    user_logout(session->database, session->current_user->username);
    session->current_user = NULL;
    session->is_authenticated = 0;
    session->is_admin = 0;
    session->violations = 0;
}

int login_system_is_authenticated(GameSession* session) {
    return (session && session->is_authenticated) ? 1 : 0;
}

int login_system_is_admin(GameSession* session) {
    return (session && session->is_admin) ? 1 : 0;
}

void login_system_display_login_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║                  🔐 LOGIN 🔐                      ║\n");
    printf("║         SYS_RESCUE: OS Algorithm Simulator         ║\n");
    printf("║                   v1.0.0                          ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void login_system_display_register_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║               📝 REGISTER 📝                       ║\n");
    printf("║         SYS_RESCUE: OS Algorithm Simulator         ║\n");
    printf("║                   v1.0.0                          ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void login_system_admin_panel(GameSession* session) {
    if (!session || !login_system_is_admin(session) || !session->database) {
        printf("❌ Admin access denied!\n");
        return;
    }
    
    int running = 1;
    while (running) {
        printf("\n╔════════════════════════════════════════════════════╗\n");
        printf("║            👨‍💼 ADMIN PANEL 👨‍💼                    ║\n");
        printf("║  1. View All Users                               ║\n");
        printf("║  2. Ban User                                     ║\n");
        printf("║  3. Unban User                                   ║\n");
        printf("║  4. View User Statistics                         ║\n");
        printf("║  5. Kill Misbehaving User Process                ║\n");
        printf("║  6. View Active Sessions                         ║\n");
        printf("║  7. Exit Admin Panel                             ║\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");
        
        printf("Select option [1-7]: ");
        char choice[10];
        if (fgets(choice, sizeof(choice), stdin) == NULL) continue;
        
        int option = atoi(choice);
        
        switch (option) {
            case 1: {
                printf("\n📋 ALL USERS:\n");
                printf("────────────────────────────────────────\n");
                for (int i = 0; i < session->database->user_count; i++) {
                    User* u = &session->database->users[i];
                    printf("Username: %s\n", u->username);
                    printf("  Role: %s\n", u->role == ROLE_ADMIN ? "ADMIN" : "USER");
                    printf("  Status: %s\n", u->is_banned ? "🔒 BANNED" : "✅ Active");
                    printf("  Total Sessions: %d\n", u->total_sessions);
                    printf("────────────────────────────────────────\n");
                }
                break;
            }
            
            case 2: {
                printf("Enter username to ban: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) != NULL) {
                    username[strcspn(username, "\n")] = 0;
                    if (user_ban(session->database, username) == 0) {
                        printf("✅ User '%s' has been BANNED.\n", username);
                    } else {
                        printf("❌ User not found.\n");
                    }
                }
                break;
            }
            
            case 3: {
                printf("Enter username to unban: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) != NULL) {
                    username[strcspn(username, "\n")] = 0;
                    if (user_unban(session->database, username) == 0) {
                        printf("✅ User '%s' has been UNBANNED.\n", username);
                    } else {
                        printf("❌ User not found.\n");
                    }
                }
                break;
            }
            
            case 4: {
                printf("Enter username: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) != NULL) {
                    username[strcspn(username, "\n")] = 0;
                    User* user = user_get_by_name(session->database, username);
                    if (user) {
                        printf("\n📊 USER STATISTICS:\n");
                        printf("────────────────────────────────────────\n");
                        printf("Username: %s\n", user->username);
                        printf("Total Sessions: %d\n", user->total_sessions);
                        printf("Last Login: %s", ctime(&user->last_login));
                        printf("Created: %s", ctime(&user->created_at));
                        printf("────────────────────────────────────────\n");
                    } else {
                        printf("❌ User not found.\n");
                    }
                }
                break;
            }
            
            case 5: {
                printf("Enter username of misbehaving user: ");
                char username[MAX_USERNAME_LEN];
                if (fgets(username, sizeof(username), stdin) != NULL) {
                    username[strcspn(username, "\n")] = 0;
                    
                    // Find and remove the user from active sessions
                    if (global_active_sessions) {
                        // Search for player by username
                        pthread_mutex_lock(&global_active_sessions->sessions_lock);
                        int player_id = -1;
                        for (int i = 0; i < global_active_sessions->active_count; i++) {
                            if (global_active_sessions->sessions[i].is_active &&
                                strcmp(global_active_sessions->sessions[i].username, username) == 0) {
                                player_id = global_active_sessions->sessions[i].player_id;
                                break;
                            }
                        }
                        pthread_mutex_unlock(&global_active_sessions->sessions_lock);
                        
                        if (player_id != -1) {
                            // Kill the user's process
                            printf("🔪 Sending SIGKILL to user '%s'...\n", username);
                            active_sessions_remove_player(global_active_sessions, player_id);
                            printf("✅ User process terminated and removed from active sessions.\n");
                            printf("📝 Violation recorded in audit log.\n");
                        } else {
                            printf("❌ User '%s' not found in active sessions.\n", username);
                        }
                    } else {
                        printf("❌ Active sessions manager not available.\n");
                    }
                }
                break;
            }
            
            case 6: {
                if (!global_active_sessions) {
                    printf("❌ No active sessions manager available.\n");
                    break;
                }
                
                int viewing = 1;
                while (viewing) {
                    active_sessions_print_table(global_active_sessions);
                    
                    printf("Options:\n");
                    printf("  1. View details of a player (enter player ID)\n");
                    printf("  2. Watch live updates (10 updates, 2 sec each)\n");
                    printf("  3. Back to admin menu\n");
                    printf("Select [1-3]: ");
                    
                    char sub_choice[10];
                    if (fgets(sub_choice, sizeof(sub_choice), stdin) == NULL) continue;
                    
                    int sub_option = atoi(sub_choice);
                    
                    switch (sub_option) {
                        case 1: {
                            printf("Enter player ID to view details: ");
                            char pid_str[10];
                            if (fgets(pid_str, sizeof(pid_str), stdin) != NULL) {
                                int player_id = atoi(pid_str);
                                active_sessions_print_player_details(global_active_sessions, player_id);
                            }
                            break;
                        }
                        
                        case 2: {
                            printf("\n");
                            active_sessions_display_live(global_active_sessions);
                            break;
                        }
                        
                        case 3:
                            viewing = 0;
                            break;
                            
                        default:
                            printf("❌ Invalid option.\n");
                    }
                }
                break;
            }
            
            case 7:
                running = 0;
                break;
                
            default:
                printf("❌ Invalid option.\n");
        }
    }
}

void login_system_add_violation(GameSession* session, const char* reason) {
    if (!session || !session->is_authenticated) return;
    
    session->violations++;
    
    // Record violation in session
    for (int i = 0; i < session->database->session_count; i++) {
        if (strcmp(session->database->active_sessions[i].username, session->current_user->username) == 0 &&
            session->database->active_sessions[i].is_active) {
            session->database->active_sessions[i].violations++;
            printf("⚠️  Violation recorded: %s\n", reason);
            break;
        }
    }
}

int login_system_check_violations(GameSession* session) {
    if (!session || !session->is_authenticated) return 0;
    
    // Ban user if violations exceed threshold
    if (session->violations >= 3) {
        printf("🚫 Too many violations! User banned from system.\n");
        user_ban(session->database, session->current_user->username);
        return 1;
    }
    
    return 0;
}
