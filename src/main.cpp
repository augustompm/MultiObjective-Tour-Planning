#include "nsga2.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>

using namespace tourist;

namespace {
constexpr size_t TIME_PRECISION = 1;
constexpr size_t COST_PRECISION = 2;
}

void printSolution(const Solution& solution, size_t index) {
    const auto& route = solution.getRoute();
    const auto& objectives = solution.getObjectives();
    
    std::cout << "\n=== Solução #" << (index + 1) << " ===\n";
    std::cout << "Custo Total: R$ " << std::fixed << std::setprecision(COST_PRECISION) 
              << objectives[0] << "\n";
    std::cout << "Tempo Total: " << std::setprecision(TIME_PRECISION) 
              << objectives[1] << " minutos\n";
    std::cout << "Atrações Visitadas: " << std::abs(static_cast<int>(objectives[2])) << "\n";
    
    std::cout << "\nRoteiro Detalhado:\n";
    const auto& sequence = route.getSequence();
    for (size_t i = 0; i < sequence.size(); ++i) {
        const auto* attraction = sequence[i];
        auto [lat, lon] = attraction->getCoordinates();
        
        std::cout << (i + 1) << ". " << attraction->getName() << "\n";
        std::cout << "   - Tempo de visita: " << attraction->getVisitTime() << " minutos\n";
        std::cout << "   - Custo: R$ " << std::fixed << std::setprecision(COST_PRECISION) 
                  << attraction->getCost() << "\n";
        std::cout << "   - Coordenadas: (" << std::fixed << std::setprecision(6) 
                  << lat << ", " << lon << ")\n";
        
        int open_hour = attraction->getOpeningTime() / 60;
        int open_min = attraction->getOpeningTime() % 60;
        int close_hour = attraction->getClosingTime() / 60;
        int close_min = attraction->getClosingTime() % 60;
        
        std::cout << "   - Horário: " 
                  << std::setfill('0') << std::setw(2) << open_hour << ":" 
                  << std::setfill('0') << std::setw(2) << open_min << " - "
                  << std::setfill('0') << std::setw(2) << close_hour << ":" 
                  << std::setfill('0') << std::setw(2) << close_min << "\n";
    }
}

void exportResults(const std::vector<Solution>& solutions, 
                  const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo: " + filename);
    }

    file << "Solucao;CustoTotal;TempoTotal;NumAtracoes;Sequencia\n";
    
    for (size_t i = 0; i < solutions.size(); ++i) {
        const auto& solution = solutions[i];
        const auto& objectives = solution.getObjectives();
        const auto& route = solution.getRoute();
        
        file << std::fixed << std::setprecision(COST_PRECISION);
        file << (i + 1) << ";";
        file << objectives[0] << ";";
        file << objectives[1] << ";";
        file << std::abs(static_cast<int>(objectives[2])) << ";";
        
        for (const auto* attraction : route.getSequence()) {
            file << attraction->getName() << "|";
        }
        file << "\n";
    }
}

int main() {
    try {
        std::cout << "\n=== Planejador de Rotas Turísticas ===\n\n";
        std::cout << "Carregando dados...\n";
        
        const auto attractions = utils::Parser::loadAttractions("data/attractions.txt");
        
        if (attractions.empty()) {
            throw std::runtime_error("Nenhuma atração carregada de attractions.txt");
        }
        std::cout << "Atrações carregadas. Primeira atração: " << attractions[0].getName() << "\n";

        std::cout << "\nDados carregados com sucesso:\n";
        std::cout << "- " << attractions.size() << " atrações turísticas\n\n";
        
        std::cout << "Configurando NSGA-II...\n";
        NSGA2::Parameters params;
        params.population_size = 100;
        params.max_generations = 100;
        params.crossover_rate = 0.9;
        params.mutation_rate = 0.1;
        try {
            std::cout << "Validando parâmetros...\n";
            params.validate();
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Erro na validação dos parâmetros: ") + e.what());
        }
        
        std::cout << "=== Configuração da Otimização ===\n";
        std::cout << "Tamanho da população: " << params.population_size << "\n";
        std::cout << "Número de gerações: " << params.max_generations << "\n";
        std::cout << "Taxa de crossover: " << params.crossover_rate << "\n";
        std::cout << "Taxa de mutação: " << params.mutation_rate << "\n";
        std::cout << "Limite de tempo diário: " << utils::Config::DAILY_TIME_LIMIT << " minutos\n\n";
        
        std::cout << "Inicializando NSGA-II...\n";
        NSGA2 nsga2(attractions, params);
        std::cout << "NSGA-II inicializado com sucesso\n";
        
        std::cout << "Iniciando otimização...\n";
        const auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            const auto solutions = nsga2.run();
            std::cout << "Otimização concluída com sucesso. Soluções encontradas: "
                      << solutions.size() << "\n";
            
            const auto end_time = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
            
            std::cout << "\n=== Resultados da Otimização ===\n";
            std::cout << "Tempo de execução: " << duration.count() << " segundos\n";
            std::cout << "Soluções não-dominadas encontradas: " << solutions.size() << "\n\n";
            
            if (solutions.empty()) {
                std::cout << "Nenhuma solução válida encontrada. Considere relaxar as restrições.\n";
                return 0;
            }
            
            std::vector<double> reference_point = {10000.0, 720.0, 0.0};
            const double hypervolume = utils::Metrics::calculateHypervolume(solutions, reference_point);
            const double spread = utils::Metrics::calculateSpread(solutions);
            
            std::cout << "=== Métricas de Qualidade ===\n";
            std::cout << "- Hipervolume: " << std::fixed << std::setprecision(4) << hypervolume << "\n";
            std::cout << "- Spread: " << std::fixed << std::setprecision(4) << spread << "\n\n";
            
            const size_t num_to_show = std::min(size_t(5), solutions.size());
            std::cout << "=== Melhores Soluções ===\n";
            std::cout << "Mostrando " << num_to_show << " soluções representativas:\n";
            
            for (size_t i = 0; i < num_to_show; ++i) {
                printSolution(solutions[i], i);
            }
            
            const std::string output_file = "resultados_nsga2.csv";
            exportResults(solutions, output_file);
            std::cout << "\nResultados detalhados exportados para: " << output_file << "\n";
            
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("Erro durante a otimização: ") + e.what());
        }
        
    } catch (const std::exception& e) {
        std::cerr << "\nERRO CRÍTICO: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}