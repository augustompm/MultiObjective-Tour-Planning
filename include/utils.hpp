#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <unordered_map>

namespace tourist {

// Forward declarations
class Attraction;
class Solution;

namespace utils {

// Enumeração para modos de transporte
enum class TransportMode {
    WALK,
    CAR
};

// Estrutura para armazenar as matrizes de distância e tempo
struct TransportMatrices {
    static std::vector<std::vector<double>> car_distances;  // em metros
    static std::vector<std::vector<double>> walk_distances; // em metros
    static std::vector<std::vector<double>> car_times;      // em minutos
    static std::vector<std::vector<double>> walk_times;     // em minutos
    static std::unordered_map<std::string, size_t> attraction_indices;
    static std::vector<std::string> attraction_names;
    static bool matrices_loaded;
};

// Configurações globais do sistema
struct Config {
    static constexpr double COST_CAR_PER_KM = 6.0;     // R$6 por km
    static constexpr int DAILY_TIME_LIMIT = 840;       // 14 horas em minutos
    static constexpr int WALK_TIME_PREFERENCE = 15;    // preferência por caminhada abaixo de 15 min
    static constexpr double TOLERANCE = 0.1;     // 10% de tolerância (para penalização)

    struct WeightConfig {
        double total_cost;
        double transport_time;
        double attractions_visited;
    };

    static WeightConfig getBalancedWeights() { return {-2.0, -2.0, 1.5}; }
    static WeightConfig getTimePriorityWeights() { return {-1.5, -3.0, 2.0}; }
    static WeightConfig getCostPriorityWeights() { return {-3.0, -1.5, 1.0}; }
};

// Classe para funções relacionadas a transporte
class Transport {
public:
    // Obtém a distância entre duas atrações usando o modo especificado
    static double getDistance(const std::string& from, const std::string& to, TransportMode mode);
    
    // Obtém o tempo de viagem entre duas atrações usando o modo especificado
    static double getTravelTime(const std::string& from, const std::string& to, TransportMode mode);
    
    // Calcula o custo de transporte entre duas atrações usando o modo especificado
    static double getTravelCost(const std::string& from, const std::string& to, TransportMode mode);
    
    // Determina o modo de transporte recomendado entre duas atrações (baseado na regra de 15 min)
    static TransportMode determinePreferredMode(const std::string& from, const std::string& to);
    
    // Converte o modo de transporte para string para exibição
    static std::string getModeString(TransportMode mode);
    
    // Formata um tempo em minutos para hora e minuto (formato HH:MM)
    static std::string formatTime(double minutes);
};

// Métricas de avaliação
class Metrics {
public:
    // Calcula o hipervolume para um conjunto de soluções
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
    
    // Carrega as matrizes de distância e tempo dos arquivos CSV
    static bool loadTransportMatrices(const std::string& car_distances_file,
                                     const std::string& walk_distances_file,
                                     const std::string& car_times_file,
                                     const std::string& walk_times_file);
    
private:
    static std::pair<double, double> parseCoordinates(const std::string& coords);
    static std::vector<std::string> split(const std::string& s, char delimiter);
    static std::vector<std::vector<double>> parseMatrixFile(const std::string& filename);
};

} // namespace utils
} // namespace tourist