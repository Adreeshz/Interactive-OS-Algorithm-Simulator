#ifndef LOGIN_SYSTEM_H
#define LOGIN_SYSTEM_H

#include "user_management.h"
#include "question_pool.h"

typedef struct {
    User* current_user;
    UserDatabase* database;
    int is_authenticated;
    int is_admin;
    ProficiencyMetrics* proficiency[6];  // One for each level (0-5)
    int violations;
} GameSession;

// Function declarations
GameSession* login_system_init(void);
void login_system_destroy(GameSession* session);
int login_system_login(GameSession* session, const char* username, const char* password);
int login_system_register(GameSession* session, const char* username, const char* password);
void login_system_logout(GameSession* session);
int login_system_is_authenticated(GameSession* session);
int login_system_is_admin(GameSession* session);
void login_system_display_login_screen(void);
void login_system_display_register_screen(void);
void login_system_admin_panel(GameSession* session);
void login_system_add_violation(GameSession* session, const char* reason);
int login_system_check_violations(GameSession* session);

#endif // LOGIN_SYSTEM_H
