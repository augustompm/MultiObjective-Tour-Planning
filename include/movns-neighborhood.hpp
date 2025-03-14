// File: movns-neighborhood.hpp

#pragma once

#include "movns-solution.hpp"
#include <vector>
#include <random>
#include <functional>

namespace tourist {
namespace movns {

/**
 * @class Neighborhood
 * @brief Interface base para estruturas de vizinhança
 */
class Neighborhood {
public:
    virtual ~Neighborhood() = default;
    
    /**
     * @brief Gera um vizinho aleatório da solução atual
     * 
     * @param solution Solução atual
     * @param all_attractions Todas as atrações disponíveis
     * @param rng Gerador de números aleatórios
     * @return MOVNSSolution Solução vizinha gerada
     */
    virtual MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const = 0;
    
    /**
     * @brief Nome da estrutura de vizinhança
     * 
     * @return std::string Nome descritivo
     */
    virtual std::string getName() const = 0;
};

/**
 * @class TransportModeChangeNeighborhood
 * @brief Vizinhança N₁: Alteração de modo de transporte
 */
class TransportModeChangeNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "Transport Mode Change (N₁)";
    }
};

/**
 * @class LocationReallocationNeighborhood
 * @brief Vizinhança N₂: Realocação de atrações
 */
class LocationReallocationNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "Location Reallocation (N₂)";
    }
};

/**
 * @class LocationExchangeNeighborhood
 * @brief Vizinhança N₃: Troca de pares de locais
 */
class LocationExchangeNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "Location Exchange (N₃)";
    }
};

/**
 * @class SubsequenceInversionNeighborhood
 * @brief Vizinhança N₄: Inversão de subsequência
 */
class SubsequenceInversionNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "Subsequence Inversion (N₄)";
    }
};

/**
 * @class LocationReplacementNeighborhood
 * @brief Vizinhança N₅: Substituição de local
 */
class LocationReplacementNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "Location Replacement (N₅)";
    }
};

/**
 * @class NeighborhoodFactory
 * @brief Fábrica para criar instâncias de estruturas de vizinhança
 */
class NeighborhoodFactory {
public:
    /**
     * @brief Cria todas as estruturas de vizinhança
     * 
     * @return std::vector<std::shared_ptr<Neighborhood>> Vetor de estruturas de vizinhança
     */
    static std::vector<std::shared_ptr<Neighborhood>> createAllNeighborhoods();
    
    /**
     * @brief Seleciona uma estrutura de vizinhança aleatoriamente
     * 
     * @param neighborhoods Lista de estruturas de vizinhança disponíveis
     * @param rng Gerador de números aleatórios
     * @return std::shared_ptr<Neighborhood> Estrutura de vizinhança selecionada
     */
    static std::shared_ptr<Neighborhood> selectRandomNeighborhood(
        const std::vector<std::shared_ptr<Neighborhood>>& neighborhoods,
        std::mt19937& rng);
};

} // namespace movns
} // namespace tourist