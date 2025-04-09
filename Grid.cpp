#include "precomp.h"

namespace Tmpl8 {

    Grid::Grid(int screen_width, int screen_height, float cell_size)
        : width(screen_width), height(screen_height), cell_size(cell_size)
    {
        // Calculate the number of cells in the grid
        grid_width = (int)(width / cell_size) + 1;
        grid_height = (int)(height / cell_size) + 1;

        // Initialize the grid with empty vectors
        grid_cells.resize(grid_width);
        for (int i = 0; i < grid_width; i++) {
            grid_cells[i].resize(grid_height);
        }
    }

    Grid::~Grid()
    {
        // Grid doesn't need to free up memory because it only contains pointers
        // that point to tanks managed elsewhere
    }

    void Grid::add_tanks(std::vector<Tank>& tanks)
    {
        // Clear the grid first
        clear();

        // Add each tank to the grid
        for (auto& tank : tanks) {
            if (tank.active) {
                std::array<int, 2> cell_idx = get_cell_index(tank.position);
                if (is_valid_cell(cell_idx[0], cell_idx[1])) {
                    grid_cells[cell_idx[0]][cell_idx[1]].push_back(&tank);
                }
            }
        }
    }

    std::vector<Tank*> Grid::find_tanks_in_radius(const vec2& position, float radius, allignments alignment)
    {
        std::vector<Tank*> result;

        // Determine the cells that overlap with the radius
        float radius_squared = radius * radius;
        int cell_radius = (int)(radius / cell_size) + 1;

        std::array<int, 2> center_cell = get_cell_index(position);

        // Loop through all cells within the radius
        for (int dx = -cell_radius; dx <= cell_radius; dx++) {
            for (int dy = -cell_radius; dy <= cell_radius; dy++) {
                int cell_x = center_cell[0] + dx;
                int cell_y = center_cell[1] + dy;

                if (is_valid_cell(cell_x, cell_y)) {
                    // Go through all the tanks in this cell
                    for (Tank* tank : grid_cells[cell_x][cell_y]) {
                        // Check that the tank is within the radius and has the correct alignment
                        vec2 diff = tank->position - position;
                        if (diff.sqr_length() <= radius_squared &&
                            (alignment == tank->allignment || alignment == BLUE && tank->allignment == RED || alignment == RED && tank->allignment == BLUE)) {
                            result.push_back(tank);
                        }
                    }
                }
            }
        }

        return result;
    }

    Tank* Grid::find_closest_enemy(const Tank& current_tank)
    {
        float closest_distance = std::numeric_limits<float>::infinity();
        Tank* closest_tank = nullptr;

        // Determine the search radius (start with a small value and increase if necessary)
        float search_radius = 50.0f;
        const float max_search_radius = 1500.0f; // Maximum search distance

        while (search_radius <= max_search_radius) {
            // Find tanks within current radius
            std::vector<Tank*> nearby_tanks = find_tanks_in_radius(current_tank.position, search_radius,
                (current_tank.allignment == RED) ? BLUE : RED);

            // Find the nearest tank
            for (Tank* tank : nearby_tanks) {
                if (tank->active && tank->allignment != current_tank.allignment) {
                    float sqr_dist = (tank->position - current_tank.position).sqr_length();
                    if (sqr_dist < closest_distance) {
                        closest_distance = sqr_dist;
                        closest_tank = tank;
                    }
                }
            }

            // If we find a tank, stop looking
            if (closest_tank != nullptr) {
                break;
            }

            // Increase the search radius
            search_radius *= 2.0f;
        }

        // If no enemy found, take the first active enemy (fallback)
        if (closest_tank == nullptr) {
            // This really shouldn't happen if there are still active enemies
            // But as a safety fallback it's good
            return nullptr; // The game class should handle this
        }

        return closest_tank;
    }

    void Grid::calculate_tank_collisions(std::vector<Tank>& tanks)
    {
        // Add all tanks to the grid
        add_tanks(tanks);

        // Loop through all active tanks
        for (Tank& tank : tanks) {
            if (!tank.active) continue;

            // Determine the current cell of the tank
            std::array<int, 2> cell_idx = get_cell_index(tank.position);

            // Check collision with tanks in the same and adjacent cells
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    int neighbor_x = cell_idx[0] + dx;
                    int neighbor_y = cell_idx[1] + dy;

                    if (!is_valid_cell(neighbor_x, neighbor_y)) continue;

                    // Check all tanks in this cell
                    for (Tank* other_tank : grid_cells[neighbor_x][neighbor_y]) {
                        // Skip itself and inactive tanks
                        if (&tank == other_tank || !other_tank->active) continue;

                        // Calculate collision
                        vec2 dir = tank.position - other_tank->position;
                        float dir_squared_len = dir.sqr_length();

                        float col_squared_len = (tank.collision_radius + other_tank->collision_radius);
                        col_squared_len *= col_squared_len;

                        if (dir_squared_len < col_squared_len) {
                            tank.push(dir.normalized(), 1.f);
                        }
                    }
                }
            }
        }
    }

    void Grid::clear()
    {
        for (int i = 0; i < grid_width; i++) {
            for (int j = 0; j < grid_height; j++) {
                grid_cells[i][j].clear();
            }
        }
    }

    std::array<int, 2> Grid::get_cell_index(const vec2& position) const
    {
        int x = (int)(position.x / cell_size);
        int y = (int)(position.y / cell_size);
        return { x, y };
    }

    bool Grid::is_valid_cell(int x, int y) const
    {
        return (x >= 0 && x < grid_width && y >= 0 && y < grid_height);
    }

} // namespace Tmpl8