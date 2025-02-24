// File: src/models.cpp

#include "models.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <algorithm>

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

// Route Implementation
Route::Route() {}

Route::Route(const std::vector<const Attraction*>& attractions)
    : sequence_(attractions) {
}

void Route::addAttraction(const Attraction& attraction) {
    // Store the pointer to the actual Attraction object, not to the reference
    sequence_.push_back(&attraction);
}

double Route::getTotalCost() const {
    double total = 0;
    
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

// Função getTotalTime() modificada conforme solicitado
double Route::getTotalTime() const {
    // Se não houver atrações, retorna 0
    if (sequence_.empty()) return 0.0;
    
    double total_time = 0.0;
    int current_time = 9 * 60; // Início às 9h
    
    // Adiciona o tempo de visita da primeira atração
    if (!sequence_[0]->isOpenAt(current_time)) {
        // Espera até que a primeira atração abra, se necessário
        current_time = std::max(current_time, sequence_[0]->getOpeningTime());
        // Se a atração já estiver fechada, a rota é inválida
        if (current_time > sequence_[0]->getClosingTime()) {
            return utils::Config::DAILY_TIME_LIMIT + 1;
        }
    }
    
    total_time += sequence_[0]->getVisitTime();
    current_time += sequence_[0]->getVisitTime();
    
    // Calcula para as atrações subsequentes
    for (size_t i = 0; i < sequence_.size() - 1; ++i) {
        auto dist = utils::Distance::calculate(
            sequence_[i]->getCoordinates(),
            sequence_[i+1]->getCoordinates()
        );
        
        // Determina a velocidade com base na distância
        double speed = utils::Config::SPEED_CAR;
        if (dist < 1.0) {
            speed = utils::Config::SPEED_WALK;
        } else if (dist < 10.0) {
            // Determina se é horário de pico
            bool is_peak_hour = ((current_time / 60) >= 7 && (current_time / 60) <= 10) || 
                                ((current_time / 60) >= 17 && (current_time / 60) <= 20);
            speed = is_peak_hour ? utils::Config::SPEED_BUS_PEAK : utils::Config::SPEED_BUS_OFFPEAK;
        }
        
        auto travel_time = utils::Distance::calculateTravelTime(dist, speed);
        total_time += travel_time;
        current_time += travel_time;
        
        // Verifica se a próxima atração está aberta na chegada
        if (!sequence_[i+1]->isOpenAt(current_time)) {
            // Se ainda vai abrir, espera
            if (current_time < sequence_[i+1]->getOpeningTime()) {
                int wait_time = sequence_[i+1]->getOpeningTime() - current_time;
                total_time += wait_time;
                current_time += wait_time;
            } else {
                // Se já fechou, a rota é inválida
                return utils::Config::DAILY_TIME_LIMIT + 1;
            }
        }
        
        total_time += sequence_[i+1]->getVisitTime();
        current_time += sequence_[i+1]->getVisitTime();
    }
    
    return total_time;
}

bool Route::isValid() const {
    return checkTimeConstraints() && checkMaxDailyTime();
}

bool Route::isValidSequence() const {
    if (sequence_.empty()) return true;
    
    int current_time = 9 * 60; // Início às 9h
    
    // Verifica se a primeira atração está aberta
    if (!sequence_[0]->isOpenAt(current_time)) {
        return false;
    }
    
    current_time += sequence_[0]->getVisitTime();
    
    for (size_t i = 0; i < sequence_.size() - 1; ++i) {
        auto dist = utils::Distance::calculate(
            sequence_[i]->getCoordinates(),
            sequence_[i+1]->getCoordinates()
        );
        current_time += utils::Distance::calculateTravelTime(dist, utils::Config::SPEED_CAR);
        
        // Verifica horário de funcionamento
        if (!sequence_[i+1]->isOpenAt(current_time)) {
            return false;
        }
        
        current_time += sequence_[i+1]->getVisitTime();
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
