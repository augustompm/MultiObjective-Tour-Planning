#pragma once

#include "models.hpp"
#include <string>
#include <vector>
#include <cmath>

namespace tourist {
namespace utils {

// Configurações globais do sistema
struct Config {
    static constexpr double COST_CAR_PER_10KM = 6.0;
    static constexpr double COST_BUS = 4.30;
    static constexpr double SPEED_CAR = 40.0;
    static constexpr double SPEED_BUS_PEAK = 20.0;
    static constexpr double SPEED_BUS_OFFPEAK = 30.0;
    static constexpr double SPEED_WALK = 5.0;
    static constexpr int DAILY_TIME_LIMIT = 12 * 60; // 12 horas em minutos

    struct WeightConfig {
        double total_cost;
        double transport_time;
        double attractions_visited;
    };

    static WeightConfig getBalancedWeights() { return {-2.0, -2.0, 1.5}; }
    static WeightConfig getTimePriorityWeights() { return {-1.5, -3.0, 2.0}; }
    static WeightConfig getCostPriorityWeights() { return {-3.0, -1.5, 1.0}; }
};

// Cálculos de distância e tempo
class Distance {
public:
    // Calcula distância em km entre dois pontos usando fórmula de Haversine
    static double calculate(std::pair<double, double> p1, std::pair<double, double> p2);
    
    // Calcula tempo de viagem baseado na distância e modo de transporte
    static double calculateTravelTime(double distance, double speed);
    
    // Calcula custo de transporte baseado na distância
    static double calculateTravelCost(double distance);

private:
    static constexpr double EARTH_RADIUS = 6371.0; // Raio da Terra em km
    static constexpr double DEG_TO_RAD = M_PI / 180.0;
};

// Métricas de avaliação
class Metrics {
public:
    // Calcula o hipervolume de um conjunto de soluções
    static double calculateHypervolume(const std::vector<Solution>& solutions, 
                                     const std::vector<double>& reference_point);
    
    // Calcula o spread (diversidade) das soluções
    static double calculateSpread(const std::vector<Solution>& solutions);
    
    // Calcula a cobertura (coverage) entre dois conjuntos de soluções
    static double calculateCoverage(const std::vector<Solution>& solutions1,
                                  const std::vector<Solution>& solutions2);
};

// Parser de arquivos
class Parser {
public:
    // Carrega dados das atrações do arquivo
    static std::vector<Attraction> loadAttractions(const std::string& filename);
    
    // Carrega dados dos hotéis do arquivo
    static std::vector<Hotel> loadHotels(const std::string& filename);

private:
    static std::pair<double, double> parseCoordinates(const std::string& coords);
    static std::vector<std::string> split(const std::string& s, char delimiter);
};

} // namespace utils
} // namespace tourist