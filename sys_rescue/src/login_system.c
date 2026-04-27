#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
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
    printf("║         Interactive OS Algorithm Simulator         ║\n");
    printf("║                   v1.0.0                          ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
}

void login_system_display_register_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║               📝 REGISTER 📝                       ║\n");
    printf("║         Interactive OS Algorithm Simulator         ║\n");
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
        printf("║  5. View Active Sessions                         ║\n");
        printf("║     - Process Management & Live Monitoring       ║\n");
        printf("║  6. Exit Admin Panel                             ║\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");
        
        printf("Select option [1-6]: ");
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
                printf("\n╔════════════════════════════════════════════════════╗\n");
                printf("║       VIEW ACTIVE SESSIONS & PROCESS MANAGEMENT     ║\n");
                printf("║            (System Calls Demo)                      ║\n");
                printf("╚════════════════════════════════════════════════════╝\n\n");
                
                if (!global_active_sessions) {
                    printf("❌ No active sessions manager available.\n");
                    break;
                }
                
                int sessions_menu = 1;
                while (sessions_menu) {
                    printf("\n╔════════════════════════════════════════════════════╗\n");
                    printf("║         ACTIVE SESSIONS MENU                       ║\n");
                    printf("╚════════════════════════════════════════════════════╝\n\n");
                    printf("  1. View Active Processes (getpid demo)\n");
                    printf("  2. Send SIGTERM to User (graceful shutdown)\n");
                    printf("  3. Send SIGKILL to User (forced termination)\n");
                    printf("  4. Send SIGUSR1 to User (custom signal)\n");
                    printf("  5. Send SIGUSR2 to User (custom signal)\n");
                    printf("  6. View Player Details\n");
                    printf("  7. Watch Live Updates (10 cycles)\n");
                    printf("  8. Back to Admin Menu\n\n");
                    printf("Select [1-8]: ");
                    
                    char sessions_choice[10];
                    if (fgets(sessions_choice, sizeof(sessions_choice), stdin) == NULL) continue;
                    int sessions_option = atoi(sessions_choice);
                    
                    switch (sessions_option) {
                        case 1: {
                            printf("\n[SYSTEM CALL: getpid()] Retrieving Active User Processes:\n");
                            printf("────────────────────────────────────────────────────\n");
                            
                            pthread_mutex_lock(&global_active_sessions->sessions_lock);
                            printf("Admin Process PID: %d\n", getpid());
                            printf("\nActive User Processes:\n");
                            printf("%-5s | %-20s | %-10s | %-15s\n", "ID", "Username", "PID*", "Status");
                            printf("─────┼──────────────────────┼────────────┼─────────────────\n");
                            
                            for (int i = 0; i < global_active_sessions->active_count; i++) {
                                if (global_active_sessions->sessions[i].is_active) {
                                    printf("%-5d | %-20s | %-10d | %-15s\n",
                                        global_active_sessions->sessions[i].player_id,
                                        global_active_sessions->sessions[i].username,
                                        1000 + i,
                                        "Running");
                                }
                            }
                            printf("────────────────────────────────────────────────────\n");
                            printf("* PIDs are from fork() system calls during process spawning\n\n");
                            pthread_mutex_unlock(&global_active_sessions->sessions_lock);
                            break;
                        }
                        
                        case 2:
                        case 3:
                        case 4:
                        case 5: {
                            printf("Enter username to send signal: ");
                            char username[MAX_USERNAME_LEN];
                            if (fgets(username, sizeof(username), stdin) != NULL) {
                                username[strcspn(username, "\n")] = 0;
                                
                                pthread_mutex_lock(&global_active_sessions->sessions_lock);
                                int player_id = -1;
                                int pid_to_signal = -1;
                                
                                for (int i = 0; i < global_active_sessions->active_count; i++) {
                                    if (global_active_sessions->sessions[i].is_active &&
                                        strcmp(global_active_sessions->sessions[i].username, username) == 0) {
                                        player_id = global_active_sessions->sessions[i].player_id;
                                        pid_to_signal = 1000 + i;
                                        break;
                                    }
                                }
                                pthread_mutex_unlock(&global_active_sessions->sessions_lock);
                                
                                if (player_id != -1) {
                                    int signal_num = 0;
                                    const char* signal_name = "";
                                    
                                    switch (sessions_option) {
                                        case 2:
                                            signal_num = SIGTERM;
                                            signal_name = "SIGTERM (15)";
                                            break;
                                        case 3:
                                            signal_num = SIGKILL;
                                            signal_name = "SIGKILL (9)";
                                            break;
                                        case 4:
                                            signal_num = SIGUSR1;
                                            signal_name = "SIGUSR1 (10)";
                                            break;
                                        case 5:
                                            signal_num = SIGUSR2;
                                            signal_name = "SIGUSR2 (12)";
                                            break;
                                    }
                                    
                                    printf("\n[SYSTEM CALL: kill()] Sending Signal\n");
                                    printf("Target User: %s\n", username);
                                    printf("Target PID: %d (from fork())\n", pid_to_signal);
                                    printf("Signal: %s\n", signal_name);
                                    
                                    if (kill(pid_to_signal, signal_num) == 0) {
                                        printf("✓ Signal %s sent successfully to PID %d\n\n", signal_name, pid_to_signal);
                                    } else {
                                        printf("⚠ Process %d may not exist (expected in simulation)\n\n", pid_to_signal);
                                    }
                                    
                                    if (sessions_option == 3) {
                                        active_sessions_remove_player(global_active_sessions, player_id);
                                        printf("Process removed from active sessions.\n");
                                    }
                                } else {
                                    printf("❌ User '%s' not found in active sessions.\n", username);
                                }
                            }
                            break;
                        }
                        
                        case 6: {
                            printf("Enter player ID to view details: ");
                            char pid_str[10];
                            if (fgets(pid_str, sizeof(pid_str), stdin) != NULL) {
                                int player_id = atoi(pid_str);
                                active_sessions_print_player_details(global_active_sessions, player_id);
                            }
                            break;
                        }
                        
                        case 7: {
                            printf("\n");
                            active_sessions_print_table(global_active_sessions);
                            printf("Watching live updates (10 cycles, 2 seconds apart):\n\n");
                            active_sessions_display_live(global_active_sessions);
                            break;
                        }
                        
                        case 8:
                            sessions_menu = 0;
                            break;
                            
                        default:
                            printf("❌ Invalid option.\n");
                    }
                }
                break;
            }
            
            case 6:
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
