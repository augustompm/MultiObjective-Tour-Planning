// File: include/base.hpp

#pragma once

#include <vector>
#include <cmath> // Adicione esta linha para resolver o erro de std::abs

namespace tourist {

// Forward declarations
class Solution;

// Base class for all solution representations
class SolutionBase {
public:
    virtual ~SolutionBase() = default;
    
    // Get the objective values of this solution
    virtual std::vector<double> getObjectives() const = 0;
    
    // Check if this solution dominates another solution (in Pareto sense)
    virtual bool dominates(const SolutionBase& other) const = 0;

    // Utility method to compare solution objectives with tolerance
    bool isSimilarTo(const SolutionBase& other, double tolerance = 1e-6) const {
        const auto& self_objectives = getObjectives();
        const auto& other_objectives = other.getObjectives();
        
        if (self_objectives.size() != other_objectives.size()) {
            return false;
        }
        
        for (size_t i = 0; i < self_objectives.size(); ++i) {
            if (std::abs(self_objectives[i] - other_objectives[i]) > tolerance) {
                return false;
            }
        }
        
        return true;
    }    
    
    // Utility method to check if this solution is dominated by a set of objective values
    bool isDominatedBy(const std::vector<double>& other_objectives) const {
        const auto& self_objectives = getObjectives();
        
        // Check if all objectives are at least as good and at least one is better
        bool at_least_one_better = false;
        bool all_at_least_as_good = true;
        
        for (size_t i = 0; i < self_objectives.size(); ++i) {
            if (self_objectives[i] > other_objectives[i]) {
                all_at_least_as_good = false;
                break;
            }
            if (self_objectives[i] < other_objectives[i]) {
                at_least_one_better = true;
            }
        }
        
        return all_at_least_as_good && at_least_one_better;
    }
};

// Base class for all evolutionary algorithms
class EvolutionaryAlgorithm {
public:
    virtual ~EvolutionaryAlgorithm() = default;
    
    // Execute the algorithm and return the set of non-dominated solutions
    virtual std::vector<Solution> run() = 0;

protected:
    // Common methods for all evolutionary algorithms can be added here
};

} // namespace tourist