#pragma once

namespace Tmpl8 {

    // Forward declaration
    class Tank;

    // Grid class for spatial partitioning of the game objects
    // This speeds up collision detection and finding nearby objects considerably
    class Grid
    {
    public:
        // Initialize the grid with the given dimensions and cell size
        Grid(int screen_width, int screen_height, float cell_size);
        ~Grid();

        // Add all tanks to the grid
        void add_tanks(std::vector<Tank>& tanks);

        // Find tanks within a certain radius around a position
        std::vector<Tank*> find_tanks_in_radius(const vec2& position, float radius, allignments alignment = BLUE);

        // Find the nearest tank of a given alignment
        Tank* find_closest_enemy(const Tank& current_tank);

        // Calculate collision forces between tanks in the grid
        void calculate_tank_collisions(std::vector<Tank>& tanks);

        // Clear the grid
        void clear();

    private:
        // Calculates the cell index for a given position
        std::array<int, 2> get_cell_index(const vec2& position) const;

        // Check if a cell index is within the boundaries of the grid
        bool is_valid_cell(int x, int y) const;

        // Data structure for the grid: a 2D vector of vectors with pointers to tanks
        std::vector<std::vector<std::vector<Tank*>>> grid_cells;

        // Dimensions of the grid
        int width, height;
        float cell_size;
        int grid_width, grid_height;
    };

} // namespace Tmpl8