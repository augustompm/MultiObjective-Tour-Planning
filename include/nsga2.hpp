#pragma once

#include "base.hpp"
#include "models.hpp"
#include <vector>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>

namespace tourist {

class NSGA2 final : public EvolutionaryAlgorithm {
public:
    struct Parameters {
        size_t population_size;
        size_t max_generations;
        double crossover_rate;
        double mutation_rate;

        Parameters()
            : population_size(100)
            , max_generations(100)
            , crossover_rate(0.9)
            , mutation_rate(0.1) {}

        Parameters(size_t pop_size, size_t max_gen, double cross_rate, double mut_rate)
            : population_size(pop_size)
            , max_generations(max_gen)
            , crossover_rate(cross_rate)
            , mutation_rate(mut_rate) {}
        
        void validate() const {
            if (population_size == 0) throw std::invalid_argument("Population size must be positive");
            if (max_generations == 0) throw std::invalid_argument("Generation count must be positive");
            if (crossover_rate < 0.0 || crossover_rate > 1.0) throw std::invalid_argument("Crossover rate must be between 0 and 1");
            if (mutation_rate < 0.0 || mutation_rate > 1.0) throw std::invalid_argument("Mutation rate must be between 0 and 1");
        }
    };

protected:
    class Individual final {
    public:
        explicit Individual(std::vector<int> chromosome);
        void evaluate(const NSGA2& algorithm);
        bool dominates(const Individual& other) const;
        Route constructRoute(const NSGA2& algorithm) const;

        int getRank() const { return rank_; }
        double getCrowdingDistance() const { return crowding_distance_; }
        const std::vector<double>& getObjectives() const { return objectives_; }
        void setRank(int rank) { rank_ = rank; }
        void setCrowdingDistance(double distance) { crowding_distance_ = distance; }

    private:
        std::vector<int> chromosome_;
        std::vector<double> objectives_;
        int rank_{0};
        double crowding_distance_{0.0};

        friend class NSGA2;
    };

public:
    NSGA2(const std::vector<Attraction>& attractions, Parameters params = Parameters());

    NSGA2(const NSGA2&) = delete;
    NSGA2& operator=(const NSGA2&) = delete;
    NSGA2(NSGA2&&) = delete;
    NSGA2& operator=(NSGA2&&) = delete;

    std::vector<Solution> run() override;

private:
    using IndividualPtr = std::shared_ptr<Individual>;
    using Population = std::vector<IndividualPtr>;
    using Front = std::vector<IndividualPtr>;

    void initializePopulation();
    void evaluatePopulation(Population& pop);
    std::vector<Front> fastNonDominatedSort(const Population& pop) const;
    void calculateCrowdingDistances(Front& front) const;
    Population createOffspring(const Population& parents);
    Population selectNextGeneration(const Population& parents, const Population& offspring);
    IndividualPtr tournamentSelection(const Population& pop);
    IndividualPtr crossover(const IndividualPtr& parent1, const IndividualPtr& parent2);
    void mutate(IndividualPtr individual);
    void logProgress(size_t generation, const Population& pop) const;
    double calculateHypervolume(const Population& pop) const;
    static bool compareByRankAndCrowding(const IndividualPtr& a, const IndividualPtr& b);

    bool isValidChromosome(const std::vector<int>& chrom) const;
    std::vector<int> repairChromosome(std::vector<int>& chrom) const;

    const std::vector<Attraction>& attractions_;
    const Parameters params_;
    Population population_;
    mutable std::mt19937 rng_{std::random_device{}()};
};

} // namespace tourist