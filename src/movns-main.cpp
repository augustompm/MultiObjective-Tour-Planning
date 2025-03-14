// File: movns-main.cpp

#include "movns-algorithm.hpp"
#include "movns-metrics.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <filesystem> // Add this include for filesystem support

using namespace tourist;
using namespace tourist::movns;

int main() {
    try {
        std::cout << "\n=== Multi-Objective Variable Neighborhood Search for Tourist Routing ===\n\n";
        
        // Create results directory if it doesn't exist
        std::filesystem::path results_dir = "../results";
        if (!std::filesystem::exists(results_dir)) {
            std::filesystem::create_directory(results_dir);
        }
        
        // Configure file paths
        const std::string attractions_file = "data/attractions.txt";
        const std::string car_dist_file = "../OSRM/matriz_distancias_carro_metros.csv";
        const std::string walk_dist_file = "../OSRM/matriz_distancias_pe_metros.csv";
        const std::string car_time_file = "../OSRM/matriz_tempos_carro_min.csv";
        const std::string walk_time_file = "../OSRM/matriz_tempos_pe_min.csv";
        std::filesystem::path output_path = results_dir / "movns-resultados.csv";
        std::filesystem::path generations_path = results_dir / "movns-geracoes.csv";
        
        // Load transport matrices
        std::cout << "Loading transport matrices...\n";
        if (!utils::Parser::loadTransportMatrices(car_dist_file, walk_dist_file, car_time_file, walk_time_file)) {
            throw std::runtime_error("Failed to load transport matrices");
        }
        std::cout << "Transport matrices loaded successfully.\n";
        
        // Load attractions
        std::cout << "Loading attractions...\n";
        const auto attractions = utils::Parser::loadAttractions(attractions_file);
        if (attractions.empty()) {
            throw std::runtime_error("No attractions loaded from " + attractions_file);
        }
        std::cout << "Loaded " << attractions.size() << " attractions.\n\n";
        
        // Configure MOVNS parameters
        MOVNS::Parameters params;
        params.max_iterations = 5000;
        params.max_time_seconds = 300; 
        params.max_iterations_no_improvement = 500;
        
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
        
        // Display results
        std::cout << "\n=== Optimization Results ===\n";
        std::cout << "Execution time: " << duration.count() << " seconds\n";
        std::cout << "Non-dominated solutions: " << solutions.size() << "\n\n";
        
        // Export solutions to results directory
        std::cout << "Copying results to the results directory...\n";
        
        // Check if files were created successfully
        if (std::filesystem::exists(output_path)) {
            std::cout << "File " << output_path.string() << " created successfully.\n";
        } else {
            std::cout << "File " << output_path.string() << " not found!\n";
        }
        
        if (std::filesystem::exists(generations_path)) {
            std::cout << "File " << generations_path.string() << " created successfully.\n";
        } else {
            std::cout << "File " << generations_path.string() << " not found!\n";
        }
        
        // Display top solutions
        const size_t num_to_show = std::min(size_t(3), solutions.size());
        std::cout << "\n=== Top " << num_to_show << " Solutions ===\n";
        
        for (size_t i = 0; i < num_to_show; ++i) {
            const auto& sol = solutions[i];
            std::cout << "\nSolution #" << (i + 1) << ":\n";
            std::cout << "  Total Cost: R$ " << sol.getTotalCost() << "\n";
            std::cout << "  Total Time: " << sol.getTotalTime() << " minutes\n";
            std::cout << "  Attractions: " << sol.getNumAttractions() << "\n";
            std::cout << "  Neighborhoods: " << sol.getNumNeighborhoods() << "\n";
            
            // List attractions
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