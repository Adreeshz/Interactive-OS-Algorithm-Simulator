#ifndef ACTIVE_SESSIONS_H
#define ACTIVE_SESSIONS_H

#include <time.h>
#include <pthread.h>

#define MAX_ACTIVE_PLAYERS 10

typedef enum {
    PLAYING_LEVEL_0,
    PLAYING_LEVEL_1,
    PLAYING_LEVEL_2,
    PLAYING_LEVEL_3,
    PLAYING_LEVEL_4,
    PLAYING_LEVEL_5,
    IN_ADMIN_PANEL,
    IDLE,
    DISCONNECTED
} SessionState;

typedef struct {
    int player_id;
    char username[50];
    SessionState state;
    int current_level;
    int score;
    int questions_answered;
    int correct_answers;
    int violations;
    time_t login_time;
    time_t last_activity_time;
    int is_active;
    pthread_t sim_thread_id;
} ActiveGameSession;

typedef struct {
    ActiveGameSession sessions[MAX_ACTIVE_PLAYERS];
    int active_count;
    pthread_mutex_t sessions_lock;
    int simulation_running;
    pthread_t sim_thread;  // Store the simulation thread for proper cleanup
} ActiveSessionsManager;

// Function declarations
ActiveSessionsManager* active_sessions_init(void);
void active_sessions_destroy(ActiveSessionsManager* manager);

// Simulation functions
void active_sessions_start_simulation(ActiveSessionsManager* manager);
void active_sessions_stop_simulation(ActiveSessionsManager* manager);
void active_sessions_display_live(ActiveSessionsManager* manager);

// Session management
int active_sessions_add_player(ActiveSessionsManager* manager, const char* username);
int active_sessions_remove_player(ActiveSessionsManager* manager, int player_id);
void active_sessions_update_score(ActiveSessionsManager* manager, int player_id, int points);
void active_sessions_move_to_level(ActiveSessionsManager* manager, int player_id, int level);
void active_sessions_record_violation(ActiveSessionsManager* manager, int player_id);

// Display
void active_sessions_print_table(ActiveSessionsManager* manager);
void active_sessions_print_player_details(ActiveSessionsManager* manager, int player_id);

#endif // ACTIVE_SESSIONS_H
