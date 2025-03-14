#ifndef MOVNS_METRICS_HPP
#define MOVNS_METRICS_HPP

#include "movns-solution.hpp"
#include <vector>
#include <string>
#include <tuple>

namespace tourist {
namespace movns {

class Metrics {
public:
    static double calculateHypervolume(
        const std::vector<MOVNSSolution>& solutions,
        const std::vector<double>& reference_point);
    
    static double calculateBinaryCoverage(
        const std::vector<MOVNSSolution>& solutions1,
        const std::vector<MOVNSSolution>& solutions2);
    
    static std::vector<Solution> convertToNSGA2Format(
        const std::vector<MOVNSSolution>& solutions);
    
    /**
     * @brief Exports solutions to a CSV file
     * 
     * @param solutions Set of solutions to export
     * @param filename Output file name
     * @param iteration_history Iteration history (optional)
     */
    static void exportToCSV(
        const std::vector<MOVNSSolution>& solutions,
        const std::string& filename,
        const std::vector<std::tuple<size_t, size_t, double, double, int, int>>& iteration_history = {});
    
    /**
     * @brief Removes duplicate and invalid solutions from the approximation set
     * 
     * Based on the concept of efficient solution space exploration
     * (Mladenović and Hansen, 1997)
     * 
     * @param solutions Set of solutions
     * @return std::vector<MOVNSSolution> Set of solutions without duplicates
     */
    static std::vector<MOVNSSolution> filterDuplicatesAndInvalid(
        const std::vector<MOVNSSolution>& solutions);
    
    /**
     * @brief Applies ε-dominance to reduce the size of the Pareto front
     * 
     * Based on Zitzler et al. (2003) "Performance Assessment of Multiobjective Optimizers: 
     * An Analysis and Review"
     * 
     * @param solutions Set of solutions
     * @param epsilon Granularity parameter for each objective
     * @return std::vector<MOVNSSolution> Reduced set of solutions
     */
    static std::vector<MOVNSSolution> applyEpsilonDominance(
        const std::vector<MOVNSSolution>& solutions,
        const std::vector<double>& epsilon);
    
    /**
     * @brief Exports generation history to a CSV file
     * 
     * @param iteration_history Iteration history
     * @param filename Output file name
     */
    static void exportGenerationHistory(
        const std::vector<std::tuple<size_t, size_t, double, double, int, int>>& iteration_history,
        const std::string& filename);
};

} // namespace movns
} // namespace tourist

#endif // MOVNS_METRICS_HPP