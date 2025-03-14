// File: movns-metrics.hpp

#pragma once

#include "movns-solution.hpp"
#include <vector>

namespace tourist {
namespace movns {

/**
 * @class Metrics
 * @brief Métricas para avaliação e comparação de conjuntos de soluções
 */
class Metrics {
public:
    /**
     * @brief Calcula o hipervolume de um conjunto de soluções
     * 
     * @param solutions Conjunto de soluções
     * @param reference_point Ponto de referência para cálculo do hipervolume
     * @return double Valor do hipervolume
     */
    static double calculateHypervolume(
        const std::vector<MOVNSSolution>& solutions,
        const std::vector<double>& reference_point);
    
    /**
     * @brief Calcula a métrica de cobertura binária entre dois conjuntos
     * 
     * @param solutions1 Primeiro conjunto de soluções
     * @param solutions2 Segundo conjunto de soluções
     * @return double Proporção de soluções em solutions2 dominadas por soluções em solutions1
     */
    static double calculateBinaryCoverage(
        const std::vector<MOVNSSolution>& solutions1,
        const std::vector<MOVNSSolution>& solutions2);
    
    /**
     * @brief Converte soluções MOVNS para o formato utilizado no NSGA-II
     * 
     * @param solutions Conjunto de soluções MOVNS
     * @return std::vector<Solution> Soluções no formato NSGA-II
     */
    static std::vector<Solution> convertToNSGA2Format(
        const std::vector<MOVNSSolution>& solutions);
    
    /**
     * @brief Exporta os resultados para um arquivo CSV
     * 
     * @param solutions Conjunto de soluções
     * @param filename Nome do arquivo de saída
     */
    static void exportToCSV(
        const std::vector<MOVNSSolution>& solutions,
        const std::string& filename);
};

} // namespace movns
} // namespace tourist