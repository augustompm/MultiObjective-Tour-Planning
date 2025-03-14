// File: src/movns-utils.cpp

#include "movns-utils.hpp"
#include <algorithm>
#include <random>
#include <unordered_set>
#include <numeric>

namespace tourist {
namespace movns {

MOVNSSolution Utils::generateRandomSolution(
    const std::vector<Attraction>& attractions,
    size_t /* max_attractions */) {
    
    // Criar gerador de números aleatórios
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Alterar para sempre começar com pelo menos 2 atrações
    // para garantir que os operadores de vizinhança funcionem corretamente
    size_t min_attractions = 2;
    size_t target_attractions = std::min(8UL, attractions.size());
    std::uniform_int_distribution<size_t> num_dist(min_attractions, std::min(target_attractions, attractions.size()));
    size_t num_attractions = num_dist(gen);
    
    // Se não houver atrações suficientes, usar o máximo disponível
    if (attractions.size() < min_attractions) {
        num_attractions = attractions.size();
    }
    
    // Criar cópia dos índices e embaralhar
    std::vector<size_t> indices(attractions.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), gen);
    
    // Tentativa com diferentes tamanhos de solução para aumentar chances de sucesso
    for (size_t attempt_size = num_attractions; attempt_size >= min_attractions; --attempt_size) {
        // Selecionar as primeiras attempt_size como nossa sequência
        MOVNSSolution solution;
        
        // Tentar construir uma solução com attempt_size atrações
        for (size_t i = 0; i < attempt_size; ++i) {
            const Attraction& attraction = attractions[indices[i]];
            
            // Definir modo de transporte (primeiro ponto não tem modo associado)
            if (i == 0) {
                solution.addAttraction(attraction);
            } else {
                // Determinar modo preferencial
                utils::TransportMode preferredMode = utils::Transport::determinePreferredMode(
                    solution.getAttractions().back()->getName(),
                    attraction.getName()
                );
                
                // Para longas distâncias, forçar uso do carro
                double travel_time = utils::Transport::getTravelTime(
                    solution.getAttractions().back()->getName(),
                    attraction.getName(),
                    utils::TransportMode::WALK
                );
                
                if (travel_time > utils::Config::WALK_TIME_PREFERENCE) {
                    preferredMode = utils::TransportMode::CAR;
                }
                
                solution.addAttraction(attraction, preferredMode);
            }
            
            // Se a solução já não é válida, não adicionar mais atrações
            if (!solution.isValid()) {
                break;
            }
        }
        
        // Verificar se a solução respeita as restrições
        if (solution.isValid() && static_cast<size_t>(solution.getNumAttractions()) >= min_attractions) {
            return solution;
        }
    }
    
    // Se nenhuma solução válida foi encontrada, tenta uma abordagem diferente
    // Construir incrementalmente, validando a cada passo
    MOVNSSolution solution;
    
    // Adicionar a primeira atração
    solution.addAttraction(attractions[indices[0]]);
    
    // Tentar adicionar mais atrações uma por uma até termos pelo menos 2
    for (size_t i = 1; i < attractions.size() && static_cast<size_t>(solution.getNumAttractions()) < num_attractions; ++i) {
        MOVNSSolution temp_solution = solution;
        const Attraction& attraction = attractions[indices[i]];
        
        // Determinar modo preferencial
        utils::TransportMode preferredMode = utils::Transport::determinePreferredMode(
            temp_solution.getAttractions().back()->getName(),
            attraction.getName()
        );
        
        // Para longas distâncias, forçar uso do carro
        double travel_time = utils::Transport::getTravelTime(
            temp_solution.getAttractions().back()->getName(),
            attraction.getName(),
            utils::TransportMode::WALK
        );
        
        if (travel_time > utils::Config::WALK_TIME_PREFERENCE) {
            preferredMode = utils::TransportMode::CAR;
        }
        
        temp_solution.addAttraction(attraction, preferredMode);
        
        // Se a solução continua válida, aceitar a nova atração
        if (temp_solution.isValid()) {
            solution = temp_solution;
        }
    }
    
    // Se conseguimos uma solução mínima válida, retornar
    if (solution.isValid() && static_cast<size_t>(solution.getNumAttractions()) >= min_attractions) {
        return solution;
    }
    
    // Última tentativa: gerar uma solução mínima viável com exatamente 2 atrações
    // Forçar tentar todas as combinações possíveis de pares de atrações
    for (size_t i = 0; i < attractions.size(); ++i) {
        const Attraction& first = attractions[i];
        
        for (size_t j = 0; j < attractions.size(); ++j) {
            if (i == j) continue;
            
            const Attraction& second = attractions[j];
            
            MOVNSSolution pair_solution;
            pair_solution.addAttraction(first);
            
            // Determinar modo preferencial
            utils::TransportMode preferredMode = utils::Transport::determinePreferredMode(
                first.getName(), second.getName()
            );
            
            // Forçar carro para garantir viabilidade
            pair_solution.addAttraction(second, utils::TransportMode::CAR);
            
            if (pair_solution.isValid()) {
                return pair_solution;
            }
        }
    }
    
    // Se mesmo após tentar todas as combinações não conseguimos, 
    // criar uma solução com apenas uma atração
    MOVNSSolution fallback;
    fallback.addAttraction(attractions[0]);
    
    return fallback;
}

bool Utils::isValidSolution(const MOVNSSolution& solution) {
    // Verifica todas as restrições
    return solution.isValid();
}

bool Utils::isViableTransportMode(
    const Attraction& from,
    const Attraction& to,
    utils::TransportMode mode) {
    
    // Se o modo for caminhada, verificar se o tempo é menor que o limite
    if (mode == utils::TransportMode::WALK) {
        double walk_time = utils::Transport::getTravelTime(
            from.getName(), to.getName(), utils::TransportMode::WALK);
        return walk_time <= utils::Config::WALK_TIME_PREFERENCE;
    }
    
    // Carro é sempre viável
    return true;
}

const Attraction* Utils::findAttractionByName(
    const std::vector<Attraction>& attractions,
    const std::string& name) {
    
    auto it = std::find_if(attractions.begin(), attractions.end(),
                         [&name](const Attraction& attr) {
                             return attr.getName() == name;
                         });
    
    if (it != attractions.end()) {
        return &(*it);
    }
    
    return nullptr;
}

const Attraction* Utils::selectRandomAvailableAttraction(
    const std::vector<Attraction>& all_attractions,
    const MOVNSSolution& current_solution,
    std::mt19937& rng) {
    
    // Coletar os nomes das atrações na solução atual
    std::vector<std::string> current_names;
    for (const auto* attr : current_solution.getAttractions()) {
        current_names.push_back(attr->getName());
    }
    
    // Filtrar atrações disponíveis (não presentes na solução atual)
    std::vector<const Attraction*> available;
    for (const auto& attr : all_attractions) {
        if (std::find(current_names.begin(), current_names.end(), attr.getName()) == current_names.end()) {
            available.push_back(&attr);
        }
    }
    
    // Se não há atrações disponíveis, retornar nullptr
    if (available.empty()) {
        return nullptr;
    }
    
    // Priorizar atrações com diferentes bairros para aumentar a diversidade
    std::unordered_set<std::string> current_neighborhoods;
    for (const auto* attr : current_solution.getAttractions()) {
        current_neighborhoods.insert(attr->getNeighborhood());
    }
    
    // Atrações em novos bairros
    std::vector<const Attraction*> new_neighborhood_attractions;
    for (const auto* attr : available) {
        if (current_neighborhoods.find(attr->getNeighborhood()) == current_neighborhoods.end()) {
            new_neighborhood_attractions.push_back(attr);
        }
    }
    
    // Se temos atrações em novos bairros, priorize-as (70% de chance)
    std::uniform_real_distribution<double> neighborhood_dist(0.0, 1.0);
    if (!new_neighborhood_attractions.empty() && neighborhood_dist(rng) < 0.7) {
        std::uniform_int_distribution<size_t> dist(0, new_neighborhood_attractions.size() - 1);
        return new_neighborhood_attractions[dist(rng)];
    }
    
    // Caso contrário, selecionar aleatoriamente entre todas as disponíveis
    std::uniform_int_distribution<size_t> dist(0, available.size() - 1);
    return available[dist(rng)];
}

} // namespace movns
} // namespace tourist