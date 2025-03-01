#include "utils.hpp"
#include "models.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace tourist {
namespace utils {

// Implementação das funções métricas que usam tipos completos

double calculateHypervolume(const std::vector<Solution>& solutions,
                           const std::vector<double>& reference_point) {
    if (solutions.empty()) return 0.0;
    
    double volume = 1.0;
    size_t num_objectives = solutions[0].getObjectives().size();
    
    for (size_t i = 0; i < num_objectives; ++i) {
        double min_val = reference_point[i];
        for (const auto& sol : solutions) {
            min_val = std::min(min_val, sol.getObjectives()[i]);
        }
        volume *= (reference_point[i] - min_val);
    }
    
    return volume;
}

double calculateSpread(const std::vector<Solution>& solutions) {
    if (solutions.size() < 2) return 0.0;
    
    std::vector<double> distances;
    distances.reserve(solutions.size() - 1);
    
    for (size_t i = 0; i < solutions.size() - 1; ++i) {
        double dist = 0.0;
        const auto& obj1 = solutions[i].getObjectives();
        const auto& obj2 = solutions[i+1].getObjectives();
        
        for (size_t j = 0; j < obj1.size(); ++j) {
            dist += std::pow(obj1[j] - obj2[j], 2);
        }
        distances.push_back(std::sqrt(dist));
    }
    
    double avg_dist = std::accumulate(distances.begin(), distances.end(), 0.0) / distances.size();
    double spread = 0.0;
    
    for (double dist : distances) {
        spread += std::abs(dist - avg_dist);
    }
    
    return spread / (distances.size() * avg_dist);
}

double calculateCoverage(const std::vector<Solution>& solutions1,
                        const std::vector<Solution>& solutions2) {
    if (solutions2.empty()) return 1.0;
    if (solutions1.empty()) return 0.0;
    
    int dominated_count = 0;
    for (const auto& sol2 : solutions2) {
        for (const auto& sol1 : solutions1) {
            if (sol1.dominates(sol2)) {
                dominated_count++;
                break;
            }
        }
    }
    
    return static_cast<double>(dominated_count) / solutions2.size();
}

// Implementação dos métodos proxy da classe Metrics

double Metrics::calculateHypervolume(const std::vector<Solution>& solutions, 
                                   const std::vector<double>& reference_point) {
    return tourist::utils::calculateHypervolume(solutions, reference_point);
}

double Metrics::calculateSpread(const std::vector<Solution>& solutions) {
    return tourist::utils::calculateSpread(solutions);
}

double Metrics::calculateCoverage(const std::vector<Solution>& solutions1,
                                const std::vector<Solution>& solutions2) {
    return tourist::utils::calculateCoverage(solutions1, solutions2);
}

} // namespace utils
} // namespace tourist