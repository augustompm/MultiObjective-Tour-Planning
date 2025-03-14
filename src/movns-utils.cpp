// File: movns-utils.cpp

#include "movns-utils.hpp"
#include <algorithm>
#include <random>

namespace tourist {
namespace movns {

MOVNSSolution Utils::generateRandomSolution(
    const std::vector<Attraction>& attractions,
    size_t max_attractions) {
    
    // Criar gerador de números aleatórios
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Determinar quantas atrações incluir (entre 2 e max_attractions)
    std::uniform_int_distribution<size_t> num_dist(2, std::min(max_attractions, attractions.size()));
    size_t num_attractions = num_dist(gen);
    
    // Criar cópia dos índices e embaralhar
    std::vector<size_t> indices(attractions.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), gen);
    
    // Selecionar as primeiras num_attractions como nossa sequência
    MOVNSSolution solution;
    for (size_t i = 0; i < num_attractions; ++i) {
        const Attraction& attraction = attractions[indices[i]];
        
        // Definir modo de transporte (primeiro ponto não tem modo associado)
        if (i == 0) {
            solution.addAttraction(attraction);
        } else {
            // Determinar modo preferencial
            utils::TransportMode mode = utils::Transport::determinePreferredMode(
                solution.getAttractions().back()->getName(),
                attraction.getName()
            );
            solution.addAttraction(attraction, mode);
        }
    }
    
    // Verificar se a solução respeita as restrições
    if (solution.isValid()) {
        return solution;
    }
    
    // Se a solução não for válida, tentar reduzir o número de atrações até encontrar uma válida
    while (!solution.isValid() && solution.getNumAttractions() > 2) {
        // Remover a última atração
        solution.removeAttraction(solution.getNumAttractions() - 1);
    }
    
    return solution;
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
    
    // Selecionar aleatoriamente uma das atrações disponíveis
    if (!available.empty()) {
        std::uniform_int_distribution<size_t> dist(0, available.size() - 1);
        return available[dist(rng)];
    }
    
    return nullptr;
}

} // namespace movns
} // namespace tourist