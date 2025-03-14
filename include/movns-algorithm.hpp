#ifndef MOVNS_ALGORITHM_HPP
#define MOVNS_ALGORITHM_HPP

#include "movns-solution.hpp"
#include "movns-neighborhood.hpp"
#include "models.hpp"
#include <vector>
#include <memory>
#include <random>
#include <chrono>
#include <unordered_map>
#include <set>

namespace tourist {
namespace movns {

/**
 * @class MOVNS
 * @brief Implementation of Multi-Objective Variable Neighborhood Search algorithm
 */
class MOVNS {
public:
    /**
     * @brief Configuration structure for MOVNS algorithm
     */
    struct Parameters {
        // Constructor with default values
        Parameters() : max_iterations(1000), max_time_seconds(300), max_iterations_no_improvement(100) {}
        
        size_t max_iterations;         // Maximum number of iterations
        size_t max_time_seconds;       // Maximum execution time in seconds
        size_t max_iterations_no_improvement; // Maximum iterations without improvement
        
        // Parameter validation
        void validate() const;
    };
    
    /**
     * @brief MOVNS constructor
     * 
     * @param attractions List of available attractions
     * @param params Configuration parameters
     */
    MOVNS(const std::vector<Attraction>& attractions, Parameters params = Parameters());
    
    /**
     * @brief Run the MOVNS algorithm
     * 
     * @return std::vector<MOVNSSolution> Set of non-dominated solutions
     */
    std::vector<MOVNSSolution> run();
    
private:
    // Type alias for the approximation set
    using ApproximationSet = std::vector<MOVNSSolution>;
    
    // Exploration state for each solution
    struct SolutionExplorationState {
        std::set<std::string> explored_neighborhoods;
        bool fully_explored = false;
    };
    
    // Problem data
    const std::vector<Attraction>& attractions_;
    const Parameters params_;
    
    // Search state
    ApproximationSet p_approx_;
    std::vector<std::shared_ptr<Neighborhood>> neighborhoods_;
    std::unordered_map<size_t, SolutionExplorationState> exploration_state_;
    
    // Iteration history - updated to include neighborhood count
    std::vector<std::tuple<size_t, size_t, double, double, int, int>> iteration_history_;
    
    // Random number generator
    mutable std::mt19937 rng_{std::random_device{}()};
    
    // Helper methods
    void initializeApproximationSet();
    bool updateApproximationSet(const MOVNSSolution& solution);
    MOVNSSolution selectSolutionForExploration();
    bool isFullyExplored(const MOVNSSolution& solution);
    void markNeighborhoodAsExplored(const MOVNSSolution& solution, const Neighborhood& neighborhood);
    std::vector<MOVNSSolution> sortSolutions(const ApproximationSet& solutions) const;
    void logProgress(size_t iteration, size_t iterations_no_improvement) const;
    
    // Local search
    MOVNSSolution localSearch(MOVNSSolution solution);
};

} // namespace movns
} // namespace tourist

#endif // MOVNS_ALGORITHM_HPP