// File: include/movns-neighborhood.hpp

#pragma once

#include <vector>
#include <memory>
#include <random>
#include "movns-solution.hpp"  // Include for MOVNSSolution

namespace tourist {
namespace movns {

/**
 * @class Neighborhood
 * @brief Base abstract class for neighborhood operators in MOVNS
 */
class Neighborhood {
public:
    virtual ~Neighborhood() = default;
    
    /**
     * @brief Generate a random neighbor from the current solution
     * 
     * @param solution Current solution
     * @param all_attractions All available attractions
     * @param rng Random number generator
     * @return MOVNSSolution A neighboring solution
     */
    virtual MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const = 0;
    
    /**
     * @brief Get the name of this neighborhood
     * 
     * @return std::string Neighborhood name
     */
    virtual std::string getName() const = 0;
};

/**
 * @class TransportModeChangeNeighborhood
 * @brief Neighborhood that changes transport mode between two attractions
 */
class TransportModeChangeNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "TransportModeChange";
    }
};

/**
 * @class LocationReallocationNeighborhood
 * @brief Neighborhood that moves an attraction to a different position in the sequence
 */
class LocationReallocationNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "LocationReallocation";
    }
};

/**
 * @class LocationExchangeNeighborhood
 * @brief Neighborhood that swaps two attractions in the sequence
 */
class LocationExchangeNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "LocationExchange";
    }
};

/**
 * @class SubsequenceInversionNeighborhood
 * @brief Neighborhood that inverts a subsequence of attractions
 */
class SubsequenceInversionNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "SubsequenceInversion";
    }
};

/**
 * @class LocationReplacementNeighborhood
 * @brief Neighborhood that replaces an attraction with another one not in the solution
 */
class LocationReplacementNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "LocationReplacement";
    }
};

/**
 * @class AttractionRemovalNeighborhood
 * @brief Neighborhood that removes a random attraction from the solution
 */
class AttractionRemovalNeighborhood : public Neighborhood {
public:
    MOVNSSolution generateRandomNeighbor(
        const MOVNSSolution& solution,
        const std::vector<Attraction>& all_attractions,
        std::mt19937& rng) const override;
    
    std::string getName() const override {
        return "AttractionRemoval";
    }
};

/**
 * @class NeighborhoodFactory
 * @brief Factory class for creating and managing neighborhoods
 */
class NeighborhoodFactory {
public:
    /**
     * @brief Create all available neighborhood operators
     * 
     * @return std::vector<std::shared_ptr<Neighborhood>> List of neighborhood operators
     */
    static std::vector<std::shared_ptr<Neighborhood>> createAllNeighborhoods();
    
    /**
     * @brief Select a random neighborhood from the available ones
     * 
     * @param neighborhoods List of available neighborhoods
     * @param rng Random number generator
     * @return std::shared_ptr<Neighborhood> Selected neighborhood
     */
    static std::shared_ptr<Neighborhood> selectRandomNeighborhood(
        const std::vector<std::shared_ptr<Neighborhood>>& neighborhoods,
        std::mt19937& rng);
};

} // namespace movns
} // namespace tourist