#include "precomp.h"
namespace fs = std::filesystem;
namespace Tmpl8
{
    Terrain::Terrain()
    {
        // Load in terrain sprites
        grass_img = std::make_unique<Surface>("assets/tile_grass.png");
        forest_img = std::make_unique<Surface>("assets/tile_forest.png");
        rocks_img = std::make_unique<Surface>("assets/tile_rocks.png");
        mountains_img = std::make_unique<Surface>("assets/tile_mountains.png");
        water_img = std::make_unique<Surface>("assets/tile_water.png");

        tile_grass = std::make_unique<Sprite>(grass_img.get(), 1);
        tile_forest = std::make_unique<Sprite>(forest_img.get(), 1);
        tile_rocks = std::make_unique<Sprite>(rocks_img.get(), 1);
        tile_water = std::make_unique<Sprite>(water_img.get(), 1);
        tile_mountains = std::make_unique<Sprite>(mountains_img.get(), 1);

        // Load terrain layout file
        fs::path terrain_file_path{ "assets/terrain.txt" };
        std::ifstream terrain_file(terrain_file_path);

        if (terrain_file.is_open())
        {
            std::string terrain_line;
            std::getline(terrain_file, terrain_line);
            std::istringstream lineStream(terrain_line);

            int rows;
            lineStream >> rows;

            for (size_t row = 0; row < rows; row++)
            {
                std::getline(terrain_file, terrain_line);
                for (size_t col = 0; col < terrain_line.size(); col++)
                {
                    switch (std::toupper(terrain_line.at(col)))
                    {
                    case 'G': tiles[row][col].tile_type = TileType::GRASS; break;
                    case 'F': tiles[row][col].tile_type = TileType::FORREST; break;
                    case 'R': tiles[row][col].tile_type = TileType::ROCKS; break;
                    case 'M': tiles[row][col].tile_type = TileType::MOUNTAINS; break;
                    case 'W': tiles[row][col].tile_type = TileType::WATER; break;
                    default:  tiles[row][col].tile_type = TileType::GRASS; break;
                    }
                }
            }
        }
        else
        {
            std::cout << "Could not open terrain file! Defaulting to grass.." << std::endl;
        }

        // Initialize tiles for path planning
        for (size_t y = 0; y < tiles.size(); y++)
        {
            for (size_t x = 0; x < tiles[y].size(); x++)
            {
                tiles[y][x].position_x = x;
                tiles[y][x].position_y = y;

                if (is_accessible(y, x + 1)) tiles[y][x].exits.push_back(&tiles[y][x + 1]);
                if (is_accessible(y, x - 1)) tiles[y][x].exits.push_back(&tiles[y][x - 1]);
                if (is_accessible(y + 1, x)) tiles[y][x].exits.push_back(&tiles[y + 1][x]);
                if (is_accessible(y - 1, x)) tiles[y][x].exits.push_back(&tiles[y - 1][x]);
            }
        }
    }

    void Terrain::update()
    {
        // Placeholder for future animations
    }

    void Terrain::draw(Surface* target) const
    {
        for (size_t y = 0; y < tiles.size(); y++)
        {
            for (size_t x = 0; x < tiles[y].size(); x++)
            {
                int posX = x * sprite_size;
                int posY = y * sprite_size;

                switch (tiles[y][x].tile_type)
                {
                case TileType::GRASS: tile_grass->draw(target, posX, posY); break;
                case TileType::FORREST: tile_forest->draw(target, posX, posY); break;
                case TileType::ROCKS: tile_rocks->draw(target, posX, posY); break;
                case TileType::MOUNTAINS: tile_mountains->draw(target, posX, posY); break;
                case TileType::WATER: tile_water->draw(target, posX, posY); break;
                default: tile_grass->draw(target, posX, posY); break;
                }
            }
        }
    }

    // A node used in the A* algorithm to represent a tile in the search
    struct Node
    {
        TerrainTile* tile;   // Pointer to the tile this node represents
        float g_cost;        // Cost from the start tile to this node
        float h_cost;        // Estimated cost (heuristic) from this node to the target
        Node* parent;        // Pointer to the previous node in the path

        // Total estimated cost of the path through this node
        float f_cost() const { return g_cost + h_cost; }
    };


    // Comparison function used in the priority queue to sort nodes by lowest f_cost
    struct Compare_nodes {
        bool operator()(const Node* a, const Node* b) {
            return a->f_cost() > b->f_cost(); // Lower f_cost has higher priority
        }
    };


    // Heuristic function: estimates distance from a to b using Manhattan distance
    float Terrain::heuristic(const TerrainTile* a, const TerrainTile* b) {
        return std::abs((float)a->position_x - b->position_x) +
            std::abs((float)a->position_y - b->position_y);
    }

	// A* pathfinding algorithm
    std::vector<vec2> Terrain::get_route(const Tank& tank, const vec2& target) {
        // Convert pixel coordinates to grid indices (tile positions)
        size_t start_x = tank.position.x / sprite_size;
        size_t start_y = tank.position.y / sprite_size;
        size_t target_x = target.x / sprite_size;
        size_t target_y = target.y / sprite_size;

        // Get pointers to the starting and target tiles
        TerrainTile* start_tile = &tiles[start_y][start_x];
        TerrainTile* target_tile = &tiles[target_y][target_x];

        // Open set: priority queue for nodes to be evaluated (sorted by estimated cost)
        std::priority_queue<Node*, std::vector<Node*>, Compare_nodes> open_set;

        // Map to store all created nodes for memory management and path lookup
        std::unordered_map<TerrainTile*, Node*> all_nodes;

        // Create the starting node with g_cost = 0 and h_cost from heuristic
        Node* start_node = new Node{ start_tile, 0, heuristic(start_tile, target_tile), nullptr };
        open_set.push(start_node);
        all_nodes[start_tile] = start_node;

        // Main loop: continue until there are no more nodes to evaluate
        while (!open_set.empty())
        {
            // Get the node with the lowest estimated total cost (g + h)
            Node* current = open_set.top();
            open_set.pop();

            // If we reached the goal, reconstruct the path
            if (current->tile == target_tile)
            {
                std::vector<vec2> path;
                while (current)
                {
                    // Convert tile coordinates back to pixel positions
                    path.emplace_back(current->tile->position_x * sprite_size,
                        current->tile->position_y * sprite_size);
                    current = current->parent;
                }
                // Reverse the path to start from the beginning
                std::reverse(path.begin(), path.end());

                // Free memory for all created nodes
                for (auto& node : all_nodes) delete node.second;

                return path; // Return the final path
            }

            // Loop through all neighboring tiles (accessible neighbors)
            for (TerrainTile* neighbor : current->tile->exits)
            {
                float new_g_cost = current->g_cost + 1.0f; // Cost from start to neighbor (assumes uniform cost)

                // If neighbor has not been visited or a shorter path is found
                if (!all_nodes.count(neighbor) || new_g_cost < all_nodes[neighbor]->g_cost)
                {
                    // Create a new node for this neighbor with updated costs and parent
                    Node* new_node = new Node{ neighbor, new_g_cost, heuristic(neighbor, target_tile), current };
                    open_set.push(new_node); // Add it to the open set
                    all_nodes[neighbor] = new_node; // Save it in the map
                }
            }
        }

        // No path found: clean up all allocated nodes
        for (auto& node : all_nodes) delete node.second;

        return {}; // Return empty path if unreachable
    }

	float Terrain::get_speed_modifier(const vec2& position) const {
		size_t x = position.x / sprite_size;
		size_t y = position.y / sprite_size;

		switch (tiles[y][x].tile_type)
		{
		case TileType::GRASS: return 1.0f;
		case TileType::FORREST: return 0.5f;
		case TileType::ROCKS: return 0.25f;
		case TileType::MOUNTAINS: return 0.1f;
		case TileType::WATER: return 0.0f;
		default: return 1.0f;
		}
	}

    bool Terrain::is_accessible(int y, int x)
    {
        //Bounds check
        if ((x >= 0 && x < terrain_width) && (y >= 0 && y < terrain_height))
        {
            //Inaccessible terrain check
            if (tiles.at(y).at(x).tile_type != TileType::MOUNTAINS && tiles.at(y).at(x).tile_type != TileType::WATER)
            {
                return true;
            }
        }

        return false;
    }
}