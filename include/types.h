#ifndef TYPES_H
#define TYPES_H

// تعريف Position في ملف مستقل
typedef struct {
    int x;
    int y;
    int z;
} Position;

// تعريف Position3D
typedef struct {
    int x, y, z;
} Position3D;

// تعريف Map3D الكامل
typedef struct {
    int width, height, depth;
    char*** grid;
    Position3D start;
    Position3D exit;
    int obstacles;
    int survivors;
} Map3D;

// تعريف Chromosome الكامل
typedef struct {
    int id;
    float fitness;
    int* moves;  // استخدام int بدلاً من Direction للتسهيل
    int num_moves;
    Position3D start_pos;
    int survivors_rescued;
    int coverage_cells;
} Chromosome;

// تعريف Population الكامل
typedef struct {
    Chromosome** individuals;
    int size;
    float best_fitness;
    float worst_fitness;
    float average_fitness;
    Chromosome* best_chromosome;
} Population;

#endif 

