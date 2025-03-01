// File: src/models.cpp

#include "models.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace tourist {

// Implementação da classe Attraction
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

// Implementação da classe RouteSegment
RouteSegment::RouteSegment(const Attraction* from, const Attraction* to, utils::TransportMode mode)
    : from_(from)
    , to_(to)
    , mode_(mode) {
    
    if (from_ == nullptr) {
        throw std::invalid_argument("From attraction cannot be null");
    }
    if (to_ == nullptr) {
        throw std::invalid_argument("To attraction cannot be null");
    }
}

double RouteSegment::getDistance() const {
    return utils::Transport::getDistance(from_->getName(), to_->getName(), mode_);
}

double RouteSegment::getTravelTime() const {
    return utils::Transport::getTravelTime(from_->getName(), to_->getName(), mode_);
}

double RouteSegment::getTravelCost() const {
    return utils::Transport::getTravelCost(from_->getName(), to_->getName(), mode_);
}

std::string RouteSegment::toString() const {
    std::stringstream ss;
    ss << "From: " << from_->getName() 
       << " To: " << to_->getName() 
       << " Mode: " << utils::Transport::getModeString(mode_)
       << " Time: " << getTravelTime() << " min"
       << " Cost: R$" << getTravelCost();
    return ss.str();
}

// Implementação da classe Route
Route::Route() {}

Route::Route(const std::vector<const Attraction*>& attractions)
    : attractions_(attractions) {
    
    // Inicializa os modos de transporte para cada segmento da rota
    if (attractions_.size() > 1) {
        transport_modes_.reserve(attractions_.size() - 1);
        
        for (size_t i = 0; i < attractions_.size() - 1; ++i) {
            // Determina o modo de transporte preferencial para cada segmento
            utils::TransportMode mode = utils::Transport::determinePreferredMode(
                attractions_[i]->getName(), attractions_[i+1]->getName());
            transport_modes_.push_back(mode);
        }
    }
}

const std::vector<RouteSegment> Route::getSegments() const {
    std::vector<RouteSegment> segments;
    if (attractions_.size() < 2) return segments;
    
    segments.reserve(attractions_.size() - 1);
    for (size_t i = 0; i < attractions_.size() - 1; ++i) {
        segments.emplace_back(attractions_[i], attractions_[i+1], transport_modes_[i]);
    }
    
    return segments;
}

void Route::addAttraction(const Attraction& attraction, utils::TransportMode mode) {
    const Attraction* attr_ptr = &attraction;
    
    if (!attractions_.empty()) {
        const Attraction* prev_attr = attractions_.back();
        
        // Se o modo não foi especificado, determina o modo preferencial
        if (mode == utils::TransportMode::CAR) {
            mode = utils::Transport::determinePreferredMode(
                prev_attr->getName(), attr_ptr->getName());
        }
        
        transport_modes_.push_back(mode);
    }
    
    attractions_.push_back(attr_ptr);
}

double Route::getTotalCost() const {
    double total = 0.0;
    
    // Custo das atrações
    for (const auto* attraction : attractions_) {
        total += attraction->getCost();
    }
    
    // Custo de transporte
    for (size_t i = 0; i < attractions_.size() - 1; ++i) {
        total += utils::Transport::getTravelCost(
            attractions_[i]->getName(),
            attractions_[i+1]->getName(),
            transport_modes_[i]
        );
    }
    
    return total;
}

double Route::getTotalTime() const {
    if (attractions_.empty()) return 0.0;
    
    double total_time = 0.0;
    int current_time = 9 * 60; // Início às 9h
    
    // Adiciona o tempo de visita da primeira atração
    if (!attractions_[0]->isOpenAt(current_time)) {
        // Espera até que a primeira atração abra, se necessário
        current_time = std::max(current_time, attractions_[0]->getOpeningTime());
        // Se a atração já estiver fechada, a rota é inválida
        if (current_time > attractions_[0]->getClosingTime()) {
            return utils::Config::DAILY_TIME_LIMIT + 1; // Retorna tempo inválido
        }
    }
    
    total_time += attractions_[0]->getVisitTime();
    current_time += attractions_[0]->getVisitTime();
    
    // Calcula para as atrações subsequentes
    for (size_t i = 0; i < attractions_.size() - 1; ++i) {
        double travel_time = utils::Transport::getTravelTime(
            attractions_[i]->getName(),
            attractions_[i+1]->getName(),
            transport_modes_[i]
        );
        
        total_time += travel_time;
        current_time += travel_time;
        
        // Verifica se a próxima atração está aberta na chegada
        if (!attractions_[i+1]->isOpenAt(current_time)) {
            // Se ainda vai abrir, espera
            if (current_time < attractions_[i+1]->getOpeningTime()) {
                int wait_time = attractions_[i+1]->getOpeningTime() - current_time;
                total_time += wait_time;
                current_time += wait_time;
            } else {
                // Se já fechou, a rota é inválida
                return utils::Config::DAILY_TIME_LIMIT + 1; // Retorna tempo inválido
            }
        }
        
        total_time += attractions_[i+1]->getVisitTime();
        current_time += attractions_[i+1]->getVisitTime();
    }
    
    return total_time;
}

bool Route::isValid() const {
    return checkTimeConstraints() && checkMaxDailyTime();
}

bool Route::isValidSequence() const {
    if (attractions_.empty()) return true;
    
    int current_time = 9 * 60; // Início às 9h
    
    // Verifica se a primeira atração está aberta
    if (!attractions_[0]->isOpenAt(current_time)) {
        if (current_time < attractions_[0]->getOpeningTime()) {
            // Pode esperar até a abertura
            current_time = attractions_[0]->getOpeningTime();
        } else {
            // Já está fechada
            return false;
        }
    }
    
    current_time += attractions_[0]->getVisitTime();
    
    for (size_t i = 0; i < attractions_.size() - 1; ++i) {
        // Adiciona tempo de viagem
        double travel_time = utils::Transport::getTravelTime(
            attractions_[i]->getName(),
            attractions_[i+1]->getName(),
            transport_modes_[i]
        );
        current_time += travel_time;
        
        // Verifica horário de funcionamento
        if (!attractions_[i+1]->isOpenAt(current_time)) {
            if (current_time < attractions_[i+1]->getOpeningTime()) {
                // Pode esperar até a abertura
                current_time = attractions_[i+1]->getOpeningTime();
            } else {
                // Já está fechada
                return false;
            }
        }
        
        current_time += attractions_[i+1]->getVisitTime();
    }
    
    return true;
}

bool Route::checkTimeConstraints() const {
    return isValidSequence();
}

bool Route::checkMaxDailyTime() const {
    return getTotalTime() <= utils::Config::DAILY_TIME_LIMIT;
}

// Implementação da classe Solution
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
    double total_time = route_.getTotalTime();
    double time_penalty = 0.0;
    
    // Aplica penalidade para rotas que ultrapassam o limite diário
    if (total_time > utils::Config::DAILY_TIME_LIMIT) {
        // Penalização forte: 10x o tempo excedente
        time_penalty = (total_time - utils::Config::DAILY_TIME_LIMIT) * 10.0;
    }
    
    objectives_ = {
        route_.getTotalCost(),                     // Minimizar custo total
        total_time + time_penalty,                 // Minimizar tempo total (com penalidade)
        -static_cast<double>(route_.getNumAttractions())  // Maximizar número de atrações
    };
}

} // namespace tourist