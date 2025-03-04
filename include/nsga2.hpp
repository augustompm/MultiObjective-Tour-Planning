// File: include/nsga2.hpp

#pragma once

#include "base.hpp"
#include "models.hpp"
#include <vector>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <cstddef>
#include <utility>
#include <functional>
#include <algorithm>
#include <limits>

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
    class Individual {
    public:
        struct AttractionGene {
            int attraction_index;
            double arrival_time;      // Hora de chegada na atração (em minutos desde o início do dia)
            double departure_time;    // Hora de saída da atração
            double visit_duration;    // Duração da visita
            
            AttractionGene(int idx = -1);
        };
        
        struct TransportGene {
            utils::TransportMode mode;
            double start_time;       // Hora de início do deslocamento
            double end_time;         // Hora de chegada ao destino
            double duration;         // Duração do deslocamento
            double cost;             // Custo do deslocamento
            double distance;         // Distância percorrida
            
            TransportGene(utils::TransportMode m = utils::TransportMode::CAR);
        };
        
        // Construtor que recebe uma sequência de genes de atração e transporte
        Individual(std::vector<AttractionGene> attraction_genes, std::vector<TransportGene> transport_genes);
        // Construtor que recebe apenas índices de atrações e calcula os detalhes internamente
        explicit Individual(std::vector<int> attraction_indices);
        
        void evaluate(const NSGA2& algorithm);
        bool dominates(const Individual& other) const;
        Route constructRoute(const NSGA2& algorithm) const;
        
        int getRank() const { return rank_; }
        double getCrowdingDistance() const { return crowding_distance_; }
        const std::vector<double>& getObjectives() const { return objectives_; }
        
        void setRank(int rank) { rank_ = rank; }
        void setCrowdingDistance(double distance) { crowding_distance_ = distance; }
        
        // Métodos auxiliares para trabalhar com a nova representação
        double getTotalTime() const { return total_time_; }
        double getTotalCost() const { return total_cost_; }
        int getNumAttractions() const { return static_cast<int>(attraction_genes_.size()); }
        
        // Add a pointer to the NSGA2 objective ranges for normalization.
        const std::vector<std::pair<double, double>>* objective_ranges_ptr = nullptr;
        
        // Reference to the objective ranges for normalization in dominates()
        void setObjectiveRanges(const std::vector<std::pair<double, double>>& ranges) {
            objective_ranges_ptr = &ranges;
        }

    private:
        std::vector<AttractionGene> attraction_genes_;    // Genes de atrações
        std::vector<TransportGene> transport_genes_;      // Genes de transporte entre atrações
        std::vector<double> objectives_;                 // Valores dos objetivos [custo, tempo, -atrações]
        int rank_{0};                                   // Rank na fronteira de Pareto
        double crowding_distance_{0.0};                 // Distância de crowding para preservar diversidade
        
        // Atributos agregados para controle rápido
        double total_time_{0.0};                        // Tempo total da rota
        double total_cost_{0.0};                        // Custo total da rota
        bool is_valid_{false};                         // Flag indicando se a rota é válida
        
        // Método para calcular todos os tempos e custos ao longo da rota
        void calculateTimeAndCosts(const NSGA2& algorithm);
        // Método para determinar os modos de transporte ideais, se não especificados
        void determineTransportModes(const NSGA2& algorithm);
        
        friend class NSGA2;
    };

public:
    NSGA2(const std::vector<Attraction>& attractions, Parameters params = Parameters());

    NSGA2(const NSGA2&) = delete;
    NSGA2& operator=(const NSGA2&) = delete;
    NSGA2(NSGA2&&) = delete;
    NSGA2& operator=(NSGA2&&) = delete;

    // Executa o algoritmo e retorna as soluções não dominadas
    std::vector<Solution> run() override;

private:
    using IndividualPtr = std::shared_ptr<Individual>;
    using Population = std::vector<IndividualPtr>;
    using Front = std::vector<IndividualPtr>;

    // Métodos principais do NSGA-II
    void initializePopulation();
    void evaluatePopulation(Population& pop);
    std::vector<Front> fastNonDominatedSort(const Population& pop) const;
    void calculateCrowdingDistances(Front& front) const;
    Population createOffspring(const Population& parents);
    Population selectNextGeneration(const Population& parents, const Population& offspring);
    
    // Operadores genéticos
    IndividualPtr tournamentSelection(const Population& pop);
    IndividualPtr crossover(const IndividualPtr& parent1, const IndividualPtr& parent2);
    void mutate(IndividualPtr individual);
    void mutateTransportModes(IndividualPtr individual);
    
    // Utilitários
    void logProgress(size_t generation, const Population& pop) const;
    double calculateHypervolume(const Population& pop) const;
    static bool compareByRankAndCrowding(const IndividualPtr& a, const IndividualPtr& b);

    // Validação de cromossomos
    bool isValidChromosome(const std::vector<int>& chrom) const;
    std::vector<int> repairChromosome(std::vector<int>& chrom) const;

    // Dados do problema
    const std::vector<Attraction>& attractions_;
    const Parameters params_;
    Population population_;
    mutable std::mt19937 rng_{std::random_device{}()};

    // Range (min, max) for each objective, used for normalization
    std::vector<std::pair<double, double>> objective_ranges_;
    
    // Function to calculate objective ranges from a population
    void updateObjectiveRanges(const Population& pop);
    
    // Ponto de referência fixo para cálculo de hipervolume consistente
    static const std::vector<double> TRACKING_REFERENCE_POINT;
    
    // Inicializa o ponto de referência com base nos dados das atrações
    void initializeReferencePoint();
};

} // namespace tourist