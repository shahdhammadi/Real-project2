#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <stdbool.h>

typedef struct {
    int x, y, z;
} Position;

typedef struct {
    Position pos;
    int priority;
    float risk;
    bool rescued;
    float heat_signal;
    float co2_level;
    int sensor_confidence;
} Survivor;

typedef struct {
    int width, height, depth;
    int ***grid;
    Survivor *survivors;
    int survivor_count;
    Position start_position;
    Position exit_position;
} Map3D;

typedef struct {
    int map_width;
    int map_height;
    int map_depth;
    float obstacle_ratio;
    float survivor_ratio;
    
    int num_robots;
    Position robot_start;
    
    int population_size;
    int generations;
    int tournament_size;
    float crossover_rate;
    float mutation_rate;
    float elitism_rate;
    
    float w_survivors;
    float w_coverage;
    float w_length;
    float w_risk;
    
    int num_workers;
    int max_path_length;
    int log_level;
    char output_file[256];
} Settings;

// الدوال الرئيسية
Map3D *create_map(int width, int height, int depth);
void initialize_map(Map3D *map, float obstacle_ratio, float survivor_ratio);
void free_map(Map3D *map);
Settings *load_settings(const char *filename);
void print_settings(const Settings *settings);
bool is_valid_position(const Map3D *map, Position pos);
bool is_obstacle(const Map3D *map, Position pos);
bool is_survivor(const Map3D *map, Position pos);
Survivor *get_survivor_at(const Map3D *map, Position pos);
void print_map(const Map3D *map);
void save_map_to_file(const Map3D *map, const char *filename);
void save_report_to_file(const Map3D *map);

// الدوال المساعدة
float calculate_risk_from_priority(int priority);
int calculate_unique_priority(Map3D *map, Position pos, float base_rubble_density);
int count_nearby_rubble(Map3D *map, Position pos);

Position random_position(const Map3D *map);
Position random_near_position(Position center, const Map3D *map, int radius);
bool is_near_edge(const Map3D *map, Position pos);
bool is_in_central_area(const Map3D *map, Position pos);

// في map_loader.h، أضف:
Position get_random_free_position(const Map3D *map);
Position get_random_edge_position(const Map3D *map);
#endif