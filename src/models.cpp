// File: src/models.cpp

#include "models.hpp"
#include "utils.hpp"
#include <stdexcept>

namespace tourist {

// Attraction Implementation
Attraction::Attraction(std::string name, double lat, double lon, int visit_time, 
                     double cost, int opening_time, int closing_time)
    : name_(std::move(name))
    , latitude_(lat)
    , longitude_(lon)
    , visit_time_(visit_time)
    , cost_(cost)
    , opening_time_(opening_time)
    , closing_time_(closing_time) {
    
    if (visit_time_ < 0) {
        throw std::invalid_argument("Visit time cannot be negative");
    }
    if (cost_ < 0) {
        throw std::invalid_argument("Cost cannot be negative");
    }
    if (opening_time_ < 0 || opening_time_ >= 24*60) {
        throw std::invalid_argument("Invalid opening time");
    }
    if (closing_time_ < 0 || closing_time_ >= 24*60) {
        throw std::invalid_argument("Invalid closing time");
    }
}

// Hotel Implementation
Hotel::Hotel(std::string name, double daily_rate, double lat, double lon)
    : name_(std::move(name))
    , daily_rate_(daily_rate)
    , latitude_(lat)
    , longitude_(lon) {
    
    if (daily_rate_ < 0) {
        throw std::invalid_argument("Daily rate cannot be negative");
    }
}

// Route Implementation
Route::Route(const Hotel& hotel) 
    : hotel_(hotel) {
}

Route::Route(const Hotel& hotel, const std::vector<Attraction>& attractions)
    : hotel_(hotel) {
    for (const auto& attraction : attractions) {
        addAttraction(attraction);
    }
}

// Fix to prevent storing pointers to temporary references
void Route::addAttraction(const Attraction& attraction) {
    // Store the pointer to the actual Attraction object, not to the reference
    sequence_.push_back(&attraction);
}

double Route::getTotalCost() const {
    double total = hotel_.getDailyRate() * 2; // 2 dias
    
    // Custo das atrações
    for (const auto* attraction : sequence_) {
        total += attraction->getCost();
    }
    
    // Custo de transporte
    for (size_t i = 0; i < sequence_.size() - 1; ++i) {
        auto dist = utils::Distance::calculate(
            sequence_[i]->getCoordinates(),
            sequence_[i+1]->getCoordinates()
        );
        total += utils::Distance::calculateTravelCost(dist);
    }
    
    return total;
}

double Route::getTotalTime() const {
    double total = 0;
    int current_time = 9 * 60; // Início às 9h
    
    for (size_t i = 0; i < sequence_.size(); ++i) {
        // Tempo de viagem
        if (i > 0) {
            auto dist = utils::Distance::calculate(
                sequence_[i-1]->getCoordinates(),
                sequence_[i]->getCoordinates()
            );
            auto travel_time = utils::Distance::calculateTravelTime(dist, utils::Config::SPEED_CAR);
            total += travel_time;
            current_time += travel_time;
        }
        
        // Verifica se atração está aberta
        if (!sequence_[i]->isOpenAt(current_time)) {
            return utils::Config::DAILY_TIME_LIMIT + 1; // Rota inválida
        }
        
        // Adiciona tempo de visita
        total += sequence_[i]->getVisitTime();
        current_time += sequence_[i]->getVisitTime();
    }
    
    return total;
}

bool Route::isValid() const {
    return checkTimeConstraints() && checkMaxDailyTime();
}

bool Route::isValidSequence() const {
    if (sequence_.empty()) return true;
    
    int current_time = 9 * 60; // Início às 9h
    
    for (size_t i = 0; i < sequence_.size(); ++i) {
        // Calcula tempo de chegada
        if (i > 0) {
            auto dist = utils::Distance::calculate(
                sequence_[i-1]->getCoordinates(),
                sequence_[i]->getCoordinates()
            );
            current_time += utils::Distance::calculateTravelTime(dist, utils::Config::SPEED_CAR);
        }
        
        // Verifica horário de funcionamento
        if (!sequence_[i]->isOpenAt(current_time)) {
            return false;
        }
        
        current_time += sequence_[i]->getVisitTime();
    }
    
    return true;
}

bool Route::checkTimeConstraints() const {
    return isValidSequence();
}

bool Route::checkMaxDailyTime() const {
    return getTotalTime() <= utils::Config::DAILY_TIME_LIMIT;
}

// Solution Implementation
Solution::Solution(const Route& route)
    : route_(route) {
    calculateObjectives();
}

std::vector<double> Solution::getObjectives() const {
    return objectives_;
}

bool Solution::dominates(const SolutionBase& other) const {
    return isDominatedBy(other.getObjectives());
}

void Solution::calculateObjectives() {
    objectives_ = {
        route_.getTotalCost(),
        route_.getTotalTime(),
        -static_cast<double>(route_.getNumAttractions())
    };
}

} // namespace tourist