// File: movns-solution.hpp

#pragma once

#include "models.hpp"  // Para usar a classe 'Attraction' existente no NSGA-II
#include "base.hpp"    // Para herdar de SolutionBase se necessário
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

namespace tourist {

/**
 * @class MOVNSSolution
 * @brief Representa uma solução para o problema de roteirização turística no contexto do MOVNS
 */
class MOVNSSolution {
public:
    // Construtores
    MOVNSSolution();
    explicit MOVNSSolution(const std::vector<const Attraction*>& attractions);
    
    // Getters e Setters
    const std::vector<const Attraction*>& getAttractions() const;
    const std::vector<utils::TransportMode>& getTransportModes() const;
    
    // Métodos para manipulação da solução
    void addAttraction(const Attraction& attraction, utils::TransportMode mode = utils::TransportMode::CAR);
    void removeAttraction(size_t index);
    void swapAttractions(size_t index1, size_t index2);
    void insertAttraction(const Attraction& attraction, size_t position, utils::TransportMode mode = utils::TransportMode::CAR);
    
    // Cálculos de objetivos
    double getTotalCost() const;
    double getTotalTime() const;
    int getNumAttractions() const;
    int getNumNeighborhoods() const;
    std::vector<double> getObjectives() const;
    
    // Validação
    bool isValid() const;
    bool respectsTimeLimit() const;
    bool respectsWalkingLimit() const;
    
    // Conversão para string no formato exigido
    std::string toString() const;
    
    // Operadores de comparação
    bool dominates(const MOVNSSolution& other) const;
    bool operator==(const MOVNSSolution& other) const;

private:
    std::vector<const Attraction*> attractions_;
    std::vector<utils::TransportMode> transport_modes_;
    
    // Informações temporais
    struct TimeInfo {
        double arrival_time;
        double departure_time;
        double wait_time;
    };
    std::vector<TimeInfo> time_info_;
    
    // Métodos privados
    void recalculateTimeInfo();
    bool checkTimeConstraints() const;
};

} // namespace tourist