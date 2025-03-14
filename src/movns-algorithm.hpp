// File: movns-algorithm.hpp

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
 * @brief Implementação do algoritmo Multi-Objective Variable Neighborhood Search
 */
class MOVNS {
public:
    /**
     * @brief Estrutura de configuração para o algoritmo MOVNS
     */
    struct Parameters {
        // Constructor com valores padrão
        Parameters() : max_iterations(1000), max_time_seconds(300), max_iterations_no_improvement(100) {}
        
        size_t max_iterations;         // Número máximo de iterações
        size_t max_time_seconds;       // Tempo máximo de execução em segundos
        size_t max_iterations_no_improvement; // Máximo de iterações sem melhoria
        
        // Validação dos parâmetros
        void validate() const;
    };
    
    /**
     * @brief Construtor do MOVNS
     * 
     * @param attractions Lista de atrações disponíveis
     * @param params Parâmetros de configuração
     */
    MOVNS(const std::vector<Attraction>& attractions, Parameters params = Parameters());
    
    /**
     * @brief Executa o algoritmo MOVNS
     * 
     * @return std::vector<MOVNSSolution> Conjunto de soluções não-dominadas
     */
    std::vector<MOVNSSolution> run();
    
private:
    // Tipo para facilitar referência ao conjunto de aproximação
    using ApproximationSet = std::vector<MOVNSSolution>;
    
    // Estado de exploração das vizinhanças para cada solução
    struct SolutionExplorationState {
        std::set<std::string> explored_neighborhoods;
        bool fully_explored = false;
    };
    
    // Dados do problema
    const std::vector<Attraction>& attractions_;
    const Parameters params_;
    
    // Estado da busca
    ApproximationSet p_approx_;
    std::vector<std::shared_ptr<Neighborhood>> neighborhoods_;
    std::unordered_map<size_t, SolutionExplorationState> exploration_state_;
    
    // Histórico de iterações
    std::vector<std::tuple<size_t, size_t, double, double, int>> iteration_history_;
    
    // Gerador de números aleatórios
    mutable std::mt19937 rng_{std::random_device{}()};
    
    // Métodos auxiliares
    void initializeApproximationSet();
    bool updateApproximationSet(const MOVNSSolution& solution);
    MOVNSSolution selectSolutionForExploration();
    bool isFullyExplored(const MOVNSSolution& solution);
    void markNeighborhoodAsExplored(const MOVNSSolution& solution, const Neighborhood& neighborhood);
    std::vector<MOVNSSolution> sortSolutions(const ApproximationSet& solutions) const;
    void logProgress(size_t iteration, size_t iterations_no_improvement) const;
    
    // Busca local
    MOVNSSolution localSearch(MOVNSSolution solution);
};

} // namespace movns
} // namespace tourist

#endif // MOVNS_ALGORITHM_HPP