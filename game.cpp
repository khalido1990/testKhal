#include "precomp.h" // include (only) this in every .cpp file

constexpr auto num_tanks_blue = 2048;
constexpr auto num_tanks_red = 2048;

constexpr auto tank_max_health = 1000;
constexpr auto rocket_hit_value = 60;
constexpr auto particle_beam_hit_value = 50;

constexpr auto tank_max_speed = 1.0;

constexpr auto health_bar_width = 70;

constexpr auto max_frames = 2000;

//Global performance timer
constexpr auto REF_PERFORMANCE = 143811; //UPDATE THIS WITH YOUR REFERENCE PERFORMANCE (see console after 2k frames)
static timer perf_timer;
static float duration;

//Load sprite files and initialize sprites
static Surface* tank_red_img = new Surface("assets/Tank_Proj2.png");
static Surface* tank_blue_img = new Surface("assets/Tank_Blue_Proj2.png");
static Surface* rocket_red_img = new Surface("assets/Rocket_Proj2.png");
static Surface* rocket_blue_img = new Surface("assets/Rocket_Blue_Proj2.png");
static Surface* particle_beam_img = new Surface("assets/Particle_Beam.png");
static Surface* smoke_img = new Surface("assets/Smoke.png");
static Surface* explosion_img = new Surface("assets/Explosion.png");

static Sprite tank_red(tank_red_img, 12);
static Sprite tank_blue(tank_blue_img, 12);
static Sprite rocket_red(rocket_red_img, 12);
static Sprite rocket_blue(rocket_blue_img, 12);
static Sprite smoke(smoke_img, 4);
static Sprite explosion(explosion_img, 9);
static Sprite particle_beam_sprite(particle_beam_img, 3);

const static vec2 tank_size(7, 9);
const static vec2 rocket_size(6, 6);

const static float tank_radius = 3.f;
const static float rocket_radius = 5.f;

// -----------------------------------------------------------
// Initialize the simulation state
// This function does not count for the performance multiplier
// (Feel free to optimize anyway though ;) )
// -----------------------------------------------------------
void Game::init()
{
    frame_count_font = new Font("assets/digital_small.png", "ABCDEFGHIJKLMNOPQRSTUVWXYZ:?!=-0123456789.");

    grid = new Grid(SCRWIDTH, SCRHEIGHT, 20.0f);
    tanks.reserve(num_tanks_blue + num_tanks_red);

    uint max_rows = 24;

    float start_blue_x = tank_size.x + 40.0f;
    float start_blue_y = tank_size.y + 30.0f;

    float start_red_x = 1088.0f;
    float start_red_y = tank_size.y + 30.0f;

    float spacing = 7.5f;

    //Spawn blue tanks
    for (int i = 0; i < num_tanks_blue; i++)
    {
        vec2 position{ start_blue_x + ((i % max_rows) * spacing), start_blue_y + ((i / max_rows) * spacing) };
        tanks.push_back(Tank(position.x, position.y, BLUE, &tank_blue, &smoke, 1100.f, position.y + 16, tank_radius, tank_max_health, tank_max_speed));
    }
    //Spawn red tanks
    for (int i = 0; i < num_tanks_red; i++)
    {
        vec2 position{ start_red_x + ((i % max_rows) * spacing), start_red_y + ((i / max_rows) * spacing) };
        tanks.push_back(Tank(position.x, position.y, RED, &tank_red, &smoke, 100.f, position.y + 16, tank_radius, tank_max_health, tank_max_speed));
    }

    particle_beams.push_back(Particle_beam(vec2(590, 327), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(Particle_beam(vec2(64, 64), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
    particle_beams.push_back(Particle_beam(vec2(1200, 600), vec2(100, 50), &particle_beam_sprite, particle_beam_hit_value));
}

// -----------------------------------------------------------
// Close down application
// -----------------------------------------------------------
void Game::shutdown()
{
}

// -----------------------------------------------------------
// Iterates through all tanks and returns the closest enemy tank for the given tank
// -----------------------------------------------------------
Tank& Game::find_closest_enemy(Tank& current_tank)
{
    // Gebruik het grid om de dichtstbijzijnde vijand te vinden
    Tank* enemy = grid->find_closest_enemy(current_tank);

    // Als geen vijand gevonden is (zou niet moeten gebeuren), val terug op de oude methode
    if (enemy == nullptr) {
        // Fallback op originele methode
        float closest_distance = numeric_limits<float>::infinity();
        int closest_index = 0;

        for (int i = 0; i < tanks.size(); i++)
        {
            if (tanks.at(i).allignment != current_tank.allignment && tanks.at(i).active)
            {
                float sqr_dist = fabsf((tanks.at(i).get_position() - current_tank.get_position()).sqr_length());
                if (sqr_dist < closest_distance)
                {
                    closest_distance = sqr_dist;
                    closest_index = i;
                }
            }
        }

        return tanks.at(closest_index);
    }

    // Anders, geef de gevonden vijand terug
    return *enemy;
}

//Checks if a point lies on the left of an arbitrary angled line
bool Tmpl8::Game::left_of_line(vec2 line_start, vec2 line_end, vec2 point)
{
    return ((line_end.x - line_start.x) * (point.y - line_start.y) - (line_end.y - line_start.y) * (point.x - line_start.x)) < 0;
}

// -----------------------------------------------------------
// Calculate initial routes for all tanks (only called once)
// -----------------------------------------------------------
void Game::calculate_initial_routes()
{
    for (Tank& t : tanks)
    {
        t.set_route(background_terrain.get_route(t, t.target));
    }
}

// -----------------------------------------------------------
// Handle tank collisions and push tanks away from each other
// -----------------------------------------------------------
void Game::handle_tank_collisions()
{
    grid->calculate_tank_collisions(tanks);
}

// -----------------------------------------------------------
// Update tanks movement and handle shooting
// -----------------------------------------------------------
void Game::update_tanks()
{
    for (Tank& tank : tanks)
    {
        if (!tank.active) continue;

        // Move tanks according to speed and nudges, also reload
        tank.tick(background_terrain);

        // Shoot at closest target if reloaded
        if (tank.rocket_reloaded())
        {
            Tank& target = find_closest_enemy(tank);

            rockets.push_back(Rocket(tank.position,
                (target.get_position() - tank.position).normalized() * 3,
                rocket_radius,
                tank.allignment,
                ((tank.allignment == RED) ? &rocket_red : &rocket_blue)));

            tank.reload_rocket();
        }
    }
}

// -----------------------------------------------------------
// Update smoke plumes
// -----------------------------------------------------------
void Game::update_smoke_plumes()
{
    for (Smoke& smoke : smokes)
    {
        smoke.tick();
    }
}

// -----------------------------------------------------------
// Find the leftmost active tank to start the convex hull algorithm
// -----------------------------------------------------------
vec2 Game::find_leftmost_active_tank()
{
    vec2 leftmost_position;
    bool first_found = false;

    for (Tank& tank : tanks)
    {
        if (tank.active)
        {
            if (!first_found)
            {
                leftmost_position = tank.position;
                first_found = true;
            }
            else if (tank.position.x <= leftmost_position.x)
            {
                leftmost_position = tank.position;
            }
        }
    }

    return leftmost_position;
}

// -----------------------------------------------------------
// Check if there's at least one active tank in the game
// -----------------------------------------------------------
bool Game::has_active_tanks()
{
    for (Tank& tank : tanks)
    {
        if (tank.active) return true;
    }
    return false;
}

// -----------------------------------------------------------
// Find the first active tank's index
// -----------------------------------------------------------
int Game::find_first_active_tank_index()
{
    for (size_t i = 0; i < tanks.size(); i++)
    {
        if (tanks[i].active) return i;
    }
    return -1;  // Return -1 if no active tank is found
}

// -----------------------------------------------------------
// Calculate "forcefield" hull around active tanks using convex hull algorithm
// -----------------------------------------------------------
void Game::calculate_forcefield_hull()
{
    forcefield_hull.clear();

    // Quick check if there are any active tanks
    if (!has_active_tanks()) return;

    // Find leftmost tank position to start convex hull
    vec2 point_on_hull = find_leftmost_active_tank();
    forcefield_hull.push_back(point_on_hull);

    // Get reference point for comparison
    int first_active_idx = find_first_active_tank_index();
    if (first_active_idx == -1) return;  // Safety check

    // Gift wrapping algorithm (Jarvis march)
    while (true)
    {
        vec2 endpoint = tanks[first_active_idx].position;

        for (Tank& tank : tanks)
        {
            if (!tank.active) continue;

            if ((endpoint == point_on_hull) || left_of_line(point_on_hull, endpoint, tank.position))
            {
                endpoint = tank.position;
            }
        }

        point_on_hull = endpoint;

        // Check if we've completed the hull
        if (endpoint == forcefield_hull[0]) break;

        forcefield_hull.push_back(point_on_hull);
    }
}

// -----------------------------------------------------------
// Update rockets and check for collisions with tanks
// -----------------------------------------------------------
void Game::update_rockets_tank_collisions()
{
    for (Rocket& rocket : rockets)
    {
        if (!rocket.active) continue;

        rocket.tick();

        // Check if rocket collides with enemy tank
        for (Tank& tank : tanks)
        {
            if (!tank.active || tank.allignment == rocket.allignment) continue;

            if (rocket.intersects(tank.position, tank.collision_radius))
            {
                explosions.push_back(Explosion(&explosion, tank.position));

                if (tank.hit(rocket_hit_value))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(7, 24)));
                }

                rocket.active = false;
                break;
            }
        }
    }
}

// -----------------------------------------------------------
// Check rockets against forcefield and disable if they collide
// -----------------------------------------------------------
void Game::check_rockets_forcefield_collisions()
{
    if (forcefield_hull.empty()) return;

    for (Rocket& rocket : rockets)
    {
        if (!rocket.active) continue;

        for (size_t i = 0; i < forcefield_hull.size(); i++)
        {
            if (circle_segment_intersect(
                forcefield_hull.at(i),
                forcefield_hull.at((i + 1) % forcefield_hull.size()),
                rocket.position,
                rocket.collision_radius))
            {
                explosions.push_back(Explosion(&explosion, rocket.position));
                rocket.active = false;
                break;
            }
        }
    }
}

// -----------------------------------------------------------
// Remove inactive rockets
// -----------------------------------------------------------
void Game::remove_inactive_rockets()
{
    rockets.erase(
        std::remove_if(rockets.begin(), rockets.end(),
            [](const Rocket& rocket) { return !rocket.active; }),
        rockets.end());
}

// -----------------------------------------------------------
// Update particle beams and check for collisions with tanks
// -----------------------------------------------------------
void Game::update_particle_beams()
{
    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.tick(tanks);

        // Damage all tanks within the beam's damage window
        for (Tank& tank : tanks)
        {
            if (!tank.active) continue;

            if (particle_beam.rectangle.intersects_circle(tank.get_position(), tank.get_collision_radius()))
            {
                if (tank.hit(particle_beam.damage))
                {
                    smokes.push_back(Smoke(smoke, tank.position - vec2(0, 48)));
                }
            }
        }
    }
}

// -----------------------------------------------------------
// Update explosions and remove completed ones
// -----------------------------------------------------------
void Game::update_explosions()
{
    for (Explosion& explosion : explosions)
    {
        explosion.tick();
    }

    explosions.erase(
        std::remove_if(explosions.begin(), explosions.end(),
            [](const Explosion& explosion) { return explosion.done(); }),
        explosions.end());
}

// -----------------------------------------------------------
// Main update function that calls all the specialized update functions
// -----------------------------------------------------------
void Game::update(float deltaTime)
{
    // Calculate routes only once at the beginning
    if (frame_count == 0)
    {
        calculate_initial_routes();
    }

    // Update game entities in appropriate order
    grid->add_tanks(tanks);  // Update grid FIRST
    handle_tank_collisions();  // Then handle collisions using updated grid
    update_tanks();
    update_smoke_plumes();
    calculate_forcefield_hull();
    update_rockets_tank_collisions();
    check_rockets_forcefield_collisions();
    remove_inactive_rockets();
    update_particle_beams();
    update_explosions();
}

// -----------------------------------------------------------
// Draw all sprites to the screen
// (It is not recommended to multi-thread this function)
// -----------------------------------------------------------
void Game::draw()
{
    // clear the graphics window
    screen->clear(0);

    //Draw background
    background_terrain.draw(screen);

    //Draw sprites
    for (int i = 0; i < num_tanks_blue + num_tanks_red; i++)
    {
        tanks.at(i).draw(screen);

        vec2 tank_pos = tanks.at(i).get_position();
    }

    for (Rocket& rocket : rockets)
    {
        rocket.draw(screen);
    }

    for (Smoke& smoke : smokes)
    {
        smoke.draw(screen);
    }

    for (Particle_beam& particle_beam : particle_beams)
    {
        particle_beam.draw(screen);
    }

    for (Explosion& explosion : explosions)
    {
        explosion.draw(screen);
    }

    //Draw forcefield (mostly for debugging, its kinda ugly..)
    for (size_t i = 0; i < forcefield_hull.size(); i++)
    {
        vec2 line_start = forcefield_hull.at(i);
        vec2 line_end = forcefield_hull.at((i + 1) % forcefield_hull.size());
        line_start.x += HEALTHBAR_OFFSET;
        line_end.x += HEALTHBAR_OFFSET;
        screen->line(line_start, line_end, 0x0000ff);
    }

    //Draw sorted health bars
    for (int t = 0; t < 2; t++)
    {
        const int NUM_TANKS = ((t < 1) ? num_tanks_blue : num_tanks_red);

        const int begin = ((t < 1) ? 0 : num_tanks_blue);
        std::vector<const Tank*> sorted_tanks;
        MergeSort::sort_tanks_health(tanks, sorted_tanks, begin, begin + NUM_TANKS);
        sorted_tanks.erase(std::remove_if(sorted_tanks.begin(), sorted_tanks.end(), [](const Tank* tank) { return !tank->active; }), sorted_tanks.end());

        draw_health_bars(sorted_tanks, t);
    }
}

// -----------------------------------------------------------
// Draw the health bars based on the given tanks health values
// -----------------------------------------------------------
void Tmpl8::Game::draw_health_bars(const std::vector<const Tank*>& sorted_tanks, const int team)
{
    int health_bar_start_x = (team < 1) ? 0 : (SCRWIDTH - HEALTHBAR_OFFSET) - 1;
    int health_bar_end_x = (team < 1) ? health_bar_width : health_bar_start_x + health_bar_width - 1;

    for (int i = 0; i < SCRHEIGHT - 1; i++)
    {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x, health_bar_end_y, REDMASK);
    }

    //Draw the <SCRHEIGHT> least healthy tank health bars
    int draw_count = std::min(SCRHEIGHT, (int)sorted_tanks.size());
    for (int i = 0; i < draw_count - 1; i++)
    {
        //Health bars are 1 pixel each
        int health_bar_start_y = i * 1;
        int health_bar_end_y = health_bar_start_y + 1;

        float health_fraction = (1 - ((double)sorted_tanks.at(i)->health / (double)tank_max_health));

        if (team == 0) { screen->bar(health_bar_start_x + (int)((double)health_bar_width * health_fraction), health_bar_start_y, health_bar_end_x, health_bar_end_y, GREENMASK); }
        else { screen->bar(health_bar_start_x, health_bar_start_y, health_bar_end_x - (int)((double)health_bar_width * health_fraction), health_bar_end_y, GREENMASK); }
    }
}

// -----------------------------------------------------------
// When we reach max_frames print the duration and speedup multiplier
// Updating REF_PERFORMANCE at the top of this file with the value
// on your machine gives you an idea of the speedup your optimizations give
// -----------------------------------------------------------
void Tmpl8::Game::measure_performance()
{
    char buffer[128];
    if (frame_count >= max_frames)
    {
        if (!lock_update)
        {
            duration = perf_timer.elapsed();
            cout << "Duration was: " << duration << " (Replace REF_PERFORMANCE with this value)" << endl;
            lock_update = true;
        }

        frame_count--;
    }

    if (lock_update)
    {
        screen->bar(420 + HEALTHBAR_OFFSET, 170, 870 + HEALTHBAR_OFFSET, 430, 0x030000);
        int ms = (int)duration % 1000, sec = ((int)duration / 1000) % 60, min = ((int)duration / 60000);
        sprintf(buffer, "%02i:%02i:%03i", min, sec, ms);
        frame_count_font->centre(screen, buffer, 200);
        sprintf(buffer, "SPEEDUP: %4.1f", REF_PERFORMANCE / duration);
        frame_count_font->centre(screen, buffer, 340);
    }
}

// -----------------------------------------------------------
// Main application tick function
// -----------------------------------------------------------
void Game::tick(float deltaTime)
{
    if (!lock_update)
    {
        update(deltaTime);
    }
    draw();

    measure_performance();

    // print something in the graphics window
    //screen->Print("hello world", 2, 2, 0xffffff);

    // print something to the text window
    //cout << "This goes to the console window." << std::endl;

    //Print frame count
    frame_count++;
    string frame_count_string = "FRAME: " + std::to_string(frame_count);
    frame_count_font->print(screen, frame_count_string.c_str(), 350, 580);
}