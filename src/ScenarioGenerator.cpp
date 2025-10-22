#include "enmod/ScenarioGenerator.h"
#include "enmod/Types.h"
#include "enmod/json.hpp"
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <vector> // Include vector

using json = nlohmann::json;

// --- Helper function Definitions ---
auto createSpreadingFire = [](json& config, Position origin, int start_time, const std::string& size_str, int size) {
    if (!(origin.row >= 0 && origin.row < size && origin.col >= 0 && origin.col < size)) return;
    bool is_wall = false;
    if (config.contains("walls")) {
        is_wall = std::any_of(config["walls"].begin(), config["walls"].end(),
            [&origin](const json& wall_j){ return wall_j.value("row", -1) == origin.row && wall_j.value("col", -1) == origin.col; });
    }
    if (is_wall) return;

    config["dynamic_events"].push_back({
        {"type", "fire"}, {"time_step", start_time},
        {"position", {{"row", origin.row}, {"col", origin.col}}},
        {"size", size_str}
    });
    // Fire spreading logic (optional, but good to keep)
    int spread_rate = 4;
    int max_spread = (size_str == "small") ? 1 : 2; // Keep spread minimal for this test
    for (int i = 1; i <= max_spread; ++i) {
        int current_time = start_time + i * spread_rate;
        Position p;
        p = {origin.row - i, origin.col};
        if (p.row >= 0) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", p.row}, {"col", p.col}}}, {"size", "small"}});
        p = {origin.row + i, origin.col};
        if (p.row < size) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", p.row}, {"col", p.col}}}, {"size", "small"}});
        p = {origin.row, origin.col - i};
        if (p.col >= 0) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", p.row}, {"col", p.col}}}, {"size", "small"}});
        p = {origin.row, origin.col + i};
        if (p.col < size) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", p.row}, {"col", p.col}}}, {"size", "small"}});
    }
};

// (createBlockedPath and createDynamicSmoke can be removed if not used, or kept)
auto createBlockedPath = [](json& config, Position pos, int time_step, int size) { /* ... (definition as before) ... */ };
auto createDynamicSmoke = [](json& config, Position pos, int time_step, const std::string& intensity = "light", int size = 0) { /* ... (definition as before) ... */ };
// --- End Helper Functions ---


json ScenarioGenerator::generate(int size, const std::string& name) {
    json config;
    config["name"] = name;
    config["rows"] = size;
    config["cols"] = size;

    std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, size - 1);

    Position start_pos = {1, 1}; // Fixed start
    config["start"] = {{"row", start_pos.row}, {"col", start_pos.col}};

    // --- Define Exits ---
    config["exits"] = json::array();
    Position primary_exit = {size - 2, size - 2}; // Main target exit

    // --- MODIFICATION: ONLY ONE EXIT ---
    if (primary_exit.row >= 0 && primary_exit.col >= 0 && !(primary_exit == start_pos)) {
        config["exits"].push_back({{"row", primary_exit.row}, {"col", primary_exit.col}});
    } else if (size > 1) { // Fallback for very small grids
         primary_exit = {size - 1, size - 1};
         if (!(primary_exit == start_pos)) {
            config["exits"].push_back({{"row", primary_exit.row}, {"col", primary_exit.col}});
         }
    }
    
    // Ensure at least one valid exit exists
    if (config["exits"].empty()) {
         if (size > 1) { // Add a default if {1,1} and {size-2, size-2} collided
             primary_exit = {size - 1, 0};
             config["exits"].push_back({{"row", primary_exit.row}, {"col", primary_exit.col}});
         } else {
             throw std::runtime_error("Cannot generate a valid scenario for grid size " + std::to_string(size));
         }
    }
    primary_exit = {config["exits"][0]["row"], config["exits"][0]["col"]};


    // --- Minimal Wall Placement ---
    // Place very few walls to keep the path predictable
    config["walls"] = json::array();
    int num_walls = (size * size) / 20; // Even fewer walls
    std::vector<Position> placed_walls; 
    int placed_count = 0;
    int max_tries = num_walls * 10;
    int tries = 0;

    Position panic_point = { (start_pos.row + primary_exit.row) / 2 , (start_pos.col + primary_exit.col) / 2 };
    Position fire_point = { panic_point.row, panic_point.col + 1 }; // Fire appears adjacent

    while(placed_count < num_walls && tries < max_tries) {
        tries++;
        int r = dist(rng);
        int c = dist(rng);
        Position p = {r, c};

        if (!(p.row >= 0 && p.row < size && p.col >= 0 && p.col < size)) continue;

        bool is_start = (p == start_pos);
        bool is_exit = std::any_of(config["exits"].begin(), config["exits"].end(),
            [&p](const json& exit_j){ return exit_j.value("row", -1) == p.row && exit_j.value("col", -1) == p.col; });
        bool already_placed = std::find(placed_walls.begin(), placed_walls.end(), p) != placed_walls.end();

        // Avoid start, exits, and the immediate panic test area
        bool near_panic_area = (std::abs(p.row - panic_point.row) <= 2 && std::abs(p.col - panic_point.col) <= 2);

        if (!is_start && !is_exit && !already_placed && !near_panic_area) {
            config["walls"].push_back({{"row", p.row}, {"col", p.col}});
            placed_walls.push_back(p);
            placed_count++;
        }
    }


    // --- No Initial Smoke ---
    config["smoke"] = json::array();

    // --- Dynamic Events ---
    config["dynamic_events"] = json::array();

    // Calculate time to reach the panic point (cell agent will be in)
    // Assumes a direct diagonal-ish path
    int time_to_panic_point = std::max(1, (panic_point.row - start_pos.row) + (panic_point.col - start_pos.col));

    // --- CREATE THE PANIC TRAP ---
    // Schedule the fire to appear at the 'fire_point'
    // at the exact time step the agent arrives at the adjacent 'panic_point'.
    createSpreadingFire(config, fire_point, time_to_panic_point, "small", size);

    // Add a couple of other minor dynamic events, far away from the trap
    Position random_smoke_pos = {size - 2, 1};
    if (random_smoke_pos.row >= 0 && random_smoke_pos.col >= 0) {
        createDynamicSmoke(config, random_smoke_pos, time_to_panic_point / 2, "light", size);
    }
    Position random_block_pos = {1, size - 2};
     if (random_block_pos.row >= 0 && random_block_pos.col >= 0) {
        createBlockedPath(config, random_block_pos, time_to_panic_point + 2, size);
     }

    return config;
}