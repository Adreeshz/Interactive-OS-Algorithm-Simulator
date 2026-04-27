#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <crypt.h>
#include <unistd.h>
#include "user_management.h"

static char* simple_hash(const char* password) {
    static char hash[MAX_PASSWORD_LEN];
    unsigned long hash_value = 5381;
    int c;
    int i = 0;
    
    while ((c = *password++) && i < MAX_PASSWORD_LEN - 1) {
        hash_value = ((hash_value << 5) + hash_value) + c;
        i++;
    }
    
    snprintf(hash, MAX_PASSWORD_LEN, "%lx", hash_value);
    return hash;
}

UserDatabase* user_db_init(void) {
    UserDatabase* db = (UserDatabase*)malloc(sizeof(UserDatabase));
    if (!db) return NULL;
    
    db->user_count = 0;
    db->session_count = 0;
    memset(db->users, 0, sizeof(db->users));
    memset(db->active_sessions, 0, sizeof(db->active_sessions));
    
    // Create data directory if it doesn't exist
    system("mkdir -p data");
    
    // Load existing users or create defaults
    user_db_load(db);
    
    // If no users exist, create default admin
    if (db->user_count == 0) {
        admin_create_default_user(db);
    }
    
    return db;
}

void user_db_destroy(UserDatabase* db) {
    if (db) {
        user_db_save(db);
        free(db);
    }
}

int user_register(UserDatabase* db, const char* username, const char* password) {
    if (!db || !username || !password) return -1;
    if (db->user_count >= MAX_USERS) return -2;  // Database full
    if (user_exists(db, username)) return -3;     // User already exists
    if (strlen(username) > MAX_USERNAME_LEN - 1) return -4;  // Username too long
    if (strlen(password) > MAX_PASSWORD_LEN - 1) return -5;  // Password too long
    
    User* new_user = &db->users[db->user_count];
    strncpy(new_user->username, username, MAX_USERNAME_LEN - 1);
    strncpy(new_user->password_hash, simple_hash(password), MAX_PASSWORD_LEN - 1);
    new_user->role = ROLE_USER;
    new_user->is_banned = 0;
    new_user->created_at = time(NULL);
    new_user->last_login = 0;
    new_user->total_sessions = 0;
    
    db->user_count++;
    user_db_save(db);
    
    return 0;  // Success
}

int user_login(UserDatabase* db, const char* username, const char* password) {
    if (!db || !username || !password) return -1;
    
    User* user = user_get_by_name(db, username);
    if (!user) return -2;  // User not found
    if (user->is_banned) return -3;  // User is banned
    
    char* hash = simple_hash(password);
    if (strcmp(hash, user->password_hash) != 0) {
        return -4;  // Wrong password
    }
    
    // Create session
    UserSession* session = &db->active_sessions[db->session_count];
    strncpy(session->username, username, MAX_USERNAME_LEN - 1);
    session->login_time = time(NULL);
    session->logout_time = 0;
    session->is_active = 1;
    session->violations = 0;
    
    db->session_count++;
    user->last_login = time(NULL);
    user->total_sessions++;
    
    user_db_save(db);
    return 0;  // Success
}

int user_logout(UserDatabase* db, const char* username) {
    if (!db || !username) return -1;
    
    for (int i = 0; i < db->session_count; i++) {
        if (strcmp(db->active_sessions[i].username, username) == 0 && 
            db->active_sessions[i].is_active) {
            db->active_sessions[i].is_active = 0;
            db->active_sessions[i].logout_time = time(NULL);
            user_db_save(db);
            return 0;
        }
    }
    
    return -2;  // Session not found
}

User* user_get_by_name(UserDatabase* db, const char* username) {
    if (!db || !username) return NULL;
    
    for (int i = 0; i < db->user_count; i++) {
        if (strcmp(db->users[i].username, username) == 0) {
            return &db->users[i];
        }
    }
    
    return NULL;
}

int user_exists(UserDatabase* db, const char* username) {
    return user_get_by_name(db, username) != NULL;
}

void user_db_save(UserDatabase* db) {
    if (!db) return;
    
    FILE* file = fopen(USER_DB_FILE, "w");
    if (!file) return;
    
    // Save users
    fprintf(file, "=== USERS ===\n");
    for (int i = 0; i < db->user_count; i++) {
        User* u = &db->users[i];
        fprintf(file, "%s|%s|%d|%d|%ld|%ld|%d\n",
                u->username, u->password_hash, u->role, u->is_banned,
                u->created_at, u->last_login, u->total_sessions);
    }
    
    // Save sessions
    fprintf(file, "=== SESSIONS ===\n");
    for (int i = 0; i < db->session_count; i++) {
        UserSession* s = &db->active_sessions[i];
        fprintf(file, "%s|%ld|%ld|%d|%d\n",
                s->username, s->login_time, s->logout_time, s->is_active, s->violations);
    }
    
    fclose(file);
}

void user_db_load(UserDatabase* db) {
    if (!db) return;
    
    FILE* file = fopen(USER_DB_FILE, "r");
    if (!file) return;
    
    char line[512];
    int reading_users = 0;
    int reading_sessions = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        
        if (strcmp(line, "=== USERS ===") == 0) {
            reading_users = 1;
            reading_sessions = 0;
            continue;
        }
        
        if (strcmp(line, "=== SESSIONS ===") == 0) {
            reading_users = 0;
            reading_sessions = 1;
            continue;
        }
        
        if (reading_users && strlen(line) > 0) {
            User* u = &db->users[db->user_count];
            char* token = strtok(line, "|");
            int field = 0;
            
            while (token && field < 7) {
                switch (field) {
                    case 0: strncpy(u->username, token, MAX_USERNAME_LEN - 1); break;
                    case 1: strncpy(u->password_hash, token, MAX_PASSWORD_LEN - 1); break;
                    case 2: u->role = atoi(token); break;
                    case 3: u->is_banned = atoi(token); break;
                    case 4: u->created_at = (time_t)atol(token); break;
                    case 5: u->last_login = (time_t)atol(token); break;
                    case 6: u->total_sessions = atoi(token); break;
                }
                token = strtok(NULL, "|");
                field++;
            }
            
            db->user_count++;
            if (db->user_count >= MAX_USERS) break;
        }
        
        if (reading_sessions && strlen(line) > 0) {
            UserSession* s = &db->active_sessions[db->session_count];
            char* token = strtok(line, "|");
            int field = 0;
            
            while (token && field < 5) {
                switch (field) {
                    case 0: strncpy(s->username, token, MAX_USERNAME_LEN - 1); break;
                    case 1: s->login_time = (time_t)atol(token); break;
                    case 2: s->logout_time = (time_t)atol(token); break;
                    case 3: s->is_active = atoi(token); break;
                    case 4: s->violations = atoi(token); break;
                }
                token = strtok(NULL, "|");
                field++;
            }
            
            db->session_count++;
            if (db->session_count >= MAX_USERS) break;
        }
    }
    
    fclose(file);
}

int user_ban(UserDatabase* db, const char* username) {
    User* user = user_get_by_name(db, username);
    if (!user) return -1;
    
    user->is_banned = 1;
    user_db_save(db);
    return 0;
}

int user_unban(UserDatabase* db, const char* username) {
    User* user = user_get_by_name(db, username);
    if (!user) return -1;
    
    user->is_banned = 0;
    user_db_save(db);
    return 0;
}

int user_check_violations(UserDatabase* db, const char* username) {
    for (int i = 0; i < db->session_count; i++) {
        if (strcmp(db->active_sessions[i].username, username) == 0 &&
            db->active_sessions[i].is_active) {
            return db->active_sessions[i].violations;
        }
    }
    return 0;
}

char* user_hash_password(const char* password) {
    return simple_hash(password);
}

int user_verify_password(const char* password, const char* hash) {
    return strcmp(simple_hash(password), hash) == 0;
}

void admin_create_default_user(UserDatabase* db) {
    // Create default admin user
    User* admin = &db->users[0];
    strncpy(admin->username, "admin", MAX_USERNAME_LEN - 1);
    strncpy(admin->password_hash, simple_hash("admin123"), MAX_PASSWORD_LEN - 1);
    admin->role = ROLE_ADMIN;
    admin->is_banned = 0;
    admin->created_at = time(NULL);
    admin->last_login = 0;
    admin->total_sessions = 0;
    
    db->user_count = 1;
    user_db_save(db);
}
