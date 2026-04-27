#ifndef QUESTION_POOL_H
#define QUESTION_POOL_H

#include <time.h>

#define MAX_QUESTIONS 50
#define MAX_QUESTION_LEN 512
#define MAX_ANSWER_LEN 256
#define MAX_DIFFICULTY_LEVELS 4

typedef enum {
    BEGINNER = 0,
    INTERMEDIATE = 1,
    ADVANCED = 2,
    PROFICIENT = 3
} DifficultyLevel;

typedef struct {
    int id;
    char question[MAX_QUESTION_LEN];
    char answer[MAX_ANSWER_LEN];
    char hint[MAX_QUESTION_LEN];
    DifficultyLevel difficulty;
    int level_id;  // Which game level this question belongs to
    int points;
} Question;

typedef struct {
    Question questions[MAX_QUESTIONS];
    int question_count;
} QuestionPool;

typedef struct {
    float beginner_score;
    float intermediate_score;
    float advanced_score;
    float proficient_score;
    DifficultyLevel current_level;
    int questions_answered;
    int questions_correct;
    float proficiency_percentage;
} ProficiencyMetrics;

// Function declarations
QuestionPool* question_pool_init(void);
void question_pool_destroy(QuestionPool* pool);
Question* question_get_by_difficulty(QuestionPool* pool, int level_id, DifficultyLevel difficulty);
Question* question_get_random(QuestionPool* pool, int level_id, DifficultyLevel difficulty);
void question_pool_load_from_file(QuestionPool* pool, const char* filename);
void question_pool_add(QuestionPool* pool, const Question* question);
DifficultyLevel difficulty_get_next(DifficultyLevel current);
DifficultyLevel difficulty_get_previous(DifficultyLevel current);
ProficiencyMetrics* proficiency_init(void);
void proficiency_update(ProficiencyMetrics* metrics, int is_correct, DifficultyLevel level);
void proficiency_display(ProficiencyMetrics* metrics);
char* proficiency_get_grade(ProficiencyMetrics* metrics);

#endif // QUESTION_POOL_H
