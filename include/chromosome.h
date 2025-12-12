#ifndef CHROMOSOME_H
#define CHROMOSOME_H

#include "map_loader.h"
#include <stdbool.h>

// ============= تعريف الاتجاهات =============
typedef enum {
    DIR_RIGHT = 0,      // زيادة في x
    DIR_LEFT = 1,       // نقصان في x
    DIR_UP = 2,         // زيادة في y
    DIR_DOWN = 3,       // نقصان في y
    DIR_FORWARD = 4,    // زيادة في z
    DIR_BACKWARD = 5,   // نقصان في z
    DIR_WAIT = 6        // البقاء في المكان
} Direction;

// ============= هيكل الكروموسوم (مسار واحد) =============
typedef struct {
    // البيانات الأساسية
    Position start_pos;          // نقطة البداية
    Direction *moves;            // مصفوفة الاتجاهات
    int num_moves;               // عدد الحركات الفعلية
    int max_moves;               // السعة القصوى
    
    // نتائج التقييم
    float fitness;               // قيمة اللياقة
    int survivors_rescued;       // عدد الناجين المغطى عليهم
    int coverage_cells;          // عدد الخلايا المغطاة
    float total_length;          // طول المسار
    float total_risk;            // إجمالي المخاطرة
    float time_estimate;         // الوقت المقدر
    
    // معلومات إضافية
    int id;                      // معرف فريد
    bool valid;                  // هل المسار صالح؟
    Position *actual_path;       // المسار الفعلي (مخزن مؤقت)
    int actual_path_length;      // طول المسار الفعلي
} Chromosome;

// ============= هيكل المجتمع =============
typedef struct {
    Chromosome *individuals;     // مصفوفة الكروموسومات
    int size;                    // حجم المجتمع
    int generation;              // رقم الجيل الحالي
    
    // إحصائيات
    Chromosome *best;            // أفضل كروموسوم
    float best_fitness;          // أفضل لياقة
    float avg_fitness;           // متوسط اللياقة
    float worst_fitness;         // أسوأ لياقة
} Population;

// ============= دوال الكروموسوم =============

// الإنشاء والإعداد
Chromosome* create_chromosome(Position start, int max_steps);
Chromosome* create_random_chromosome(Position start, int max_steps);
void init_chromosome(Chromosome *chrom, Position start, int max_steps);
void init_random_chromosome(Chromosome *chrom, Position start, int max_steps);
void copy_chromosome(Chromosome *dest, const Chromosome *src);
Chromosome* clone_chromosome(const Chromosome *src);
void free_chromosome(Chromosome *chrom);

// التحويل والتحقق
Position* decode_chromosome_to_path(const Chromosome *chrom, int *path_length);
Position* decode_chromosome_with_bounds(const Chromosome *chrom, int *path_length, 
                                        const Map3D *map);
bool is_valid_move(const Chromosome *chrom, int move_index, const Map3D *map);
bool validate_chromosome(const Chromosome *chrom, const Map3D *map);
void repair_chromosome(Chromosome *chrom, const Map3D *map);

// التقييم
float evaluate_chromosome_fitness(Chromosome *chrom, const Map3D *map, 
                                  float w_survivors, float w_coverage, 
                                  float w_length, float w_risk);
int count_survivors_on_path(const Chromosome *chrom, const Map3D *map);
int count_coverage_cells(const Chromosome *chrom, const Map3D *map);
float calculate_path_length(const Chromosome *chrom);
float calculate_path_risk(const Chromosome *chrom, const Map3D *map);

// العمليات الجينية
void mutate_chromosome(Chromosome *chrom, float mutation_rate, const Map3D *map);
void mutate_direction(Chromosome *chrom, int move_index);
void mutate_insert_move(Chromosome *chrom, int position);
void mutate_delete_move(Chromosome *chrom, int position);
void mutate_swap_moves(Chromosome *chrom, int pos1, int pos2);

void crossover_chromosomes(const Chromosome *parent1, const Chromosome *parent2,
                           Chromosome *child1, Chromosome *child2, 
                           float crossover_rate);

// العرض والطباعة
void print_chromosome(const Chromosome *chrom);
void print_chromosome_directions(const Chromosome *chrom);
void print_chromosome_path(const Chromosome *chrom);
void print_chromosome_stats(const Chromosome *chrom);
void save_chromosome_to_file(const Chromosome *chrom, const char *filename);

// ============= دوال المجتمع =============

// الإنشاء والإعداد
Population* create_population(int size);
Population* create_initial_population(Position start_pos, int pop_size, 
                                      int max_steps, const Map3D *map);
void init_population(Population *pop, Position start_pos, int max_steps, 
                     const Map3D *map);
void free_population(Population *pop);

// التقييم والفرز
void evaluate_population(Population *pop, const Map3D *map,
                         float w_survivors, float w_coverage,
                         float w_length, float w_risk);
void sort_population_by_fitness(Population *pop);
Chromosome* get_best_chromosome(Population *pop);
Chromosome* get_worst_chromosome(Population *pop);
void calculate_population_stats(Population *pop);

// الانتقاء
Chromosome* tournament_selection(const Population *pop, int tournament_size);
Chromosome* roulette_wheel_selection(const Population *pop);
Chromosome* rank_selection(const Population *pop);

// العرض والطباعة
void print_population(const Population *pop);
void print_population_stats(const Population *pop);
void print_population_best(const Population *pop);
void save_population_to_file(const Population *pop, const char *filename);
void save_best_chromosomes_to_file(const Population *pop, int count, 
                                   const char *filename);

// ============= دوال مساعدة =============
const char* direction_to_string(Direction dir);
const char* direction_to_symbol(Direction dir);
Direction get_random_direction();
Direction get_opposite_direction(Direction dir);
bool positions_equal(Position p1, Position p2);
float distance_between_positions(Position p1, Position p2);
int manhattan_distance(Position p1, Position p2);

// ============= دوال خاصة بالمشروع =============
Chromosome* generate_smart_chromosome(Position start, int max_steps, 
                                      const Map3D *map);
Chromosome* generate_coverage_chromosome(Position start, int max_steps, 
                                         const Map3D *map);
Chromosome* generate_survivor_focused_chromosome(Position start, int max_steps, 
                                                 const Map3D *map);
bool is_position_in_path(const Chromosome *chrom, Position pos);
int count_unique_positions(const Chromosome *chrom);
void optimize_chromosome(Chromosome *chrom, const Map3D *map);

#endif // CHROMOSOME_H