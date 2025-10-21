#include "enmod/ScenarioGenerator.h"
#include "enmod/Types.h"
#include "enmod/json.hpp"
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>

using json = nlohmann::json;

json ScenarioGenerator::generate(int size, const std::string& name) {
    json config;
    config["name"] = name;
    config["rows"] = size;
    config["cols"] = size;

    std::mt19937 rng(static_cast<unsigned int>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, size - 1);
    
    Position start_pos = {1, 1};
    config["start"] = {{"row", start_pos.row}, {"col", start_pos.col}};

    config["exits"] = json::array();
    Position primary_exit = {size - 2, size - 2};
    config["exits"].push_back({{"row", primary_exit.row}, {"col", primary_exit.col}});
    if (size > 10) config["exits"].push_back({{"row", 0}, {"col", size / 2}});
    if (size > 20) config["exits"].push_back({{"row", size / 2}, {"col", 0}});

    config["walls"] = json::array();
    int num_walls = (size * size) / 5;
    for (int i = 0; i < num_walls; ++i) {
        int r = dist(rng);
        int c = dist(rng);
        Position p = {r, c};
        bool is_start = (p == start_pos);
        bool is_exit = std::any_of(config["exits"].begin(), config["exits"].end(), 
            [&p](const json& exit_j){ return exit_j["row"] == p.row && exit_j["col"] == p.col; });
        if (!is_start && !is_exit) {
            config["walls"].push_back({{"row", r}, {"col", c}});
        }
    }

    config["smoke"] = json::array();
    int num_smoke = (size * size) / 20;
     for (int i = 0; i < num_smoke; ++i) {
        int r = dist(rng);
        int c = dist(rng);
        Position p = {r, c};
        bool is_start = (p == start_pos);
        bool is_exit = std::any_of(config["exits"].begin(), config["exits"].end(), 
            [&p](const json& exit_j){ return exit_j["row"] == p.row && exit_j["col"] == p.col; });
        if (!is_start && !is_exit) {
            std::string intensity = (rng() % 5 == 0) ? "heavy" : "light";
            config["smoke"].push_back({{"row", r}, {"col", c}, {"intensity", intensity}});
        }
    }

    config["dynamic_events"] = json::array();
    
    // --- Helper function to create a spreading fire ---
    auto createSpreadingFire = [&](Position origin, int start_time, const std::string& size_str) {
        config["dynamic_events"].push_back({
            {"type", "fire"}, {"time_step", start_time},
            {"position", {{"row", origin.row}, {"col", origin.col}}},
            {"size", size_str}
        });

        int spread_rate = 4; // Spreads every 4 time steps
        int max_spread = (size_str == "medium") ? 2 : ((size_str == "large") ? 3 : 1);

        for (int i = 1; i <= max_spread; ++i) {
            int current_time = start_time + i * spread_rate;
            if (origin.row - i >= 0) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", origin.row - i}, {"col", origin.col}}}, {"size", "small"}});
            if (origin.row + i < size) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", origin.row + i}, {"col", origin.col}}}, {"size", "small"}});
            if (origin.col - i >= 0) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", origin.row}, {"col", origin.col - i}}}, {"size", "small"}});
            if (origin.col + i < size) config["dynamic_events"].push_back({{"type", "fire"}, {"time_step", current_time}, {"position", {{"row", origin.row}, {"col", origin.col + i}}}, {"size", "small"}});
        }
    };

    // --- Helper function to create a blocked path ---
    auto createBlockedPath = [&](Position pos, int time_step) {
        config["dynamic_events"].push_back({
            {"type", "path_block"},
            {"time_step", time_step},
            {"position", {{"row", pos.row}, {"col", pos.col}}}
        });
    };

    int path_dist1 = std::abs(start_pos.row - primary_exit.row) + std::abs(start_pos.col - primary_exit.col);

    // --- Create Spreading Fire Events ---
    Position fire_pos1 = {start_pos.row + (path_dist1 / 2), start_pos.col + (path_dist1 / 2)};
    createSpreadingFire(fire_pos1, path_dist1 / 3, "medium");

    if (size > 12) {
        Position fire_pos2 = {size - 3, 3};
        createSpreadingFire(fire_pos2, path_dist1 / 2, "small");
    }

    // --- Create Path Blockage Events ---
    if (size > 10) {
        Position block_pos1 = { start_pos.row + (path_dist1 / 4), start_pos.col + (path_dist1 / 4) };
        createBlockedPath(block_pos1, path_dist1 / 2);
    }

    if (size > 20) {
        Position block_pos2 = {size - 4, 4};
        createBlockedPath(block_pos2, path_dist1);
    }

    return config;
}