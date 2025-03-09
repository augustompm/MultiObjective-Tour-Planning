// File: src/nsga2-base.cpp
// Implementation of NSGA-II algorithm based on:
// "A Fast and Elitist Multiobjective Genetic Algorithm: NSGA-II" by Deb et al., 2002

#include "nsga2-base.hpp"
#include "utils.hpp"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <cmath>
#include <unordered_set>
#include <iomanip>  // For std::setprecision

namespace tourist {

// Validate parameters for NSGA-II
void NSGA2Base::Parameters::validate() const {
    if (population_size == 0) throw std::invalid_argument("Population size must be positive");
    if (max_generations == 0) throw std::invalid_argument("Generation count must be positive");
    if (crossover_rate < 0.0 || crossover_rate > 1.0) 
        throw std::invalid_argument("Crossover rate must be between 0 and 1");
    if (mutation_rate < 0.0 || mutation_rate > 1.0) 
        throw std::invalid_argument("Mutation rate must be between 0 and 1");
}

// Individual implementation
NSGA2Base::Individual::Individual(std::vector<int> attraction_indices)
    : chromosome_(std::move(attraction_indices)) {
    
    // Initialize transport modes between attractions
    if (chromosome_.size() > 1) {
        transport_modes_.resize(chromosome_.size() - 1, utils::TransportMode::CAR);
    }
}

void NSGA2Base::Individual::evaluate(const NSGA2Base& algorithm) {
    // Create a Route to evaluate objectives
    Route route = constructRoute(algorithm);
    
    // Get objectives from the route
    double total_cost = route.getTotalCost();
    double total_time = route.getTotalTime();
    int num_attractions = route.getNumAttractions();
    
    // Check if route is valid
    bool is_valid = route.isValid();
    
    // Apply penalties for invalid routes or empty routes
    if (!is_valid || num_attractions == 0) {
        objectives_ = {
            1000.0,                               // High cost penalty
            utils::Config::DAILY_TIME_LIMIT,      // Excessive time penalty
            -1.0                                  // Few attractions penalty
        };
    } else {
        // Check if time exceeds the limit with tolerance
        double time_penalty = 0.0;
        double max_time = utils::Config::DAILY_TIME_LIMIT * (1.0 + utils::Config::TOLERANCE);
        
        if (total_time > max_time) {
            // Calculate time penalty proportionally
            double violation = total_time - max_time;
            time_penalty = violation * (1.0 + violation / max_time);
        }
        
        // Set objectives with accurate cost calculation
        objectives_ = {
            total_cost,                           // Minimize cost
            total_time + time_penalty,            // Minimize time
            -static_cast<double>(num_attractions) // Maximize attractions (negative for minimization)
        };
    }
}

bool NSGA2Base::Individual::dominates(const Individual& other) const {
    // According to Deb's paper Section III:
    // A solution i is said to dominate solution j if:
    // 1) Solution i is no worse than j in all objectives
    // 2) Solution i is strictly better than j in at least one objective
    
    const auto& other_obj = other.getObjectives();
    
    // Check dominance in all objectives
    bool at_least_one_better = false;
    
    for (size_t i = 0; i < objectives_.size(); ++i) {
        if (i == 2) {
            // For attractions (objective 2, negated), lower is better (maximizing original value)
            if (objectives_[i] > other_obj[i]) {
                return false;  // Solution i is worse in this objective
            }
            if (objectives_[i] < other_obj[i]) {
                at_least_one_better = true;
            }
        } else {
            // For cost and time (objectives 0 and 1), lower is better (minimizing)
            if (objectives_[i] > other_obj[i]) {
                return false;  // Solution i is worse in this objective
            }
            if (objectives_[i] < other_obj[i]) {
                at_least_one_better = true;
            }
        }
    }
    
    return at_least_one_better;
}

void NSGA2Base::Individual::determineTransportModes(const NSGA2Base& algorithm) {
    if (chromosome_.size() <= 1) return;
    
    // Initialize transport modes
    transport_modes_.resize(chromosome_.size() - 1);
    
    // Determine optimal mode for each segment
    for (size_t i = 0; i < chromosome_.size() - 1; ++i) {
        int from_idx = chromosome_[i];
        int to_idx = chromosome_[i + 1];
        
        // Check for valid indices
        if (from_idx >= 0 && static_cast<size_t>(from_idx) < algorithm.attractions_.size() &&
            to_idx >= 0 && static_cast<size_t>(to_idx) < algorithm.attractions_.size()) {
            
            try {
                // Get attraction names
                const std::string& from_name = algorithm.attractions_[from_idx].getName();
                const std::string& to_name = algorithm.attractions_[to_idx].getName();
                
                // Get walking time between attractions
                double walk_time = utils::Transport::getTravelTime(
                    from_name, to_name, utils::TransportMode::WALK
                );
                
                // Use walking if time is acceptable, otherwise use car
                if (walk_time <= utils::Config::WALK_TIME_PREFERENCE) {
                    transport_modes_[i] = utils::TransportMode::WALK;
                } else {
                    transport_modes_[i] = utils::TransportMode::CAR;
                }
            } catch (const std::exception&) {
                // Default to car if there's an error
                transport_modes_[i] = utils::TransportMode::CAR;
            }
        } else {
            // Default to car for invalid indices
            transport_modes_[i] = utils::TransportMode::CAR;
        }
    }
}

Route NSGA2Base::Individual::constructRoute(const NSGA2Base& algorithm) const {
    Route route;
    
    // Check for empty chromosome
    if (chromosome_.empty()) {
        return route;
    }
    
    try {
        // Add first attraction to the route
        if (chromosome_[0] >= 0 && 
            static_cast<size_t>(chromosome_[0]) < algorithm.attractions_.size()) {
            const auto& first_attr = algorithm.attractions_[chromosome_[0]];
            route.addAttraction(first_attr);
            
            // Add subsequent attractions with transport modes
            for (size_t i = 1; i < chromosome_.size(); ++i) {
                if (chromosome_[i] >= 0 && 
                    static_cast<size_t>(chromosome_[i]) < algorithm.attractions_.size()) {
                    const auto& attr = algorithm.attractions_[chromosome_[i]];
                    
                    // Use the corresponding transport mode (default to CAR if not available)
                    utils::TransportMode mode = (i-1 < transport_modes_.size()) ? 
                                              transport_modes_[i-1] : utils::TransportMode::CAR;
                    
                    route.addAttraction(attr, mode);
                }
            }
        }
    } catch (const std::exception&) {
        // Handle error silently
    }
    
    return route;
}

// NSGA2Base implementation
NSGA2Base::NSGA2Base(const std::vector<Attraction>& attractions, Parameters params)
    : attractions_(attractions)
    , params_(std::move(params)) {
    
    // Validate parameters
    params_.validate();
    
    // Ensure we have attractions
    if (attractions.empty()) {
        throw std::runtime_error("No attractions provided");
    }
    
    // Verify transport matrices are loaded
    if (!utils::TransportMatrices::matrices_loaded) {
        throw std::runtime_error("Transport matrices must be loaded before initializing NSGA-II");
    }
}

void NSGA2Base::initializePopulation() {
    population_.clear();
    population_.reserve(params_.population_size);
    
    // Create a base chromosome with all attraction indices
    std::vector<int> base_chrom(attractions_.size());
    std::iota(base_chrom.begin(), base_chrom.end(), 0);  // Fill with 0, 1, 2, ..., n-1
    
    // Create initial population with diverse solutions
    for (size_t i = 0; i < params_.population_size; ++i) {
        // Determine chromosome size - create diversity in initial population
        size_t chrom_size;
        if (i < params_.population_size / 3) {
            // First third: full-sized chromosomes
            chrom_size = std::min(size_t(8), attractions_.size()); // Limit to 8 attractions max
        } else if (i < params_.population_size * 2 / 3) {
            // Middle third: medium-sized chromosomes
            std::uniform_int_distribution<size_t> dist(
                std::min(size_t(3), attractions_.size() / 2), 
                std::min(size_t(6), attractions_.size()));
            chrom_size = dist(rng_);
        } else {
            // Last third: small-sized chromosomes
            std::uniform_int_distribution<size_t> dist(1, 
                std::min(size_t(4), attractions_.size() / 2));
            chrom_size = dist(rng_);
        }
        
        // Create a copy of the base chromosome
        auto chrom = base_chrom;
        
        // Shuffle to create a random order
        std::shuffle(chrom.begin(), chrom.end(), rng_);
        
        // Resize to desired length
        if (chrom_size < chrom.size()) {
            chrom.resize(chrom_size);
        }
        
        // Create and add the individual
        auto ind = std::make_shared<Individual>(std::move(chrom));
        ind->determineTransportModes(*this);
        population_.push_back(std::move(ind));
    }
    
    // Evaluate initial population
    evaluatePopulation(population_);
}

void NSGA2Base::evaluatePopulation(Population& pop) {
    for (auto& ind : pop) {
        ind->evaluate(*this);
    }
}

// Fast Non-dominated Sorting Approach (Section III-A)
// Implementation follows exactly the algorithm described in Figure 1 of the paper
std::vector<NSGA2Base::Front> NSGA2Base::fastNonDominatedSort(const Population& pop) const {
    std::vector<Front> fronts;
    
    // Initialize data structures as per Fig. 1 in Deb's paper
    std::vector<std::vector<size_t>> S(pop.size());  // S_p = set of solutions dominated by p
    std::vector<size_t> n(pop.size(), 0);            // n_p = domination counter (solutions that dominate p)
    
    // For each p in P
    for (size_t p = 0; p < pop.size(); ++p) {
        S[p].clear();
        n[p] = 0;
        
        // For each q in P
        for (size_t q = 0; q < pop.size(); ++q) {
            if (p == q) continue;
            
            // If p dominates q
            if (pop[p]->dominates(*pop[q])) {
                // Add q to the set of solutions dominated by p
                S[p].push_back(q);
            } 
            // If q dominates p
            else if (pop[q]->dominates(*pop[p])) {
                // Increment the domination counter of p
                n[p]++;
            }
        }
        
        // If no one dominates p, p belongs to the first front
        if (n[p] == 0) {
            pop[p]->setRank(0);  // First front has rank 0
            if (fronts.empty()) {
                fronts.push_back(Front());
            }
            fronts[0].push_back(pop[p]);
        }
    }
    
    // Generate subsequent fronts
    size_t i = 0;
    while (i < fronts.size()) {
        Front next_front;
        
        // For each p in F_i
        for (const auto& p : fronts[i]) {
            size_t p_idx = std::distance(pop.begin(), 
                                      std::find(pop.begin(), pop.end(), p));
            
            // For each q in S_p (solutions dominated by p)
            for (size_t q : S[p_idx]) {
                // Decrement domination counter
                n[q]--;
                
                // If domination counter becomes 0, add to next front
                if (n[q] == 0) {
                    pop[q]->setRank(i + 1);
                    next_front.push_back(pop[q]);
                }
            }
        }
        
        // If next front is empty, we're done
        if (next_front.empty()) {
            break;
        }
        
        // Add next front
        fronts.push_back(next_front);
        
        // Increment front counter
        i++;
    }
    
    return fronts;
}

// Crowding Distance Assignment (Section III-B)
void NSGA2Base::calculateCrowdingDistances(Front& front) const {
    size_t n = front.size();
    
    // Skip if front is empty or has only one solution
    if (n <= 1) {
        if (n == 1) {
            front[0]->setCrowdingDistance(std::numeric_limits<double>::infinity());
        }
        return;
    }
    
    // Initialize distances to zero
    for (auto& ind : front) {
        ind->setCrowdingDistance(0.0);
    }
    
    // Number of objectives
    size_t m = front[0]->getObjectives().size();
    
    // For each objective m (as per III-B)
    for (size_t obj = 0; obj < m; ++obj) {
        // Sort the population based on objective m
        std::sort(front.begin(), front.end(),
                 [obj](const IndividualPtr& a, const IndividualPtr& b) {
                     return a->getObjectives()[obj] < b->getObjectives()[obj];
                 });
        
        // Boundary points are assigned an infinite distance
        front[0]->setCrowdingDistance(std::numeric_limits<double>::infinity());
        front[n-1]->setCrowdingDistance(std::numeric_limits<double>::infinity());
        
        // Calculate range for this objective
        double obj_min = front[0]->getObjectives()[obj];
        double obj_max = front[n-1]->getObjectives()[obj];
        double range = obj_max - obj_min;
        
        // Skip if all solutions have the same value for this objective
        if (std::fabs(range) < 1e-10) continue;
        
        // For i = 2 to (l-1) - compute normalized distances
        for (size_t i = 1; i < n - 1; ++i) {
            // Add the normalized distance to existing crowding distance
            double distance = (front[i+1]->getObjectives()[obj] - 
                             front[i-1]->getObjectives()[obj]) / range;
            front[i]->setCrowdingDistance(front[i]->getCrowdingDistance() + distance);
        }
    }
}

NSGA2Base::Population NSGA2Base::createOffspring(const Population& parents) {
    Population offspring;
    offspring.reserve(parents.size());
    
    // Create offspring using tournament selection, crossover, and mutation
    while (offspring.size() < parents.size()) {
        // Select parents using tournament selection
        auto parent1 = tournamentSelection(parents);
        auto parent2 = tournamentSelection(parents);
        
        // Skip if parents are the same
        if (parent1 == parent2 && parents.size() > 1) {
            continue;
        }
        
        // Apply crossover with probability
        std::uniform_real_distribution<> prob_dist(0.0, 1.0);
        auto child = (prob_dist(rng_) <= params_.crossover_rate) ?
                    crossover(parent1, parent2) :
                    std::make_shared<Individual>(parent1->getChromosome());
        
        // Apply mutation with probability
        if (prob_dist(rng_) <= params_.mutation_rate) {
            mutate(child);
        }
        
        // Add to offspring population
        offspring.push_back(std::move(child));
    }
    
    // Evaluate offspring
    evaluatePopulation(offspring);
    
    return offspring;
}

// Binary Tournament Selection with crowded-comparison operator (Section III-C)
NSGA2Base::IndividualPtr NSGA2Base::tournamentSelection(const Population& pop) {
    std::uniform_int_distribution<size_t> dist(0, pop.size() - 1);
    
    // Select two random individuals
    auto ind1 = pop[dist(rng_)];
    auto ind2 = pop[dist(rng_)];
    
    // Crowded Tournament Selection
    // A solution i wins tournament if:
    // 1) It has a better rank than j, or
    // 2) It has the same rank but better crowding distance than j
    
    if (ind1->getRank() < ind2->getRank()) {
        return ind1;  // ind1 has better (lower) rank
    } 
    else if (ind2->getRank() < ind1->getRank()) {
        return ind2;  // ind2 has better (lower) rank
    }
    else {
        // Same rank, select based on crowding distance (higher is better)
        return (ind1->getCrowdingDistance() > ind2->getCrowdingDistance()) ? ind1 : ind2;
    }
}

// Crossover operator - we use a problem-specific crossover (PMX - Partially Matched Crossover)
NSGA2Base::IndividualPtr NSGA2Base::crossover(const IndividualPtr& parent1, const IndividualPtr& parent2) {
    // Get parent chromosomes
    const auto& p1_chrom = parent1->getChromosome();
    const auto& p2_chrom = parent2->getChromosome();
    
    if (p1_chrom.empty() || p2_chrom.empty()) {
        // Handle empty chromosomes
        return std::make_shared<Individual>(p1_chrom.empty() ? p2_chrom : p1_chrom);
    }
    
    // Determine child size (between min and max parent sizes, but limit max size)
    size_t min_size = std::min(p1_chrom.size(), p2_chrom.size());
    size_t max_size = std::min(size_t(8), std::max(p1_chrom.size(), p2_chrom.size()));
    std::uniform_int_distribution<size_t> size_dist(min_size, max_size);
    size_t child_size = size_dist(rng_);
    
    // Create a map to track which attractions are already included
    std::vector<bool> included(attractions_.size(), false);
    
    // Select crossover points
    std::uniform_int_distribution<size_t> point_dist(0, p1_chrom.size() - 1);
    size_t cx_point1 = point_dist(rng_);
    size_t cx_point2 = point_dist(rng_);
    
    // Ensure cx_point1 <= cx_point2
    if (cx_point1 > cx_point2) {
        std::swap(cx_point1, cx_point2);
    }
    
    // Start with an empty child chromosome
    std::vector<int> child_chrom;
    child_chrom.reserve(child_size);
    
    // First, copy the segment from parent1 between crossover points
    for (size_t i = cx_point1; i <= cx_point2 && i < p1_chrom.size(); ++i) {
        int gene = p1_chrom[i];
        if (gene >= 0 && static_cast<size_t>(gene) < attractions_.size()) {
            child_chrom.push_back(gene);
            included[gene] = true;
        }
    }
    
    // Fill the remaining positions with genes from parent2
    for (size_t i = 0; i < p2_chrom.size() && child_chrom.size() < child_size; ++i) {
        int gene = p2_chrom[i];
        if (gene >= 0 && static_cast<size_t>(gene) < attractions_.size() && !included[gene]) {
            child_chrom.push_back(gene);
            included[gene] = true;
        }
    }
    
    // If still need more genes, add from parent1
    for (size_t i = 0; i < p1_chrom.size() && child_chrom.size() < child_size; ++i) {
        // Skip the segment already copied
        if (i >= cx_point1 && i <= cx_point2) continue;
        
        int gene = p1_chrom[i];
        if (gene >= 0 && static_cast<size_t>(gene) < attractions_.size() && !included[gene]) {
            child_chrom.push_back(gene);
            included[gene] = true;
        }
    }
    
    // If still not enough, try random attractions
    if (child_chrom.size() < child_size && child_chrom.size() < attractions_.size()) {
        std::vector<int> available;
        for (size_t i = 0; i < attractions_.size(); ++i) {
            if (!included[i]) {
                available.push_back(i);
            }
        }
        
        std::shuffle(available.begin(), available.end(), rng_);
        
        for (int gene : available) {
            if (child_chrom.size() >= child_size) break;
            child_chrom.push_back(gene);
        }
    }
    
    // Create the child individual
    auto child = std::make_shared<Individual>(std::move(child_chrom));
    
    // Determine optimal transport modes
    child->determineTransportModes(*this);
    
    return child;
}

// Mutation operator - we use a problem-specific mutation
void NSGA2Base::mutate(IndividualPtr individual) {
    auto& chrom = const_cast<std::vector<int>&>(individual->getChromosome());
    
    if (chrom.size() < 2) {
        return;  // Need at least 2 genes for mutation
    }
    
    // Choose a mutation type
    std::uniform_int_distribution<int> mut_type_dist(0, 2);
    int mutation_type = mut_type_dist(rng_);
    
    switch (mutation_type) {
        case 0: {
            // Swap Mutation: Swap two random attractions
            std::uniform_int_distribution<size_t> pos_dist(0, chrom.size() - 1);
            size_t pos1 = pos_dist(rng_);
            size_t pos2 = pos_dist(rng_);
            
            // Ensure different positions
            while (pos1 == pos2 && chrom.size() > 1) {
                pos2 = pos_dist(rng_);
            }
            
            // Swap genes
            std::swap(chrom[pos1], chrom[pos2]);
            break;
        }
        
        case 1: {
            // Insert Mutation: Move a random attraction to a new position
            std::uniform_int_distribution<size_t> pos_dist(0, chrom.size() - 1);
            size_t from_pos = pos_dist(rng_);
            size_t to_pos = pos_dist(rng_);
            
            // Skip if positions are the same
            if (from_pos == to_pos) break;
            
            // Store the gene to move
            int gene = chrom[from_pos];
            
            // Remove from original position
            chrom.erase(chrom.begin() + from_pos);
            
            // Adjust to_pos if needed
            if (to_pos > from_pos) {
                to_pos--;
            }
            
            // Insert at new position
            chrom.insert(chrom.begin() + to_pos, gene);
            break;
        }
        
        case 2: {
            // Add/Remove Mutation: Add or remove an attraction
            std::uniform_real_distribution<> prob_dist(0.0, 1.0);
            
            if (chrom.size() < std::min(size_t(8), attractions_.size()) && prob_dist(rng_) < 0.5) {
                // Add a new attraction
                
                // Find attractions not already in the chromosome
                std::vector<int> available;
                std::vector<bool> used(attractions_.size(), false);
                
                for (int gene : chrom) {
                    if (gene >= 0 && static_cast<size_t>(gene) < attractions_.size()) {
                        used[gene] = true;
                    }
                }
                
                for (size_t i = 0; i < attractions_.size(); ++i) {
                    if (!used[i]) {
                        available.push_back(i);
                    }
                }
                
                // Add a random available attraction
                if (!available.empty()) {
                    std::uniform_int_distribution<size_t> idx_dist(0, available.size() - 1);
                    int new_gene = available[idx_dist(rng_)];
                    
                    // Insert at a random position
                    std::uniform_int_distribution<size_t> pos_dist(0, chrom.size());
                    size_t pos = pos_dist(rng_);
                    
                    chrom.insert(chrom.begin() + pos, new_gene);
                }
            } 
            else if (chrom.size() > 1) {
                // Remove a random attraction
                std::uniform_int_distribution<size_t> pos_dist(0, chrom.size() - 1);
                size_t pos = pos_dist(rng_);
                
                chrom.erase(chrom.begin() + pos);
            }
            break;
        }
    }
    
    // Update transport modes after mutation
    individual->determineTransportModes(*this);
}

// "The overall algorithm" (Section III-C)
NSGA2Base::Population NSGA2Base::selectNextGeneration(const Population& parents, const Population& offspring) {
    // Rt = Pt ∪ Qt (combine parent and offspring populations)
    Population combined;
    combined.reserve(parents.size() + offspring.size());
    combined.insert(combined.end(), parents.begin(), parents.end());
    combined.insert(combined.end(), offspring.begin(), offspring.end());
    
    // F = fast-non-dominated-sort(Rt)
    auto fronts = fastNonDominatedSort(combined);
    
    // Pt+1 = ∅ and i = 1
    Population next_gen;
    next_gen.reserve(params_.population_size);
    
    // Until |Pt+1| + |Fi| ≤ N (where N is population size)
    size_t i = 0;
    while (i < fronts.size() && next_gen.size() + fronts[i].size() <= params_.population_size) {
        // Calculate crowding distance in Fi
        calculateCrowdingDistances(fronts[i]);
        
        // Pt+1 = Pt+1 ∪ Fi
        next_gen.insert(next_gen.end(), fronts[i].begin(), fronts[i].end());
        
        // i = i + 1
        i++;
    }
    
    // If we need more solutions to fill the population
    if (next_gen.size() < params_.population_size && i < fronts.size()) {
        // Calculate crowding distance in Fi
        calculateCrowdingDistances(fronts[i]);
        
        // Sort Fi by crowded comparison operator
        std::sort(fronts[i].begin(), fronts[i].end(), 
                 [](const IndividualPtr& a, const IndividualPtr& b) {
                     // Higher crowding distance is better for same rank
                     return a->getCrowdingDistance() > b->getCrowdingDistance();
                 });
        
        // Add solutions from Fi to Pt+1 until |Pt+1| = N
        size_t remaining = params_.population_size - next_gen.size();
        next_gen.insert(next_gen.end(), fronts[i].begin(), 
                      fronts[i].begin() + std::min(remaining, fronts[i].size()));
    }
    
    return next_gen;
}

std::vector<Solution> NSGA2Base::run() {
    // Create file for tracking generations data
    std::ofstream generations_file("geracoes_nsga2_base.csv", std::ios::out);
    if (generations_file.is_open()) {
        generations_file << "Generation;Front size;Best Cost;Best Time;Max Attractions" << std::endl;
    }
    
    // Initialize population
    initializePopulation();
    
    // Main NSGA-II loop - evolve for max_generations
    for (size_t gen = 0; gen < params_.max_generations; ++gen) {
        // Create offspring through selection, crossover, and mutation
        Population offspring = createOffspring(population_);
        
        // Select next generation from combined parent and offspring populations
        population_ = selectNextGeneration(population_, offspring);
        
        // Calculate non-dominated fronts for the new population
        auto fronts = fastNonDominatedSort(population_);
        
        // Log progress
        logProgress(gen, fronts);
        
        // Record generation data for first front
        if (generations_file.is_open() && !fronts.empty() && !fronts[0].empty()) {
            // Find best values for each objective in the first front
            double best_cost = std::numeric_limits<double>::max();
            double best_time = std::numeric_limits<double>::max();
            double max_attractions = 0;
            
            // Flag to track if we found valid solutions
            bool found_valid = false;
            
            for (const auto& ind : fronts[0]) {
                const auto& obj = ind->getObjectives();
                Route test_route = ind->constructRoute(*this);
                
                // Check if values are valid (not penalty values) and route is valid
                if (obj[0] < 999.0 && obj[1] < utils::Config::DAILY_TIME_LIMIT && 
                    test_route.isValid() && !test_route.getAttractions().empty()) {
                    found_valid = true;
                    best_cost = std::min(best_cost, obj[0]);
                    best_time = std::min(best_time, obj[1]);
                    max_attractions = std::max(max_attractions, -obj[2]); // Negate to get actual number
                }
            }
            
            // Only write if we found at least one valid solution
            if (found_valid) {
                generations_file << gen << ";" 
                                << fronts[0].size() << ";" 
                                << best_cost << ";" 
                                << best_time << ";"
                                << max_attractions << std::endl;
            }
        }
    }
    
    // Find non-dominated solutions in final population
    auto final_fronts = fastNonDominatedSort(population_);
    std::vector<Solution> solutions;
    
    // Convert individuals in the first front to Solution objects
    if (!final_fronts.empty()) {
        solutions.reserve(final_fronts[0].size());
        
        // First, deduplicate similar solutions
        std::unordered_set<std::string> solution_hashes;
        
        for (const auto& ind : final_fronts[0]) {
            Route route = ind->constructRoute(*this);
            
            // Only add valid routes with at least one attraction
            if (!route.getAttractions().empty() && route.isValid()) {
                // Create a hash based on the sequence of attractions
                std::string hash;
                std::string reverse_hash;
                
                const auto& attractions = route.getAttractions();
                
                // Forward hash
                for (const auto* attr : attractions) {
                    hash += attr->getName() + "|";
                }
                
                // Reverse hash (to detect inverted routes)
                for (auto it = attractions.rbegin(); it != attractions.rend(); ++it) {
                    reverse_hash += (*it)->getName() + "|";
                }
                
                // Only add if neither this solution nor its reverse already exists
                if (solution_hashes.find(hash) == solution_hashes.end() && 
                    solution_hashes.find(reverse_hash) == solution_hashes.end()) {
                    solutions.push_back(Solution(route));
                    solution_hashes.insert(hash);
                    solution_hashes.insert(reverse_hash); // Also prevent reverse from being added later
                }
            }
        }
    }
    
    // Sort solutions by diversity - make sure we have a diverse set
    if (solutions.size() > 1) {
        std::sort(solutions.begin(), solutions.end(),
                 [](const Solution& a, const Solution& b) {
                     const auto& obj_a = a.getObjectives();
                     const auto& obj_b = b.getObjectives();
                     
                     // First by number of attractions (descending)
                     if (obj_a[2] != obj_b[2]) {
                         return obj_a[2] < obj_b[2]; // More negative = more attractions
                     }
                     
                     // Then by cost (ascending)
                     if (std::abs(obj_a[0] - obj_b[0]) > 1e-6) {
                         return obj_a[0] < obj_b[0];
                     }
                     
                     // Then by time (ascending)
                     return obj_a[1] < obj_b[1];
                 });
    }
    
    if (generations_file.is_open()) {
        generations_file.close();
    }
    
    return solutions;
}

void NSGA2Base::logProgress(size_t generation, const std::vector<Front>& fronts) const {
    if (!fronts.empty()) {
        std::cout << "Generation " << generation 
                << ": Front size = " << fronts[0].size();
        
        // Find best solution values
        if (!fronts[0].empty()) {
            double best_cost = std::numeric_limits<double>::max();
            double best_time = std::numeric_limits<double>::max();
            double max_attractions = 0;
            bool found_valid = false;
            
            for (const auto& ind : fronts[0]) {
                // Calculate REAL values by constructing the route
                Route test_route = ind->constructRoute(*this);
                
                if (test_route.isValid() && !test_route.getAttractions().empty()) {
                    found_valid = true;
                    
                    // Use actual route values, not objective values
                    double actual_cost = test_route.getTotalCost();
                    double actual_time = test_route.getTotalTime();
                    int actual_attractions = test_route.getNumAttractions();
                    
                    best_cost = std::min(best_cost, actual_cost);
                    best_time = std::min(best_time, actual_time);
                    max_attractions = std::max(max_attractions, static_cast<double>(actual_attractions));
                }
            }
            
            // Only display valid solutions
            if (found_valid) {
                std::cout << ", Best solution: [Cost=" << std::fixed << std::setprecision(2) << best_cost 
                        << ", Time=" << std::fixed << std::setprecision(1) << best_time 
                        << ", Attractions=" << static_cast<int>(max_attractions) << "]";
            } else {
                std::cout << ", No valid solutions yet";
            }
        }
        
        std::cout << std::endl;
    }
}

// Crowded comparison operator (Section III-B)
bool NSGA2Base::compareByRankAndCrowding(const IndividualPtr& a, const IndividualPtr& b) {
    // If ranks are different, return the one with lower rank
    if (a->getRank() != b->getRank()) {
        return a->getRank() < b->getRank();
    }
    
    // If ranks are the same, return the one with higher crowding distance
    return a->getCrowdingDistance() > b->getCrowdingDistance();
}

} // namespace tourist