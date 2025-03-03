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
       << " Start: " << start_time_ << " min"
       << " End: " << end_time_ << " min"
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
    
    // Inicializa as informações temporais
    time_info_.resize(attractions_.size());
    
    // Recalcula as informações temporais
    recalculateTimeInfo();
}

const std::vector<RouteSegment> Route::getSegments() const {
    std::vector<RouteSegment> segments;
    if (attractions_.size() < 2) return segments;
    
    segments.reserve(attractions_.size() - 1);
    for (size_t i = 0; i < attractions_.size() - 1; ++i) {
        RouteSegment segment(attractions_[i], attractions_[i+1], transport_modes_[i]);
        
        // Define informações temporais do segmento
        if (i < time_info_.size()) {
            segment.setStartTime(time_info_[i].departure_time);
            segment.setEndTime(time_info_[i+1].arrival_time);
        }
        
        segments.push_back(segment);
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
    time_info_.resize(attractions_.size());
    
    // Recalcula as informações temporais
    recalculateTimeInfo();
}

void Route::recalculateTimeInfo() {
    if (attractions_.empty()) return;
    
    // Hora de início do dia (9:00 = 540 minutos desde meia-noite)
    double current_time = 9 * 60;
    
    // Processa a primeira atração
    auto& first_time_info = time_info_[0];
    
    // Verifica se a atração está aberta na hora atual
    if (!attractions_[0]->isOpenAt(current_time)) {
        if (current_time < attractions_[0]->getOpeningTime()) {
            // Atração ainda não abriu, espera até a abertura
            double wait_time = attractions_[0]->getOpeningTime() - current_time;
            first_time_info.wait_time = wait_time;
            current_time = attractions_[0]->getOpeningTime();
        }
        // Se já fechou, manteremos a informação para validação posterior
    }
    
    first_time_info.arrival_time = current_time;
    current_time += attractions_[0]->getVisitTime();
    first_time_info.departure_time = current_time;
    
    // Processa as atrações subsequentes
    for (size_t i = 1; i < attractions_.size(); ++i) {
        auto& time_info = time_info_[i];
        
        // Calcula o tempo de deslocamento
        double travel_time = utils::Transport::getTravelTime(
            attractions_[i-1]->getName(),
            attractions_[i]->getName(),
            transport_modes_[i-1]
        );
        
        current_time += travel_time;
        
        // Verifica se a atração está aberta na hora de chegada
        if (!attractions_[i]->isOpenAt(current_time)) {
            if (current_time < attractions_[i]->getOpeningTime()) {
                // Atração ainda não abriu, espera até a abertura
                double wait_time = attractions_[i]->getOpeningTime() - current_time;
                time_info.wait_time = wait_time;
                current_time = attractions_[i]->getOpeningTime();
            }
            // Se já fechou, manteremos a informação para validação posterior
        }
        
        time_info.arrival_time = current_time;
        current_time += attractions_[i]->getVisitTime();
        time_info.departure_time = current_time;
    }
}

double Route::getTotalCost() const {
    double total = 0.0;
    
    // Custo das atrações
    for (const auto* attraction : attractions_) {
        total += attraction->getCost();
    }
    
    // Custo de transporte
    for (size_t i = 0; i < attractions_.size() - 1 && i < transport_modes_.size(); ++i) {
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
    
    // Soma o tempo de visita de todas as atrações
    for (size_t i = 0; i < attractions_.size(); ++i) {
        total_time += attractions_[i]->getVisitTime();
        
        // Adiciona o tempo de espera, se houver
        if (i < time_info_.size()) {
            total_time += time_info_[i].wait_time;
        }
    }
    
    // Soma o tempo de deslocamento entre as atrações
    for (size_t i = 0; i < attractions_.size() - 1 && i < transport_modes_.size(); ++i) {
        total_time += utils::Transport::getTravelTime(
            attractions_[i]->getName(),
            attractions_[i+1]->getName(),
            transport_modes_[i]
        );
    }
    
    return total_time;
}

bool Route::isValid() const {
    return checkTimeConstraints() && checkMaxDailyTime();
}

bool Route::isValidSequence() const {
    if (attractions_.empty()) return true;
    
    // A validade da sequência já é verificada no método recalculateTimeInfo
    // Precisamos apenas confirmar se todas as atrações estão abertas nos horários calculados
    
    for (size_t i = 0; i < attractions_.size(); ++i) {
        if (i < time_info_.size()) {
            double arrival_time = time_info_[i].arrival_time;
            double departure_time = time_info_[i].departure_time;
            
            // Verifica se a atração está aberta na chegada
            if (!attractions_[i]->isOpenAt(static_cast<int>(arrival_time))) {
                return false;
            }
            
            // Verifica se a atração ainda está aberta na saída
            if (!attractions_[i]->isOpenAt(static_cast<int>(departure_time))) {
                return false;
            }
        }
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