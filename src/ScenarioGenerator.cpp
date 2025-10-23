#include "enmod/ScenarioGenerator.h"
#include "enmod/Types.h"
#include "enmod/json.hpp"
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <vector>
#include <set>

using json = nlohmann::json;

// --- Helper function Definitions ---
// *** Using the SPREADING versions of the helpers that return bool ***
auto createSpreadingFire = [](json& config, Position origin, int start_time, const std::string& size_str, int grid_size) -> bool { // Return bool
    if (!(origin.row >= 0 && origin.row < grid_size && origin.col >= 0 && origin.col < grid_size)) return false;
    bool is_wall = false;
    if (config.contains("walls")) {
        is_wall = std::any_of(config["walls"].begin(), config["walls"].end(),
            [&origin](const json& wall_j){ return wall_j.value("row", -1) == origin.row && wall_j.value("col", -1) == origin.col; });
    }
     bool is_start = (config["start"]["row"] == origin.row && config["start"]["col"] == origin.col);
    bool is_exit = false;
     if (config.contains("exits")) {
         is_exit = std::any_of(config["exits"].begin(), config["exits"].end(),
            [&origin](const json& exit_j){ return exit_j.value("row", -1) == origin.row && exit_j.value("col", -1) == origin.col; });
     }
    if (is_wall || is_start || is_exit) return false;

    config["dynamic_events"].push_back({
        {"type", "fire"}, {"time_step", start_time},
        {"position", {{"row", origin.row}, {"col", origin.col}}},
        {"size", size_str}
    });

    int base_spread_rate = 4;
    int max_spread = std::max(1, grid_size / 10);
    std::set<Position> fire_locations;
    fire_locations.insert(origin);
    std::vector<Position> current_front = {origin};

    for (int spread_step = 1; spread_step <= max_spread; ++spread_step) {
        int current_time = start_time + spread_step * base_spread_rate;
        std::vector<Position> next_front;
        for (const auto& current_fire_pos : current_front) {
            int dr[] = {-1, -1, -1,  0,  0,  1,  1,  1};
            int dc[] = {-1,  0,  1, -1,  1, -1,  0,  1};
            for(int i = 0; i < 8; ++i) {
                Position p = {current_fire_pos.row + dr[i], current_fire_pos.col + dc[i]};
                if (p.row >= 0 && p.row < grid_size && p.col >= 0 && p.col < grid_size &&
                    fire_locations.find(p) == fire_locations.end()) {
                    bool p_is_wall = false;
                    if (config.contains("walls")) {
                        p_is_wall = std::any_of(config["walls"].begin(), config["walls"].end(),
                            [&p](const json& wall_j){ return wall_j.value("row", -1) == p.row && wall_j.value("col", -1) == p.col; });
                    }
                     bool p_is_start = (config["start"]["row"] == p.row && config["start"]["col"] == p.col);
                     bool p_is_exit = false;
                     if (config.contains("exits")) {
                         p_is_exit = std::any_of(config["exits"].begin(), config["exits"].end(),
                            [&p](const json& exit_j){ return exit_j.value("row", -1) == p.row && exit_j.value("col", -1) == p.col; });
                     }
                    if (!p_is_wall && !p_is_start && !p_is_exit) {
                        config["dynamic_events"].push_back({
                            {"type", "fire"}, {"time_step", current_time},
                            {"position", {{"row", p.row}, {"col", p.col}}}, {"size", "small"}
                        });
                        fire_locations.insert(p);
                        next_front.push_back(p);
                    }
                }
            }
        }
        current_front = std::move(next_front);
        if (current_front.empty()) break;
    }
    return true; // Indicate success
};

auto createDynamicSmoke = [](json& config, Position origin, int start_time, const std::string& intensity = "light", int grid_size = 0) -> bool { // Return bool
     if (!(origin.row >= 0 && origin.row < grid_size && origin.col >= 0 && origin.col < grid_size)) return false;
    bool is_wall = false;
    if (config.contains("walls")) {
        is_wall = std::any_of(config["walls"].begin(), config["walls"].end(),
            [&origin](const json& wall_j){ return wall_j.value("row", -1) == origin.row && wall_j.value("col", -1) == origin.col; });
    }
    bool is_start = (config["start"]["row"] == origin.row && config["start"]["col"] == origin.col);
    bool is_exit = false;
     if (config.contains("exits")) {
         is_exit = std::any_of(config["exits"].begin(), config["exits"].end(),
            [&origin](const json& exit_j){ return exit_j.value("row", -1) == origin.row && exit_j.value("col", -1) == origin.col; });
     }
    if (is_wall || is_start || is_exit) return false;

    config["dynamic_events"].push_back({
        {"type", "smoke"}, {"time_step", start_time},
        {"position", {{"row", origin.row}, {"col", origin.col}}},
        {"intensity", intensity}
    });

    int spread_rate = 5;
    int max_spread = std::max(1, grid_size / 15);
    std::set<Position> smoke_locations;
    smoke_locations.insert(origin);
    std::vector<Position> current_front = {origin};

    for (int spread_step = 1; spread_step <= max_spread; ++spread_step) {
        int current_time = start_time + spread_step * spread_rate;
        std::vector<Position> next_front;
        std::string current_intensity = (spread_step <= max_spread / 2 && intensity == "heavy") ? "heavy" : "light";
        for (const auto& current_smoke_pos : current_front) {
            int dr[] = {-1, 1, 0, 0};
            int dc[] = {0, 0, -1, 1};
            for(int i = 0; i < 4; ++i) {
                 Position p = {current_smoke_pos.row + dr[i], current_smoke_pos.col + dc[i]};
                 if (p.row >= 0 && p.row < grid_size && p.col >= 0 && p.col < grid_size &&
                     smoke_locations.find(p) == smoke_locations.end()) {
                    bool p_is_wall = false;
                    if (config.contains("walls")) {
                        p_is_wall = std::any_of(config["walls"].begin(), config["walls"].end(),
                            [&p](const json& wall_j){ return wall_j.value("row", -1) == p.row && wall_j.value("col", -1) == p.col; });
                    }
                     bool p_is_start = (config["start"]["row"] == p.row && config["start"]["col"] == p.col);
                     bool p_is_exit = false;
                     if (config.contains("exits")) {
                         p_is_exit = std::any_of(config["exits"].begin(), config["exits"].end(),
                            [&p](const json& exit_j){ return exit_j.value("row", -1) == p.row && exit_j.value("col", -1) == p.col; });
                     }
                    if (!p_is_wall && !p_is_start && !p_is_exit) {
                        config["dynamic_events"].push_back({
                            {"type", "smoke"}, {"time_step", current_time},
                            {"position", {{"row", p.row}, {"col", p.col}}}, {"intensity", current_intensity}
                        });
                        smoke_locations.insert(p);
                        next_front.push_back(p);
                    }
                 }
            }
        }
         current_front = std::move(next_front);
         if (current_front.empty()) break;
    }
     return true; // Indicate success
};

auto createBlockedPathEvent = [](json& config, Position pos, int time_step) -> bool { // Return bool
     bool is_wall = false;
     if (config.contains("walls")) {
        is_wall = std::any_of(config["walls"].begin(), config["walls"].end(),
            [&pos](const json& wall_j){ return wall_j.value("row", -1) == pos.row && wall_j.value("col", -1) == pos.col; });
     }
      bool is_start = (config["start"]["row"] == pos.row && config["start"]["col"] == pos.col);
     bool is_exit = false;
     if (config.contains("exits")) {
         is_exit = std::any_of(config["exits"].begin(), config["exits"].end(),
            [&pos](const json& exit_j){ return exit_j.value("row", -1) == pos.row && exit_j.value("col", -1) == pos.col; });
     }
     if (!is_wall && !is_start && !is_exit) {
        config["dynamic_events"].push_back({
            {"type", "path_block"}, {"time_step", time_step},
            {"position", {{"row", pos.row}, {"col", pos.col}}}
        });
        return true; // Indicate success
     }
     return false; // Indicate failure
};
// --- End Helper Functions ---


json ScenarioGenerator::generate(int size, const std::string& name) {
    json config;
    config["name"] = name;
    config["rows"] = size;
    config["cols"] = size;

    std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist_pos(0, size - 1);
    // Use random time only for optional minor hazards
    int max_time = std::max(10, size * size / 5);
    int min_later_time = std::max(5, size / 2);
    std::uniform_int_distribution<int> dist_later_time(min_later_time, max_time);


    Position start_pos = {1, 1};
    config["start"] = {{"row", start_pos.row}, {"col", start_pos.col}};

    // --- Define Exits (Single Exit) ---
    config["exits"] = json::array();
    Position primary_exit = {size - 2, size - 2};
    // ... (rest of exit placement logic) ...
     if (primary_exit.row >= 0 && primary_exit.col >= 0 && !(primary_exit == start_pos)) {
        config["exits"].push_back({{"row", primary_exit.row}, {"col", primary_exit.col}});
    } else if (size > 1) {
         primary_exit = {size - 1, size - 1};
         if (!(primary_exit == start_pos)) {
            config["exits"].push_back({{"row", primary_exit.row}, {"col", primary_exit.col}});
         }
    }
    if (config["exits"].empty()) {
         if (size > 1) {
             primary_exit = {size - 1, 0};
             config["exits"].push_back({{"row", primary_exit.row}, {"col", primary_exit.col}});
         } else {
             throw std::runtime_error("Cannot generate a valid scenario for grid size " + std::to_string(size));
         }
    }
    primary_exit = {config["exits"][0]["row"], config["exits"][0]["col"]};


    // --- Minimal Wall Placement (Avoiding panic area) ---
    config["walls"] = json::array();
    int num_walls = (size * size) / 20; // Keep wall density low
    std::set<Position> placed_locations; // Track walls, start, exit
    placed_locations.insert(start_pos);
    placed_locations.insert(primary_exit);

    // *** Calculate panic point area to keep clear of walls ***
    Position panic_point = { (start_pos.row + primary_exit.row) / 2 , (start_pos.col + primary_exit.col) / 2 };
    // Define the fire point relative to the panic point (e.g., adjacent or at panic point)
    Position fire_point = { panic_point.row, panic_point.col + 1 }; // Adjacent again

    int placed_count = 0;
    int max_tries = (num_walls + size) * 5;
    int tries = 0;
    while(placed_count < num_walls && tries < max_tries) {
        tries++;
        int r = dist_pos(rng);
        int c = dist_pos(rng);
        Position p = {r, c};

        // Avoid start, exit, and the immediate area around the panic_point and fire_point
        bool near_panic_area = (std::abs(p.row - panic_point.row) <= 1 && std::abs(p.col - panic_point.col) <= 1) ||
                               (std::abs(p.row - fire_point.row) <= 1 && std::abs(p.col - fire_point.col) <= 1);

        if (placed_locations.find(p) == placed_locations.end() && !near_panic_area) {
            config["walls"].push_back({{"row", p.row}, {"col", p.col}});
            placed_locations.insert(p);
            placed_count++;
        }
    }


    // --- No Initial Smoke ---
    config["smoke"] = json::array();

    // --- Dynamic Events ---
    config["dynamic_events"] = json::array();

    // *** Restore Panic Trap Logic ***
    // Calculate time to reach the panic point
    int time_to_panic_point = std::max(1, (panic_point.row - start_pos.row) + (panic_point.col - start_pos.col));

    // Trigger fire slightly earlier
    int fire_trigger_time = std::max(1, time_to_panic_point - 1);

    // --- CREATE THE SPREADING PANIC TRAP ---
    std::string fire_size = "medium"; // Use medium or large for wider initial impact
    if (createSpreadingFire(config, fire_point, fire_trigger_time, fire_size, size)) {
         placed_locations.insert(fire_point); // Mark location
    } else {
         // Fallback maybe? Or just accept fire might fail to place if panic point is near edge/wall
         // For simplicity, we'll assume it places successfully most of the time
    }

    // --- Optional: Add spreading smoke slightly before the fire along the path ---
    Position smoke_trigger_pos = { (start_pos.row + panic_point.row) / 2, (start_pos.col + panic_point.col) / 2}; // Roughly halfway to panic point
    int smoke_trigger_time = std::max(1, fire_trigger_time / 2); // Trigger smoke earlier
    if (placed_locations.find(smoke_trigger_pos) == placed_locations.end()) {
        if (createDynamicSmoke(config, smoke_trigger_pos, smoke_trigger_time, "heavy", size)) {
            placed_locations.insert(smoke_trigger_pos);
        }
    }


    // --- Optional: Add a few additional, LATER, minor hazards (non-spreading or small spread) ---
    int num_later_hazards = std::max(0, size / 20); // Even fewer extras
    int later_hazards_placed = 0;
    tries = 0;
    max_tries = (num_later_hazards + size) * 5;

    while(later_hazards_placed < num_later_hazards && tries < max_tries) {
         tries++;
         int r = dist_pos(rng);
         int c = dist_pos(rng);
         Position p = {r, c};
         // Ensure it's not near the main fire event or start/exit
         bool too_close_main_fire = (std::abs(p.row - fire_point.row) <= size/5 && std::abs(p.col - fire_point.col) <= size/5);
         if (placed_locations.find(p) == placed_locations.end() && !too_close_main_fire) {
             int time = dist_later_time(rng);
             bool event_added = false;
             // Add only non-spreading events or very small fires later
             if (rng() % 3 == 0) {
                  event_added = createBlockedPathEvent(config, p, time);
             } else {
                 event_added = createDynamicSmoke(config, p, time, "light", 0); // size 0 -> no spread
             }
             if (event_added) {
                placed_locations.insert(p);
                later_hazards_placed++;
             }
         }
    }

    return config;
}