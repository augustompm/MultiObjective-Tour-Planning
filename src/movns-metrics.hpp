// File: movns-metrics.hpp

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
     * @brief Exporta as soluções para um arquivo CSV
     * 
     * @param solutions Conjunto de soluções a exportar
     * @param filename Nome do arquivo
     * @param iteration_history Histórico de iterações (opcional)
     */
    static void exportToCSV(
        const std::vector<MOVNSSolution>& solutions,
        const std::string& filename,
        const std::vector<std::tuple<size_t, size_t, double, double, int>>& iteration_history = {});
    
    // New methods
    /**
     * @brief Elimina soluções duplicadas e inválidas do conjunto de aproximação
     * 
     * Baseado no conceito de exploração eficiente do espaço de soluções
     * (Mladenović e Hansen, 1997)
     * 
     * @param solutions Conjunto de soluções
     * @return std::vector<MOVNSSolution> Conjunto de soluções sem duplicatas
     */
    static std::vector<MOVNSSolution> filterDuplicatesAndInvalid(
        const std::vector<MOVNSSolution>& solutions);
    
    /**
     * @brief Aplica ε-dominância para reduzir o tamanho da fronteira Pareto
     * 
     * Baseado no trabalho de Zitzler et al. (2003) "Performance Assessment of Multiobjective Optimizers: 
     * An Analysis and Review"
     * 
     * @param solutions Conjunto de soluções
     * @param epsilon Parâmetro de granularidade para cada objetivo
     * @return std::vector<MOVNSSolution> Conjunto reduzido de soluções
     */
    static std::vector<MOVNSSolution> applyEpsilonDominance(
        const std::vector<MOVNSSolution>& solutions,
        const std::vector<double>& epsilon);
    
    /**
     * @brief Exporta o histórico de gerações para um arquivo CSV
     * 
     * @param iteration_history Histórico de iterações
     * @param filename Nome do arquivo
     */
    static void exportGenerationHistory(
        const std::vector<std::tuple<size_t, size_t, double, double, int>>& iteration_history,
        const std::string& filename);
};

} // namespace movns
} // namespace tourist

#endif // MOVNS_METRICS_HPP