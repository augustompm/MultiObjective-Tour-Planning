// File: include/nsga2-base.hpp
// Implementation of NSGA-II algorithm based on:
// "A Fast and Elitist Multiobjective Genetic Algorithm: NSGA-II" by Deb et al., 2002

#pragma once

#include "base.hpp"
#include "models.hpp"
#include <vector>
#include <memory>
#include <random>
#include <functional>
#include <algorithm>
#include <limits>

namespace tourist {

// Forward declarations
class Solution;
class Route;

class NSGA2Base final : public EvolutionaryAlgorithm {
public:
    // Configuration parameters for NSGA-II
    struct Parameters {
        size_t population_size;     // Size of population
        size_t max_generations;     // Maximum number of generations
        double crossover_rate;      // Probability of crossover
        double mutation_rate;       // Probability of mutation

        // Default constructor with reasonable values
        Parameters()
            : population_size(100)
            , max_generations(100)
            , crossover_rate(0.9)
            , mutation_rate(0.1) {}

        // Constructor with custom values
        Parameters(size_t pop_size, size_t max_gen, double cross_rate, double mut_rate)
            : population_size(pop_size)
            , max_generations(max_gen)
            , crossover_rate(cross_rate)
            , mutation_rate(mut_rate) {}
        
        // Validate parameters
        void validate() const;
    };

protected:
    // Individual representation for NSGA-II
    class Individual {
    public:
        // Construct an individual from a sequence of attraction indices
        explicit Individual(std::vector<int> attraction_indices);
        
        // Evaluate this individual against all objectives
        void evaluate(const NSGA2Base& algorithm);
        
        // Check if this individual dominates another
        bool dominates(const Individual& other) const;
        
        // Create a Route from this individual
        Route constructRoute(const NSGA2Base& algorithm) const;
        
        // Getters and setters
        int getRank() const { return rank_; }
        double getCrowdingDistance() const { return crowding_distance_; }
        const std::vector<double>& getObjectives() const { return objectives_; }
        const std::vector<int>& getChromosome() const { return chromosome_; }
        const std::vector<utils::TransportMode>& getTransportModes() const { return transport_modes_; }
        
        void setRank(int rank) { rank_ = rank; }
        void setCrowdingDistance(double distance) { crowding_distance_ = distance; }
        
    private:
        std::vector<int> chromosome_;                       // Indices of attractions in visit order
        std::vector<utils::TransportMode> transport_modes_; // Transport mode between attractions
        std::vector<double> objectives_;                    // Objective values [cost, time, -attractions]
        int rank_{0};                                      // Non-domination rank (lower is better)
        double crowding_distance_{0.0};                     // Crowding distance for diversity
        
        // Calculate optimal transport modes
        void determineTransportModes(const NSGA2Base& algorithm);
        
        friend class NSGA2Base;  // Allow NSGA2Base to access private members
    };

public:
    // Constructor
    NSGA2Base(const std::vector<Attraction>& attractions, Parameters params = Parameters());
    
    // Prevent copying and moving
    NSGA2Base(const NSGA2Base&) = delete;
    NSGA2Base& operator=(const NSGA2Base&) = delete;
    NSGA2Base(NSGA2Base&&) = delete;
    NSGA2Base& operator=(NSGA2Base&&) = delete;
    
    // Run the algorithm and return non-dominated solutions
    std::vector<Solution> run() override;

private:
    using IndividualPtr = std::shared_ptr<Individual>;
    using Population = std::vector<IndividualPtr>;
    using Front = std::vector<IndividualPtr>;

    // Core NSGA-II methods (following the paper exactly)
    void initializePopulation();
    void evaluatePopulation(Population& pop);
    
    // Fast non-dominated sorting approach (Section III-A)
    std::vector<Front> fastNonDominatedSort(const Population& pop) const;
    
    // Crowding distance assignment (Section III-B)
    void calculateCrowdingDistances(Front& front) const;
    
    // Main loop selection method (Section III-C)
    Population selectNextGeneration(const Population& parents, const Population& offspring);
    
    // Genetic operators
    Population createOffspring(const Population& parents);
    IndividualPtr tournamentSelection(const Population& pop);
    IndividualPtr crossover(const IndividualPtr& parent1, const IndividualPtr& parent2);
    void mutate(IndividualPtr individual);
    
    // Utility methods
    void logProgress(size_t generation, const std::vector<Front>& fronts) const;
    void createAttractionMapping();
    
    // Crowded comparison operator (Section III-B)
    static bool compareByRankAndCrowding(const IndividualPtr& a, const IndividualPtr& b);
    
    // Problem data
    const std::vector<Attraction>& attractions_;
    const Parameters params_;
    Population population_;
    mutable std::mt19937 rng_{std::random_device{}()};
};

} // namespace tourist