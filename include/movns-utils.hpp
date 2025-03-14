// File: include/movns-utils.hpp

#pragma once

#include "movns-solution.hpp"
#include "models.hpp"
#include <vector>
#include <string>
#include <random>

namespace tourist {
namespace movns {

/**
 * @class Utils
 * @brief Utility functions for the MOVNS algorithm
 */
class Utils {
public:
    /**
     * @brief Generates a random initial solution
     * 
     * @param attractions Set of available attractions
     * @param max_attractions Maximum number of attractions to include
     * @return MOVNSSolution Generated initial solution
     */
    static MOVNSSolution generateRandomSolution(
        const std::vector<Attraction>& attractions,
        size_t max_attractions = 8);
    
    /**
     * @brief Checks if a solution is valid (respects all constraints)
     * 
     * @param solution Solution to check
     * @return true If the solution is valid
     * @return false Otherwise
     */
    static bool isValidSolution(const MOVNSSolution& solution);
    
    /**
     * @brief Checks if a transport mode is viable between two attractions
     * 
     * @param from Origin attraction
     * @param to Destination attraction
     * @param mode Transport mode to check
     * @return true If the mode is viable
     * @return false Otherwise
     */
    static bool isViableTransportMode(
        const Attraction& from,
        const Attraction& to,
        utils::TransportMode mode);
    
    /**
     * @brief Finds an attraction by name
     * 
     * @param attractions List of attractions
     * @param name Name of the attraction to find
     * @return const Attraction* Pointer to the found attraction or nullptr
     */
    static const Attraction* findAttractionByName(
        const std::vector<Attraction>& attractions,
        const std::string& name);
    
    /**
     * @brief Selects a random available attraction not included in the current solution
     * 
     * @param all_attractions All available attractions
     * @param current_solution Current solution
     * @param rng Random number generator
     * @return const Attraction* Selected attraction or nullptr if none available
     */
    static const Attraction* selectRandomAvailableAttraction(
        const std::vector<Attraction>& all_attractions,
        const MOVNSSolution& current_solution,
        std::mt19937& rng);
};

} // namespace movns
} // namespace touristc