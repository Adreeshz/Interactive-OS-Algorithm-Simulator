#ifndef USER_MANAGEMENT_H
#define USER_MANAGEMENT_H

#include <time.h>

#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 100
#define MAX_USERS 100
#define USER_DB_FILE "data/users.txt"
#define SESSION_DB_FILE "data/sessions.txt"

typedef enum {
    ROLE_USER = 0,
    ROLE_ADMIN = 1
} UserRole;

typedef struct {
    char username[MAX_USERNAME_LEN];
    char password_hash[MAX_PASSWORD_LEN];
    UserRole role;
    int is_banned;
    time_t created_at;
    time_t last_login;
    int total_sessions;
} User;

typedef struct {
    char username[MAX_USERNAME_LEN];
    time_t login_time;
    time_t logout_time;
    int is_active;
    int violations;
} UserSession;

typedef struct {
    User users[MAX_USERS];
    int user_count;
    UserSession active_sessions[MAX_USERS];
    int session_count;
} UserDatabase;

// Function declarations
UserDatabase* user_db_init(void);
void user_db_destroy(UserDatabase* db);
int user_register(UserDatabase* db, const char* username, const char* password);
int user_login(UserDatabase* db, const char* username, const char* password);
int user_logout(UserDatabase* db, const char* username);
User* user_get_by_name(UserDatabase* db, const char* username);
int user_exists(UserDatabase* db, const char* username);
void user_db_save(UserDatabase* db);
void user_db_load(UserDatabase* db);
int user_ban(UserDatabase* db, const char* username);
int user_unban(UserDatabase* db, const char* username);
int user_check_violations(UserDatabase* db, const char* username);
char* user_hash_password(const char* password);
int user_verify_password(const char* password, const char* hash);
void admin_create_default_user(UserDatabase* db);

#endif // USER_MANAGEMENT_H
