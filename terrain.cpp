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

    struct Node
    {
        TerrainTile* tile;
        float g_cost;
        float h_cost;
        Node* parent;
        float f_cost() const { return g_cost + h_cost; }
    };

    struct Compare_nodes {
        bool operator()(const Node* a, const Node* b) { return a->f_cost() > b->f_cost(); }
    };

    float Terrain::heuristic(const TerrainTile* a, const TerrainTile* b) {
        return std::abs((float)a->position_x - b->position_x) +
            std::abs((float)a->position_y - b->position_y);
    }

	// A* pathfinding algorithm
    std::vector<vec2> Terrain::get_route(const Tank& tank, const vec2& target) {
        size_t start_x = tank.position.x / sprite_size;
        size_t start_y = tank.position.y / sprite_size;
        size_t target_x = target.x / sprite_size;
        size_t target_y = target.y / sprite_size;

        TerrainTile* start_tile = &tiles[start_y][start_x];
        TerrainTile* target_tile = &tiles[target_y][target_x];

        std::priority_queue<Node*, std::vector<Node*>, Compare_nodes> open_set;
        std::unordered_map<TerrainTile*, Node*> all_nodes;

        Node* start_node = new Node{ start_tile, 0, heuristic(start_tile, target_tile), nullptr };
        open_set.push(start_node);
        all_nodes[start_tile] = start_node;

        while (!open_set.empty())
        {
            Node* current = open_set.top();
            open_set.pop();

            if (current->tile == target_tile)
            {
                std::vector<vec2> path;
                while (current)
                {
                    path.emplace_back(current->tile->position_x * sprite_size,
                        current->tile->position_y * sprite_size);
                    current = current->parent;
                }
                std::reverse(path.begin(), path.end());

                for (auto& node : all_nodes) delete node.second;
                return path;
            }

            for (TerrainTile* neighbor : current->tile->exits)
            {
                float new_g_cost = current->g_cost + 1.0f;
                if (!all_nodes.count(neighbor) || new_g_cost < all_nodes[neighbor]->g_cost)
                {
                    Node* new_node = new Node{ neighbor, new_g_cost, heuristic(neighbor, target_tile), current };
                    open_set.push(new_node);
                    all_nodes[neighbor] = new_node;
                }
            }
        }

        for (auto& node : all_nodes) delete node.second;
        return {};
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

	bool Terrain::is_accessible(int y, int x) {
		return y >= 0 && y < terrain_height && x >= 0 && x < terrain_width &&
			tiles[y][x].tile_type != TileType::WATER;
	}
}