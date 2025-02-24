#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstddef>
#include <stdexcept>
#include <algorithm>

namespace tourist {

// Forward declarations
class Solution;
class Route;
class Attraction;
class Hotel;

// Base class for all evolutionary algorithms
class EvolutionaryAlgorithm {
public:
    virtual ~EvolutionaryAlgorithm() = default;
    
    // Configuração do algoritmo
    void setPopulationSize(size_t size) { 
        if (size == 0) throw std::invalid_argument("Population size must be positive");
        population_size_ = size; 
    }
    
    void setGenerations(size_t gens) { 
        if (gens == 0) throw std::invalid_argument("Number of generations must be positive");
        num_generations_ = gens; 
    }
    
    void setCrossoverRate(double rate) {
        if (rate < 0.0 || rate > 1.0) throw std::invalid_argument("Crossover rate must be between 0 and 1");
        crossover_rate_ = rate;
    }
    
    void setMutationRate(double rate) {
        if (rate < 0.0 || rate > 1.0) throw std::invalid_argument("Mutation rate must be between 0 and 1");
        mutation_rate_ = rate;
    }

    // Método principal que cada algoritmo deve implementar
    virtual std::vector<Solution> run() = 0;
    
    // Configurações comuns
    size_t getPopulationSize() const { return population_size_; }
    size_t getGenerations() const { return num_generations_; }
    double getCrossoverRate() const { return crossover_rate_; }
    double getMutationRate() const { return mutation_rate_; }

protected:
    size_t population_size_{100};
    size_t num_generations_{100};
    double crossover_rate_{0.9};
    double mutation_rate_{0.1};
};

// Base class for solutions/individuals
class SolutionBase {
public:
    virtual ~SolutionBase() = default;
    
    // Métodos principais que toda solução deve implementar
    virtual std::vector<double> getObjectives() const = 0;
    virtual bool dominates(const SolutionBase& other) const = 0;
    
    // Utilitários comuns para comparação
    bool operator<(const SolutionBase& other) const {
        return dominates(other);
    }
    
    bool operator>(const SolutionBase& other) const {
        return other.dominates(*this);
    }
    
    bool operator==(const SolutionBase& other) const {
        const auto& this_obj = getObjectives();
        const auto& other_obj = other.getObjectives();
        return this_obj.size() == other_obj.size() &&
               std::equal(this_obj.begin(), this_obj.end(), other_obj.begin());
    }

protected:
    std::vector<double> objectives_;
    
    // Funções auxiliares comuns para comparação de dominância
    bool isDominatedBy(const std::vector<double>& other_objectives) const {
        const auto& this_obj = getObjectives();
        if (this_obj.size() != other_objectives.size()) throw std::invalid_argument("Incompatible objective vectors");
        
        bool at_least_one_worse = false;
        for (size_t i = 0; i < this_obj.size(); ++i) {
            if (this_obj[i] > other_objectives[i]) return true;
            if (this_obj[i] < other_objectives[i]) at_least_one_worse = true;
        }
        return !at_least_one_worse;
    }
};

} // namespace tourist