#include "chromosome.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#ifndef INFINITY
#define INFINITY 1e30
#endif

// ============= Chromosome Functions =============

Chromosome* create_chromosome(Position start, int max_steps) {
    Chromosome *chrom = (Chromosome*)malloc(sizeof(Chromosome));
    if (!chrom) return NULL;
    
    chrom->moves = (Direction*)malloc(max_steps * sizeof(Direction));
    if (!chrom->moves) {
        free(chrom);
        return NULL;
    }
    
    chrom->start_pos = start;
    chrom->num_moves = 0;
    chrom->max_moves = max_steps;
    
    // Initialize values
    chrom->fitness = 0.0f;
    chrom->survivors_rescued = 0;
    chrom->coverage_cells = 0;
    chrom->total_length = 0.0f;
    chrom->total_risk = 0.0f;
    chrom->time_estimate = 0.0f;
    
    chrom->id = rand() % 1000000;
    chrom->valid = false;
    chrom->actual_path = NULL;
    chrom->actual_path_length = 0;
    
    return chrom;
}

Chromosome* create_random_chromosome(Position start, int max_steps) {
    Chromosome *chrom = create_chromosome(start, max_steps);
    if (!chrom) return NULL;
    
    chrom->num_moves = max_steps;
    
    // Generate random moves
    for (int i = 0; i < max_steps; i++) {
        chrom->moves[i] = get_random_direction();
    }
    
    return chrom;
}

void init_chromosome(Chromosome *chrom, Position start, int max_steps) {
    if (chrom->moves) free(chrom->moves);
    
    chrom->moves = (Direction*)malloc(max_steps * sizeof(Direction));
    chrom->start_pos = start;
    chrom->num_moves = 0;
    chrom->max_moves = max_steps;
    
    chrom->fitness = 0.0f;
    chrom->survivors_rescued = 0;
    chrom->coverage_cells = 0;
    chrom->total_length = 0.0f;
    chrom->total_risk = 0.0f;
    chrom->time_estimate = 0.0f;
    
    chrom->id = rand() % 1000000;
    chrom->valid = false;
    chrom->actual_path = NULL;
    chrom->actual_path_length = 0;
}

void init_random_chromosome(Chromosome *chrom, Position start, int max_steps) {
    init_chromosome(chrom, start, max_steps);
    chrom->num_moves = max_steps;
    
    for (int i = 0; i < max_steps; i++) {
        chrom->moves[i] = get_random_direction();
    }
}

void copy_chromosome(Chromosome *dest, const Chromosome *src) {
    if (dest->moves) free(dest->moves);
    
    dest->moves = (Direction*)malloc(src->max_moves * sizeof(Direction));
    memcpy(dest->moves, src->moves, src->num_moves * sizeof(Direction));
    
    dest->start_pos = src->start_pos;
    dest->num_moves = src->num_moves;
    dest->max_moves = src->max_moves;
    
    dest->fitness = src->fitness;
    dest->survivors_rescued = src->survivors_rescued;
    dest->coverage_cells = src->coverage_cells;
    dest->total_length = src->total_length;
    dest->total_risk = src->total_risk;
    dest->time_estimate = src->time_estimate;
    
    dest->id = src->id;
    dest->valid = src->valid;
    
    // Don't copy actual_path as it's temporary
    dest->actual_path = NULL;
    dest->actual_path_length = 0;
}

Chromosome* clone_chromosome(const Chromosome *src) {
    Chromosome *dest = create_chromosome(src->start_pos, src->max_moves);
    if (!dest) return NULL;
    
    copy_chromosome(dest, src);
    return dest;
}

void free_chromosome(Chromosome *chrom) {
    if (chrom) {
        if (chrom->moves) free(chrom->moves);
        if (chrom->actual_path) free(chrom->actual_path);
        free(chrom);
    }
}

// ============= Decoding Directions to Actual Path =============

Position* decode_chromosome_to_path(const Chromosome *chrom, int *path_length) {
    // Path contains num_moves + 1 points
    *path_length = chrom->num_moves + 1;
    Position *path = (Position*)malloc((*path_length) * sizeof(Position));
    
    // Starting point
    path[0] = chrom->start_pos;
    Position current = chrom->start_pos;
    
    // Apply each move
    for (int i = 0; i < chrom->num_moves; i++) {
        Position next = current;
        
        switch (chrom->moves[i]) {
            case DIR_RIGHT: next.x++; break;
            case DIR_LEFT: next.x--; break;
            case DIR_UP: next.y++; break;
            case DIR_DOWN: next.y--; break;
            case DIR_FORWARD: next.z++; break;
            case DIR_BACKWARD: next.z--; break;
            case DIR_WAIT: break; // No change
        }
        
        current = next;
        path[i + 1] = current;
    }
    
    return path;
}

Position* decode_chromosome_with_bounds(const Chromosome *chrom, int *path_length, 
                                        const Map3D *map) {
    *path_length = chrom->num_moves + 1;
    Position *path = (Position*)malloc((*path_length) * sizeof(Position));
    
    path[0] = chrom->start_pos;
    Position current = chrom->start_pos;
    
    for (int i = 0; i < chrom->num_moves; i++) {
        Position next = current;
        
        switch (chrom->moves[i]) {
            case DIR_RIGHT: 
                if (current.x < map->width - 1) next.x++; 
                break;
            case DIR_LEFT: 
                if (current.x > 0) next.x--; 
                break;
            case DIR_UP: 
                if (current.y < map->height - 1) next.y++; 
                break;
            case DIR_DOWN: 
                if (current.y > 0) next.y--; 
                break;
            case DIR_FORWARD: 
                if (current.z < map->depth - 1) next.z++; 
                break;
            case DIR_BACKWARD: 
                if (current.z > 0) next.z--; 
                break;
            case DIR_WAIT: 
                break;
        }
        
        // Update only if coordinates changed and within bounds
        if (!positions_equal(current, next)) {
            current = next;
        }
        
        path[i + 1] = current;
    }
    
    return path;
}

// ============= Evaluation Functions =============

float evaluate_chromosome_fitness(Chromosome *chrom, const Map3D *map, 
                                  float w_survivors, float w_coverage, 
                                  float w_length, float w_risk) {
    
    // 1. Convert to path
    int path_length;
    Position *path = decode_chromosome_with_bounds(chrom, &path_length, map);
    
    // 2. Calculate components
    chrom->survivors_rescued = count_survivors_on_path(chrom, map);
    chrom->coverage_cells = count_coverage_cells(chrom, map);
    chrom->total_length = calculate_path_length(chrom);
    chrom->total_risk = calculate_path_risk(chrom, map);
    
    // 3. Apply fitness formula
    chrom->fitness = (w_survivors * chrom->survivors_rescued) +
                     (w_coverage * chrom->coverage_cells) -
                     (w_length * chrom->total_length) -
                     (w_risk * chrom->total_risk);
    
    // 4. Estimated time (approximate)
    chrom->time_estimate = chrom->total_length * 0.5f; // 0.5 seconds per unit
    
    // 5. Validate
    chrom->valid = validate_chromosome(chrom, map);
    
    // 6. Save actual path (temporary)
    if (chrom->actual_path) free(chrom->actual_path);
    chrom->actual_path = path;
    chrom->actual_path_length = path_length;
    
    return chrom->fitness;
}

int count_survivors_on_path(const Chromosome *chrom, const Map3D *map) {
    if (!chrom->actual_path) return 0;
    
    int count = 0;
    bool visited[map->width][map->height][map->depth];
    memset(visited, 0, sizeof(visited));
    
    for (int i = 0; i < chrom->actual_path_length; i++) {
        Position pos = chrom->actual_path[i];
        
        if (pos.x >= 0 && pos.x < map->width &&
            pos.y >= 0 && pos.y < map->height &&
            pos.z >= 0 && pos.z < map->depth) {
            
            if (!visited[pos.x][pos.y][pos.z]) {
                visited[pos.x][pos.y][pos.z] = true;
                
                // Check for survivor in this cell
                for (int s = 0; s < map->survivor_count; s++) {
                    if (positions_equal(map->survivors[s].pos, pos)) {
                        count++;
                        break;
                    }
                }
                
                // Check neighboring cells (radius 1)
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dz = -1; dz <= 1; dz++) {
                            Position neighbor = {
                                pos.x + dx,
                                pos.y + dy,
                                pos.z + dz
                            };
                            
                            if (neighbor.x >= 0 && neighbor.x < map->width &&
                                neighbor.y >= 0 && neighbor.y < map->height &&
                                neighbor.z >= 0 && neighbor.z < map->depth &&
                                !visited[neighbor.x][neighbor.y][neighbor.z]) {
                                
                                visited[neighbor.x][neighbor.y][neighbor.z] = true;
                                
                                for (int s = 0; s < map->survivor_count; s++) {
                                    if (positions_equal(map->survivors[s].pos, neighbor)) {
                                        count++;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return count;
}

int count_coverage_cells(const Chromosome *chrom, const Map3D *map) {
    if (!chrom->actual_path) return 0;
    
    bool visited[map->width][map->height][map->depth];
    memset(visited, 0, sizeof(visited));
    int count = 0;
    
    for (int i = 0; i < chrom->actual_path_length; i++) {
        Position pos = chrom->actual_path[i];
        
        if (pos.x >= 0 && pos.x < map->width &&
            pos.y >= 0 && pos.y < map->height &&
            pos.z >= 0 && pos.z < map->depth) {
            
            if (!visited[pos.x][pos.y][pos.z]) {
                visited[pos.x][pos.y][pos.z] = true;
                count++;
            }
        }
    }
    
    return count;
}

float calculate_path_length(const Chromosome *chrom) {
    if (!chrom->actual_path || chrom->actual_path_length < 2) return 0.0f;
    
    float length = 0.0f;
    for (int i = 0; i < chrom->actual_path_length - 1; i++) {
        Position p1 = chrom->actual_path[i];
        Position p2 = chrom->actual_path[i+1];
        
        float dx = p2.x - p1.x;
        float dy = p2.y - p1.y;
        float dz = p2.z - p1.z;
        
        length += sqrtf(dx*dx + dy*dy + dz*dz);
    }
    
    return length;
}

float calculate_path_risk(const Chromosome *chrom, const Map3D *map) {
    if (!chrom->actual_path) return 0.0f;
    
    float risk = 0.0f;
    
    for (int i = 0; i < chrom->actual_path_length; i++) {
        Position pos = chrom->actual_path[i];
        
        if (pos.x >= 0 && pos.x < map->width &&
            pos.y >= 0 && pos.y < map->height &&
            pos.z >= 0 && pos.z < map->depth) {
            
            // Risk from nearby obstacles
            for (int dx = -2; dx <= 2; dx++) {
                for (int dy = -2; dy <= 2; dy++) {
                    for (int dz = -2; dz <= 2; dz++) {
                        Position neighbor = {
                            pos.x + dx,
                            pos.y + dy,
                            pos.z + dz
                        };
                        
                        if (neighbor.x >= 0 && neighbor.x < map->width &&
                            neighbor.y >= 0 && neighbor.y < map->height &&
                            neighbor.z >= 0 && neighbor.z < map->depth) {
                            
                            if (map->grid[neighbor.x][neighbor.y][neighbor.z] == 1) { // Obstacle
                                float distance = sqrtf(dx*dx + dy*dy + dz*dz);
                                risk += 1.0f / (distance + 1.0f);
                            }
                        }
                    }
                }
            }
        }
    }
    
    return risk;
}

// ============= Printing Functions =============

void print_chromosome(const Chromosome *chrom) {
    printf("┌─────────────────────────────────────┐\n");
    printf("│         Chromosome %-10d         │\n", chrom->id);
    printf("├─────────────────────────────────────┤\n");
    printf("│ Start Position: (%d, %d, %d)\n", 
           chrom->start_pos.x, chrom->start_pos.y, chrom->start_pos.z);
    printf("│ Number of Moves: %d\n", chrom->num_moves);
    printf("├─────────────────────────────────────┤\n");
    
    printf("│ Directions: ");
    for (int i = 0; i < chrom->num_moves && i < 15; i++) {
        printf("%s ", direction_to_symbol(chrom->moves[i]));
    }
    if (chrom->num_moves > 15) printf("...");
    printf("\n");
    
    printf("├─────────────────────────────────────┤\n");
    printf("│ Fitness: %.2f\n", chrom->fitness);
    printf("│ Survivors Covered: %d\n", chrom->survivors_rescued);
    printf("│ Cells Covered: %d\n", chrom->coverage_cells);
    printf("│ Path Length: %.2f\n", chrom->total_length);
    printf("│ Risk: %.2f\n", chrom->total_risk);
    printf("│ Estimated Time: %.2f seconds\n", chrom->time_estimate);
    printf("│ Status: %s\n", chrom->valid ? "Valid ✓" : "Invalid ✗");
    printf("└─────────────────────────────────────┘\n");
}

void print_chromosome_directions(const Chromosome *chrom) {
    printf("Directions (Chromosome %d):\n", chrom->id);
    for (int i = 0; i < chrom->num_moves; i++) {
        printf("%s ", direction_to_string(chrom->moves[i]));
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

void print_chromosome_path(const Chromosome *chrom) {
    if (!chrom->actual_path) {
        printf("Path not available\n");
        return;
    }
    
    printf("Full Path (Chromosome %d):\n", chrom->id);
    printf("Start → ");
    
    for (int i = 0; i < chrom->actual_path_length; i++) {
        printf("(%d,%d,%d)", 
               chrom->actual_path[i].x,
               chrom->actual_path[i].y,
               chrom->actual_path[i].z);
        
        if (i < chrom->actual_path_length - 1) {
            if (i < chrom->num_moves) {
                printf(" %s ", direction_to_symbol(chrom->moves[i]));
            } else {
                printf(" → ");
            }
        }
        
        if ((i + 1) % 3 == 0 && i < chrom->actual_path_length - 1) {
            printf("\n");
        }
    }
    printf(" → End\n");
}

// ============= Population Functions =============

Population* create_population(int size) {
    Population *pop = (Population*)malloc(sizeof(Population));
    if (!pop) return NULL;
    
    pop->individuals = (Chromosome*)malloc(size * sizeof(Chromosome));
    if (!pop->individuals) {
        free(pop);
        return NULL;
    }
    
    // Initialize chromosomes
    for (int i = 0; i < size; i++) {
        pop->individuals[i].moves = NULL;
        pop->individuals[i].actual_path = NULL;
    }
    
    pop->size = size;
    pop->generation = 0;
    pop->best = NULL;
    pop->best_fitness = -INFINITY;
    pop->avg_fitness = 0.0f;
    pop->worst_fitness = INFINITY;
    
    return pop;
}

Population* create_initial_population(Position start_pos, int pop_size, 
                                      int max_steps, const Map3D *map) {
    Population *pop = create_population(pop_size);
    if (!pop) return NULL;
    
    for (int i = 0; i < pop_size; i++) {
        // Use random initialization only
        init_random_chromosome(&pop->individuals[i], start_pos, max_steps);
        pop->individuals[i].id = 1000 + i;
    }
    
    return pop;
}

void free_population(Population *pop) {
    if (pop) {
        if (pop->individuals) {
            for (int i = 0; i < pop->size; i++) {
                if (pop->individuals[i].moves) {
                    free(pop->individuals[i].moves);
                }
                if (pop->individuals[i].actual_path) {
                    free(pop->individuals[i].actual_path);
                }
            }
            free(pop->individuals);
        }
        free(pop);
    }
}

// ============= Helper Functions =============

const char* direction_to_string(Direction dir) {
    switch(dir) {
        case DIR_RIGHT: return "RIGHT";
        case DIR_LEFT: return "LEFT";
        case DIR_UP: return "UP";
        case DIR_DOWN: return "DOWN";
        case DIR_FORWARD: return "FORWARD";
        case DIR_BACKWARD: return "BACKWARD";
        case DIR_WAIT: return "WAIT";
        default: return "UNKNOWN";
    }
}

const char* direction_to_symbol(Direction dir) {
    switch(dir) {
        case DIR_RIGHT: return "→";
        case DIR_LEFT: return "←";
        case DIR_UP: return "↑";
        case DIR_DOWN: return "↓";
        case DIR_FORWARD: return "↗";
        case DIR_BACKWARD: return "↙";
        case DIR_WAIT: return "●";
        default: return "?";
    }
}

Direction get_random_direction() {
    return (Direction)(rand() % 7); // 7 possible directions
}

bool positions_equal(Position p1, Position p2) {
    return (p1.x == p2.x && p1.y == p2.y && p1.z == p2.z);
}

// ============= Validation Function =============

bool validate_chromosome(const Chromosome *chrom, const Map3D *map) {
    if (!chrom || !chrom->moves || chrom->num_moves <= 0) {
        return false;
    }
    
    Position current = chrom->start_pos;
    
    for (int i = 0; i < chrom->num_moves; i++) {
        Position next = current;
        
        switch (chrom->moves[i]) {
            case DIR_RIGHT: next.x++; break;
            case DIR_LEFT: next.x--; break;
            case DIR_UP: next.y++; break;
            case DIR_DOWN: next.y--; break;
            case DIR_FORWARD: next.z++; break;
            case DIR_BACKWARD: next.z--; break;
            case DIR_WAIT: break;
        }
        
        // Check map boundaries
        if (next.x < 0 || next.x >= map->width ||
            next.y < 0 || next.y >= map->height ||
            next.z < 0 || next.z >= map->depth) {
            return false;
        }
        
        // Check for obstacles
        if (map->grid[next.x][next.y][next.z] == 1) {
            return false;
        }
        
        current = next;
    }
    
    return true;
}

// ============= Smart Chromosome Generator =============

Chromosome* generate_smart_chromosome(Position start, int max_steps, const Map3D *map) {
    Chromosome *chrom = create_chromosome(start, max_steps);
    if (!chrom) return NULL;
    
    chrom->num_moves = max_steps;
    Position current = start;
    
    for (int i = 0; i < max_steps; i++) {
        Direction possible_dirs[7];
        int num_possible = 0;
        
        // Check each direction
        for (int dir = 0; dir < 7; dir++) {
            Position test = current;
            
            switch (dir) {
                case DIR_RIGHT: test.x++; break;
                case DIR_LEFT: test.x--; break;
                case DIR_UP: test.y++; break;
                case DIR_DOWN: test.y--; break;
                case DIR_FORWARD: test.z++; break;
                case DIR_BACKWARD: test.z--; break;
                case DIR_WAIT: break;
            }
            
            if (test.x >= 0 && test.x < map->width &&
                test.y >= 0 && test.y < map->height &&
                test.z >= 0 && test.z < map->depth &&
                map->grid[test.x][test.y][test.z] == 0) {
                possible_dirs[num_possible++] = (Direction)dir;
            }
        }
        
        if (num_possible > 0) {
            int choice = rand() % num_possible;
            chrom->moves[i] = possible_dirs[choice];
            
            // Update position
            switch (chrom->moves[i]) {
                case DIR_RIGHT: current.x++; break;
                case DIR_LEFT: current.x--; break;
                case DIR_UP: current.y++; break;
                case DIR_DOWN: current.y--; break;
                case DIR_FORWARD: current.z++; break;
                case DIR_BACKWARD: current.z--; break;
                case DIR_WAIT: break;
            }
        } else {
            chrom->moves[i] = DIR_WAIT;
        }
    }
    
    return chrom;
}

// ============= Survivor-Focused Chromosome Generator =============

Chromosome* generate_survivor_focused_chromosome(Position start, int max_steps, const Map3D *map) {
    Chromosome *chrom = create_chromosome(start, max_steps);
    if (!chrom) return NULL;
    
    chrom->num_moves = max_steps;
    Position current = start;
    
    if (map->survivor_count == 0) {
        return generate_smart_chromosome(start, max_steps, map);
    }
    
    for (int i = 0; i < max_steps; i++) {
        int closest_survivor = -1;
        float min_distance = INFINITY;
        
        for (int s = 0; s < map->survivor_count; s++) {
            Position survivor_pos = map->survivors[s].pos;
            float dist = sqrtf(pow(survivor_pos.x - current.x, 2) +
                              pow(survivor_pos.y - current.y, 2) +
                              pow(survivor_pos.z - current.z, 2));
            
            if (dist < min_distance) {
                min_distance = dist;
                closest_survivor = s;
            }
        }
        
        Direction best_dir = DIR_WAIT;
        float best_score = -INFINITY;
        
        for (int dir = 0; dir < 7; dir++) {
            Position test = current;
            
            switch (dir) {
                case DIR_RIGHT: test.x++; break;
                case DIR_LEFT: test.x--; break;
                case DIR_UP: test.y++; break;
                case DIR_DOWN: test.y--; break;
                case DIR_FORWARD: test.z++; break;
                case DIR_BACKWARD: test.z--; break;
                case DIR_WAIT: break;
            }
            
            if (test.x >= 0 && test.x < map->width &&
                test.y >= 0 && test.y < map->height &&
                test.z >= 0 && test.z < map->depth &&
                map->grid[test.x][test.y][test.z] == 0) {
                
                Position survivor_pos = map->survivors[closest_survivor].pos;
                float dist_to_survivor = sqrtf(pow(survivor_pos.x - test.x, 2) +
                                               pow(survivor_pos.y - test.y, 2) +
                                               pow(survivor_pos.z - test.z, 2));
                
                float score = 1.0f / (dist_to_survivor + 1.0f);
                
                if (score > best_score) {
                    best_score = score;
                    best_dir = (Direction)dir;
                }
            }
        }
        
        chrom->moves[i] = best_dir;
        
        switch (best_dir) {
            case DIR_RIGHT: current.x++; break;
            case DIR_LEFT: current.x--; break;
            case DIR_UP: current.y++; break;
            case DIR_DOWN: current.y--; break;
            case DIR_FORWARD: current.z++; break;
            case DIR_BACKWARD: current.z--; break;
            case DIR_WAIT: break;
        }
    }
    
    return chrom;
}

// ============= Other Helper Functions =============

Direction get_opposite_direction(Direction dir) {
    switch(dir) {
        case DIR_RIGHT: return DIR_LEFT;
        case DIR_LEFT: return DIR_RIGHT;
        case DIR_UP: return DIR_DOWN;
        case DIR_DOWN: return DIR_UP;
        case DIR_FORWARD: return DIR_BACKWARD;
        case DIR_BACKWARD: return DIR_FORWARD;
        case DIR_WAIT: return DIR_WAIT;
        default: return DIR_WAIT;
    }
}

float distance_between_positions(Position p1, Position p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dz = p2.z - p1.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

int manhattan_distance(Position p1, Position p2) {
    return abs(p2.x - p1.x) + abs(p2.y - p1.y) + abs(p2.z - p1.z);
}

bool is_position_in_path(const Chromosome *chrom, Position pos) {
    if (!chrom->actual_path) return false;
    
    for (int i = 0; i < chrom->actual_path_length; i++) {
        if (positions_equal(chrom->actual_path[i], pos)) {
            return true;
        }
    }
    return false;
}

int count_unique_positions(const Chromosome *chrom) {
    if (!chrom->actual_path || chrom->actual_path_length == 0) return 0;
    
    // Simple method to count unique positions
    int unique = 0;
    for (int i = 0; i < chrom->actual_path_length; i++) {
        bool is_duplicate = false;
        for (int j = 0; j < i; j++) {
            if (positions_equal(chrom->actual_path[i], chrom->actual_path[j])) {
                is_duplicate = true;
                break;
            }
        }
        if (!is_duplicate) unique++;
    }
    return unique;
}