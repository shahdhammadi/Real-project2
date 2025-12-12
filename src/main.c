#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "map_loader.h"
#include "chromosome.h"

// Robot definition
typedef struct
{
    int id;
    Position current_position;
    Position *path;
    int path_length;
    int survivors_rescued;
    double fitness;
} Robot;

// Simulation results definition
typedef struct
{
    Robot *robots;
    int num_robots;
    double total_fitness;
    int total_survivors_rescued;
    int generation;
    double execution_time;
} SimulationResult;

// Function to display main menu
void print_menu()
{
    printf("\n╔════════════════════════════════════════════════════╗\n");
    printf("║     COLLAPSED BUILDING RESCUE SIMULATION           ║\n");
    printf("╠════════════════════════════════════════════════════╣\n");
    printf("║ 1. 📂 Load settings from file                     ║\n");
    printf("║ 2. 🗺️  Create new map                             ║\n");
    printf("║ 3. 🚀 generate_and_print_10_chromosomes           ║\n");
    printf("║ 4. ❌ Exit                                        ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");
    printf("Please choose an option (1-4): ");
}

// Function to free simulation result memory
void free_simulation_result(SimulationResult *result)
{
    if (!result)
        return;

    for (int i = 0; i < result->num_robots; i++)
    {
        if (result->robots[i].path)
        {
            free(result->robots[i].path);
        }
    }

    if (result->robots)
        free(result->robots);
    free(result);
}

// Function to run Python visualizer
void run_python_visualizer()
{
    printf("\n🎨 Running Python Map Visualizer...\n");
    
    // بناء أمر تشغيل Python
    char command[256];
    snprintf(command, sizeof(command), 
             "python3 draw-map.py data/saved_map.txt");
    
    printf("Executing: %s\n", command);
    
    int result = system(command);
    
    if (result != 0)
    {
        printf("❌ Python visualizer failed to run.\n");
        printf("💡 Make sure:\n");
        printf("   1. Python 3 is installed\n");
        printf("   2. matplotlib is installed (pip install matplotlib)\n");
        printf("   3. draw-map.py exists in the same directory\n");
    }
}
void generate_and_print_10_chromosomes() {
    printf("\n🧬 Generate and Print 10 Initial Chromosomes\n");
    printf("===========================================\n\n");
    
    // 1. Get input from user
    int max_steps;
    printf("Enter the number of steps for each path (1-15): ");
    scanf("%d", &max_steps);
    getchar();
    
    if (max_steps < 2) max_steps = 2;
    if (max_steps > 10) max_steps = 10;
    
    // 2. Create a simple test map
    printf("\n🔹 Creating Test Map...\n");
    Map3D *map = create_map(10, 10, 3);
    if (!map) {
        printf("❌ Error Creating Map!\n");
        return;
    }
    
    initialize_map(map, 0.2, 0.05);  // 20% obstacles, 5% survivors
    
    // 3. Start position
    Position start = {0, 0, 0};
    
    // 4. Create 10 chromosomes
    printf("🔹 Creating 10 random chromosomes...\n");
    Population *population = create_initial_population(start, 10, max_steps, map);
    
    if (!population) {
        printf("❌ Error Creating Population!\n");
        free_map(map);
        return;
    }
    
    printf("\n✅ 10 chromosomes created successfully!\n");
    printf("📊 Printing chromosomes now...\n\n");
    
    // 5. Print all chromosomes
    for (int i = 0; i < population->size; i++) {
        printf("═══════════════════════════════════════════════\n");
        printf("               Chromosome %02d                \n", i + 1);
        printf("═══════════════════════════════════════════════\n");
        
        // Print chromosome
        print_chromosome(&population->individuals[i]);
        
        // Print first 10 moves in detail
        printf("\nFirst 10 Moves:\n");
        printf("No  | Direction | Symbol\n");
        printf("----|-----------|-------\n");
        
        for (int j = 0; j < 10 && j < population->individuals[i].num_moves; j++) {
            Direction dir = population->individuals[i].moves[j];
            printf("%3d | %-9s | %s\n", 
                   j + 1, 
                   direction_to_string(dir),
                   direction_to_symbol(dir));
        }
        
        // Display path if it's short
        if (max_steps <= 20) {
            printf("\nComplete Path:\n");
            print_chromosome_path(&population->individuals[i]);
        } else {
            printf("\n(Path too long for full display)\n");
        }
        
        printf("\n");
        
        // Pause every 5 chromosomes
        if ((i + 1) % 5 == 0 && i < population->size - 1) {
            printf("Displayed %d chromosomes. Press Enter to continue...", i + 1);
            getchar();
            printf("\n");
        }
    }
    
    // 6. General statistics
    printf("\n📈 General Statistics for 10 Chromosomes:\n");
    printf("========================================\n");
    
    // Count directions
    int dir_counts[7] = {0};
    char *dir_names[] = {"RIGHT", "LEFT", "UP", "DOWN", "FORWARD", "BACKWARD", "WAIT"};
    char *dir_symbols[] = {"→", "←", "↑", "↓", "↗", "↙", "●"};
    
    for (int i = 0; i < population->size; i++) {
        for (int j = 0; j < population->individuals[i].num_moves; j++) {
            dir_counts[population->individuals[i].moves[j]]++;
        }
    }
    
    int total_moves = 10 * max_steps;
    printf("Total Moves: %d\n", total_moves);
    printf("\nDirection Distribution:\n");
    
    for (int i = 0; i < 7; i++) {
        float percentage = (dir_counts[i] * 100.0f) / total_moves;
        printf("  %s %s: %6d (%5.1f%%) ", 
               dir_symbols[i], dir_names[i], dir_counts[i], percentage);
        
        // Progress bar
        int bars = (int)(percentage / 2);
        for (int b = 0; b < bars; b++) printf("█");
        printf("\n");
    }
    
    // 7. Save to file
    printf("\n💾 Saving chromosomes to file...\n");
    
    FILE *file = fopen("10_chromosomes.txt", "w");
    if (file) {
        fprintf(file, "10 Initial Chromosomes\n");
        fprintf(file, "=======================\n\n");
        
        for (int i = 0; i < population->size; i++) {
            fprintf(file, "Chromosome %02d (ID: %d):\n", 
                    i + 1, population->individuals[i].id);
            fprintf(file, "  Start Position: (%d,%d,%d)\n", 
                    population->individuals[i].start_pos.x,
                    population->individuals[i].start_pos.y,
                    population->individuals[i].start_pos.z);
            
            fprintf(file, "  Directions: ");
            for (int j = 0; j < population->individuals[i].num_moves; j++) {
                fprintf(file, "%s ", 
                        direction_to_string(population->individuals[i].moves[j]));
                if ((j + 1) % 10 == 0) fprintf(file, "\n                ");
            }
            fprintf(file, "\n\n");
        }
        
        fclose(file);
        printf("✅ Chromosomes saved to file '10_chromosomes.txt'\n");
    } else {
        printf("❌ Error saving file!\n");
    }
    
    // 8. Display brief examples
    printf("\n🔍 Brief Examples of 5 Chromosomes:\n");
    printf("===================================\n");
    
    // 9. Cleanup
    printf("\n🧹 Cleaning memory...\n");
    free_population(population);
    free_map(map);
    
    printf("\n✅ Finished generating and printing 10 chromosomes!\n");
    printf("Press Enter to return to menu...");
    getchar();
}

// Main function
int main(int argc, char *argv[])
{
    // Main variables
    Settings *settings = NULL;
    Map3D *map = NULL;
    SimulationResult *last_result = NULL;

    // Check for command line arguments
    char *config_file = "config/settings.txt";
    if (argc > 1)
    {
        config_file = argv[1];
        printf("Using settings file: %s\n", config_file);
    }
    else
    {
        printf("Using default settings file: config/settings.txt\n");
    }

    int choice;
    char input[100];

    do
    {
        print_menu();

        if (fgets(input, sizeof(input), stdin) != NULL)
        {
            choice = atoi(input);

            switch (choice)
            {
            case 1: // Load settings
                if (settings)
                {
                    free(settings);
                    settings = NULL;
                }
                settings = load_settings(config_file);
                if (settings)
                {
                  //  print_settings(settings);
                    printf("✅ Settings loaded successfully.\n");
                }
                else
                {
                    printf("❌ Failed to load settings. Please check settings file.\n");
                }
                break;

            case 2: // Create new map
                if (!settings)
                {
                    printf("⚠️ Please load settings first (Option 1)\n");
                    break;
                }

                if (map)
                {
                    free_map(map);
                    map = NULL;
                }
                
                printf("\n🧱 Creating new map...\n");
                map = create_map(settings->map_width,
                                 settings->map_height,
                                 settings->map_depth);

                if (map)
                {
                    initialize_map(map, settings->obstacle_ratio,
                                   settings->survivor_ratio);
                    map->start_position = settings->robot_start;
                    
                    printf("\n✅ New map created successfully!\n");
                    printf("   Dimensions: %d × %d × %d\n", 
                           map->width, map->height, map->depth);
                    printf("   Survivors: %d\n", map->survivor_count);
                    
                    // طباعة ملخص سريع
                    print_map(map);
                }
                else
                {
                    printf("❌ Failed to create map.\n");
                }
                break;


            case 3: // Run simulation
                
                    generate_and_print_10_chromosomes();
                

                break;
        
            case 4: // Exit
                printf("\n════════════════════════════════════════════════════════════\n");
                printf("👋 Thank you for using the Collapsed Building Rescue System!\n");
                printf("   Goodbye!\n");
                printf("════════════════════════════════════════════════════════════\n");
                break;

            default:
                printf("❌ Invalid choice. Please enter a number between 1-6.\n");
            }
        }
        
        // مسح الإدخال السابق
        fflush(stdin);
        
    } while (choice != 4);

    // Free memory before exiting
    if (settings)
    {
        free(settings);
        printf("✓ Settings memory freed\n");
    }
    if (map)
    {
        free_map(map);
        printf("✓ Map memory freed\n");
    }
    if (last_result)
    {
        free_simulation_result(last_result);
        printf("✓ Simulation results memory freed\n");
    }

    printf("\n🎯 Program terminated successfully.\n");
    return 0;
}