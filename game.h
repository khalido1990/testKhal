#pragma once

namespace Tmpl8
{
//forward declarations
class Tank;
class Rocket;
class Smoke;
class Particle_beam;

class Game
{
  public:
    void set_target(Surface* surface) { screen = surface; }
    void init();
    void shutdown();
    void update(float deltaTime);
    void draw();
    void tick(float deltaTime);
    void draw_health_bars(const std::vector<const Tank*>& sorted_tanks, const int team);
    void measure_performance();

    Tank& find_closest_enemy(Tank& current_tank);

    void mouse_up(int button)
    { /* implement if you want to detect mouse button presses */
    }

    void mouse_down(int button)
    { /* implement if you want to detect mouse button presses */
    }

    void mouse_move(int x, int y)
    { /* implement if you want to detect mouse movement */
    }

    void key_up(int key)
    { /* implement if you want to handle keys */
    }

    void key_down(int key)
    { /* implement if you want to handle keys */
    }

  private:

    ThreadPool* thread_pool;
    std::mutex tanks_mutex; // For protecting tank updates

    void calculate_initial_routes();
    void handle_tank_collisions();
    void update_tanks();
    void update_smoke_plumes();
    vec2 find_leftmost_active_tank();
    bool has_active_tanks();
    int find_first_active_tank_index();
    void calculate_forcefield_hull();
    void update_rockets_tank_collisions();
    void check_rockets_forcefield_collisions();
    void remove_inactive_rockets();
    void update_particle_beams();
    void update_explosions();
    Surface* screen;

    vector<Tank> tanks;
    vector<Rocket> rockets;
    vector<Smoke> smokes;
    vector<Explosion> explosions;
    vector<Particle_beam> particle_beams;

    Grid* grid;

    Terrain background_terrain;
    std::vector<vec2> forcefield_hull;

    Font* frame_count_font;
    long long frame_count = 0;

    bool lock_update = false;

    //Checks if a point lies on the left of an arbitrary angled line
    bool left_of_line(vec2 line_start, vec2 line_end, vec2 point);
};

}; // namespace Tmpl8