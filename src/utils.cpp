#include "utils.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <numeric>  // para std::accumulate
#include <stdexcept>
#include <algorithm>
#include <cstdlib>

namespace tourist {
namespace utils {

// Implementações da classe Distance
double Distance::calculate(std::pair<double, double> p1, std::pair<double, double> p2) {
    double lat1 = p1.first * DEG_TO_RAD;
    double lon1 = p1.second * DEG_TO_RAD;
    double lat2 = p2.first * DEG_TO_RAD;
    double lon2 = p2.second * DEG_TO_RAD;

    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double a = std::sin(dlat/2) * std::sin(dlat/2) +
               std::cos(lat1) * std::cos(lat2) *
               std::sin(dlon/2) * std::sin(dlon/2);
    
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return EARTH_RADIUS * c;
}

double Distance::calculateTravelTime(double distance, double speed) {
    return (distance / speed) * 60; // Converte para minutos
}

double Distance::calculateTravelCost(double distance) {
    if (distance < 1.0) {
        return 0.0; // A pé
    } else if (distance < 10.0) {
        return Config::COST_BUS; // Ônibus
    } else {
        return (distance / 10.0) * Config::COST_CAR_PER_10KM; // Carro
    }
}

// Implementações da classe Metrics
double Metrics::calculateHypervolume(const std::vector<Solution>& solutions,
                                   const std::vector<double>& reference_point) {
    if (solutions.empty()) return 0.0;
    
    // Implementação do cálculo de hipervolume usando o método de Monte Carlo
    // Para simplificar, usamos apenas os pontos extremos
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
    
    // Calcula a distância média entre soluções consecutivas
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
    
    // Pula a linha de cabeçalho
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        auto parts = split(line, ';');
        if (parts.size() != 6) {
            throw std::runtime_error("Invalid attraction data format");
        }
        
        auto coords = parseCoordinates(parts[1]);
        attractions.emplace_back(
            parts[0],                          // nome
            coords.first,                      // latitude
            coords.second,                     // longitude
            std::stoi(parts[2]),              // tempo de visita
            std::stod(parts[3]),              // custo
            std::stoi(parts[4]),              // horário abertura
            std::stoi(parts[5])               // horário fechamento
        );
    }
    
    return attractions;
}

std::vector<Hotel> Parser::loadHotels(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open hotels file: " + filename);
    }

    std::vector<Hotel> hotels;
    std::string line;
    
    // Pula a linha de cabeçalho
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        auto parts = split(line, ';');
        if (parts.size() != 3) {
            throw std::runtime_error("Invalid hotel data format");
        }
        
        auto coords = parseCoordinates(parts[2]);
        hotels.emplace_back(
            parts[0],                          // nome
            std::stod(parts[1]),              // diária
            coords.first,                      // latitude
            coords.second                      // longitude
        );
    }
    
    return hotels;
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
        // Remove espaços em branco
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