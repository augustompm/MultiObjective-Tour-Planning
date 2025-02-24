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

namespace tourist {

class Attraction {
public:
    Attraction(std::string name, double lat, double lon, int visit_time, 
              double cost, int opening_time, int closing_time);

    // Getters
    const std::string& getName() const { return name_; }
    std::pair<double, double> getCoordinates() const { return {latitude_, longitude_}; }
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
        return name_ == other.name_ &&
               std::abs(latitude_ - other.latitude_) < 1e-10 &&
               std::abs(longitude_ - other.longitude_) < 1e-10;
    }

private:
    std::string name_;
    double latitude_, longitude_;
    int visit_time_;        // em minutos
    double cost_;           // em reais
    int opening_time_;      // em minutos desde meia-noite
    int closing_time_;      // em minutos desde meia-noite
};

class Hotel {
public:
    Hotel(std::string name, double daily_rate, double lat, double lon);

    // Getters
    const std::string& getName() const { return name_; }
    double getDailyRate() const { return daily_rate_; }
    std::pair<double, double> getCoordinates() const { return {latitude_, longitude_}; }

    // Identidade
    bool operator==(const Hotel& other) const {
        return name_ == other.name_ &&
               std::abs(daily_rate_ - other.daily_rate_) < 1e-10 &&
               std::abs(latitude_ - other.latitude_) < 1e-10 &&
               std::abs(longitude_ - other.longitude_) < 1e-10;
    }

private:
    std::string name_;
    double daily_rate_;
    double latitude_, longitude_;
};

class Route {
public:
    // Constructores
    Route(const Hotel& hotel);
    Route(const Hotel& hotel, const std::vector<Attraction>& attractions);

    // Getters
    const Hotel& getHotel() const { return hotel_; }
    const std::vector<const Attraction*>& getSequence() const { return sequence_; }
    
    // Operações
    void addAttraction(const Attraction& attraction);
    void clear() { sequence_.clear(); }
    size_t size() const { return sequence_.size(); }
    bool empty() const { return sequence_.empty(); }
    
    // Cálculos
    double getTotalCost() const;   // Custo total incluindo hotel, transporte e atrações
    double getTotalTime() const;   // Tempo total incluindo visitas e deslocamentos
    int getNumAttractions() const { return static_cast<int>(sequence_.size()); }
    
    // Validação
    bool isValid() const;
    bool isValidSequence() const;  // Verifica se a sequência respeita horários e tempo máximo
    
    // Identidade
    bool operator==(const Route& other) const {
        return hotel_ == other.hotel_ &&
               sequence_.size() == other.sequence_.size() &&
               std::equal(sequence_.begin(), sequence_.end(), other.sequence_.begin(),
                        [](const Attraction* a, const Attraction* b) { return *a == *b; });
    }

private:
    const Hotel& hotel_;
    std::vector<const Attraction*> sequence_;

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
        return route_ == other.route_ && objectives_ == other.objectives_;
    }

private:
    Route route_;
    void calculateObjectives();  // Calcula os objetivos baseado na rota
};

} // namespace tourist