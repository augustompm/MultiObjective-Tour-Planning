// File: movns-metrics.cpp

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

/**
 * @brief Elimina soluções duplicadas e inválidas do conjunto de aproximação
 * 
 * Baseado no conceito de exploração eficiente do espaço de soluções
 * (Mladenović e Hansen, 1997)
 * 
 * @param solutions Conjunto de soluções
 * @return std::vector<MOVNSSolution> Conjunto de soluções sem duplicatas
 */
std::vector<MOVNSSolution> Metrics::filterDuplicatesAndInvalid(
    const std::vector<MOVNSSolution>& solutions) {
    
    // Rastrear combinações únicas de atrações
    std::set<std::string> unique_attraction_sequences;
    std::vector<MOVNSSolution> filtered;
    
    for (const auto& solution : solutions) {
        // Verificar se a solução é válida (não tem atrações repetidas)
        const auto& attractions = solution.getAttractions();
        std::unordered_set<std::string> attraction_set;
        bool has_duplicates = false;
        
        for (const auto* attraction : attractions) {
            if (attraction_set.find(attraction->getName()) != attraction_set.end()) {
                has_duplicates = true;
                break;
            }
            attraction_set.insert(attraction->getName());
        }
        
        if (has_duplicates) {
            continue; // Pular soluções com atrações repetidas
        }
        
        // Construir uma string representando a sequência de atrações
        std::string attraction_sequence;
        for (const auto* attraction : attractions) {
            attraction_sequence += attraction->getName() + "|";
        }
        
        // Verificar se esta sequência já foi vista
        if (unique_attraction_sequences.find(attraction_sequence) == unique_attraction_sequences.end()) {
            unique_attraction_sequences.insert(attraction_sequence);
            filtered.push_back(solution);
        }
    }
    
    std::cout << "Filtered out " << (solutions.size() - filtered.size()) << " duplicate or invalid solutions." << std::endl;
    return filtered;
}

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
std::vector<MOVNSSolution> Metrics::applyEpsilonDominance(
    const std::vector<MOVNSSolution>& solutions,
    const std::vector<double>& epsilon) {
    
    if (solutions.empty()) {
        return solutions;
    }
    
    // Mapeamento de células para soluções
    std::map<std::vector<int>, MOVNSSolution> grid_cells;
    
    // Para cada solução
    for (const auto& solution : solutions) {
        // Calcular coordenadas da célula para esta solução
        std::vector<int> cell_coords;
        const auto& objectives = solution.getObjectives();
        
        // Garantir que epsilon tenha o tamanho correto
        std::vector<double> eps = epsilon;
        if (eps.size() < objectives.size()) {
            eps.resize(objectives.size(), 0.01); // Valor padrão se não especificado
        }
        
        // Mapear a solução para uma célula da grade
        for (size_t i = 0; i < objectives.size(); ++i) {
            int cell_coord = static_cast<int>(objectives[i] / eps[i]);
            cell_coords.push_back(cell_coord);
        }
        
        // Verificar se a célula já tem uma solução
        auto it = grid_cells.find(cell_coords);
        if (it == grid_cells.end()) {
            // Nova célula, adicionar esta solução
            grid_cells[cell_coords] = solution;
        } else {
            // Célula já tem uma solução, manter a melhor
            // Para objetivos de minimização (0, 1), menor é melhor
            // Para objetivos de maximização (2, 3), maior é melhor (mas estão negativos no vetor)
            bool better = false;
            bool worse = false;
            
            for (size_t i = 0; i < objectives.size(); ++i) {
                if (objectives[i] < it->second.getObjectives()[i]) {
                    better = true;
                } else if (objectives[i] > it->second.getObjectives()[i]) {
                    worse = true;
                }
            }
            
            // Se a solução atual é melhor em pelo menos um objetivo e não pior em nenhum
            if (better && !worse) {
                grid_cells[cell_coords] = solution;
            }
            // Em caso de indefinição, preferir soluções com mais atrações (objetivo 2, negado)
            else if (!better && !worse && objectives[2] < it->second.getObjectives()[2]) {
                grid_cells[cell_coords] = solution;
            }
        }
    }
    
    // Extrair as soluções das células
    std::vector<MOVNSSolution> reduced_solutions;
    for (const auto& cell : grid_cells) {
        reduced_solutions.push_back(cell.second);
    }
    
    std::cout << "Reduced solution set from " << solutions.size() << " to " << reduced_solutions.size() 
              << " using ε-dominance." << std::endl;
    
    return reduced_solutions;
}

/**
 * @brief Exporta o histórico de gerações para um arquivo CSV
 * 
 * @param iteration_history Histórico de iterações
 * @param filename Nome do arquivo
 */
void Metrics::exportGenerationHistory(
    const std::vector<std::tuple<size_t, size_t, double, double, int>>& iteration_history,
    const std::string& filename) {
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    
    // Escrever cabeçalho
    file << "Generation;Front size;Best Cost;Best Time;Max Attractions\n";
    
    // Escrever dados de cada geração
    for (const auto& [generation, front_size, best_cost, best_time, max_attractions] : iteration_history) {
        file << generation << ";"
             << front_size << ";"
             << std::fixed << std::setprecision(2) << best_cost << ";"
             << std::fixed << std::setprecision(2) << best_time << ";"
             << max_attractions << "\n";
    }
    
    file.close();
}

// Replace the existing exportToCSV function with this new version
void Metrics::exportToCSV(
    const std::vector<MOVNSSolution>& solutions,
    const std::string& filename,
    const std::vector<std::tuple<size_t, size_t, double, double, int>>& iteration_history) {
    
    // Aplicar filtragens para melhorar a qualidade das soluções
    std::vector<MOVNSSolution> filtered = filterDuplicatesAndInvalid(solutions);
    
    // Definir valores de epsilon para cada objetivo
    // [custo, tempo, -atrações, -bairros]
    std::vector<double> epsilon = {5.0, 15.0, 0.5, 0.5};
    std::vector<MOVNSSolution> reduced = applyEpsilonDominance(filtered, epsilon);
    
    // Exportar soluções filtradas
    std::ofstream file("../results/" + filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filename);
    }
    
    // Escrever cabeçalho
    file << "Solucao;CustoTotal;TempoTotal;NumAtracoes;NumBairros;HoraInicio;HoraFim;Bairros;Sequencia;TemposChegada;TemposPartida;ModosTransporte\n";
    
    // Escrever cada solução
    for (size_t i = 0; i < reduced.size(); ++i) {
        file << (i + 1) << ";" << reduced[i].toString() << "\n";
    }
    
    file.close();
    
    // Exportar histórico de gerações
    std::string gen_filename = "../results/movns-geracoes.csv";
    exportGenerationHistory(iteration_history, gen_filename);
}

} // namespace movns
} // namespace tourist