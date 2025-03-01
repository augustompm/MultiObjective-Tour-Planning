#pragma once

#include "base.hpp"
#include "models.hpp"
#include <vector>
#include <memory>
#include <random>
#include <functional>

namespace tourist {

class MOEAD final : public EvolutionaryAlgorithm {
public:
    struct Parameters {
        size_t population_size;
        size_t max_generations;
        size_t neighborhood_size;
        double crossover_rate;
        double mutation_rate;

        Parameters()
            : population_size(100)
            , max_generations(100)
            , neighborhood_size(20)
            , crossover_rate(0.9)
            , mutation_rate(0.1) {}

        Parameters(size_t pop_size, size_t max_gen, size_t neigh_size, double cross_rate, double mut_rate)
            : population_size(pop_size)
            , max_generations(max_gen)
            , neighborhood_size(neigh_size)
            , crossover_rate(cross_rate)
            , mutation_rate(mut_rate) {}
        
        void validate() const;
    };

protected:
    class Subproblem {
    public:
        Subproblem(std::vector<int> chromosome, std::vector<double> weights);
        void evaluate(const MOEAD& algorithm);
        Route constructRoute(const MOEAD& algorithm) const;
        
        double getScalarizedValue() const { return scalarized_value_; }
        const std::vector<double>& getObjectives() const { return objectives_; }
        const std::vector<double>& getWeights() const { return weights_; }
        
    private:
        std::vector<int> chromosome_;                        // Índices das atrações
        std::vector<utils::TransportMode> transport_modes_;  // Modos de transporte
        std::vector<double> weights_;                        // Pesos dos objetivos
        std::vector<double> objectives_;                     // Valores dos objetivos
        double scalarized_value_;                            // Valor escalarizado
        
        void determineTransportModes(const MOEAD& algorithm);
        double calculateTchebycheff(const std::vector<double>& objectives, 
                                    const std::vector<double>& weights,
                                    const std::vector<double>& reference_point) const;
    };

public:
    MOEAD(const std::vector<Attraction>& attractions, Parameters params = Parameters());

    MOEAD(const MOEAD&) = delete;
    MOEAD& operator=(const MOEAD&) = delete;
    MOEAD(MOEAD&&) = delete;
    MOEAD& operator=(MOEAD&&) = delete;

    // Executa o algoritmo e retorna as soluções não dominadas
    std::vector<Solution> run() override;

private:
    using SubproblemPtr = std::shared_ptr<Subproblem>;
    using Population = std::vector<SubproblemPtr>;

    // Métodos principais do MOEA/D
    void initializePopulation();
    void initializeWeights();
    void initializeNeighborhood();
    void updateReferencePoint(const std::vector<double>& objectives);
    void evaluatePopulation();
    void evolve();
    void updateNeighborSolutions(size_t idx, const SubproblemPtr& child);
    
    // Operadores genéticos
    SubproblemPtr crossover(const SubproblemPtr& parent1, const SubproblemPtr& parent2);
    void mutate(SubproblemPtr individual);
    
    // Utilitários
    void logProgress(size_t generation) const;
    
    // Dados do problema
    const std::vector<Attraction>& attractions_;
    const Parameters params_;
    Population population_;
    std::vector<std::vector<size_t>> neighborhoods_;
    std::vector<std::vector<double>> weight_vectors_;
    std::vector<double> reference_point_;
    
    mutable std::mt19937 rng_{std::random_device{}()};
};

} // namespace tourist