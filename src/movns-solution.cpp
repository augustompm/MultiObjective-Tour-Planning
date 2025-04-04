// File: src/movns-solution.cpp

#include "movns-solution.hpp"
#include "utils.hpp"
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <iomanip>
#include <cmath>

namespace tourist {

MOVNSSolution::MOVNSSolution() {}

MOVNSSolution::MOVNSSolution(const std::vector<const Attraction*>& attractions) 
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

const std::vector<const Attraction*>& MOVNSSolution::getAttractions() const {
    return attractions_;
}

const std::vector<utils::TransportMode>& MOVNSSolution::getTransportModes() const {
    return transport_modes_;
}

void MOVNSSolution::addAttraction(const Attraction& attraction, utils::TransportMode mode) {
    const Attraction* attr_ptr = &attraction;
    
    // Check if attraction is already in the solution
    auto it = std::find_if(attractions_.begin(), attractions_.end(),
                          [attr_ptr](const Attraction* existing) {
                              return existing->getName() == attr_ptr->getName();
                          });
    
    // If attraction already exists, don't add it again
    if (it != attractions_.end()) {
        return;
    }
    
    if (!attractions_.empty()) {
        const Attraction* prev_attr = attractions_.back();
        
        // Se o modo não foi especificado, determina o modo preferencial
        if (mode == utils::TransportMode::CAR) { // Changed from UNDEFINED to CAR
            mode = utils::Transport::determinePreferredMode(
                prev_attr->getName(), attr_ptr->getName());
            
            // For long distances, force car use
            double travel_time = utils::Transport::getTravelTime(
                prev_attr->getName(), attr_ptr->getName(), utils::TransportMode::WALK);
                
            if (travel_time > utils::Config::WALK_TIME_PREFERENCE) {
                mode = utils::TransportMode::CAR;
            }
        }
        
        transport_modes_.push_back(mode);
    }
    
    attractions_.push_back(attr_ptr);
    time_info_.resize(attractions_.size());
    
    // Recalcula as informações temporais
    recalculateTimeInfo();
}

void MOVNSSolution::removeAttraction(size_t index) {
    if (index >= attractions_.size()) {
        throw std::out_of_range("Index out of range");
    }
    
    attractions_.erase(attractions_.begin() + index);
    
    // Ajusta os modos de transporte
    if (index == 0 && !attractions_.empty()) {
        // Removeu a primeira atração
        transport_modes_.erase(transport_modes_.begin());
    } else if (index == attractions_.size() && !transport_modes_.empty()) {
        // Removeu a última atração
        transport_modes_.pop_back();
    } else if (!transport_modes_.empty()) {
        // Removeu uma atração do meio
        if (index > 0 && index <= transport_modes_.size()) {
            // Combine os modos de transporte anterior e posterior
            utils::TransportMode newMode = utils::Transport::determinePreferredMode(
                attractions_[index-1]->getName(), 
                index < attractions_.size() ? attractions_[index]->getName() : attractions_[index-1]->getName());
                
            transport_modes_.erase(transport_modes_.begin() + index - 1);
            if (index <= transport_modes_.size()) {
                transport_modes_[index-1] = newMode;
            }
        }
    }
    
    time_info_.resize(attractions_.size());
    recalculateTimeInfo();
}

void MOVNSSolution::swapAttractions(size_t index1, size_t index2) {
    if (index1 >= attractions_.size() || index2 >= attractions_.size()) {
        throw std::out_of_range("Index out of range");
    }
    
    // Troca as atrações
    std::swap(attractions_[index1], attractions_[index2]);
    
    // Atualiza modos de transporte afetados
    std::vector<size_t> affected_indices;
    if (index1 > 0) affected_indices.push_back(index1 - 1);
    if (index1 < transport_modes_.size()) affected_indices.push_back(index1);
    if (index2 > 0 && index2 - 1 != index1 && index2 - 1 != index1 - 1) 
        affected_indices.push_back(index2 - 1);
    if (index2 < transport_modes_.size() && index2 != index1 && index2 != index1 + 1) 
        affected_indices.push_back(index2);
    
    // Atualiza os modos de transporte afetados
    for (size_t idx : affected_indices) {
        transport_modes_[idx] = utils::Transport::determinePreferredMode(
            attractions_[idx]->getName(), attractions_[idx+1]->getName());
    }
    
    recalculateTimeInfo();
}

void MOVNSSolution::insertAttraction(const Attraction& attraction, size_t position, utils::TransportMode /* mode */) {
    if (position > attractions_.size()) {
        throw std::out_of_range("Position out of range");
    }
    
    const Attraction* attr_ptr = &attraction;
    
    // Check if attraction is already in the solution
    auto it = std::find_if(attractions_.begin(), attractions_.end(),
                          [attr_ptr](const Attraction* existing) {
                              return existing->getName() == attr_ptr->getName();
                          });
    
    // If attraction already exists, don't add it again
    if (it != attractions_.end()) {
        return;
    }
    
    // Insere a atração na posição especificada
    attractions_.insert(attractions_.begin() + position, attr_ptr);
    
    // Atualiza os modos de transporte
    if (attractions_.size() > 1) {
        if (position == 0) {
            // Inserção no início
            utils::TransportMode newMode = utils::Transport::determinePreferredMode(
                attr_ptr->getName(), attractions_[1]->getName());
                
            // For long distances, force car use
            double travel_time = utils::Transport::getTravelTime(
                attr_ptr->getName(), attractions_[1]->getName(), utils::TransportMode::WALK);
                
            if (travel_time > utils::Config::WALK_TIME_PREFERENCE) {
                newMode = utils::TransportMode::CAR;
            }
                
            transport_modes_.insert(transport_modes_.begin(), newMode);
        } else if (position == attractions_.size() - 1) {
            // Inserção no final
            utils::TransportMode newMode = utils::Transport::determinePreferredMode(
                attractions_[position-1]->getName(), attr_ptr->getName());
                
            // For long distances, force car use
            double travel_time = utils::Transport::getTravelTime(
                attractions_[position-1]->getName(), attr_ptr->getName(), utils::TransportMode::WALK);
                
            if (travel_time > utils::Config::WALK_TIME_PREFERENCE) {
                newMode = utils::TransportMode::CAR;
            }
                
            transport_modes_.push_back(newMode);
        } else {
            // Inserção no meio
            utils::TransportMode beforeMode = utils::Transport::determinePreferredMode(
                attractions_[position-1]->getName(), attr_ptr->getName());
                
            // For long distances, force car use
            double travel_time_before = utils::Transport::getTravelTime(
                attractions_[position-1]->getName(), attr_ptr->getName(), utils::TransportMode::WALK);
                
            if (travel_time_before > utils::Config::WALK_TIME_PREFERENCE) {
                beforeMode = utils::TransportMode::CAR;
            }
                
            utils::TransportMode afterMode = utils::Transport::determinePreferredMode(
                attr_ptr->getName(), attractions_[position+1]->getName());
                
            // For long distances, force car use
            double travel_time_after = utils::Transport::getTravelTime(
                attr_ptr->getName(), attractions_[position+1]->getName(), utils::TransportMode::WALK);
                
            if (travel_time_after > utils::Config::WALK_TIME_PREFERENCE) {
                afterMode = utils::TransportMode::CAR;
            }
                
            // Atualiza o modo anterior
            if (position - 1 < transport_modes_.size()) {
                transport_modes_[position-1] = beforeMode;
            }
            
            // Insere novo modo
            transport_modes_.insert(transport_modes_.begin() + position, afterMode);
        }
    }
    
    time_info_.resize(attractions_.size());
    recalculateTimeInfo();
}

double MOVNSSolution::getTotalCost() const {
    double total = 0.0;
    
    // Custo das atrações
    for (const auto* attraction : attractions_) {
        double attraction_cost = attraction->getCost();
        // Ensure the cost is at least 0 - protect against negative costs
        total += std::max(0.0, attraction_cost);
    }
    
    // Custo de transporte
    for (size_t i = 0; i < attractions_.size() - 1 && i < transport_modes_.size(); ++i) {
        double travel_cost = utils::Transport::getTravelCost(
            attractions_[i]->getName(),
            attractions_[i+1]->getName(),
            transport_modes_[i]
        );
        
        // Ensure travel cost is at least 0 - protect against negative costs
        total += std::max(0.0, travel_cost);
    }
    
    return total;
}

double MOVNSSolution::getTotalTime() const {
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

int MOVNSSolution::getNumAttractions() const {
    return static_cast<int>(attractions_.size());
}

int MOVNSSolution::getNumNeighborhoods() const {
    std::unordered_set<std::string> neighborhoods;
    for (const auto* attraction : attractions_) {
        neighborhoods.insert(attraction->getNeighborhood());
    }
    return static_cast<int>(neighborhoods.size());
}

std::vector<double> MOVNSSolution::getObjectives() const {
    double total_time = getTotalTime();
    double time_penalty = 0.0;
    
    // Aplica penalidade para rotas que ultrapassam o limite diário
    double max_time = utils::Config::DAILY_TIME_LIMIT * (1.0 + utils::Config::TOLERANCE);
    if (total_time > max_time) {
        double violation = total_time - max_time;
        time_penalty = violation * (1.0 + violation / max_time);
    }
    
    // Count unique neighborhoods
    std::unordered_set<std::string> neighborhoods;
    for (const auto* attraction : attractions_) {
        neighborhoods.insert(attraction->getNeighborhood());
    }
    
    // Get the cost, ensuring it's at least 0
    double cost = std::max(0.0, getTotalCost());
    
    return {
        cost,                                 // Minimizar custo total
        total_time + time_penalty,           // Minimizar tempo total (com penalidade)
        -static_cast<double>(getNumAttractions()),  // Maximizar número de atrações
        -static_cast<double>(neighborhoods.size())  // Maximizar número de bairros
    };
}

bool MOVNSSolution::isValid() const {
    // If empty, not valid
    if (attractions_.empty()) {
        return false;
    }
    
    // Check for duplicate attractions
    std::unordered_set<std::string> seen_attractions;
    for (const auto* attraction : attractions_) {
        if (!seen_attractions.insert(attraction->getName()).second) {
            return false; // Found a duplicate
        }
    }
    
    // Include the time limit check to prevent constraint violations
    return checkTimeConstraints() && respectsWalkingLimit() && respectsTimeLimit();
}

bool MOVNSSolution::respectsTimeLimit() const {
    // Daily time limit constraint (840 minutes)
    return getTotalTime() <= utils::Config::DAILY_TIME_LIMIT;
}

bool MOVNSSolution::respectsWalkingLimit() const {
    // Walking time constraint (15 minutes maximum)
    for (size_t i = 0; i < attractions_.size() - 1 && i < transport_modes_.size(); ++i) {
        if (transport_modes_[i] == utils::TransportMode::WALK) {
            double travel_time = utils::Transport::getTravelTime(
                attractions_[i]->getName(),
                attractions_[i+1]->getName(),
                utils::TransportMode::WALK
            );
            
            if (travel_time > utils::Config::WALK_TIME_PREFERENCE) {
                return false;
            }
        }
    }
    return true;
}

bool MOVNSSolution::checkTimeConstraints() const {
    if (attractions_.empty()) return true;
    
    // Relax the constraint check to allow more solutions during exploration
    // We'll only enforce attraction visit times and opening hours
    
    // Verify attraction opening/closing times
    for (size_t i = 0; i < attractions_.size(); ++i) {
        if (i < time_info_.size()) {
            double arrival_time = time_info_[i].arrival_time;
            double departure_time = time_info_[i].departure_time;
            
            // Check if attraction is open at arrival time
            if (!attractions_[i]->isOpenAt(static_cast<int>(arrival_time))) {
                return false;
            }
            
            // Check if attraction is still open at departure time
            if (!attractions_[i]->isOpenAt(static_cast<int>(departure_time))) {
                return false;
            }
            
            // Check if visit duration is respected
            double visit_duration = departure_time - arrival_time - time_info_[i].wait_time;
            if (std::abs(visit_duration - attractions_[i]->getVisitTime()) > 1.0) { // Allow small tolerance
                return false;
            }
        }
    }
    
    return true;
}

bool MOVNSSolution::dominates(const MOVNSSolution& other) const {
    const auto& self_obj = getObjectives();
    const auto& other_obj = other.getObjectives();
    
    // Verificação de dominância: melhor ou igual em todos e estritamente melhor em pelo menos um
    bool at_least_one_better = false;
    
    for (size_t i = 0; i < self_obj.size(); ++i) {
        if (self_obj[i] > other_obj[i]) { // Para objetivos que queremos minimizar
            return false;
        }
        if (self_obj[i] < other_obj[i]) {
            at_least_one_better = true;
        }
    }
    
    return at_least_one_better;
}

bool MOVNSSolution::operator==(const MOVNSSolution& other) const {
    // Alterar a comparação para verificar se as sequências são idênticas (incluindo a ordem)
    // em vez de apenas verificar se as mesmas atrações estão presentes
    
    if (attractions_.size() != other.attractions_.size()) {
        return false;
    }
    
    // Comparar cada atração na mesma posição
    for (size_t i = 0; i < attractions_.size(); ++i) {
        if (attractions_[i]->getName() != other.attractions_[i]->getName()) {
            return false;
        }
    }
    
    // Comparar cada modo de transporte na mesma posição
    for (size_t i = 0; i < transport_modes_.size() && i < other.transport_modes_.size(); ++i) {
        if (transport_modes_[i] != other.transport_modes_[i]) {
            return false;
        }
    }
    
    return true;
}

std::string MOVNSSolution::toString() const {
    std::stringstream ss;
    
    // Calcular informações para o formato
    double start_time = 9 * 60;
    double end_time = start_time + getTotalTime();
    
    // Calcular unique neighborhoods
    std::unordered_set<std::string> neighborhoods;
    for (const auto* attraction : attractions_) {
        neighborhoods.insert(attraction->getNeighborhood());
    }
    
    // Construir string com o formato esperado
    ss << std::fixed << std::setprecision(2);
    ss << getTotalCost() << ";";
    ss << getTotalTime() << ";";
    ss << getNumAttractions() << ";";
    ss << neighborhoods.size() << ";";
    ss << utils::Transport::formatTime(start_time) << ";";
    ss << utils::Transport::formatTime(end_time) << ";";
    
    // Neighborhood list
    for (const auto& neighborhood : neighborhoods) {
        ss << neighborhood << "|";
    }
    ss << ";";
    
    // Sequência de atrações
    for (const auto* attraction : attractions_) {
        ss << attraction->getName() << "|";
    }
    ss << ";";
    
    // Tempos de chegada
    for (const auto& info : time_info_) {
        ss << utils::Transport::formatTime(info.arrival_time) << "|";
    }
    ss << ";";
    
    // Tempos de partida
    for (const auto& info : time_info_) {
        ss << utils::Transport::formatTime(info.departure_time) << "|";
    }
    ss << ";";
    
    // Modos de transporte
    for (const auto& mode : transport_modes_) {
        ss << utils::Transport::getModeString(mode) << "|";
    }
    
    return ss.str();
}

void MOVNSSolution::recalculateTimeInfo() {
    if (attractions_.empty()) return;
    
    // Hora de início do dia (9:00 = 540 minutos desde meia-noite)
    double current_time = 9 * 60;
    
    // Processa a primeira atração
    auto& first_time_info = time_info_[0];
    first_time_info.wait_time = 0.0;  // Reset wait time
    
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
        time_info.wait_time = 0.0;  // Reset wait time
        
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

} // namespace tourist