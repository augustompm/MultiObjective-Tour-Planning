// File: src/movns-metrics.cpp

#include "movns-metrics.hpp"
#include "models.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <unordered_set>
#include <set>
#include <map>
#include <iostream>
#include <iomanip>

namespace tourist {
namespace movns {

double Metrics::calculateHypervolume(
    const std::vector<MOVNSSolution>& solutions,
    const std::vector<double>& reference_point) {
    
    // Implementação simplificada para hipervolume
    // Para uma implementação completa, seria melhor usar a classe HypervolumeCalculator existente
    
    if (solutions.empty()) {
        return 0.0;
    }
    
    // Converter soluções MOVNS para o formato utilizado pela função de hipervolume
    std::vector<Solution> nsga_solutions = convertToNSGA2Format(solutions);
    
    // Usar a implementação de hipervolume existente
    return utils::Metrics::calculateHypervolume(nsga_solutions, reference_point);
}

double Metrics::calculateBinaryCoverage(
    const std::vector<MOVNSSolution>& solutions1,
    const std::vector<MOVNSSolution>& solutions2) {
    
    if (solutions2.empty()) {
        return 1.0; // Por definição
    }
    
    if (solutions1.empty()) {
        return 0.0;
    }
    
    // Contar quantas soluções em solutions2 são dominadas por pelo menos uma solução em solutions1
    size_t dominated_count = 0;
    
    for (const auto& sol2 : solutions2) {
        bool is_dominated = false;
        for (const auto& sol1 : solutions1) {
            if (sol1.dominates(sol2)) {
                is_dominated = true;
                break;
            }
        }
        if (is_dominated) {
            dominated_count++;
        }
    }
    
    return static_cast<double>(dominated_count) / solutions2.size();
}

std::vector<Solution> Metrics::convertToNSGA2Format(
    const std::vector<MOVNSSolution>& solutions) {
    
    std::vector<Solution> nsga_solutions;
    
    for (const auto& movns_sol : solutions) {
        // Criar uma rota a partir da solução MOVNS
        Route route;
        
        const auto& attractions = movns_sol.getAttractions();
        const auto& modes = movns_sol.getTransportModes();
        
        // Adicionar a primeira atração
        if (!attractions.empty()) {
            route.addAttraction(*attractions[0]);
            
            // Adicionar as demais atrações com seus modos de transporte
            for (size_t i = 1; i < attractions.size(); ++i) {
                utils::TransportMode mode = (i - 1 < modes.size()) ? 
                                          modes[i - 1] : utils::TransportMode::CAR;
                route.addAttraction(*attractions[i], mode);
            }
        }
        
        // Criar solução NSGA-II a partir da rota
        nsga_solutions.emplace_back(route);
    }
    
    return nsga_solutions;
}

std::vector<MOVNSSolution> Metrics::filterDuplicatesAndInvalid(
    const std::vector<MOVNSSolution>& solutions) {
    
    // Rastrear sequências únicas de atrações
    std::vector<MOVNSSolution> filtered;
    std::map<std::string, bool> seen_solutions;
    
    for (const auto& solution : solutions) {
        // Skip invalid solutions
        if (!solution.isValid()) {
            continue;
        }
        
        // Create a simplified hash of the solution based on attractions
        std::string solution_hash;
        const auto& attractions = solution.getAttractions();
        
        // Sort attraction names for unique identification regardless of order
        std::vector<std::string> attraction_names;
        for (const auto* attr : attractions) {
            attraction_names.push_back(attr->getName());
        }
        std::sort(attraction_names.begin(), attraction_names.end());
        
        for (const auto& name : attraction_names) {
            solution_hash += name + "|";
        }
        
        // Only add if this hash is new
        if (seen_solutions.find(solution_hash) == seen_solutions.end()) {
            seen_solutions[solution_hash] = true;
            filtered.push_back(solution);
        }
    }
    
    std::cout << "Filtered out " << (solutions.size() - filtered.size()) 
              << " duplicate or invalid solutions." << std::endl;
    return filtered;
}

std::vector<MOVNSSolution> Metrics::applyEpsilonDominance(
    const std::vector<MOVNSSolution>& solutions,
    const std::vector<double>& epsilon) {
    
    if (solutions.empty()) {
        return solutions;
    }
    
    // For this application, we'll use a more relaxed epsilon-dominance
    // to preserve diversity in the Pareto front
    
    std::vector<MOVNSSolution> non_dominated = solutions;
    std::vector<bool> dominated(solutions.size(), false);
    
    // For each pair of solutions
    for (size_t i = 0; i < solutions.size(); ++i) {
        if (dominated[i]) continue; // Skip if already dominated
        
        const auto& obj_i = solutions[i].getObjectives();
        
        for (size_t j = i + 1; j < solutions.size(); ++j) {
            if (dominated[j]) continue; // Skip if already dominated
            
            const auto& obj_j = solutions[j].getObjectives();
            
            // Special handling for tourist routing objectives
            // Consider solutions equivalent only if they have same number of
            // attractions and neighborhoods, then apply epsilon to cost and time
            
            // First check if attractions and neighborhoods are the same
            bool same_structure = (
                std::abs(obj_i[2] - obj_j[2]) < 0.5 &&  // -attractions
                std::abs(obj_i[3] - obj_j[3]) < 0.5     // -neighborhoods
            );
            
            // Only consider dominance if structure is similar
            if (same_structure) {
                // Check if i ε-dominates j for cost and time
                bool i_dominates_j = true;
                bool j_dominates_i = true;
                
                // Compare cost and time with epsilon
                for (size_t k = 0; k < 2; ++k) {
                    double eps = k < epsilon.size() ? epsilon[k] : 5.0;
                    
                    // i doesn't dominate j for objective k
                    if (obj_i[k] > obj_j[k] + eps) {
                        i_dominates_j = false;
                    }
                    
                    // j doesn't dominate i for objective k
                    if (obj_j[k] > obj_i[k] + eps) {
                        j_dominates_i = false;
                    }
                }
                
                // Mark dominated solution
                if (i_dominates_j) {
                    dominated[j] = true;
                } else if (j_dominates_i) {
                    dominated[i] = true;
                    break; // i is dominated, can stop comparing it
                }
            }
        }
    }
    
    // Build non-dominated set
    std::vector<MOVNSSolution> result;
    for (size_t i = 0; i < solutions.size(); ++i) {
        if (!dominated[i]) {
            result.push_back(solutions[i]);
        }
    }
    
    std::cout << "Reduced solution set from " << solutions.size() << " to " << result.size() 
              << " using ε-dominance." << std::endl;
    
    return result;
}

void Metrics::exportGenerationHistory(
    const std::vector<std::tuple<size_t, size_t, double, double, int, int>>& iteration_history,
    const std::string& filename) {
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    
    // Write header with neighborhoods column
    file << "Generation;Front size;Best Cost;Best Time;Max Attractions;Max Neighborhoods\n";
    
    // Write data for each generation
    for (const auto& history_entry : iteration_history) {
        file << std::get<0>(history_entry) << ";" 
             << std::get<1>(history_entry) << ";" 
             << std::fixed << std::setprecision(2) << std::get<2>(history_entry) << ";" 
             << std::fixed << std::setprecision(2) << std::get<3>(history_entry) << ";"
             << std::get<4>(history_entry) << ";"
             << std::get<5>(history_entry) << "\n";
    }
    
    file.close();
}

void Metrics::exportToCSV(
    const std::vector<MOVNSSolution>& solutions,
    const std::string& filename,
    const std::vector<std::tuple<size_t, size_t, double, double, int, int>>& iteration_history) {
    
    // Apply filtering to improve solution quality but preserve diversity
    std::vector<MOVNSSolution> filtered = filterDuplicatesAndInvalid(solutions);
    
    // Define epsilon values for each objective
    // Using relaxed values to maintain diversity
    // [cost, time, -attractions, -neighborhoods]
    std::vector<double> epsilon = {10.0, 30.0, 0.1, 0.1};
    std::vector<MOVNSSolution> reduced = applyEpsilonDominance(filtered, epsilon);
    
    // Sort solutions for presentation
    std::sort(reduced.begin(), reduced.end(), 
             [](const MOVNSSolution& a, const MOVNSSolution& b) {
                 // First by number of neighborhoods (descending)
                 if (a.getNumNeighborhoods() != b.getNumNeighborhoods()) {
                     return a.getNumNeighborhoods() > b.getNumNeighborhoods();
                 }
                 
                 // Then by number of attractions (descending)
                 if (a.getNumAttractions() != b.getNumAttractions()) {
                     return a.getNumAttractions() > b.getNumAttractions();
                 }
                 
                 // Then by total cost (ascending)
                 if (std::abs(a.getTotalCost() - b.getTotalCost()) > 1e-6) {
                     return a.getTotalCost() < b.getTotalCost();
                 }
                 
                 // Finally by total time (ascending)
                 return a.getTotalTime() < b.getTotalTime();
             });
    
    // Limit to a maximum of 50 solutions if we have more
    const size_t max_solutions = 50;
    if (reduced.size() > max_solutions) {
        reduced.resize(max_solutions);
    }
    
    // Export valid solutions
    std::ofstream file("../results/" + filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    
    // Write header
    file << "Solucao;CustoTotal;TempoTotal;NumAtracoes;NumBairros;HoraInicio;HoraFim;Bairros;Sequencia;TemposChegada;TemposPartida;ModosTransporte\n";
    
    // Write each solution
    for (size_t i = 0; i < reduced.size(); ++i) {
        file << (i + 1) << ";" << reduced[i].toString() << "\n";
    }
    
    file.close();
    
    // Export generation history
    std::string gen_filename = "../results/movns-geracoes.csv";
    exportGenerationHistory(iteration_history, gen_filename);
}

} // namespace movns
} // namespace tourist