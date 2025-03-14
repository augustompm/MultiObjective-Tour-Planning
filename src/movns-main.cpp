// File: movns-main.cpp

#include "movns-algorithm.hpp"
#include "movns-metrics.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <vector>

using namespace tourist;
using namespace tourist::movns;

int main(int argc, char* argv[]) {
    try {
        std::cout << "\n=== Multi-Objective Variable Neighborhood Search for Tourist Routing ===\n\n";
        
        // Configurações
        const std::string attractions_file = "data/attractions.txt";
        const std::string car_dist_file = "../OSRM/matriz_distancias_carro_metros.csv";
        const std::string walk_dist_file = "../OSRM/matriz_distancias_pe_metros.csv";
        const std::string car_time_file = "../OSRM/matriz_tempos_carro_min.csv";
        const std::string walk_time_file = "../OSRM/matriz_tempos_pe_min.csv";
        const std::string output_file = "../results/movns-resultados.csv";
        
        // Carregar matrizes de distância e tempo
        std::cout << "Loading transport matrices...\n";
        if (!utils::Parser::loadTransportMatrices(car_dist_file, walk_dist_file, car_time_file, walk_time_file)) {
            throw std::runtime_error("Failed to load transport matrices");
        }
        std::cout << "Transport matrices loaded successfully.\n";
        
        // Carregar atrações
        std::cout << "Loading attractions...\n";
        const auto attractions = utils::Parser::loadAttractions(attractions_file);
        if (attractions.empty()) {
            throw std::runtime_error("No attractions loaded from " + attractions_file);
        }
        std::cout << "Loaded " << attractions.size() << " attractions.\n\n";
        
        // Configurar parâmetros do MOVNS
        MOVNS::Parameters params;
        params.max_iterations = 1000;
        params.max_time_seconds = 300; // 5 minutos
        params.max_iterations_no_improvement = 1000;
        
        // Inicializar e executar o algoritmo MOVNS
        std::cout << "=== MOVNS Configuration ===\n";
        std::cout << "Max iterations: " << params.max_iterations << "\n";
        std::cout << "Max time: " << params.max_time_seconds << " seconds\n";
        std::cout << "Max iterations without improvement: " << params.max_iterations_no_improvement << "\n\n";
        
        std::cout << "Starting MOVNS optimization...\n";
        auto start_time = std::chrono::high_resolution_clock::now();
        
        MOVNS movns(attractions, params);
        auto solutions = movns.run();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        // Exibir resultados
        std::cout << "\n=== Optimization Results ===\n";
        std::cout << "Execution time: " << duration.count() << " seconds\n";
        std::cout << "Non-dominated solutions: " << solutions.size() << "\n\n";
        
        // These lines should be commented out since export is now handled in MOVNS::run()
        // std::cout << "Exporting solutions to " << output_file << "...\n";
        // Metrics::exportToCSV(solutions, output_file);
        // std::cout << "Export completed successfully.\n";
        
        // Exibir informações sobre as 3 melhores soluções
        const size_t num_to_show = std::min(size_t(3), solutions.size());
        std::cout << "\n=== Top " << num_to_show << " Solutions ===\n";
        
        for (size_t i = 0; i < num_to_show; ++i) {
            const auto& sol = solutions[i];
            std::cout << "\nSolution #" << (i + 1) << ":\n";
            std::cout << "  Total Cost: R$ " << sol.getTotalCost() << "\n";
            std::cout << "  Total Time: " << sol.getTotalTime() << " minutes\n";
            std::cout << "  Attractions: " << sol.getNumAttractions() << "\n";
            std::cout << "  Neighborhoods: " << sol.getNumNeighborhoods() << "\n";
            
            // Listar atrações
            std::cout << "  Attractions sequence: ";
            for (const auto* attr : sol.getAttractions()) {
                std::cout << attr->getName() << " -> ";
            }
            std::cout << "End\n";
        }
        
        std::cout << "\nMOVNS execution completed successfully.\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}