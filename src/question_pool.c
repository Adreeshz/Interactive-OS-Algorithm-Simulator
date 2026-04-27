#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "question_pool.h"

QuestionPool* question_pool_init(void) {
    QuestionPool* pool = (QuestionPool*)malloc(sizeof(QuestionPool));
    if (!pool) return NULL;
    
    pool->question_count = 0;
    memset(pool->questions, 0, sizeof(pool->questions));
    
    return pool;
}

void question_pool_destroy(QuestionPool* pool) {
    if (pool) {
        free(pool);
    }
}

void question_pool_add(QuestionPool* pool, const Question* question) {
    if (!pool || !question || pool->question_count >= MAX_QUESTIONS) {
        return;
    }
    
    memcpy(&pool->questions[pool->question_count], question, sizeof(Question));
    pool->question_count++;
}

Question* question_get_by_difficulty(QuestionPool* pool, int level_id, DifficultyLevel difficulty) {
    if (!pool) return NULL;
    
    for (int i = 0; i < pool->question_count; i++) {
        if (pool->questions[i].level_id == level_id && 
            pool->questions[i].difficulty == difficulty) {
            return &pool->questions[i];
        }
    }
    
    return NULL;
}

Question* question_get_random(QuestionPool* pool, int level_id, DifficultyLevel difficulty) {
    if (!pool) return NULL;
    
    // Collect all matching questions
    Question* matching[MAX_QUESTIONS];
    int count = 0;
    
    for (int i = 0; i < pool->question_count; i++) {
        if (pool->questions[i].level_id == level_id && 
            pool->questions[i].difficulty == difficulty) {
            matching[count++] = &pool->questions[i];
        }
    }
    
    if (count == 0) return NULL;
    
    // Return random one
    return matching[rand() % count];
}

DifficultyLevel difficulty_get_next(DifficultyLevel current) {
    if (current < PROFICIENT) {
        return current + 1;
    }
    return PROFICIENT;
}

DifficultyLevel difficulty_get_previous(DifficultyLevel current) {
    if (current > BEGINNER) {
        return current - 1;
    }
    return BEGINNER;
}

ProficiencyMetrics* proficiency_init(void) {
    ProficiencyMetrics* metrics = (ProficiencyMetrics*)malloc(sizeof(ProficiencyMetrics));
    if (!metrics) return NULL;
    
    metrics->beginner_score = 0.0;
    metrics->intermediate_score = 0.0;
    metrics->advanced_score = 0.0;
    metrics->proficient_score = 0.0;
    metrics->current_level = BEGINNER;
    metrics->questions_answered = 0;
    metrics->questions_correct = 0;
    metrics->proficiency_percentage = 0.0;
    
    return metrics;
}

void proficiency_update(ProficiencyMetrics* metrics, int is_correct, DifficultyLevel level) {
    if (!metrics) return;
    
    metrics->questions_answered++;
    if (is_correct) {
        metrics->questions_correct++;
    }
    
    float points = is_correct ? 100.0 : 0.0;
    
    switch (level) {
        case BEGINNER:
            metrics->beginner_score = (metrics->beginner_score * 0.8) + (points * 0.2);
            break;
        case INTERMEDIATE:
            metrics->intermediate_score = (metrics->intermediate_score * 0.8) + (points * 0.2);
            break;
        case ADVANCED:
            metrics->advanced_score = (metrics->advanced_score * 0.8) + (points * 0.2);
            break;
        case PROFICIENT:
            metrics->proficient_score = (metrics->proficient_score * 0.8) + (points * 0.2);
            break;
    }
    
    // Calculate overall proficiency
    metrics->proficiency_percentage = 
        (metrics->beginner_score * 0.1 +
         metrics->intermediate_score * 0.3 +
         metrics->advanced_score * 0.35 +
         metrics->proficient_score * 0.25);
    
    // Adjust difficulty level based on score
    if (metrics->proficiency_percentage >= 80.0 && metrics->current_level < PROFICIENT) {
        metrics->current_level = difficulty_get_next(metrics->current_level);
    } else if (metrics->proficiency_percentage < 50.0 && metrics->current_level > BEGINNER) {
        metrics->current_level = difficulty_get_previous(metrics->current_level);
    }
}

void proficiency_display(ProficiencyMetrics* metrics) {
    if (!metrics) return;
    
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║           📊 PROFICIENCY REPORT 📊                 ║\n");
    printf("╚════════════════════════════════════════════════════╝\n\n");
    
    printf("Overall Proficiency: %.1f%% %s\n", metrics->proficiency_percentage, proficiency_get_grade(metrics));
    printf("Questions Answered: %d\n", metrics->questions_answered);
    printf("Correct Answers: %d\n", metrics->questions_correct);
    if (metrics->questions_answered > 0) {
        printf("Accuracy: %.1f%%\n\n", (metrics->questions_correct * 100.0) / metrics->questions_answered);
    }
    
    printf("Tier Scores:\n");
    printf("  🔹 Beginner:      %.1f%%\n", metrics->beginner_score);
    printf("  🟡 Intermediate:  %.1f%%\n", metrics->intermediate_score);
    printf("  🔶 Advanced:      %.1f%%\n", metrics->advanced_score);
    printf("  🔴 Proficient:    %.1f%%\n\n", metrics->proficient_score);
    
    printf("Current Level: ");
    switch (metrics->current_level) {
        case BEGINNER: printf("🔹 BEGINNER\n"); break;
        case INTERMEDIATE: printf("🟡 INTERMEDIATE\n"); break;
        case ADVANCED: printf("🔶 ADVANCED\n"); break;
        case PROFICIENT: printf("🔴 PROFICIENT\n"); break;
    }
}

char* proficiency_get_grade(ProficiencyMetrics* metrics) {
    if (!metrics) return "N/A";
    
    if (metrics->proficiency_percentage >= 90.0) return "A+ (Excellent)";
    if (metrics->proficiency_percentage >= 80.0) return "A (Very Good)";
    if (metrics->proficiency_percentage >= 70.0) return "B (Good)";
    if (metrics->proficiency_percentage >= 60.0) return "C (Satisfactory)";
    if (metrics->proficiency_percentage >= 50.0) return "D (Needs Improvement)";
    return "F (Failing)";
}

void question_pool_load_from_file(QuestionPool* pool, const char* filename) {
    if (!pool || !filename) return;
    
    FILE* file = fopen(filename, "r");
    if (!file) return;
    
    char line[MAX_QUESTION_LEN];
    while (fgets(line, sizeof(line), file) && pool->question_count < MAX_QUESTIONS) {
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0 || line[0] == '#') continue;
        
        // Simple parsing: id|level|difficulty|points|question|answer|hint
        Question q;
        char* ptr = line;
        char* token;
        int field = 0;
        
        token = strtok(ptr, "|");
        while (token && field < 7) {
            switch (field) {
                case 0: q.id = atoi(token); break;
                case 1: q.level_id = atoi(token); break;
                case 2: q.difficulty = atoi(token); break;
                case 3: q.points = atoi(token); break;
                case 4: strncpy(q.question, token, MAX_QUESTION_LEN - 1); break;
                case 5: strncpy(q.answer, token, MAX_ANSWER_LEN - 1); break;
                case 6: strncpy(q.hint, token, MAX_QUESTION_LEN - 1); break;
            }
            token = strtok(NULL, "|");
            field++;
        }
        
        question_pool_add(pool, &q);
    }
    
    fclose(file);
}
