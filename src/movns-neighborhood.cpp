// File: src/movns-neighborhood.cpp

#include "movns-neighborhood.hpp"
#include "movns-utils.hpp"
#include <vector>
#include <memory>
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
    
    // Using all_attractions reference to avoid unused parameter warning
    if (all_attractions.empty()) {
        // This is a safety check that won't be triggered in normal operation
        // but satisfies the compiler warning
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
    
    // Using all_attractions reference to avoid unused parameter warning
    if (all_attractions.empty()) {
        // This is a safety check that won't be triggered in normal operation
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
    
    // Using all_attractions reference to avoid unused parameter warning
    if (all_attractions.empty()) {
        // This is a safety check that won't be triggered in normal operation
        return solution;
    }
    
    // CORREÇÃO: Se a solução tem exatamente 2 atrações, a troca resultaria na mesma sequência
    // Neste caso, tente adicionar uma nova atração em vez de trocar
    if (solution.getNumAttractions() == 2) {
        // Tente usar o operador de substituição ou adição (LocationReplacementNeighborhood)
        auto replacement_neighborhood = LocationReplacementNeighborhood();
        return replacement_neighborhood.generateRandomNeighbor(solution, all_attractions, rng);
    }
    
    // Copiar a solução original
    MOVNSSolution new_solution = solution;
    
    // Selecionar aleatoriamente duas atrações para trocar
    std::uniform_int_distribution<size_t> idx1_dist(0, new_solution.getNumAttractions() - 1);
    size_t idx1 = idx1_dist(rng);
    
    size_t idx2;
    int max_attempts = 10; // Prevenir loops infinitos
    int attempt = 0;
    
    do {
        std::uniform_int_distribution<size_t> idx2_dist(0, new_solution.getNumAttractions() - 1);
        idx2 = idx2_dist(rng);
        attempt++;
        
        // Se não conseguirmos encontrar um índice diferente após várias tentativas,
        // retornamos a solução original para evitar loop infinito
        if (attempt >= max_attempts) {
            return solution;
        }
    } while (idx2 == idx1);
    
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
        // Para soluções com menos de 3 atrações, tente usar outro operador
        if (solution.getNumAttractions() == 2) {
            // Para soluções com 2 atrações, tente adicionar ou substituir
            auto replacement_neighborhood = LocationReplacementNeighborhood();
            return replacement_neighborhood.generateRandomNeighbor(solution, all_attractions, rng);
        }
        return solution;
    }
    
    // Using all_attractions reference to avoid unused parameter warning
    if (all_attractions.empty()) {
        // This is a safety check that won't be triggered in normal operation
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
    
    // Caso especial para solução com apenas uma atração - força adicionar uma nova
    if (solution.getNumAttractions() == 1) {
        // Obter atrações não presentes na solução atual
        std::vector<const Attraction*> available_attractions;
        for (const auto& attraction : all_attractions) {
            bool already_included = false;
            for (const auto* included_attr : solution.getAttractions()) {
                if (included_attr->getName() == attraction.getName()) {
                    already_included = true;
                    break;
                }
            }
            if (!already_included) {
                available_attractions.push_back(&attraction);
            }
        }
        
        // Se não há atrações disponíveis, retornar a original
        if (available_attractions.empty()) {
            return solution;
        }
        
        // Tentar até 3 atrações aleatórias diferentes
        for (int attempt = 0; attempt < 3; ++attempt) {
            // Selecionar aleatoriamente uma atração
            std::uniform_int_distribution<size_t> attr_dist(0, available_attractions.size() - 1);
            size_t attr_idx = attr_dist(rng);
            const Attraction* new_attraction = available_attractions[attr_idx];
            
            // Criar uma nova solução com a atração original e a nova
            MOVNSSolution new_solution;
            
            // Adicionar a atração original
            for (const auto* att : solution.getAttractions()) {
                new_solution.addAttraction(*att);
            }
            
            // Adicionar a nova atração
            new_solution.addAttraction(*new_attraction);
            
            // Verificar se a solução é válida
            if (new_solution.isValid()) {
                return new_solution;
            }
            
            // Remover essa atração das disponíveis para tentar outra
            if (attr_idx < available_attractions.size()) {
                available_attractions.erase(available_attractions.begin() + attr_idx);
            }
            
            if (available_attractions.empty()) {
                break;
            }
        }
    }
    
    // Para soluções com 2+ atrações ou se a adição falhou, proceder normalmente
    // Randomly decide whether to add (if possible) or replace an attraction
    std::uniform_real_distribution<double> decision_dist(0.0, 1.0);
    bool try_addition = decision_dist(rng) < 0.7 && solution.getNumAttractions() < 10;
    
    if (try_addition) {
        // Get attractions not in the current solution
        std::vector<const Attraction*> available_attractions;
        for (const auto& attraction : all_attractions) {
            bool already_included = false;
            for (const auto* included_attr : solution.getAttractions()) {
                if (included_attr->getName() == attraction.getName()) {
                    already_included = true;
                    break;
                }
            }
            if (!already_included) {
                available_attractions.push_back(&attraction);
            }
        }
        
        // If no attractions are available, return the original solution
        if (available_attractions.empty()) {
            return solution;
        }
        
        // Select a random available attraction
        std::uniform_int_distribution<size_t> attr_dist(0, available_attractions.size() - 1);
        const Attraction* new_attraction = available_attractions[attr_dist(rng)];
        
        // Try adding at the end first (simplest case)
        MOVNSSolution new_solution = solution;
        new_solution.addAttraction(*new_attraction);
        
        if (new_solution.isValid()) {
            return new_solution;
        }
        
        // If adding at the end fails, try different insertion positions
        for (int pos = 0; pos <= solution.getNumAttractions(); ++pos) {
            MOVNSSolution test_solution = solution;
            try {
                test_solution.insertAttraction(*new_attraction, pos);
                if (test_solution.isValid()) {
                    return test_solution;
                }
            } catch (const std::exception&) {
                // If insertion fails, try next position
                continue;
            }
        }
    }
    
    // If addition not possible or not chosen and solution has at least 2 attractions, try replacement
    if (solution.getNumAttractions() >= 2) {
        const auto& attractions = solution.getAttractions();
        
        // Randomly select an attraction to replace
        std::uniform_int_distribution<size_t> idx_dist(0, attractions.size() - 1);
        size_t idx = idx_dist(rng);
        
        // Get attractions not in the current solution
        std::vector<const Attraction*> available_attractions;
        for (const auto& attraction : all_attractions) {
            bool already_included = false;
            for (const auto* included_attr : solution.getAttractions()) {
                if (included_attr->getName() == attraction.getName()) {
                    already_included = true;
                    break;
                }
            }
            if (!already_included) {
                available_attractions.push_back(&attraction);
            }
        }
        
        // If no attractions are available, return the original solution
        if (available_attractions.empty()) {
            return solution;
        }
        
        // Select a random available attraction
        std::uniform_int_distribution<size_t> attr_dist(0, available_attractions.size() - 1);
        const Attraction* new_attraction = available_attractions[attr_dist(rng)];
        
        // Create new sequence with the replacement
        std::vector<const Attraction*> new_sequence = attractions;
        new_sequence[idx] = new_attraction;
        
        // Create new solution with the modified sequence
        MOVNSSolution new_solution;
        
        // Add the first attraction
        new_solution.addAttraction(*new_sequence[0]);
        
        // Add the remaining attractions with transport modes determined automatically
        for (size_t i = 1; i < new_sequence.size(); ++i) {
            new_solution.addAttraction(*new_sequence[i]);
        }
        
        // Check if the solution is valid
        if (new_solution.isValid()) {
            return new_solution;
        }
    }
    
    // If all attempts fail, return the original solution
    return solution;
}

// AttractionRemovalNeighborhood
MOVNSSolution AttractionRemovalNeighborhood::generateRandomNeighbor(
    const MOVNSSolution& solution,
    const std::vector<Attraction>& all_attractions,
    std::mt19937& rng) const {
    
    // If solution has only 1 attraction, can't remove
    if (solution.getNumAttractions() <= 1) {
        // Try using a different neighborhood operator that adds attractions
        if (solution.getNumAttractions() == 1) {
            // For solutions with 1 attraction, try adding a new one
            auto replacement_neighborhood = LocationReplacementNeighborhood();
            return replacement_neighborhood.generateRandomNeighbor(solution, all_attractions, rng);
        }
        return solution;
    }
    
    // Using all_attractions reference to avoid unused parameter warning
    if (all_attractions.empty()) {
        return solution;
    }
    
    // If the solution has exactly 2 attractions and we remove one, we'd have 1 attraction
    // This is still valid, so we can proceed
    
    // Select a random attraction to remove (avoid removing the first one if possible)
    std::uniform_int_distribution<size_t> dist(
        solution.getNumAttractions() > 2 ? 1 : 0, 
        solution.getNumAttractions() - 1);
    size_t idx = dist(rng);
    
    // Create a copy of the solution and remove the selected attraction
    MOVNSSolution new_solution = solution;
    new_solution.removeAttraction(idx);
    
    // If solution is valid, return it
    if (new_solution.isValid()) {
        return new_solution;
    }
    
    // If not valid, return original
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
    neighborhoods.push_back(std::make_shared<AttractionRemovalNeighborhood>());
    
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