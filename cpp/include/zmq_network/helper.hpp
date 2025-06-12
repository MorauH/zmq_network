#ifndef HELPER_HPP
#define HELPER_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

namespace Helper {

/**
 * Reads and parses a JSON configuration file
 * @param file_path Path to the configuration file
 * @return JSON object containing the parsed configuration, or empty object on error
 */
inline nlohmann::json readConfig() {
    
    const char* env_path = std::getenv("NODE_CONFIG_PATH");
    std::string file_path = (env_path != nullptr) ? std::string(env_path) : "config.json";

    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << file_path << std::endl;
        return {};
    }
    
    nlohmann::json config;
    try {
        file >> config;
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return {};
    }
    return config;
}

/**
 * Check if an array of strings contains a match
 */
inline bool contains_string(const std::vector<std::string>& array, const std::string& target) {
    return std::find(array.begin(), array.end(), target) != array.end();
}

} // namespace Helper

#endif // HELPER_HPP