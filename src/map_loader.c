#include "map_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// ============================================================
// CONSTANTS - SINGLE DEFINITIONS
// ============================================================

// SENSOR DATA
#define MIN_HEAT_C 25.0f
#define MAX_HEAT_C 50.0f
#define MIN_CO2_PPM 500.0f
#define MAX_CO2_PPM 6000.0f

// PRIORITY
#define UNIFORM_PRIORITY 5

// DISTRIBUTION SETTINGS
#define EDGE_AVOIDANCE_RADIUS 2
#define CENTRAL_AREA_FACTOR 2.0f
#define CLUSTER_MIN_SIZE 3
#define CLUSTER_MAX_SIZE 6
#define HIGH_DENSITY_RATIO 0.5f
#define MEDIUM_DENSITY_RATIO 0.35f
#define LOW_DENSITY_RATIO 0.15f

// ============================================================
// HELPER FUNCTIONS
// ============================================================

bool is_near_edge(const Map3D *map, Position pos) {
    return (pos.x < EDGE_AVOIDANCE_RADIUS || 
            pos.x >= map->width - EDGE_AVOIDANCE_RADIUS ||
            pos.y < EDGE_AVOIDANCE_RADIUS || 
            pos.y >= map->height - EDGE_AVOIDANCE_RADIUS);
}

bool is_in_central_area(const Map3D *map, Position pos) {
    int center_x = map->width / 2;
    int center_y = map->height / 2;
    int central_radius_x = map->width / 3;
    int central_radius_y = map->height / 3;
    
    return (abs(pos.x - center_x) <= central_radius_x &&
            abs(pos.y - center_y) <= central_radius_y);
}

// ============================================================
// 1️⃣ CREATE MAP (بقى كما هو)
// ============================================================
Map3D *create_map(int width, int height, int depth)
{
    Map3D *map = (Map3D *)malloc(sizeof(Map3D));
    if (!map) return NULL;

    map->width = width;
    map->height = height;
    map->depth = depth;

    map->grid = (int ***)malloc(depth * sizeof(int **));
    for (int z = 0; z < depth; z++)
    {
        map->grid[z] = (int **)malloc(height * sizeof(int *));
        for (int y = 0; y < height; y++)
        {
            map->grid[z][y] = (int *)malloc(width * sizeof(int));
            for (int x = 0; x < width; x++)
            {
                map->grid[z][y][x] = 0;
            }
        }
    }

    map->survivors = NULL;
    map->survivor_count = 0;

    map->start_position.x = 0;
    map->start_position.y = 0;
    map->start_position.z = 0;

    map->exit_position.x = width - 1;
    map->exit_position.y = height - 1;
    map->exit_position.z = depth - 1;

    if (map->exit_position.x >= width) map->exit_position.x = width > 0 ? width - 1 : 0;
    if (map->exit_position.y >= height) map->exit_position.y = height > 0 ? height - 1 : 0;
    if (map->exit_position.z >= depth) map->exit_position.z = depth > 0 ? depth - 1 : 0;

    if (map->exit_position.x == map->start_position.x &&
        map->exit_position.y == map->start_position.y &&
        map->exit_position.z == map->start_position.z && 
        width > 1) {
        map->exit_position.x = (map->start_position.x + 1) % width;
        printf("Exit location adjusted to avoid interference with start position\n");
    }

    printf("Map created: Start=(%d,%d,%d), Exit=(%d,%d,%d)\n",
           map->start_position.x, map->start_position.y, map->start_position.z,
           map->exit_position.x, map->exit_position.y, map->exit_position.z);

    return map;
}

// ============================================================
// 2️⃣ CREATE SURVIVOR CLUSTER - UPDATED
// ============================================================
void create_survivor_cluster(Map3D *map, Position center, int cluster_size, int *survivor_index)
{
    int cluster_created = 0;
    int attempts = 0;
    int max_attempts = 50;
    
    while (cluster_created < cluster_size && attempts < max_attempts)
    {
        int dx = (rand() % 5) - 2;
        int dy = (rand() % 5) - 2;
        int dz = (rand() % 3) - 1;
        
        Position pos;
        pos.x = center.x + dx;
        pos.y = center.y + dy;
        pos.z = center.z + dz;
        
        if (is_valid_position(map, pos) && 
            map->grid[pos.z][pos.y][pos.x] == 0 &&
            !(pos.x == map->start_position.x &&
              pos.y == map->start_position.y &&
              pos.z == map->start_position.z) &&
            !(pos.x == map->exit_position.x &&
              pos.y == map->exit_position.y &&
              pos.z == map->exit_position.z) &&
            !is_near_edge(map, pos))  // ← Fixed: removed EDGE_AVOIDANCE_RADIUS parameter
        {
            map->grid[pos.z][pos.y][pos.x] = 2;
            
            map->survivors[*survivor_index].pos = pos;
            map->survivors[*survivor_index].priority = UNIFORM_PRIORITY;
            map->survivors[*survivor_index].rescued = false;
            
            float base_heat = 36.0f + ((float)rand() / RAND_MAX) * 2.0f;
            float base_co2 = 1500.0f + ((float)rand() / RAND_MAX) * 2500.0f;
            
            map->survivors[*survivor_index].heat_signal = base_heat + ((float)rand() / RAND_MAX) * 0.5f - 0.25f;
            map->survivors[*survivor_index].co2_level = base_co2 + ((float)rand() / RAND_MAX) * 200.0f - 100.0f;
            map->survivors[*survivor_index].sensor_confidence = 80 + rand() % 15;
            
            if (map->survivors[*survivor_index].heat_signal < MIN_HEAT_C) 
                map->survivors[*survivor_index].heat_signal = MIN_HEAT_C;
            if (map->survivors[*survivor_index].heat_signal > MAX_HEAT_C) 
                map->survivors[*survivor_index].heat_signal = MAX_HEAT_C;
            if (map->survivors[*survivor_index].co2_level < MIN_CO2_PPM) 
                map->survivors[*survivor_index].co2_level = MIN_CO2_PPM;
            if (map->survivors[*survivor_index].co2_level > MAX_CO2_PPM) 
                map->survivors[*survivor_index].co2_level = MAX_CO2_PPM;
            
            (*survivor_index)++;
            cluster_created++;
        }
        attempts++;
    }
}

// ============================================================
// 3️⃣ SIMPLIFIED REALISTIC INITIALIZE MAP
// ============================================================
// ============================================================
// FIXED INITIALIZE MAP - PRINTING ISSUE
// ============================================================
// ============================================================
// FIXED INITIALIZE MAP WITH DEBUGGING
// ============================================================
void initialize_map(Map3D *map, float obstacle_ratio, float survivor_ratio)
{
    if (!map) return;

    srand(time(NULL));
    int total_cells = map->width * map->height * map->depth;

    printf("\n════════════════════════════════════════════════════════════════\n");
    printf("               SIMPLIFIED REALISTIC DISTRIBUTION               \n");
    printf("════════════════════════════════════════════════════════════════\n");
    printf("📊 70%% Central, 20%% Other, 10%% Edge\n");
    printf("🔗 Clusters on all floors\n");
    printf("⚠️  All survivors priority 5\n");
    printf("════════════════════════════════════════════════════════════════\n");

    // ============================================================
    // A) FIXED RUBBLE DISTRIBUTION - PROPER PRINTING
    // ============================================================
    printf("\n🧱 Rubble Distribution:\n");
    printf("------------------------\n");
    
    // First count total obstacles for each floor
    int obstacles_per_floor[5] = {0};
    int total_obstacles_planned = 0;
    
    for (int z = 0; z < map->depth; z++)
    {
        float floor_factor;
        switch(z) {
            case 0: floor_factor = 1.0f; break;  // Ground: 100%
            case 1: floor_factor = 0.8f; break;  // Floor 1: 80%
            case 2: floor_factor = 0.6f; break;  // Floor 2: 60%
            case 3: floor_factor = 0.4f; break;  // Floor 3: 40%
            case 4: floor_factor = 0.2f; break;  // Floor 4: 20%
            default: floor_factor = 0.5f;
        }

        float floor_obstacle_ratio = obstacle_ratio * floor_factor;
        obstacles_per_floor[z] = (int)(map->width * map->height * floor_obstacle_ratio);
        total_obstacles_planned += obstacles_per_floor[z];
        
        printf(" Floor %d: %.0f%% density → %d obstacles\n", 
               z, floor_factor * 100, obstacles_per_floor[z]);
    }
    
    printf(" Total obstacles planned: %d\n", total_obstacles_planned);

    // Now actually place the obstacles
    int total_obstacles_placed = 0;
    
    for (int z = 0; z < map->depth; z++)
    {
        int obstacles_placed_this_floor = 0;
        int max_attempts = obstacles_per_floor[z] * 10;
        int attempts = 0;
        
        while (obstacles_placed_this_floor < obstacles_per_floor[z] && attempts < max_attempts)
        {
            int x = rand() % map->width;
            int y = rand() % map->height;

            // Avoid placing obstacles at start or exit positions
            if ((x == map->start_position.x &&
                 y == map->start_position.y &&
                 z == map->start_position.z) ||
                (x == map->exit_position.x &&
                 y == map->exit_position.y &&
                 z == map->exit_position.z))
            {
                attempts++;
                continue;
            }

            // Place obstacle if cell is empty
            if (map->grid[z][y][x] == 0)
            {
                map->grid[z][y][x] = 1;
                obstacles_placed_this_floor++;
                total_obstacles_placed++;
            }
            
            attempts++;
        }
        
        if (obstacles_placed_this_floor < obstacles_per_floor[z]) {
            printf("  ⚠️  Floor %d: Only placed %d out of %d obstacles\n", 
                   z, obstacles_placed_this_floor, obstacles_per_floor[z]);
        }
    }
    
    printf(" Total obstacles actually placed: %d\n", total_obstacles_placed);

    // ============================================================
    // B) SURVIVOR DISTRIBUTION - SIMPLIFIED VERSION
    // ============================================================
    int max_survivors = (int)(total_cells * survivor_ratio);
    if (max_survivors < 1) max_survivors = 1;
    
    map->survivors = (Survivor *)malloc(max_survivors * sizeof(Survivor));
    if (!map->survivors) {
        printf("❌ Memory allocation error for survivors\n");
        return;
    }
    
    printf("\n👥 SIMPLIFIED SURVIVOR DISTRIBUTION (Target: %d):\n", max_survivors);
    printf("════════════════════════════════════════════════════════\n");
    
    int survivors_created = 0;
    int survivors_per_floor[5] = {0};
    int clusters_per_floor[5] = {0};

    // Phase 1: Clusters (30% of survivors)
    int cluster_target = (int)(max_survivors * 0.3);
    printf("\n🔗 Creating Clusters (30%% = %d survivors):\n", cluster_target);
    
    for (int floor = 0; floor < map->depth && cluster_target > 0; floor++) {
        if (cluster_target < CLUSTER_MIN_SIZE) break;
        
        Position center;
        int attempts = 0;
        int found = 0;
        
        // Try to find a good center position
        while (!found && attempts < 100) {
            // Avoid edges for cluster centers
            center.x = EDGE_AVOIDANCE_RADIUS + rand() % (map->width - 2 * EDGE_AVOIDANCE_RADIUS);
            center.y = EDGE_AVOIDANCE_RADIUS + rand() % (map->height - 2 * EDGE_AVOIDANCE_RADIUS);
            center.z = floor;
            
            // Check if position is available
            if (map->grid[center.z][center.y][center.x] == 0 &&
                !is_near_edge(map, center)) {
                found = 1;
            }
            attempts++;
        }
        
        if (found) {
            int cluster_size = CLUSTER_MIN_SIZE + rand() % (CLUSTER_MAX_SIZE - CLUSTER_MIN_SIZE + 1);
            if (cluster_size > cluster_target) cluster_size = cluster_target;
            
            if (cluster_size >= CLUSTER_MIN_SIZE) {
                printf("  Floor %d: Cluster of %d people\n", floor, cluster_size);
                
                // Create the cluster
                int cluster_created = 0;
                int cluster_attempts = 0;
                int start_index = survivors_created;
                
                // Place cluster members
                while (cluster_created < cluster_size && cluster_attempts < 100) {
                    int dx = (rand() % 5) - 2;  // -2 to +2
                    int dy = (rand() % 5) - 2;  // -2 to +2
                    
                    Position pos;
                    pos.x = center.x + dx;
                    pos.y = center.y + dy;
                    pos.z = floor;
                    
                    // Check if position is valid and available
                    if (is_valid_position(map, pos) && 
                        map->grid[pos.z][pos.y][pos.x] == 0 &&
                        !(pos.x == map->start_position.x && pos.y == map->start_position.y && pos.z == map->start_position.z) &&
                        !(pos.x == map->exit_position.x && pos.y == map->exit_position.y && pos.z == map->exit_position.z) &&
                        !is_near_edge(map, pos)) {
                        
                        // Place survivor
                        map->grid[pos.z][pos.y][pos.x] = 2;
                        
                        map->survivors[survivors_created].pos = pos;
                        map->survivors[survivors_created].priority = UNIFORM_PRIORITY;
                        map->survivors[survivors_created].rescued = false;
                        
                        // Similar sensor data for cluster members
                        if (cluster_created == 0) {
                            // First member sets baseline
                            map->survivors[survivors_created].heat_signal = 36.5f + ((float)rand() / RAND_MAX) * 1.5f - 0.75f;
                            map->survivors[survivors_created].co2_level = 1500.0f + ((float)rand() / RAND_MAX) * 1500.0f;
                        } else {
                            // Other members get similar values
                            float base_heat = map->survivors[start_index].heat_signal;
                            float base_co2 = map->survivors[start_index].co2_level;
                            
                            map->survivors[survivors_created].heat_signal = base_heat + ((float)rand() / RAND_MAX) * 0.5f - 0.25f;
                            map->survivors[survivors_created].co2_level = base_co2 + ((float)rand() / RAND_MAX) * 300.0f - 150.0f;
                        }
                        
                        map->survivors[survivors_created].sensor_confidence = 85 + rand() % 10;
                        
                        // Ensure within bounds
                        if (map->survivors[survivors_created].heat_signal < MIN_HEAT_C) 
                            map->survivors[survivors_created].heat_signal = MIN_HEAT_C;
                        if (map->survivors[survivors_created].heat_signal > MAX_HEAT_C) 
                            map->survivors[survivors_created].heat_signal = MAX_HEAT_C;
                        if (map->survivors[survivors_created].co2_level < MIN_CO2_PPM) 
                            map->survivors[survivors_created].co2_level = MIN_CO2_PPM;
                        if (map->survivors[survivors_created].co2_level > MAX_CO2_PPM) 
                            map->survivors[survivors_created].co2_level = MAX_CO2_PPM;
                        
                        survivors_created++;
                        survivors_per_floor[floor]++;
                        cluster_created++;
                    }
                    
                    cluster_attempts++;
                }
                
                cluster_target -= cluster_created;
                clusters_per_floor[floor]++;
                
                if (cluster_created < cluster_size) {
                    printf("    ⚠️  Only placed %d out of %d cluster members\n", 
                           cluster_created, cluster_size);
                }
            }
        }
    }
    
    printf("✅ Created %d clusters\n", map->depth);

    // Phase 2: Distribute remaining survivors
    printf("\n📍 Distributing remaining survivors:");
    
    int remaining_to_place = max_survivors - survivors_created;
    
    if (remaining_to_place > 0) {
        printf(" (need %d more)\n", remaining_to_place);
        printf("Starting distribution...\n");
    } else {
        printf(" (no more needed)\n");
    }
    
    int placed_in_phase2 = 0;
    int max_attempts = remaining_to_place * 100;
    int attempts = 0;
    
    while (survivors_created < max_survivors && attempts < max_attempts) {
        Position pos;
        pos.z = rand() % map->depth;
        
        // Simple distribution: try random position
        pos.x = rand() % map->width;
        pos.y = rand() % map->height;
        
        // Check if position is available
        if (map->grid[pos.z][pos.y][pos.x] == 0 &&
            !(pos.x == map->start_position.x && pos.y == map->start_position.y && pos.z == map->start_position.z) &&
            !(pos.x == map->exit_position.x && pos.y == map->exit_position.y && pos.z == map->exit_position.z)) {
            
            // Place survivor
            map->grid[pos.z][pos.y][pos.x] = 2;
            
            map->survivors[survivors_created].pos = pos;
            map->survivors[survivors_created].priority = UNIFORM_PRIORITY;
            map->survivors[survivors_created].rescued = false;
            map->survivors[survivors_created].heat_signal = 36.5f + ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            map->survivors[survivors_created].co2_level = 1500.0f + ((float)rand() / RAND_MAX) * 2000.0f;
            map->survivors[survivors_created].sensor_confidence = 80 + rand() % 15;
            
            // Ensure within bounds
            if (map->survivors[survivors_created].heat_signal < MIN_HEAT_C) 
                map->survivors[survivors_created].heat_signal = MIN_HEAT_C;
            if (map->survivors[survivors_created].heat_signal > MAX_HEAT_C) 
                map->survivors[survivors_created].heat_signal = MAX_HEAT_C;
            if (map->survivors[survivors_created].co2_level < MIN_CO2_PPM) 
                map->survivors[survivors_created].co2_level = MIN_CO2_PPM;
            if (map->survivors[survivors_created].co2_level > MAX_CO2_PPM) 
                map->survivors[survivors_created].co2_level = MAX_CO2_PPM;
            
            survivors_created++;
            survivors_per_floor[pos.z]++;
            placed_in_phase2++;
            
            // Show progress every 10 survivors
            if (placed_in_phase2 % 10 == 0) {
                printf("  Progress: %d/%d placed\n", placed_in_phase2, remaining_to_place);
            }
        }
        
        attempts++;
    }
    
    if (placed_in_phase2 > 0) {
        printf("✅ Phase 2: Placed %d survivors\n", placed_in_phase2);
    }
    
    map->survivor_count = survivors_created;
    
    // Emergency placement if needed
    if (map->survivor_count < max_survivors) {
        printf("\n⚠️  Emergency placement for %d remaining survivors\n", 
               max_survivors - map->survivor_count);
        
        // Place in any available spot
        for (int z = 0; z < map->depth && survivors_created < max_survivors; z++) {
            for (int y = 0; y < map->height && survivors_created < max_survivors; y++) {
                for (int x = 0; x < map->width && survivors_created < max_survivors; x++) {
                    if (map->grid[z][y][x] == 0) {
                        map->grid[z][y][x] = 2;
                        
                        map->survivors[survivors_created].pos = (Position){x, y, z};
                        map->survivors[survivors_created].priority = UNIFORM_PRIORITY;
                        map->survivors[survivors_created].rescued = false;
                        map->survivors[survivors_created].heat_signal = 36.5f;
                        map->survivors[survivors_created].co2_level = 1500.0f;
                        map->survivors[survivors_created].sensor_confidence = 80;
                        
                        survivors_created++;
                        survivors_per_floor[z]++;
                    }
                }
            }
        }
        map->survivor_count = survivors_created;
    }

    // ============================================================
    // C) FINAL STATISTICS
    // ============================================================
    printf("\n════════════════════════════════════════════════════════════════\n");
    printf("                     FINAL DISTRIBUTION                        \n");
    printf("════════════════════════════════════════════════════════════════\n");
    
    int central_count = 0;
    int edge_count = 0;
    int other_count = 0;
    
    for (int i = 0; i < map->survivor_count; i++) {
        if (is_in_central_area(map, map->survivors[i].pos)) {
            central_count++;
        } else if (is_near_edge(map, map->survivors[i].pos)) {
            edge_count++;
        } else {
            other_count++;
        }
    }
    
    printf("\n📊 DISTRIBUTION STATISTICS:\n");
    printf("════════════════════════════════════\n");
    printf("Total Survivors:        %d\n", map->survivor_count);
    printf("Total Obstacles:        %d\n", total_obstacles_placed);
    
    printf("\n📍 Survivors by Location:\n");
    printf("  Central Area:         %d (%.1f%%)\n", 
           central_count, (float)central_count/map->survivor_count*100);
    printf("  Edge Area:            %d (%.1f%%)\n", 
           edge_count, (float)edge_count/map->survivor_count*100);
    printf("  Other Areas:          %d (%.1f%%)\n", 
           other_count, (float)other_count/map->survivor_count*100);
    
    printf("\n🏢 Survivors by Floor:\n");
    for (int z = 0; z < map->depth; z++) {
        // Count obstacles on this floor
        int floor_obstacles = 0;
        for (int y = 0; y < map->height; y++) {
            for (int x = 0; x < map->width; x++) {
                if (map->grid[z][y][x] == 1) floor_obstacles++;
            }
        }
        
        float percentage = (float)survivors_per_floor[z] / map->survivor_count * 100.0f;
        printf("  Floor %d:             %d survivors (%.1f%%) | %d obstacles\n", 
               z, survivors_per_floor[z], percentage, floor_obstacles);
    }
    
    printf("\n✅ SIMPLIFIED REALISTIC map created successfully!\n");
    printf("════════════════════════════════════════════════════════════════\n");
}
// ============================================================
// 3️⃣ LOAD SETTINGS FROM FILE
// ============================================================
Settings *load_settings(const char *filename)
{
    Settings *settings = (Settings *)malloc(sizeof(Settings));
    if (!settings)
        return NULL;

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("ERROR: Settings file '%s' not found.\n", filename);
        free(settings);
        return NULL;
    }

    char line[256];
    char key[100], value[100];
    int settings_loaded = 0;

    while (fgets(line, sizeof(line), file))
    {
        if (line[0] == '\n' || line[0] == '#')
            continue;

        if (sscanf(line, "%99[^=]=%99[^\n]", key, value) == 2)
        {
            // Clean spaces
            char *k = key;
            char *v = value;
            while (*k == ' ') k++;
            while (*v == ' ') v++;

            char *end_k = k + strlen(k) - 1;
            char *end_v = v + strlen(v) - 1;
            while (end_k > k && *end_k == ' ') *end_k-- = '\0';
            while (end_v > v && *end_v == ' ') *end_v-- = '\0';

            // Read settings
            if (strcmp(k, "MAP_WIDTH") == 0) settings->map_width = atoi(v);
            else if (strcmp(k, "MAP_HEIGHT") == 0) settings->map_height = atoi(v);
            else if (strcmp(k, "MAP_DEPTH") == 0) settings->map_depth = atoi(v);
            else if (strcmp(k, "OBSTACLE_RATIO") == 0) settings->obstacle_ratio = atof(v);
            else if (strcmp(k, "SURVIVOR_RATIO") == 0) settings->survivor_ratio = atof(v);
            else if (strcmp(k, "NUM_ROBOTS") == 0) settings->num_robots = atoi(v);
            else if (strcmp(k, "POPULATION_SIZE") == 0) settings->population_size = atoi(v);
            else if (strcmp(k, "GENERATIONS") == 0) settings->generations = atoi(v);
            else if (strcmp(k, "W_SURVIVORS") == 0) settings->w_survivors = atof(v);
            else if (strcmp(k, "W_LENGTH") == 0) settings->w_length = atof(v);
            else if (strcmp(k, "W_RISK") == 0) settings->w_risk = atof(v);
            else if (strcmp(k, "OUTPUT_FILE") == 0) strcpy(settings->output_file, v);
            
            settings_loaded = 1;
        }
    }

    fclose(file);

    if (!settings_loaded)
    {
        printf("ERROR: No valid settings found in file '%s'\n", filename);
        free(settings);
        return NULL;
    }

    printf("✅ Settings loaded from '%s'\n", filename);
    return settings;
}


// ============================================================
// 4️⃣ PRINT SETTINGS
// ============================================================
void print_settings(const Settings *settings)
{
    if (!settings)
    {
        printf("No settings available.\n");
        return;
    }

    printf("\n════════════════════════════════════════════════════════════════\n");
    printf("              RESCUE SYSTEM SETTINGS - IMPROVED DISTRIBUTION  \n");
    printf("════════════════════════════════════════════════════════════════\n");
    
    printf("Map Settings:\n");
    printf("  Dimensions: %d × %d × %d\n", 
           settings->map_width, settings->map_height, settings->map_depth);
    printf("  Obstacle ratio: %.2f\n", settings->obstacle_ratio);
    printf("  Survivor ratio: %.2f\n", settings->survivor_ratio);
    printf("\n");

    printf("Distribution Improvements:\n");
    printf("  Edge avoidance radius: %d cells\n", EDGE_AVOIDANCE_RADIUS);
    printf("  Central area factor: x%.1f\n", CENTRAL_AREA_FACTOR);
    printf("  Cluster size: %d-%d survivors\n", CLUSTER_MIN_SIZE, CLUSTER_MAX_SIZE);
    printf("  Density zones: High(%.0f%%) / Medium(%.0f%%) / Low(%.0f%%)\n",
           HIGH_DENSITY_RATIO*100, MEDIUM_DENSITY_RATIO*100, LOW_DENSITY_RATIO*100);
    printf("\n");

    printf("Genetic Algorithm Settings:\n");
    printf("  Population size: %d\n", settings->population_size);
    printf("  Number of generations: %d\n", settings->generations);
    printf("\n");

    printf("Fitness Function Weights:\n");
    printf("  Survivors weight: %.2f\n", settings->w_survivors);
    printf("  Path length weight: %.2f\n", settings->w_length);
    printf("  Risk weight: %.2f\n", settings->w_risk);
    printf("\n");

    printf("Note: All survivors have same priority (5)\n");
    printf("Note: No Risk value for survivors\n");
    printf("Note: Improved distribution with clusters and density zones\n");
    printf("════════════════════════════════════════════════════════════════\n\n");
}

// ============================================================
// 5️⃣ HELPER FUNCTIONS FOR MAP
// ============================================================

// Free map memory
void free_map(Map3D *map) {
    if (!map) return;
    
    for (int z = 0; z < map->depth; z++) {
        for (int y = 0; y < map->height; y++) {
            free(map->grid[z][y]);
        }
        free(map->grid[z]);
    }
    free(map->grid);
    
    if (map->survivors) free(map->survivors);
    free(map);
}

// Check if position is valid
bool is_valid_position(const Map3D *map, Position pos)
{
    return (pos.x >= 0 && pos.x < map->width &&
            pos.y >= 0 && pos.y < map->height &&
            pos.z >= 0 && pos.z < map->depth);
}

// Check if position has obstacle
bool is_obstacle(const Map3D *map, Position pos)
{
    if (!is_valid_position(map, pos))
        return true;
    return (map->grid[pos.z][pos.y][pos.x] == 1);
}

// Check if position has survivor
bool is_survivor(const Map3D *map, Position pos)
{
    if (!is_valid_position(map, pos))
        return false;
    return (map->grid[pos.z][pos.y][pos.x] == 2);
}

// Get survivor at specific position
Survivor *get_survivor_at(const Map3D *map, Position pos)
{
    if (!is_survivor(map, pos))
        return NULL;

    for (int i = 0; i < map->survivor_count; i++)
    {
        if (map->survivors[i].pos.x == pos.x &&
            map->survivors[i].pos.y == pos.y &&
            map->survivors[i].pos.z == pos.z)
        {
            return &map->survivors[i];
        }
    }
    return NULL;
}

// ============================================================
// 6️⃣ PRINT MAP WITH IMPROVED VISUALIZATION
// ============================================================
void print_map(const Map3D *map)
{
    if (!map)
    {
        printf("Map not available.\n");
        return;
    }

    printf("\n╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║         COLLAPSED BUILDING MAP - SIMPLIFIED DISTRIBUTION           ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Dimensions: %d × %d × %d | Survivors: %d                          ║\n",
           map->width, map->height, map->depth, map->survivor_count);
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");

    printf("\n┌────────────────────────────────────────────────────────────┐\n");
    printf("│                        MAP LEGEND                          │\n");
    printf("├────────────────────────────────────────────────────────────┤\n");
    printf("│   . = Free path                                          │\n");
    printf("│   ██ = Obstacle (debris)                                  │\n");
    printf("│   S = Survivor (all same priority)                       │\n");
    printf("└────────────────────────────────────────────────────────────┘\n");

    for (int z = 0; z < map->depth; z++)
    {
        printf("\n┌────────────────────────────────────────────────────────────┐\n");
        printf("│                         FLOOR %d                           │\n", z);
        printf("└────────────────────────────────────────────────────────────┘\n");

        int floor_survivors = 0;
        int floor_obstacles = 0;
        int floor_central = 0;
        int floor_edge = 0;
        
        for (int y = 0; y < map->height; y++) {
            for (int x = 0; x < map->width; x++) {
                if (map->grid[z][y][x] == 1) floor_obstacles++;
                else if (map->grid[z][y][x] == 2) {
                    floor_survivors++;
                    Position pos = {x, y, z};
                    if (is_in_central_area(map, pos)) floor_central++;
                    if (is_near_edge(map, pos)) floor_edge++;  // ← Fixed: removed parameter
                }
            }
        }
        
        printf("┌─ Survivors: %2d (Central: %d, Edge: %d) | Obstacles: %3d ─┐\n", 
               floor_survivors, floor_central, floor_edge, floor_obstacles);

        for (int y = 0; y < map->height; y++)
        {
            printf("│ ");
            for (int x = 0; x < map->width; x++)
            {
                Position pos = {x, y, z};

                if (pos.x == map->start_position.x &&
                    pos.y == map->start_position.y &&
                    pos.z == map->start_position.z)
                {
                    printf(" . ");
                }
                else if (pos.x == map->exit_position.x &&
                         pos.y == map->exit_position.y &&
                         pos.z == map->exit_position.z)
                {
                    printf(" E ");
                }
                else if (is_survivor(map, pos))
                {
                    printf(" S ");
                }
                else if (is_obstacle(map, pos))
                {
                    printf("██ ");
                }
                else
                {
                    printf("·  ");
                }
            }
            printf("│\n");
        }
        printf("└────────────────────────────────────────────────────────────┘\n");
    }

    printf("\n╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║               SURVIVOR INFORMATION - SIMPLIFIED                    ║\n");
    printf("╠══════════════════════════════════════════════════════════════════════╣\n");
    printf("║ All survivors have same priority (5)                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");

    if (map->survivor_count > 0)
    {
        printf("\n┌────┬──────────┬─────────┬──────┬──────────┬────────┬─────────────┐\n");
        printf("│ No │ Position │ Floor  │ Prio │ Location │ Heat   │ CO₂         │\n");
        printf("├────┼──────────┼─────────┼──────┼──────────┼────────┼─────────────┤\n");

        for (int i = 0; i < map->survivor_count; i++)
        {
            Survivor s = map->survivors[i];
            
            char location[10];
            if (is_in_central_area(map, s.pos)) strcpy(location, "Central");
            else if (is_near_edge(map, s.pos)) strcpy(location, "Edge");  // ← Fixed
            else strcpy(location, "Other");

            printf("│ %2d │ (%2d,%2d,%2d) │   %2d   │   %2d   │ %8s │ %4.1f°C │ %6.0f ppm │\n",
                   i + 1,
                   s.pos.x, s.pos.y, s.pos.z,
                   s.pos.z,
                   s.priority,
                   location,
                   s.heat_signal,
                   s.co2_level);

            if ((i + 1) % 10 == 0 && (i + 1) < map->survivor_count) {
                printf("├────┼──────────┼─────────┼──────┼──────────┼────────┼─────────────┤\n");
            }
        }
        printf("└────┴──────────┴─────────┴──────┴──────────┴────────┴─────────────┘\n");
    }
    
    printf("\n✅ Map printed successfully!\n");
}

// ============================================================
// 8️⃣ SAVE MAP TO FILE WITH IMPROVED DATA
// ============================================================
void save_map_to_file(const Map3D *map, const char *filename)
{
    if (!map || !filename)
        return;

    FILE *file = fopen(filename, "w");
    if (!file)
    {
        printf("Error: Cannot create file '%s'\n", filename);
        return;
    }

    fprintf(file, "# Collapsed Building Map with Improved Distribution System\n");
    fprintf(file, "# All survivors have same priority (5)\n");
    fprintf(file, "# No Risk value for survivors\n");
    fprintf(file, "# Sensor data indicates presence only\n");
    fprintf(file, "# IMPROVED DISTRIBUTION: Clusters + Variable Density + Edge Avoidance\n");
    fprintf(file, "WIDTH=%d\n", map->width);
    fprintf(file, "HEIGHT=%d\n", map->height);
    fprintf(file, "DEPTH=%d\n", map->depth);
    fprintf(file, "SURVIVORS=%d\n", map->survivor_count);
    fprintf(file, "PRIORITY_SYSTEM=uniform_priority\n");
    fprintf(file, "PRIORITY_VALUE=5\n");
    fprintf(file, "RISK_SYSTEM=none\n");
    fprintf(file, "DISTRIBUTION_TYPE=improved_with_clusters\n");
    fprintf(file, "EDGE_AVOIDANCE=%d\n", EDGE_AVOIDANCE_RADIUS);
    fprintf(file, "CENTRAL_FACTOR=%.1f\n", CENTRAL_AREA_FACTOR);
   
    fprintf(file, "\n# Cell Data (x,y,z,type,priority,heat,co2,confidence,location_type)\n");
    fprintf(file, "# type: 0=free,1=obstacle,2=survivor\n");
    fprintf(file, "# location_type: central,edge,normal\n");

    for (int z = 0; z < map->depth; z++)
    {
        fprintf(file, "# Floor %d data\n", z);
        for (int y = 0; y < map->height; y++)
        {
            for (int x = 0; x < map->width; x++)
            {
                int cell_type = map->grid[z][y][x];

                if (cell_type == 2)  // Survivor
                {
                    Survivor *s = get_survivor_at(map, (Position){x, y, z});
                    if (s)
                    {
                        char location_type[10];
                        if (is_in_central_area(map, s->pos)) strcpy(location_type, "central");
                        else if (is_near_edge(map, s->pos)) strcpy(location_type, "edge");
                        else strcpy(location_type, "normal");
                        
                        fprintf(file, "%d,%d,%d,2,%d,%.1f,%.0f,%d,%s\n", 
                                x, y, z, 
                                s->priority,
                                s->heat_signal,
                                s->co2_level,
                                s->sensor_confidence,
                                location_type);
                    }
                }
                else
                {
                    fprintf(file, "%d,%d,%d,%d\n", x, y, z, cell_type);
                }
            }
        }
        if (z < map->depth - 1) fprintf(file, "\n");
    }

    fclose(file);
    printf("✅ Map with improved distribution saved to: '%s'\n", filename);
}

