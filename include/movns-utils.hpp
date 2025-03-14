// File: movns-utils.hpp

#pragma once

#include "movns-solution.hpp"
#include "models.hpp"
#include <vector>
#include <string>
#include <random>

namespace tourist {
namespace movns {

/**
 * @class Utils
 * @brief Funções utilitárias para o algoritmo MOVNS
 */
class Utils {
public:
    /**
     * @brief Gera uma solução inicial aleatória
     * 
     * @param attractions Conjunto de atrações disponíveis
     * @param max_attractions Número máximo de atrações a incluir
     * @return MOVNSSolution Solução inicial gerada
     */
    static MOVNSSolution generateRandomSolution(
        const std::vector<Attraction>& attractions,
        size_t max_attractions = 8);
    
    /**
     * @brief Verifica se uma solução é válida (respeita todas as restrições)
     * 
     * @param solution Solução a ser verificada
     * @return true Se a solução é válida
     * @return false Caso contrário
     */
    static bool isValidSolution(const MOVNSSolution& solution);
    
    /**
     * @brief Verifica se um modo de transporte é viável entre duas atrações
     * 
     * @param from Atração de origem
     * @param to Atração de destino
     * @param mode Modo de transporte a verificar
     * @return true Se o modo é viável
     * @return false Caso contrário
     */
    static bool isViableTransportMode(
        const Attraction& from,
        const Attraction& to,
        utils::TransportMode mode);
    
    /**
     * @brief Encontra atração pelo nome
     * 
     * @param attractions Lista de atrações
     * @param name Nome da atração a procurar
     * @return const Attraction* Ponteiro para a atração encontrada ou nullptr
     */
    static const Attraction* findAttractionByName(
        const std::vector<Attraction>& attractions,
        const std::string& name);
    
    /**
     * @brief Seleciona uma atração aleatória não incluída na solução atual
     * 
     * @param all_attractions Todas as atrações disponíveis
     * @param current_solution Solução atual
     * @param rng Gerador de números aleatórios
     * @return const Attraction* Atração selecionada ou nullptr se nenhuma disponível
     */
    static const Attraction* selectRandomAvailableAttraction(
        const std::vector<Attraction>& all_attractions,
        const MOVNSSolution& current_solution,
        std::mt19937& rng);
};

} // namespace movns
} // namespace tourist