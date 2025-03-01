#include "utils.hpp"
#include "models.hpp"  // Add this include to get full definition of Solution and Attraction
#include <fstream>
#include <sstream>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace tourist {
namespace utils {

// Inicialização das variáveis estáticas
std::vector<std::vector<double>> TransportMatrices::car_distances;
std::vector<std::vector<double>> TransportMatrices::walk_distances;
std::vector<std::vector<double>> TransportMatrices::car_times;
std::vector<std::vector<double>> TransportMatrices::walk_times;
std::unordered_map<std::string, size_t> TransportMatrices::attraction_indices;
std::vector<std::string> TransportMatrices::attraction_names;
bool TransportMatrices::matrices_loaded = false;

// Implementações da classe Transport
double Transport::getDistance(const std::string& from, const std::string& to, TransportMode mode) {
    if (!TransportMatrices::matrices_loaded) {
        throw std::runtime_error("Transport matrices not loaded. Call loadTransportMatrices first.");
    }
    
    auto from_it = TransportMatrices::attraction_indices.find(from);
    auto to_it = TransportMatrices::attraction_indices.find(to);
    
    if (from_it == TransportMatrices::attraction_indices.end() || 
        to_it == TransportMatrices::attraction_indices.end()) {
        throw std::runtime_error("Attraction not found in transport matrices: " + 
                                (from_it == TransportMatrices::attraction_indices.end() ? from : to));
    }
    
    size_t from_idx = from_it->second;
    size_t to_idx = to_it->second;
    
    if (mode == TransportMode::WALK) {
        return TransportMatrices::walk_distances[from_idx][to_idx];
    } else {
        return TransportMatrices::car_distances[from_idx][to_idx];
    }
}

double Transport::getTravelTime(const std::string& from, const std::string& to, TransportMode mode) {
    if (!TransportMatrices::matrices_loaded) {
        throw std::runtime_error("Transport matrices not loaded. Call loadTransportMatrices first.");
    }
    
    auto from_it = TransportMatrices::attraction_indices.find(from);
    auto to_it = TransportMatrices::attraction_indices.find(to);
    
    if (from_it == TransportMatrices::attraction_indices.end() || 
        to_it == TransportMatrices::attraction_indices.end()) {
        throw std::runtime_error("Attraction not found in transport matrices: " + 
                                (from_it == TransportMatrices::attraction_indices.end() ? from : to));
    }
    
    size_t from_idx = from_it->second;
    size_t to_idx = to_it->second;
    
    if (mode == TransportMode::WALK) {
        return TransportMatrices::walk_times[from_idx][to_idx];
    } else {
        return TransportMatrices::car_times[from_idx][to_idx];
    }
}

double Transport::getTravelCost(const std::string& from, const std::string& to, TransportMode mode) {
    if (mode == TransportMode::WALK) {
        return 0.0; // Caminhada não tem custo
    } else {
        // Para carro, custo = R$6 por km (distância em metros / 1000 * 6)
        double distance_m = getDistance(from, to, TransportMode::CAR);
        double distance_km = distance_m / 1000.0;
        return distance_km * Config::COST_CAR_PER_KM;
    }
}

TransportMode Transport::determinePreferredMode(const std::string& from, const std::string& to) {
    double walk_time = getTravelTime(from, to, TransportMode::WALK);
    
    // Se o tempo de caminhada for menor que o limite de preferência, recomenda caminhada
    if (walk_time < Config::WALK_TIME_PREFERENCE) {
        return TransportMode::WALK;
    } else {
        return TransportMode::CAR;
    }
}

std::string Transport::getModeString(TransportMode mode) {
    return (mode == TransportMode::WALK) ? "Walk" : "Car";
}

// Implementações da classe Metrics
double Metrics::calculateHypervolume(const std::vector<Solution>& solutions,
                                   const std::vector<double>& reference_point) {
    if (solutions.empty()) return 0.0;
    
    double volume = 1.0;
    size_t num_objectives = solutions[0].getObjectives().size();
    
    for (size_t i = 0; i < num_objectives; ++i) {
        double min_val = reference_point[i];
        for (const auto& sol : solutions) {
            min_val = std::min(min_val, sol.getObjectives()[i]);
        }
        volume *= (reference_point[i] - min_val);
    }
    
    return volume;
}

double Metrics::calculateSpread(const std::vector<Solution>& solutions) {
    if (solutions.size() < 2) return 0.0;
    
    std::vector<double> distances;
    distances.reserve(solutions.size() - 1);
    
    for (size_t i = 0; i < solutions.size() - 1; ++i) {
        double dist = 0.0;
        const auto& obj1 = solutions[i].getObjectives();
        const auto& obj2 = solutions[i+1].getObjectives();
        
        for (size_t j = 0; j < obj1.size(); ++j) {
            dist += std::pow(obj1[j] - obj2[j], 2);
        }
        distances.push_back(std::sqrt(dist));
    }
    
    double avg_dist = std::accumulate(distances.begin(), distances.end(), 0.0) / distances.size();
    double spread = 0.0;
    
    for (double dist : distances) {
        spread += std::abs(dist - avg_dist);
    }
    
    return spread / (distances.size() * avg_dist);
}

double Metrics::calculateCoverage(const std::vector<Solution>& solutions1,
                                const std::vector<Solution>& solutions2) {
    if (solutions2.empty()) return 1.0;
    if (solutions1.empty()) return 0.0;
    
    int dominated_count = 0;
    for (const auto& sol2 : solutions2) {
        for (const auto& sol1 : solutions1) {
            if (sol1.dominates(sol2)) {
                dominated_count++;
                break;
            }
        }
    }
    
    return static_cast<double>(dominated_count) / solutions2.size();
}

// Implementações da classe Parser
std::vector<Attraction> Parser::loadAttractions(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open attractions file: " + filename);
    }

    std::vector<Attraction> attractions;
    std::string line;
    
    std::getline(file, line); // Pula a linha de cabeçalho
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        auto parts = split(line, ';');
        if (parts.size() != 6) {
            throw std::runtime_error("Invalid attraction data format");
        }
        
        auto coords = parseCoordinates(parts[1]);
        attractions.emplace_back(
            parts[0],                      // nome
            coords.first,                  // latitude
            coords.second,                 // longitude
            std::stoi(parts[2]),          // tempo de visita
            std::stod(parts[3]),          // custo
            std::stoi(parts[4]),          // horário abertura
            std::stoi(parts[5])           // horário fechamento
        );
    }
    
    return attractions;
}

bool Parser::loadTransportMatrices(const std::string& car_distances_file,
                                 const std::string& walk_distances_file,
                                 const std::string& car_times_file,
                                 const std::string& walk_times_file) {
    try {
        TransportMatrices::car_distances = parseMatrixFile(car_distances_file);
        TransportMatrices::walk_distances = parseMatrixFile(walk_distances_file);
        TransportMatrices::car_times = parseMatrixFile(car_times_file);
        TransportMatrices::walk_times = parseMatrixFile(walk_times_file);
        
        if (TransportMatrices::car_distances.empty() || 
            TransportMatrices::walk_distances.empty() ||
            TransportMatrices::car_times.empty() || 
            TransportMatrices::walk_times.empty()) {
            std::cerr << "Error: One or more matrix files are empty" << std::endl;
            return false;
        }
        
        std::ifstream file(car_distances_file);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << car_distances_file << std::endl;
            return false;
        }
        
        std::string header;
        std::getline(file, header);
        auto attraction_names = split(header, ';');
        
        if (!attraction_names.empty()) {
            attraction_names.erase(attraction_names.begin()); // Remove primeira coluna vazia
        }
        
        TransportMatrices::attraction_names = attraction_names;
        for (size_t i = 0; i < attraction_names.size(); ++i) {
            TransportMatrices::attraction_indices[attraction_names[i]] = i;
        }
        
        TransportMatrices::matrices_loaded = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading matrices: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::vector<double>> Parser::parseMatrixFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open matrix file: " + filename);
    }
    
    std::vector<std::vector<double>> matrix;
    std::string line;
    
    std::getline(file, line); // Pula a linha de cabeçalho
    
    while (std::getline(file, line)) {
        auto parts = split(line, ';');
        
        if (parts.size() <= 1) {
            std::cerr << "Warning: Invalid line in matrix file: " << line << std::endl;
            continue;
        }
        
        std::vector<double> row;
        row.reserve(parts.size() - 1);
        
        for (size_t i = 1; i < parts.size(); ++i) {
            try {
                row.push_back(std::stod(parts[i]));
            } catch (const std::exception&) {
                row.push_back(0.0); // Valor padrão em caso de erro
            }
        }
        
        matrix.push_back(row);
    }
    
    return matrix;
}

std::pair<double, double> Parser::parseCoordinates(const std::string& coords) {
    auto parts = split(coords, ',');
    if (parts.size() != 2) {
        throw std::runtime_error("Invalid coordinates format");
    }
    
    try {
        return {
            std::stod(parts[0]),  // latitude
            std::stod(parts[1])   // longitude
        };
    } catch (const std::exception& e) {
        throw std::runtime_error("Error parsing coordinates: " + std::string(e.what()));
    }
}

std::vector<std::string> Parser::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(s);
    
    while (std::getline(token_stream, token, delimiter)) {
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

} // namespace utils
} // namespace tourist