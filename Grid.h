#pragma once

namespace Tmpl8 {

    // Forward declaration
    class Tank;

    // Grid klasse voor spatial partitioning van de game objecten
    // Dit versnelt collision detection en het vinden van nabije objecten aanzienlijk
    class Grid
    {
    public:
        // Initialiseer het grid met de gegeven afmetingen en celgrootte
        Grid(int screen_width, int screen_height, float cell_size);
        ~Grid();

        // Voeg alle tanks toe aan het grid
        void add_tanks(std::vector<Tank>& tanks);

        // Update de positie van een tank in het grid
        void update_tank_position(Tank& tank);

        // Vind tanks binnen een bepaalde straal rond een positie
        std::vector<Tank*> find_tanks_in_radius(const vec2& position, float radius, allignments alignment = BLUE);

        // Vind de dichtstbijzijnde tank van een bepaalde allignment
        Tank* find_closest_enemy(const Tank& current_tank);

        // Bereken botsingskrachten tussen tanks in het grid
        void calculate_tank_collisions(std::vector<Tank>& tanks);

        // Maak het grid leeg
        void clear();

    private:
        // Berekent de cel-index voor een gegeven positie
        std::array<int, 2> get_cell_index(const vec2& position) const;

        // Controleer of een cel-index binnen de grenzen van het grid ligt
        bool is_valid_cell(int x, int y) const;

        // Datastructuur voor het grid: een 2D vector van vectoren met pointers naar tanks
        std::vector<std::vector<std::vector<Tank*>>> grid_cells;

        // Afmetingen van het grid
        int width, height;
        float cell_size;
        int grid_width, grid_height;
    };

} // namespace Tmpl8