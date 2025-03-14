// File: movns-neighborhood.cpp

#include "movns-neighborhood.hpp"
#include "movns-utils.hpp"
#include <algorithm>
#include <random>
#include <stdexcept>

namespace tourist {
namespace movns {

// TransportModeChangeNeighborhood
MOVNSSolution TransportModeChangeNeighborhood::generateRandomNeighbor(
    const MOVNSSolution& solution,
    const std::vector<Attraction>& all_attractions,
    std::mt19937& rng) const {
    
    // Se a solução tem menos de 2 atrações, não há arcos para mudar o modo
    if (solution.getNumAttractions() < 2) {
        return solution;
    }
    
    // Copiar a solução original
    MOVNSSolution neighbor = solution;
    const auto& attractions = neighbor.getAttractions();
    const auto& modes = neighbor.getTransportModes();
    
    // Selecionar aleatoriamente um arco
    std::uniform_int_distribution<size_t> dist(0, modes.size() - 1);
    size_t arc_idx = dist(rng);
    
    // Identificar o modo atual e o modo alternativo
    utils::TransportMode current_mode = modes[arc_idx];
    utils::TransportMode new_mode = (current_mode == utils::TransportMode::CAR) ? 
                                  utils::TransportMode::WALK : 
                                  utils::TransportMode::CAR;
    
    // Verificar se o novo modo é viável (para caminhada)
    if (new_mode == utils::TransportMode::WALK) {
        if (!Utils::isViableTransportMode(*attractions[arc_idx], *attractions[arc_idx + 1], new_mode)) {
            // Se não for viável, retornamos a solução original
            return solution;
        }
    }
    
    // Atualizar o modo de transporte
    // Note: Como não temos um método direto para modificar apenas o modo,
    // criamos uma nova solução e reconstruímos a rota
    MOVNSSolution new_solution;
    
    // Adicionar a primeira atração
    new_solution.addAttraction(*attractions[0]);
    
    // Adicionar as demais atrações com os modos de transporte
    for (size_t i = 1; i < attractions.size(); ++i) {
        utils::TransportMode mode = (i - 1 == arc_idx) ? new_mode : modes[i - 1];
        new_solution.addAttraction(*attractions[i], mode);
    }
    
    // Verificar se a solução é válida
    if (new_solution.isValid()) {
        return new_solution;
    }
    
    // Se não for válida, retornar a original
    return solution;
}

// LocationReallocationNeighborhood
MOVNSSolution LocationReallocationNeighborhood::generateRandomNeighbor(
    const MOVNSSolution& solution,
    const std::vector<Attraction>& all_attractions,
    std::mt19937& rng) const {
    
    // Se a solução tem menos de 2 atrações, não há realocação possível
    if (solution.getNumAttractions() < 2) {
        return solution;
    }
    
    // Copiar a solução original
    MOVNSSolution neighbor = solution;
    const auto& attractions = solution.getAttractions();
    
    // Selecionar aleatoriamente uma atração para realocar
    std::uniform_int_distribution<size_t> src_dist(0, attractions.size() - 1);
    size_t src_idx = src_dist(rng);
    
    // Selecionar aleatoriamente uma posição de destino (diferente da origem)
    size_t dest_idx;
    do {
        std::uniform_int_distribution<size_t> dest_dist(0, attractions.size() - 1);
        dest_idx = dest_dist(rng);
    } while (dest_idx == src_idx);
    
    // Criar nova solução com a realocação
    MOVNSSolution new_solution;
    
    // Adicionar atrações na nova ordem
    std::vector<const Attraction*> new_sequence;
    for (size_t i = 0; i < attractions.size(); ++i) {
        if (i != src_idx) {
            if (i == dest_idx) {
                new_sequence.push_back(attractions[src_idx]);
            }
            new_sequence.push_back(attractions[i]);
        } else if (dest_idx > src_idx) {
            new_sequence.push_back(attractions[i + 1 > attractions.size() - 1 ? 0 : i + 1]);
        }
    }
    
    // Adicionar a primeira atração
    new_solution.addAttraction(*new_sequence[0]);
    
    // Adicionar as demais atrações com modos de transporte determinados automaticamente
    for (size_t i = 1; i < new_sequence.size(); ++i) {
        new_solution.addAttraction(*new_sequence[i]);
    }
    
    // Verificar se a solução é válida
    if (new_solution.isValid()) {
        return new_solution;
    }
    
    // Se não for válida, retornar a original
    return solution;
}

// LocationExchangeNeighborhood
MOVNSSolution LocationExchangeNeighborhood::generateRandomNeighbor(
    const MOVNSSolution& solution,
    const std::vector<Attraction>& all_attractions,
    std::mt19937& rng) const {
    
    // Se a solução tem menos de 2 atrações, não há troca possível
    if (solution.getNumAttractions() < 2) {
        return solution;
    }
    
    // Copiar a solução original
    MOVNSSolution new_solution = solution;
    
    // Selecionar aleatoriamente duas atrações para trocar
    std::uniform_int_distribution<size_t> idx1_dist(0, new_solution.getNumAttractions() - 1);
    size_t idx1 = idx1_dist(rng);
    
    size_t idx2;
    do {
        std::uniform_int_distribution<size_t> idx2_dist(0, new_solution.getNumAttractions() - 1);
        idx2 = idx2_dist(rng);
    } while (idx2 == idx1 || std::abs(static_cast<int>(idx2) - static_cast<int>(idx1)) == 1);
    
    // Realizar a troca
    new_solution.swapAttractions(idx1, idx2);
    
    // Verificar se a solução é válida
    if (new_solution.isValid()) {
        return new_solution;
    }
    
    // Se não for válida, retornar a original
    return solution;
}

// SubsequenceInversionNeighborhood
MOVNSSolution SubsequenceInversionNeighborhood::generateRandomNeighbor(
    const MOVNSSolution& solution,
    const std::vector<Attraction>& all_attractions,
    std::mt19937& rng) const {
    
    // Se a solução tem menos de 3 atrações, não há subsequência para inverter
    if (solution.getNumAttractions() < 3) {
        return solution;
    }
    
    // Copiar a solução original
    const auto& attractions = solution.getAttractions();
    
    // Selecionar aleatoriamente os limites da subsequência
    std::uniform_int_distribution<size_t> start_dist(0, attractions.size() - 3);
    size_t start_idx = start_dist(rng);
    
    std::uniform_int_distribution<size_t> end_dist(start_idx + 1, attractions.size() - 1);
    size_t end_idx = end_dist(rng);
    
    // Garantir que a subsequência tem pelo menos 2 elementos
    if (end_idx - start_idx < 1) {
        end_idx = start_idx + 1;
    }
    
    // Criar nova sequência com a subsequência invertida
    std::vector<const Attraction*> new_sequence;
    
    // Adicionar elementos antes da subsequência
    for (size_t i = 0; i < start_idx; ++i) {
        new_sequence.push_back(attractions[i]);
    }
    
    // Adicionar a subsequência invertida
    for (size_t i = end_idx; i >= start_idx && i < attractions.size(); --i) {
        new_sequence.push_back(attractions[i]);
    }
    
    // Adicionar elementos após a subsequência
    for (size_t i = end_idx + 1; i < attractions.size(); ++i) {
        new_sequence.push_back(attractions[i]);
    }
    
    // Criar nova solução com a sequência modificada
    MOVNSSolution new_solution;
    
    // Adicionar a primeira atração
    new_solution.addAttraction(*new_sequence[0]);
    
    // Adicionar as demais atrações com modos de transporte determinados automaticamente
    for (size_t i = 1; i < new_sequence.size(); ++i) {
        new_solution.addAttraction(*new_sequence[i]);
    }
    
    // Verificar se a solução é válida
    if (new_solution.isValid()) {
        return new_solution;
    }
    
    // Se não for válida, retornar a original
    return solution;
}

// LocationReplacementNeighborhood
MOVNSSolution LocationReplacementNeighborhood::generateRandomNeighbor(
    const MOVNSSolution& solution,
    const std::vector<Attraction>& all_attractions,
    std::mt19937& rng) const {
    
    // Copiar a solução original
    MOVNSSolution new_solution = solution;
    const auto& attractions = solution.getAttractions();
    
    // Selecionar aleatoriamente uma atração para substituir
    std::uniform_int_distribution<size_t> idx_dist(0, attractions.size() - 1);
    size_t idx = idx_dist(rng);
    
    // Selecionar uma atração disponível para substituição
    const Attraction* new_attraction = Utils::selectRandomAvailableAttraction(
        all_attractions, solution, rng);
    
    // Se não há atrações disponíveis, retornar a solução original
    if (new_attraction == nullptr) {
        return solution;
    }
    
    // Criar nova sequência com a substituição
    std::vector<const Attraction*> new_sequence = attractions;
    new_sequence[idx] = new_attraction;
    
    // Criar nova solução com a sequência modificada
    new_solution = MOVNSSolution();
    
    // Adicionar a primeira atração
    new_solution.addAttraction(*new_sequence[0]);
    
    // Adicionar as demais atrações com modos de transporte determinados automaticamente
    for (size_t i = 1; i < new_sequence.size(); ++i) {
        new_solution.addAttraction(*new_sequence[i]);
    }
    
    // Verificar se a solução é válida
    if (new_solution.isValid()) {
        return new_solution;
    }
    
    // Se não for válida, retornar a original
    return solution;
}

// NeighborhoodFactory
std::vector<std::shared_ptr<Neighborhood>> NeighborhoodFactory::createAllNeighborhoods() {
    std::vector<std::shared_ptr<Neighborhood>> neighborhoods;
    
    neighborhoods.push_back(std::make_shared<TransportModeChangeNeighborhood>());
    neighborhoods.push_back(std::make_shared<LocationReallocationNeighborhood>());
    neighborhoods.push_back(std::make_shared<LocationExchangeNeighborhood>());
    neighborhoods.push_back(std::make_shared<SubsequenceInversionNeighborhood>());
    neighborhoods.push_back(std::make_shared<LocationReplacementNeighborhood>());
    
    return neighborhoods;
}

std::shared_ptr<Neighborhood> NeighborhoodFactory::selectRandomNeighborhood(
    const std::vector<std::shared_ptr<Neighborhood>>& neighborhoods,
    std::mt19937& rng) {
    
    if (neighborhoods.empty()) {
        throw std::runtime_error("No neighborhoods available for selection");
    }
    
    std::uniform_int_distribution<size_t> dist(0, neighborhoods.size() - 1);
    return neighborhoods[dist(rng)];
}

} // namespace movns
} // namespace tourist