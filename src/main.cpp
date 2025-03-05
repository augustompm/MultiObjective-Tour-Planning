#include "nsga2.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <filesystem>

using namespace tourist;

namespace {
constexpr size_t TIME_PRECISION = 1;
constexpr size_t COST_PRECISION = 2;
constexpr size_t DIST_PRECISION = 0;
}

void printSolution(const Solution& solution, size_t index) {
    const auto& route = solution.getRoute();
    const auto& objectives = solution.getObjectives();
    const auto& time_info = route.getTimeInfo();
    
    std::cout << "\n=== Solução #" << (index + 1) << " ===\n";
    std::cout << "Custo Total: R$ " << std::fixed << std::setprecision(COST_PRECISION) 
              << objectives[0] << "\n";
    std::cout << "Tempo Total: " << std::setprecision(TIME_PRECISION) 
              << objectives[1] << " minutos\n";
    std::cout << "Atrações Visitadas: " << std::abs(static_cast<int>(objectives[2])) << "\n";
    
    std::cout << "\nRoteiro Detalhado:\n";
    const auto& attractions = route.getAttractions();
    const auto& transport_modes = route.getTransportModes();
    
    if (attractions.empty()) {
        std::cout << "Nenhuma atração no roteiro.\n";
        return;
    }
    
    // Primeira atração
    std::cout << "1. " << attractions[0]->getName() << "\n";
    std::cout << "   - Tempo de visita: " << attractions[0]->getVisitTime() << " minutos\n";
    std::cout << "   - Custo: R$ " << std::fixed << std::setprecision(COST_PRECISION) 
              << attractions[0]->getCost() << "\n";
    
    int open_hour = attractions[0]->getOpeningTime() / 60;
    int open_min = attractions[0]->getOpeningTime() % 60;
    int close_hour = attractions[0]->getClosingTime() / 60;
    int close_min = attractions[0]->getClosingTime() % 60;
    
    std::cout << "   - Horário: " 
              << std::setfill('0') << std::setw(2) << open_hour << ":" 
              << std::setfill('0') << std::setw(2) << open_min << " - "
              << std::setfill('0') << std::setw(2) << close_hour << ":" 
              << std::setfill('0') << std::setw(2) << close_min << "\n";
    
    // Início do dia às 9:00
    double day_start_time = 9 * 60;
    
    // Linha do tempo completa do roteiro
    std::cout << "\nLinha do Tempo do Roteiro:\n";
    std::cout << utils::Transport::formatTime(day_start_time) << " - Início do dia\n";
    
    // Atrações subsequentes com informações de transporte
    for (size_t i = 1; i < attractions.size(); ++i) {
        const auto* attraction = attractions[i];
        utils::TransportMode mode = transport_modes[i-1];
        
        // Calcula distância e tempo entre atrações
        double distance = utils::Transport::getDistance(
            attractions[i-1]->getName(), attraction->getName(), mode);
        double travel_time = utils::Transport::getTravelTime(
            attractions[i-1]->getName(), attraction->getName(), mode);
        double travel_cost = utils::Transport::getTravelCost(
            attractions[i-1]->getName(), attraction->getName(), mode);
        
        std::cout << "\n" << (i + 1) << ". " << attraction->getName() << "\n";
        std::cout << "   - Transporte: " << utils::Transport::getModeString(mode) << "\n";
        std::cout << "   - Distância: " << std::fixed << std::setprecision(DIST_PRECISION) 
                  << distance << " metros\n";
        std::cout << "   - Tempo de deslocamento: " << std::setprecision(TIME_PRECISION) 
                  << travel_time << " minutos\n";
        std::cout << "   - Custo de transporte: R$ " << std::setprecision(COST_PRECISION) 
                  << travel_cost << "\n";
        std::cout << "   - Tempo de visita: " << attraction->getVisitTime() << " minutos\n";
        std::cout << "   - Custo de entrada: R$ " << std::fixed << std::setprecision(COST_PRECISION) 
                  << attraction->getCost() << "\n";
        
        open_hour = attraction->getOpeningTime() / 60;
        open_min = attraction->getOpeningTime() % 60;
        close_hour = attraction->getClosingTime() / 60;
        close_min = attraction->getClosingTime() % 60;
        
        std::cout << "   - Horário: " 
                  << std::setfill('0') << std::setw(2) << open_hour << ":" 
                  << std::setfill('0') << std::setw(2) << open_min << " - "
                  << std::setfill('0') << std::setw(2) << close_hour << ":" 
                  << std::setfill('0') << std::setw(2) << close_min << "\n";
        
        if (i < time_info.size() && time_info[i].wait_time > 0) {
            std::cout << "   - Tempo de espera: " << time_info[i].wait_time << " minutos\n";
        }
    }
    
    std::cout << utils::Transport::formatTime(day_start_time + route.getTotalTime()) << " - Fim do dia\n";
}

void exportResults(const std::vector<Solution>& solutions, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Erro ao criar arquivo: " + filename);
    }

    file << "Solucao;CustoTotal;TempoTotal;NumAtracoes;HoraInicio;HoraFim;Sequencia;TemposChegada;TemposPartida;ModosTransporte\n";
    
    for (size_t i = 0; i < solutions.size(); ++i) {
        const auto& solution = solutions[i];
        const auto& objectives = solution.getObjectives();
        const auto& route = solution.getRoute();
        const auto& attractions = route.getAttractions();
        const auto& modes = route.getTransportModes();
        const auto& time_info = route.getTimeInfo();
        
        double start_time = 9 * 60; 
        double end_time = start_time + route.getTotalTime();
        
        file << std::fixed << std::setprecision(COST_PRECISION);
        file << (i + 1) << ";";
        file << objectives[0] << ";";
        file << objectives[1] << ";";
        file << std::abs(static_cast<int>(objectives[2])) << ";";
        file << utils::Transport::formatTime(start_time) << ";";
        file << utils::Transport::formatTime(end_time) << ";";
        
        // Sequência de atrações
        for (const auto* attraction : attractions) {
            file << attraction->getName() << "|";
        }
        file << ";";
        
        // Tempos de chegada
        for (size_t j = 0; j < time_info.size(); ++j) {
            file << utils::Transport::formatTime(time_info[j].arrival_time) << "|";
        }
        file << ";";
        
        // Tempos de partida
        for (size_t j = 0; j < time_info.size(); ++j) {
            file << utils::Transport::formatTime(time_info[j].departure_time) << "|";
        }
        file << ";";
        
        // Modos de transporte
        for (const auto& mode : modes) {
            file << utils::Transport::getModeString(mode) << "|";
        }
        file << "\n";
    }
}

int main() {
    std::vector<Solution> solutions; // Define outside the try block

    try {
        std::cout << "\n=== Planejador de Rotas Turísticas Multiobjetivo ===\n\n";
        std::cout << "Carregando dados...\n";
        
        // Encontrar os arquivos de matriz mais recentes
        const std::string osrm_path = "../OSRM/";  // Use relative path from build directory
        const std::string car_dist_file = osrm_path + "matriz_distancias_carro_metros_2025-02-25_11-55-15.csv";
        const std::string walk_dist_file = osrm_path + "matriz_distancias_pe_metros_2025-02-25_11-55-15.csv";
        const std::string car_time_file = osrm_path + "matriz_tempos_carro_min_2025-02-25_11-55-15.csv";
        const std::string walk_time_file = osrm_path + "matriz_tempos_pe_min_2025-02-25_11-55-15.csv";
        // Carregar as matrizes de distância e tempo
        std::cout << "Carregando matrizes de distância e tempo...\n";
        if (!utils::Parser::loadTransportMatrices(car_dist_file, walk_dist_file, car_time_file, walk_time_file)) {
            throw std::runtime_error("Falha ao carregar as matrizes de transporte");
        }
        std::cout << "Matrizes carregadas com sucesso.\n";
        
        // Carregar atrações
        const auto attractions = utils::Parser::loadAttractions("data/attractions.txt");
        
        if (attractions.empty()) {
            throw std::runtime_error("Nenhuma atração carregada de attractions.txt");
        }
        std::cout << "Atrações carregadas: " << attractions.size() << "\n";
        
        // Verificar se todas as atrações estão nas matrizes
        std::cout << "Verificando compatibilidade de atrações com as matrizes...\n";
        for (const auto& attraction : attractions) {
            if (utils::TransportMatrices::attraction_indices.find(attraction.getName()) == 
                utils::TransportMatrices::attraction_indices.end()) {
                std::cout << "AVISO: Atração '" << attraction.getName() 
                          << "' não encontrada nas matrizes de transporte.\n";
            }
        }
        
        std::cout << "\nConfigurando NSGA-II...\n";
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
        std::cout << "Limite de tempo diário: " << utils::Config::DAILY_TIME_LIMIT << " minutos\n";
        std::cout << "Preferência por caminhada: < " << utils::Config::WALK_TIME_PREFERENCE << " minutos\n";
        std::cout << "Custo de carro: R$ " << utils::Config::COST_CAR_PER_KM << " por km\n\n";
        
        std::cout << "Inicializando NSGA-II...\n";
        NSGA2 nsga2(attractions, params);
        std::cout << "NSGA-II inicializado com sucesso\n";
        
        std::cout << "Iniciando otimização...\n";
        const auto start_time = std::chrono::high_resolution_clock::now();
        
        try {
            solutions = nsga2.run(); // Assign inside the try block
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
            
            double cost_ref = utils::Config::TOLERANCE > 0 
                ? 10000.0 * (1.0 + utils::Config::TOLERANCE) 
                : 10000.0;
            double time_ref = utils::Config::DAILY_TIME_LIMIT * (1.0 + utils::Config::TOLERANCE);
            std::vector<double> reference_point = {cost_ref, time_ref, 0.0};
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