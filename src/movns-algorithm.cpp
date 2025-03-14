// File: src/movns-algorithm.cpp

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
    
    // Debug: Log the state after initialization
    std::cout << "After initialization, approximation set size: " << p_approx_.size() << std::endl;
    
    // Configuração para medir o tempo de execução
    const auto start_time = std::chrono::high_resolution_clock::now();
    size_t iteration = 0;
    size_t iterations_no_improvement = 0;
    
    // Debug: Mark the entry into the main loop
    std::cout << "Entering main loop..." << std::endl;
    
    // Loop principal
    while (iteration < params_.max_iterations && 
           iterations_no_improvement < params_.max_iterations_no_improvement) {
        
        // Debug: Log the current iteration
        std::cout << "Starting iteration " << iteration + 1 << std::endl;
        
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
            // Debug: Check approximation set state
            std::cout << "Current approximation set size: " << p_approx_.size() << std::endl;
            
            // Verificar se o conjunto de aproximação está vazio
            if (p_approx_.empty()) {
                std::cout << "Approximation set is empty. Reinitializing..." << std::endl;
                initializeApproximationSet();
                continue;
            }
            
            // Debug: About to select solution
            std::cout << "Selecting solution for exploration..." << std::endl;
            
            // Selecionar uma solução para exploração
            MOVNSSolution x = selectSolutionForExploration();
            
            // Debug: Selected solution
            std::cout << "Selected solution with " << x.getNumAttractions() << " attractions" << std::endl;
            
            // Verificar se a solução é válida
            if (!x.isValid()) {
                std::cout << "Selected an invalid solution. Removing from set." << std::endl;
                auto it = std::find(p_approx_.begin(), p_approx_.end(), x);
                if (it != p_approx_.end()) {
                    size_t index = std::distance(p_approx_.begin(), it);
                    exploration_state_.erase(index);
                    p_approx_.erase(it);
                }
                continue;
            }
            
            // Debug: About to select neighborhood
            std::cout << "Selecting neighborhood operator..." << std::endl;
            
            // Selecionar vizinhança aleatoriamente
            auto neighborhood = NeighborhoodFactory::selectRandomNeighborhood(neighborhoods_, rng_);
            
            // Debug: Selected neighborhood
            std::cout << "Selected neighborhood: " << neighborhood->getName() << std::endl;
            
            // Verificar timeout após cada operação potencialmente custosa
            current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - iteration_start).count() > 5) {
                std::cout << "Iteration timeout after neighborhood selection." << std::endl;
                iteration_timeout = true;
                iteration++;
                iterations_no_improvement++;
                continue;
            }
            
            // Debug: About to generate neighbor
            std::cout << "Generating random neighbor..." << std::endl;
            
            // Gerar solução vizinha - PORÉM LIMITANDO NÚMERO DE TENTATIVAS PARA EVITAR LOOPS INFINITOS
            const int MAX_NEIGHBOR_ATTEMPTS = 10;
            MOVNSSolution x_prime;
            bool generated_valid_neighbor = false;
            
            for (int attempt = 0; attempt < MAX_NEIGHBOR_ATTEMPTS; attempt++) {
                x_prime = neighborhood->generateRandomNeighbor(x, attractions_, rng_);
                
                // Verificar se a solução vizinha é válida e diferente da original
                if (x_prime.isValid() && !(x == x_prime)) {
                    generated_valid_neighbor = true;
                    break;
                }
                
                // Verificar se atingimos o timeout durante as tentativas
                current_time = std::chrono::high_resolution_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(current_time - iteration_start).count() > 5) {
                    std::cout << "Timeout while attempting to generate valid neighbor." << std::endl;
                    iteration_timeout = true;
                    break;
                }
            }
            
            if (iteration_timeout) {
                iteration++;
                iterations_no_improvement++;
                continue;
            }
            
            if (!generated_valid_neighbor) {
                std::cout << "Failed to generate a valid neighbor after " << MAX_NEIGHBOR_ATTEMPTS << " attempts. Marking neighborhood as explored." << std::endl;
                markNeighborhoodAsExplored(x, *neighborhood);
                iteration++;
                iterations_no_improvement++;
                continue;
            }
            
            // Debug: Generated neighbor
            std::cout << "Generated neighbor with " << x_prime.getNumAttractions() << " attractions" << std::endl;
            
            current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - iteration_start).count() > 5) {
                std::cout << "Iteration timeout after neighbor generation." << std::endl;
                iteration_timeout = true;
                iteration++;
                iterations_no_improvement++;
                continue;
            }
            
            // Verificar se a solução vizinha é igual à original
            if (x == x_prime) {
                // Debug: No change in solution
                std::cout << "Neighbor is identical to original solution, marking as explored" << std::endl;
                
                // Se são iguais, marcar a vizinhança como explorada e continuar
                markNeighborhoodAsExplored(x, *neighborhood);
                iteration++;
                iterations_no_improvement++;
                continue;
            }
            
            // Debug: About to perform local search
            std::cout << "Performing local search..." << std::endl;
            
            // Aplicar busca local para melhorar a solução vizinha
            MOVNSSolution x_double_prime = localSearch(x_prime);
            
            // Debug: Completed local search
            std::cout << "Local search resulted in solution with " << x_double_prime.getNumAttractions() << " attractions" << std::endl;
            
            current_time = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - iteration_start).count() > 5) {
                std::cout << "Iteration timeout after local search." << std::endl;
                iteration_timeout = true;
                iteration++;
                iterations_no_improvement++;
                continue;
            }
            
            // Debug: About to update approximation set
            std::cout << "Updating approximation set..." << std::endl;
            
            // Atualizar o conjunto de aproximação
            bool improved = updateApproximationSet(x_double_prime);
            
            // Debug: Updated approximation set
            std::cout << "Approximation set updated, improvement: " << (improved ? "yes" : "no") << std::endl;
            
            // Marcar a vizinhança como explorada para esta solução
            markNeighborhoodAsExplored(x, *neighborhood);
            
            // Atualizar contadores
            iteration++;
            if (!improved) {
                iterations_no_improvement++;
            } else {
                iterations_no_improvement = 0;
            }
            
            // Debug: Completed iteration
            std::cout << "Completed iteration " << iteration << std::endl;
            
            // Log de progresso (a cada 100 iterações)
            if (iteration % 100 == 0) {
                logProgress(iteration, iterations_no_improvement);
                
                // Encontrar os melhores valores para cada objetivo
                double best_cost = std::numeric_limits<double>::max();
                double best_time = std::numeric_limits<double>::max();
                int max_attractions = 0;
                int max_neighborhoods = 0;  // Add this variable
                
                for (const auto& sol : p_approx_) {
                    const auto& objectives = sol.getObjectives();
                    best_cost = std::min(best_cost, objectives[0]);
                    best_time = std::min(best_time, objectives[1]);
                    max_attractions = std::max(max_attractions, sol.getNumAttractions());
                    max_neighborhoods = std::max(max_neighborhoods, sol.getNumNeighborhoods());  // Track neighborhoods
                }
                
                // Registrar no histórico
                iteration_history_.push_back(std::make_tuple(
                    iteration, p_approx_.size(), best_cost, best_time, max_attractions, max_neighborhoods
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
        
        // Verificar se o conjunto de aproximação está vazio após iteração
        if (p_approx_.empty()) {
            std::cout << "Approximation set became empty. Reinitializing..." << std::endl;
            initializeApproximationSet();
            continue;
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
    Metrics::exportToCSV(sorted_solutions, "movns-resultados.csv", iteration_history_);
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
    }
    
    // Reconstruir o mapa de estado de exploração com índices atualizados
    std::unordered_map<size_t, SolutionExplorationState> updated_map;
    for (const auto& [key, value] : exploration_state_) {
        if (std::find(dominated_indices.begin(), dominated_indices.end(), key) == dominated_indices.end()) {
            // Ajustar o índice baseado em quantas soluções dominadas foram removidas antes deste índice
            size_t new_idx = key;
            for (size_t removed_idx : dominated_indices) {
                if (removed_idx < key) {
                    new_idx--;
                }
            }
            updated_map[new_idx] = value;
        }
    }
    exploration_state_ = updated_map;
    
    // Adicionar a nova solução
    p_approx_.push_back(solution);
    
    // Garantir que a nova solução não está marcada como explorada
    size_t new_solution_idx = p_approx_.size() - 1;
    exploration_state_[new_solution_idx] = SolutionExplorationState();
    
    modified = true;
    
    return modified;
}

MOVNSSolution MOVNS::selectSolutionForExploration() {
    // Verificar se o conjunto de aproximação está vazio
    if (p_approx_.empty()) {
        // Inicializar com uma nova solução aleatória
        initializeApproximationSet();
        return p_approx_[0];
    }

    // Identificar soluções que ainda têm vizinhanças não exploradas
    std::vector<size_t> candidate_indices;
    
    for (size_t i = 0; i < p_approx_.size(); ++i) {
        // Verificar se a solução tem entrada no mapa de exploração
        if (exploration_state_.find(i) == exploration_state_.end() || !isFullyExplored(p_approx_[i])) {
            candidate_indices.push_back(i);
        }
    }
    
    // Se todas as soluções estão totalmente exploradas, resetar o estado de exploração
    if (candidate_indices.empty()) {
        std::cout << "All solutions fully explored. Resetting exploration state." << std::endl;
        exploration_state_.clear();
        
        // Agora todas as soluções são candidatas
        for (size_t i = 0; i < p_approx_.size(); ++i) {
            candidate_indices.push_back(i);
        }
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
        // Se não tem estado, criar um estado vazio em vez de retornar false
        exploration_state_[index] = SolutionExplorationState();
        return false;
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
    
    // Garantir que existe uma entrada no mapa
    if (exploration_state_.find(index) == exploration_state_.end()) {
        exploration_state_[index] = SolutionExplorationState();
    }
    
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
    if (!p_approx_.empty()) {
        std::cout << "Iteration " << iteration 
                  << ": Approximation set size = " << p_approx_.size()
                  << ", Iterations without improvement = " << iterations_no_improvement << std::endl;
        
        // Find best solution values
        double best_cost = std::numeric_limits<double>::max();
        double best_time = std::numeric_limits<double>::max();
        int max_attractions = 0;
        int max_neighborhoods = 0;  // Track neighborhoods
        
        for (const auto& solution : p_approx_) {
            if (solution.isValid()) {
                best_cost = std::min(best_cost, solution.getTotalCost());
                best_time = std::min(best_time, solution.getTotalTime());
                max_attractions = std::max(max_attractions, solution.getNumAttractions());
                max_neighborhoods = std::max(max_neighborhoods, solution.getNumNeighborhoods());
            }
        }
        
        // Display current best values
        std::cout << "Best values: [Cost=" << std::fixed << std::setprecision(2) << best_cost 
                  << ", Time=" << std::fixed << std::setprecision(1) << best_time 
                  << ", Attractions=" << max_attractions
                  << ", Neighborhoods=" << max_neighborhoods << "]" << std::endl;
    }
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