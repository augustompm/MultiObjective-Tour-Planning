// File: include/models.hpp

#pragma once

#include "base.hpp"
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <stdexcept>
#include <cmath>
#include <algorithm>

// Include utils.hpp to get TransportMode enum
#include "utils.hpp"

namespace tourist {

// Forward declarations to resolve circular dependencies
class Solution;
class Route;

class Attraction {
public:
    Attraction(std::string name, std::string neighborhood, double lat, double lon, int visit_time, 
              double cost, int opening_time, int closing_time);

    // Getters
    const std::string& getName() const { return name_; }
    const std::string& getNeighborhood() const { return neighborhood_; }  // Added getter
    double getLatitude() const { return latitude_; }
    double getLongitude() const { return longitude_; }
    int getVisitTime() const { return visit_time_; }
    double getCost() const { return cost_; }
    int getOpeningTime() const { return opening_time_; }
    int getClosingTime() const { return closing_time_; }
    
    // Validação
    bool isOpenAt(int time) const {
        if (time < 0 || time >= 24*60) return false;
        // Caso especial para atrações 24h
        if (opening_time_ == 0 && closing_time_ == 1439) return true;
        return time >= opening_time_ && time <= closing_time_;
    }

    // Identidade
    bool operator==(const Attraction& other) const {
        return name_ == other.name_;
    }

private:
    std::string name_;
    std::string neighborhood_;  // Added neighborhood field
    double latitude_, longitude_;
    int visit_time_;        
    double cost_;           
    int opening_time_;      
    int closing_time_;      // em minutos desde meia-noite
};

// Classe para representar um segmento da rota
class RouteSegment {
public:
    RouteSegment(const Attraction* from, const Attraction* to, utils::TransportMode mode);
    
    // Getters
    const Attraction* getFromAttraction() const { return from_; }
    const Attraction* getToAttraction() const { return to_; }
    utils::TransportMode getTransportMode() const { return mode_; }
    
    // Valores temporais adicionais (compatíveis com a nova implementação do NSGA-II)
    void setStartTime(double start_time) { start_time_ = start_time; }
    void setEndTime(double end_time) { end_time_ = end_time; }
    double getStartTime() const { return start_time_; }
    double getEndTime() const { return end_time_; }
    
    // Cálculos
    double getDistance() const;
    double getTravelTime() const;
    double getTravelCost() const;
    
    // Representação em string
    std::string toString() const;

private:
    const Attraction* from_;
    const Attraction* to_;
    utils::TransportMode mode_;
    double start_time_{0.0};  // Hora de início do deslocamento
    double end_time_{0.0};    // Hora de chegada ao destino
};

class Route {
public:
    // Estrutura auxiliar para armazenar informações temporais de cada atração
    struct AttractionTimeInfo {
        double arrival_time;      // Tempo de chegada à atração
        double departure_time;    // Tempo de saída da atração
        double wait_time;         // Tempo de espera até a abertura
        
        AttractionTimeInfo() : arrival_time(0.0), departure_time(0.0), wait_time(0.0) {}
    };
    
    // Constructores
    Route();
    explicit Route(const std::vector<const Attraction*>& attractions);

    // Getters
    const std::vector<const Attraction*>& getAttractions() const { return attractions_; }
    const std::vector<utils::TransportMode>& getTransportModes() const { return transport_modes_; }
    const std::vector<RouteSegment> getSegments() const;
    const std::vector<AttractionTimeInfo>& getTimeInfo() const { return time_info_; }
    
    // Operações
    void addAttraction(const Attraction& attraction, utils::TransportMode mode = utils::TransportMode::CAR);
    void clear() { 
        attractions_.clear(); 
        transport_modes_.clear();
        time_info_.clear();
    }
    size_t size() const { return attractions_.size(); }
    bool empty() const { return attractions_.empty(); }
    
    // Cálculos
    double getTotalCost() const;   // Custo total incluindo transporte e atrações
    double getTotalTime() const;   // Tempo total incluindo visitas e deslocamentos
    int getNumAttractions() const { return static_cast<int>(attractions_.size()); }
    
    // Recálculo de informações temporais
    void recalculateTimeInfo();
    
    // Validação
    bool isValid() const;
    bool isValidSequence() const;  // Verifica se a sequência respeita horários e tempo máximo
    
    // Identidade
    bool operator==(const Route& other) const {
        return attractions_.size() == other.attractions_.size() &&
               std::equal(attractions_.begin(), attractions_.end(), other.attractions_.begin(),
                        [](const Attraction* a, const Attraction* b) { return *a == *b; }) &&
               transport_modes_ == other.transport_modes_;
    }

private:
    std::vector<const Attraction*> attractions_;
    std::vector<utils::TransportMode> transport_modes_;
    std::vector<AttractionTimeInfo> time_info_;  // Informações temporais de cada atração

    bool checkTimeConstraints() const;
    bool checkMaxDailyTime() const;
};

class Solution : public SolutionBase {
public:
    // Constructores
    explicit Solution(const Route& route);
    Solution(const Solution& other) = default;
    Solution& operator=(const Solution& other) = default;
    
    // Interface SolutionBase
    std::vector<double> getObjectives() const override;
    bool dominates(const SolutionBase& other) const override;
    
    // Getters específicos
    const Route& getRoute() const { return route_; }
    
    // Identidade
    bool operator==(const Solution& other) const {
        return route_ == other.route_ && getObjectives() == other.getObjectives();
    }

private:
    Route route_;
    std::vector<double> objectives_;
    void calculateObjectives();  // Calcula os objetivos baseado na rota
};

} // namespace tourist