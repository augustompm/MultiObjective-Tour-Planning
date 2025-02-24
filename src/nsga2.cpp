// File: src/nsga2.cpp

#include "nsga2.hpp"
#include "utils.hpp"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cmath>      // Necessário para std::ceil
#include <set>
#include <stdexcept>
#include <random>

namespace tourist {

NSGA2::Individual::Individual(std::vector<int> chromosome)
    : chromosome_(std::move(chromosome)) {
}

void NSGA2::Individual::evaluate(const NSGA2& algorithm) {
    Route route = constructRoute(algorithm);
    if (route.getSequence().empty()) {
        throw std::runtime_error("Empty route created in evaluation");
    }
    objectives_ = {
        route.getTotalCost(),
        route.getTotalTime(),
        -static_cast<double>(route.getNumAttractions())
    };
}

bool NSGA2::Individual::dominates(const Individual& other) const {
    const auto& other_obj = other.getObjectives();
    
    // Safety check - se algum vetor estiver vazio, não é possível determinar dominância
    if (objectives_.empty() || other_obj.empty()) {
        return false;
    }
    
    // Verifica se ambos os vetores possuem o mesmo tamanho
    if (objectives_.size() != other_obj.size()) {
        return false;
    }
    
    bool at_least_one_better = false;
    for (size_t i = 0; i < objectives_.size(); ++i) {
        if (objectives_[i] > other_obj[i]) return false;
        if (objectives_[i] < other_obj[i]) at_least_one_better = true;
    }
    return at_least_one_better;
}

Route NSGA2::Individual::constructRoute(const NSGA2& algorithm) const {
    Route route(algorithm.hotels_[algorithm.hotel_index_]);
    if (algorithm.attractions_.empty()) {
        throw std::runtime_error("No attractions available");
    }
    for (const auto& idx : chromosome_) {
        if (idx >= 0 && static_cast<size_t>(idx) < algorithm.attractions_.size()) {
            // Passa uma referência para a atração real no vetor attractions_
            const Attraction& attr = algorithm.attractions_[idx];
            route.addAttraction(attr);
        }
    }
    return route;
}

NSGA2::NSGA2(const std::vector<Attraction>& attractions,
             const std::vector<Hotel>& hotels,
             size_t hotel_index,
             Parameters params)
    : attractions_(attractions)
    , hotels_(hotels)
    , hotel_index_(hotel_index)
    , params_(std::move(params))
    , rng_(std::random_device{}()) {
    params_.validate();
    if (hotel_index >= hotels.size()) throw std::runtime_error("Invalid hotel index");
    if (attractions.empty()) throw std::runtime_error("No attractions provided");
}

void NSGA2::initializePopulation() {
    population_.clear();
    population_.reserve(params_.population_size);
    std::vector<int> base_chrom(attractions_.size());
    std::iota(base_chrom.begin(), base_chrom.end(), 0);

    for (size_t i = 0; i < params_.population_size; ++i) {
        auto chrom = base_chrom;
        std::shuffle(chrom.begin(), chrom.end(), rng_);
        auto ind = std::make_shared<Individual>(std::move(chrom));
        population_.push_back(std::move(ind));
    }
    evaluatePopulation(population_);
}

void NSGA2::evaluatePopulation(Population& pop) {
    if (pop.empty()) throw std::runtime_error("Empty population");
    for (auto& ind : pop) {
        if (!ind) throw std::runtime_error("Null individual found");
        ind->evaluate(*this);
    }
}

std::vector<NSGA2::Front> NSGA2::fastNonDominatedSort(const Population& pop) const {
    std::vector<Front> fronts;
    std::vector<std::set<size_t>> dominated_sets(pop.size());
    std::vector<int> domination_counts(pop.size(), 0);

    // Primeiro passe: determinar as relações de dominação
    for (size_t i = 0; i < pop.size(); ++i) {
        for (size_t j = i + 1; j < pop.size(); ++j) {
            if (pop[i]->dominates(*pop[j])) {
                dominated_sets[i].insert(j);
                domination_counts[j]++;
            } else if (pop[j]->dominates(*pop[i])) {
                dominated_sets[j].insert(i);
                domination_counts[i]++;
            }
        }
    }

    // Encontra a primeira fronteira não dominada
    Front current_front;
    for (size_t i = 0; i < pop.size(); ++i) {
        if (domination_counts[i] == 0) {
            pop[i]->setRank(0);
            current_front.push_back(pop[i]);
        }
    }
    fronts.push_back(current_front);

    // Gera as fronteiras subsequentes
    size_t current_rank = 0;
    while (!fronts.back().empty()) {
        Front next_front;
        for (const auto& ind : fronts.back()) {
            auto it = std::find(pop.begin(), pop.end(), ind);
            if (it == pop.end()) {
                continue; // Não deve ocorrer na prática
            }
            size_t ind_idx = std::distance(pop.begin(), it);
            for (size_t j : dominated_sets[ind_idx]) {
                if (--domination_counts[j] == 0) {
                    pop[j]->setRank(current_rank + 1);
                    next_front.push_back(pop[j]);
                }
            }
        }
        if (!next_front.empty()) {
            fronts.push_back(next_front);
            current_rank++;
        } else break;
    }
    return fronts;
}

// Nova implementação de calculateCrowdingDistances
void NSGA2::calculateCrowdingDistances(Front& front) const {
    if (front.empty()) return;
    if (front.size() == 1) {
        front[0]->setCrowdingDistance(std::numeric_limits<double>::infinity());
        return;
    }

    const size_t n = front.size();

    // Reset das distâncias de crowding
    for (auto& ind : front) {
        ind->setCrowdingDistance(0.0);
    }

    // Verifica se algum indivíduo possui objetivos inválidos
    for (const auto& ind : front) {
        if (!ind || ind->getObjectives().empty()) {
            // Caso haja indivíduo inválido, atribui infinito a todos
            for (auto& ind : front) {
                if (ind) ind->setCrowdingDistance(std::numeric_limits<double>::infinity());
            }
            return;
        }
    }

    const size_t num_objectives = front[0]->getObjectives().size();

    for (size_t m = 0; m < num_objectives; m++) {
        // Ordenação segura para evitar possíveis erros com iteradores inválidos
        try {
            std::vector<std::pair<size_t, double>> index_value_pairs;
            index_value_pairs.reserve(n);
            
            for (size_t i = 0; i < n; i++) {
                if (!front[i] || front[i]->getObjectives().empty() || m >= front[i]->getObjectives().size()) {
                    continue;
                }
                index_value_pairs.emplace_back(i, front[i]->getObjectives()[m]);
            }
            
            // Ordena os pares pelo valor do objetivo
            std::sort(index_value_pairs.begin(), index_value_pairs.end(),
                      [](const auto& a, const auto& b) { return a.second < b.second; });
            
            // Se não houver indivíduos válidos suficientes, passa para o próximo objetivo
            if (index_value_pairs.size() < 2) continue;
            
            // Define os indivíduos de fronteira com distância infinita
            size_t first_idx = index_value_pairs.front().first;
            size_t last_idx = index_value_pairs.back().first;
            
            front[first_idx]->setCrowdingDistance(std::numeric_limits<double>::infinity());
            front[last_idx]->setCrowdingDistance(std::numeric_limits<double>::infinity());
            
            // Calcula a faixa do objetivo para normalização
            double obj_range = index_value_pairs.back().second - index_value_pairs.front().second;
            
            // Se os valores forem iguais, ignora este objetivo
            if (obj_range <= 0.0) continue;
            
            // Calcula a distância de crowding para os pontos internos
            for (size_t i = 1; i < index_value_pairs.size() - 1; i++) {
                size_t curr_idx = index_value_pairs[i].first;
                size_t prev_idx = index_value_pairs[i-1].first;
                size_t next_idx = index_value_pairs[i+1].first;
                
                double prev_val = front[prev_idx]->getObjectives()[m];
                double next_val = front[next_idx]->getObjectives()[m];
                
                double distance_contribution = (next_val - prev_val) / obj_range;
                front[curr_idx]->setCrowdingDistance(
                    front[curr_idx]->getCrowdingDistance() + distance_contribution
                );
            }
        }
        catch (const std::exception& e) {
            // Em caso de exceção, atribui distância infinita a todos os indivíduos
            for (auto& ind : front) {
                if (ind) ind->setCrowdingDistance(std::numeric_limits<double>::infinity());
            }
            break;
        }
    }
}

NSGA2::IndividualPtr NSGA2::tournamentSelection(const Population& pop) {
    if (pop.size() < 2) throw std::runtime_error("Population too small for tournament");
    std::uniform_int_distribution<size_t> dist(0, pop.size() - 1);
    auto ind1 = pop[dist(rng_)];
    auto ind2 = pop[dist(rng_)];
    
    if (ind1->getRank() < ind2->getRank()) return ind1;
    if (ind2->getRank() < ind1->getRank()) return ind2;
    return (ind1->getCrowdingDistance() > ind2->getCrowdingDistance()) ? ind1 : ind2;
}

NSGA2::IndividualPtr NSGA2::crossover(const IndividualPtr& parent1, const IndividualPtr& parent2) {
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> point_dist(0, attractions_.size() - 1);
    
    if (prob_dist(rng_) > params_.crossover_rate) {
        return std::make_shared<Individual>(parent1->chromosome_);
    }

    const size_t n = attractions_.size();
    size_t point1 = point_dist(rng_);
    size_t point2 = point_dist(rng_);
    if (point1 > point2) std::swap(point1, point2);

    std::vector<int> child(n, -1);
    std::vector<bool> used(n, false);

    for (size_t i = point1; i <= point2; i++) {
        child[i] = parent1->chromosome_[i];
        used[parent1->chromosome_[i]] = true;
    }

    size_t fill_pos = (point2 + 1) % n;
    for (const auto& gene : parent2->chromosome_) {
        if (!used[gene]) {
            child[fill_pos] = gene;
            fill_pos = (fill_pos + 1) % n;
            used[gene] = true;
        }
    }

    for (size_t i = 0; i < point1; ++i) {
        if (child[i] == -1) {
            for (size_t j = 0; j < n; ++j) {
                if (!used[j]) {
                    child[i] = j;
                    used[j] = true;
                    break;
                }
            }
        }
    }

    auto offspring = std::make_shared<Individual>(std::move(child));
    offspring->evaluate(*this);
    return offspring;
}

void NSGA2::mutate(IndividualPtr individual) {
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    if (prob_dist(rng_) > params_.mutation_rate) return;

    std::uniform_int_distribution<size_t> pos_dist(0, attractions_.size() - 1);
    size_t pos1 = pos_dist(rng_);
    size_t pos2 = pos_dist(rng_);
    
    std::swap(individual->chromosome_[pos1], individual->chromosome_[pos2]);
    individual->evaluate(*this);
}

NSGA2::Population NSGA2::createOffspring(const Population& parents) {
    Population offspring;
    offspring.reserve(parents.size());
    
    for (size_t i = 0; i < parents.size(); ++i) {
        auto parent1 = tournamentSelection(parents);
        auto parent2 = tournamentSelection(parents);
        auto child = crossover(parent1, parent2);
        mutate(child);
        offspring.push_back(std::move(child));
    }
    return offspring;
}

NSGA2::Population NSGA2::selectNextGeneration(const Population& parents, const Population& offspring) {
    Population combined;
    combined.reserve(parents.size() + offspring.size());
    combined.insert(combined.end(), parents.begin(), parents.end());
    combined.insert(combined.end(), offspring.begin(), offspring.end());

    auto fronts = fastNonDominatedSort(combined);
    Population next_gen;
    next_gen.reserve(params_.population_size);
    
    size_t i = 0;
    while (i < fronts.size() && next_gen.size() + fronts[i].size() <= params_.population_size) {
        calculateCrowdingDistances(fronts[i]);
        next_gen.insert(next_gen.end(), fronts[i].begin(), fronts[i].end());
        i++;
    }

    if (next_gen.size() < params_.population_size && i < fronts.size()) {
        calculateCrowdingDistances(fronts[i]);
        std::sort(fronts[i].begin(), fronts[i].end(), compareByRankAndCrowding);
        
        size_t remaining = params_.population_size - next_gen.size();
        next_gen.insert(next_gen.end(), fronts[i].begin(), 
                        fronts[i].begin() + std::min(remaining, fronts[i].size()));
    }

    return next_gen;
}

std::vector<Solution> NSGA2::run() {
    initializePopulation();
    
    for (size_t gen = 0; gen < params_.max_generations; ++gen) {
        auto offspring = createOffspring(population_);
        population_ = selectNextGeneration(population_, offspring);
        logProgress(gen, population_);
    }

    std::vector<Solution> solutions;
    auto final_front = fastNonDominatedSort(population_)[0];
    solutions.reserve(final_front.size());
    
    for (const auto& ind : final_front) {
        solutions.push_back(Solution(ind->constructRoute(*this)));
    }
    return solutions;
}

void NSGA2::logProgress(size_t generation, const Population& pop) const {
    auto fronts = fastNonDominatedSort(pop);
    if (!fronts.empty()) {
        double hv = calculateHypervolume(pop);
        std::cout << "Generation " << generation << ": Front size = " << fronts[0].size()
                  << ", Hypervolume = " << hv << std::endl;
    }
}

double NSGA2::calculateHypervolume(const Population& pop) const {
    auto fronts = fastNonDominatedSort(pop);
    if (fronts.empty()) return 0.0;
    
    auto& front = fronts[0];
    std::vector<Solution> solutions;
    solutions.reserve(front.size());
    
    for (const auto& ind : front) {
        solutions.push_back(Solution(ind->constructRoute(*this)));
    }
    
    std::vector<double> reference_point = {10000.0, 1440.0, 0.0};
    return utils::Metrics::calculateHypervolume(solutions, reference_point);
}

bool NSGA2::compareByRankAndCrowding(const IndividualPtr& a, const IndividualPtr& b) {
    if (a->getRank() != b->getRank()) return a->getRank() < b->getRank();
    return a->getCrowdingDistance() > b->getCrowdingDistance();
}

} // namespace tourist
