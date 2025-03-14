// File: movns-algorithm.cpp

#include "movns-algorithm.hpp"
#include "movns-utils.hpp"
#include "movns-metrics.hpp"  // Add this include for Metrics class
#include <iostream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <functional>
#include <unordered_set>
#include <sstream>
#include <limits>  // Add this include for std::numeric_limits

namespace tourist {
namespace movns {

void MOVNS::Parameters::validate() const {
    if (max_iterations == 0) {
        throw std::invalid_argument("Maximum number of iterations must be positive");
    }
    if (max_time_seconds == 0) {
        throw std::invalid_argument("Maximum execution time must be positive");
    }
    if (max_iterations_no_improvement == 0) {
        throw std::invalid_argument("Maximum iterations without improvement must be positive");
    }
}

MOVNS::MOVNS(const std::vector<Attraction>& attractions, Parameters params)
    : attractions_(attractions), params_(params) {
    
    params_.validate();
    
    // Inicializar estruturas de vizinhança
    neighborhoods_ = NeighborhoodFactory::createAllNeighborhoods();
    
    // Inicializar gerador de números aleatórios
    rng_.seed(std::chrono::system_clock::now().time_since_epoch().count());
}

std::vector<MOVNSSolution> MOVNS::run() {
    // Inicialização
    initializeApproximationSet();
    
    // Configuração para medir o tempo de execução
    const auto start_time = std::chrono::high_resolution_clock::now();
    size_t iteration = 0;
    size_t iterations_no_improvement = 0;
    
    // Loop principal
    while (iteration < params_.max_iterations && 
           iterations_no_improvement < params_.max_iterations_no_improvement) {
        
        // Verificar tempo máximo
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
        if (static_cast<size_t>(elapsed) >= params_.max_time_seconds) {
            std::cout << "Time limit reached after " << iteration << " iterations." << std::endl;
            break;
        }

        // NOVO: Adicionar timeout por iteração
        auto iteration_start = std::chrono::high_resolution_clock::now();
        bool iteration_timeout = false;
        
        try {
            // Selecionar uma solução para exploração
            MOVNSSolution x = selectSolutionForExploration();
            
            // Selecionar vizinhança aleatoriamente
            auto neighborhood = NeighborhoodFactory::selectRandomNeighborhood(neighborhoods_, rng_);
            
            // Verificar timeout após cada operação potencialmente custosa
            current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - iteration_start).count() > 5) {
                std::cout << "Iteration timeout after neighborhood selection." << std::endl;
                iteration_timeout = true;
                continue;
            }
            
            // Gerar solução vizinha
            MOVNSSolution x_prime = neighborhood->generateRandomNeighbor(x, attractions_, rng_);
            
            current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - iteration_start).count() > 5) {
                std::cout << "Iteration timeout after neighbor generation." << std::endl;
                iteration_timeout = true;
                continue;
            }
            
            // Aplicar busca local para melhorar a solução vizinha
            MOVNSSolution x_double_prime = localSearch(x_prime);
            
            current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - iteration_start).count() > 5) {
                std::cout << "Iteration timeout after local search." << std::endl;
                iteration_timeout = true;
                continue;
            }
            
            // Atualizar o conjunto de aproximação
            bool improved = updateApproximationSet(x_double_prime);
            
            // Marcar a vizinhança como explorada para esta solução
            markNeighborhoodAsExplored(x, *neighborhood);
            
            // Atualizar contadores
            iteration++;
            if (!improved) {
                iterations_no_improvement++;
            } else {
                iterations_no_improvement = 0;
            }
            
            // Log de progresso (a cada 100 iterações)
            if (iteration % 100 == 0) {
                logProgress(iteration, iterations_no_improvement);
                
                // Encontrar os melhores valores para cada objetivo
                double best_cost = std::numeric_limits<double>::max();
                double best_time = std::numeric_limits<double>::max();
                int max_attractions = 0;
                
                for (const auto& sol : p_approx_) {
                    const auto& objectives = sol.getObjectives();
                    best_cost = std::min(best_cost, objectives[0]);
                    best_time = std::min(best_time, objectives[1]);
                    max_attractions = std::max(max_attractions, sol.getNumAttractions());
                }
                
                // Registrar no histórico
                iteration_history_.push_back(std::make_tuple(
                    iteration, p_approx_.size(), best_cost, best_time, max_attractions
                ));
            }
        }
        catch (const std::exception& e) {
            std::cout << "Exception in iteration " << iteration << ": " << e.what() << std::endl;
            iteration_timeout = true;
        }
        
        if (iteration_timeout) {
            std::cout << "Skipping to next iteration due to timeout." << std::endl;
            iteration++;
            iterations_no_improvement++;
        }
        
        // Verificar se todas as soluções estão totalmente exploradas
        bool all_explored = true;
        for (const auto& sol : p_approx_) {
            if (!isFullyExplored(sol)) {
                all_explored = false;
                break;
            }
        }
        
        // Se todas as soluções estão exploradas, reiniciar o estado de exploração
        if (all_explored) {
            exploration_state_.clear();
            std::cout << "All solutions fully explored. Resetting exploration state." << std::endl;
        }
    }
    
    // Ordenar as soluções finais pelo mesmo critério do NSGA-II
    auto sorted_solutions = sortSolutions(p_approx_);
    
    // Log de resumo final
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    
    std::cout << "\n=== MOVNS Execution Summary ===\n";
    std::cout << "Total iterations: " << iteration << std::endl;
    std::cout << "Execution time: " << total_time << " seconds" << std::endl;
    std::cout << "Non-dominated solutions found: " << sorted_solutions.size() << std::endl;
    
    // Exportar resultados para os arquivos corretos
    Metrics::exportToCSV(sorted_solutions, "../results/movns-resultados.csv", iteration_history_);
    return sorted_solutions;
}

void MOVNS::initializeApproximationSet() {
    // Gerar uma solução inicial aleatória
    MOVNSSolution initial_solution = Utils::generateRandomSolution(attractions_);
    
    // Adicionar ao conjunto de aproximação
    p_approx_.push_back(initial_solution);
    
    std::cout << "Initial solution generated with " << initial_solution.getNumAttractions()
              << " attractions from " << initial_solution.getNumNeighborhoods() << " neighborhoods." << std::endl;
}

bool MOVNS::updateApproximationSet(const MOVNSSolution& solution) {
    // Verificar se a solução é válida
    if (!solution.isValid()) {
        return false;
    }
    
    // Flag para indicar se o conjunto foi modificado
    bool modified = false;
    
    // Verificar dominância com soluções existentes
    std::vector<size_t> dominated_indices;
    bool is_dominated = false;
    
    for (size_t i = 0; i < p_approx_.size(); ++i) {
        const auto& existing = p_approx_[i];
        
        // Se a solução existente domina a nova solução
        if (existing.dominates(solution)) {
            is_dominated = true;
            break;
        }
        
        // Se a nova solução domina a solução existente
        if (solution.dominates(existing)) {
            dominated_indices.push_back(i);
            modified = true;
        }
    }
    
    // Se a nova solução é dominada, não adicionar
    if (is_dominated) {
        return false;
    }
    
    // Remover soluções dominadas (em ordem reversa para preservar índices)
    std::sort(dominated_indices.begin(), dominated_indices.end(), std::greater<size_t>());
    for (size_t idx : dominated_indices) {
        // Remover também do mapa de estado de exploração
        exploration_state_.erase(idx);
        
        // Remover do conjunto de aproximação
        p_approx_.erase(p_approx_.begin() + idx);
        
        // Atualizar índices no mapa (decrementar índices maiores que o removido)
        std::unordered_map<size_t, SolutionExplorationState> updated_map;
        for (const auto& [key, value] : exploration_state_) {
            if (key > idx) {
                updated_map[key - 1] = value;
            } else if (key < idx) {
                updated_map[key] = value;
            }
        }
        exploration_state_ = updated_map;
    }
    
    // Adicionar a nova solução
    p_approx_.push_back(solution);
    modified = true;
    
    return modified;
}

MOVNSSolution MOVNS::selectSolutionForExploration() {
    // Identificar soluções que ainda têm vizinhanças não exploradas
    std::vector<size_t> candidate_indices;
    
    for (size_t i = 0; i < p_approx_.size(); ++i) {
        if (!isFullyExplored(p_approx_[i])) {
            candidate_indices.push_back(i);
        }
    }
    
    // Se todas as soluções estão totalmente exploradas, selecionar aleatoriamente
    if (candidate_indices.empty()) {
        std::uniform_int_distribution<size_t> dist(0, p_approx_.size() - 1);
        return p_approx_[dist(rng_)];
    }
    
    // Selecionar aleatoriamente entre os candidatos
    std::uniform_int_distribution<size_t> dist(0, candidate_indices.size() - 1);
    return p_approx_[candidate_indices[dist(rng_)]];
}

bool MOVNS::isFullyExplored(const MOVNSSolution& solution) {
    // Encontrar o índice da solução no conjunto de aproximação
    auto it = std::find(p_approx_.begin(), p_approx_.end(), solution);
    if (it == p_approx_.end()) {
        return false; // Solução não está no conjunto
    }
    
    size_t index = std::distance(p_approx_.begin(), it);
    
    // Verificar se a solução tem estado de exploração
    if (exploration_state_.find(index) == exploration_state_.end()) {
        return false; // Sem estado de exploração = não explorada
    }
    
    // Verificar se a solução está marcada como totalmente explorada
    return exploration_state_[index].fully_explored ||
           exploration_state_[index].explored_neighborhoods.size() >= neighborhoods_.size();
}

void MOVNS::markNeighborhoodAsExplored(const MOVNSSolution& solution, const Neighborhood& neighborhood) {
    // Encontrar o índice da solução no conjunto de aproximação
    auto it = std::find(p_approx_.begin(), p_approx_.end(), solution);
    if (it == p_approx_.end()) {
        return; // Solução não está no conjunto
    }
    
    size_t index = std::distance(p_approx_.begin(), it);
    
    // Adicionar a vizinhança ao conjunto de exploradas
    exploration_state_[index].explored_neighborhoods.insert(neighborhood.getName());
    
    // Verificar se todas as vizinhanças foram exploradas
    if (exploration_state_[index].explored_neighborhoods.size() >= neighborhoods_.size()) {
        exploration_state_[index].fully_explored = true;
    }
}

std::vector<MOVNSSolution> MOVNS::sortSolutions(const ApproximationSet& solutions) const {
    // Cópia para ordenação
    std::vector<MOVNSSolution> sorted = solutions;
    
    // Ordenar por:
    // 1. Número de bairros (decrescente)
    // 2. Número de atrações (decrescente)
    // 3. Custo total (crescente)
    // 4. Tempo total (crescente)
    std::sort(sorted.begin(), sorted.end(), 
             [](const MOVNSSolution& a, const MOVNSSolution& b) {
                 // Número de bairros (decrescente)
                 if (a.getNumNeighborhoods() != b.getNumNeighborhoods()) {
                     return a.getNumNeighborhoods() > b.getNumNeighborhoods();
                 }
                 
                 // Número de atrações (decrescente)
                 if (a.getNumAttractions() != b.getNumAttractions()) {
                     return a.getNumAttractions() > b.getNumAttractions();
                 }
                 
                 // Custo total (crescente)
                 if (std::abs(a.getTotalCost() - b.getTotalCost()) > 1e-6) {
                     return a.getTotalCost() < b.getTotalCost();
                 }
                 
                 // Tempo total (crescente)
                 return a.getTotalTime() < b.getTotalTime();
             });
    
    return sorted;
}

void MOVNS::logProgress(size_t iteration, size_t iterations_no_improvement) const {
    std::cout << "Iteration " << iteration 
              << ": Approximation set size = " << p_approx_.size()
              << ", Iterations without improvement = " << iterations_no_improvement << std::endl;
}

MOVNSSolution MOVNS::localSearch(MOVNSSolution solution) {
    // Usar a vizinhança de troca de modo de transporte para busca local
    auto neighborhood = std::make_shared<TransportModeChangeNeighborhood>();
    bool improved = true;
    
    while (improved) {
        improved = false;
        
        // Tentar melhorar a solução com a vizinhança de troca de modo
        for (size_t i = 0; i < 10; ++i) { // Limitar número de tentativas
            MOVNSSolution neighbor = neighborhood->generateRandomNeighbor(solution, attractions_, rng_);
            
            // Se encontrou um vizinho não dominado
            if (!solution.dominates(neighbor) && !neighbor.dominates(solution)) {
                // Comparar objetivos diretamente
                const auto& sol_obj = solution.getObjectives();
                const auto& neigh_obj = neighbor.getObjectives();
                
                // Preferir soluções com menor custo ou tempo
                if (neigh_obj[0] < sol_obj[0] || neigh_obj[1] < sol_obj[1]) {
                    solution = neighbor;
                    improved = true;
                    break;
                }
            } else if (neighbor.dominates(solution)) {
                solution = neighbor;
                improved = true;
                break;
            }
        }
    }
    
    return solution;
}

} // namespace movns
} // namespace tourist