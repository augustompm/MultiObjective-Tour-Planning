// File: include/movns-solution.hpp

#pragma once

#include "models.hpp"
#include "utils.hpp"
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <iomanip>
#include <cmath>

namespace tourist {

/**
 * @class MOVNSSolution
 * @brief Represents a solution for the Multi-Objective Variable Neighborhood Search algorithm
 */
class MOVNSSolution {
public:
    // Struct for time information of each attraction
    struct TimeInfo {
        double arrival_time;     // Arrival time at the attraction
        double departure_time;   // Departure time from the attraction
        double wait_time;        // Wait time until opening (if any)
        
        TimeInfo() : arrival_time(0.0), departure_time(0.0), wait_time(0.0) {}
    };
    
    // Constructors
    MOVNSSolution();
    explicit MOVNSSolution(const std::vector<const Attraction*>& attractions);
    
    // Attraction management methods
    const std::vector<const Attraction*>& getAttractions() const;
    const std::vector<utils::TransportMode>& getTransportModes() const;
    void addAttraction(const Attraction& attraction, utils::TransportMode mode = utils::TransportMode::CAR);
    void removeAttraction(size_t index);
    void swapAttractions(size_t index1, size_t index2);
    void insertAttraction(const Attraction& attraction, size_t position, utils::TransportMode mode = utils::TransportMode::CAR);
    
    // Objective functions
    double getTotalCost() const;
    double getTotalTime() const;
    int getNumAttractions() const;
    int getNumNeighborhoods() const;
    std::vector<double> getObjectives() const;
    
    // Constraint checks
    bool isValid() const;
    bool respectsTimeLimit() const;
    bool respectsWalkingLimit() const;
    bool checkTimeConstraints() const;
    
    // Dominance check
    bool dominates(const MOVNSSolution& other) const;
    
    // Comparison operator for equality
    bool operator==(const MOVNSSolution& other) const;
    
    // Utility methods
    std::string toString() const;
    
private:
    std::vector<const Attraction*> attractions_;
    std::vector<utils::TransportMode> transport_modes_;
    std::vector<TimeInfo> time_info_;
    
    // Recalculate time information for all attractions
    void recalculateTimeInfo();
};

} // namespace tourist