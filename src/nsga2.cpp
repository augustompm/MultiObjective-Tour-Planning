// File: src/nsga2.cpp

#include "nsga2.hpp"
#include "utils.hpp"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <cmath>
#include <set>
#include <stdexcept>
#include <random>

namespace tourist {

// Implementação de Individual
NSGA2::Individual::Individual(std::vector<int> chromosome)
    : chromosome_(std::move(chromosome)) {
    transport_modes_.resize(chromosome_.size() > 0 ? chromosome_.size() - 1 : 0, utils::TransportMode::CAR);
}

NSGA2::Individual::Individual(std::vector<int> chromosome, std::vector<utils::TransportMode> transport_modes)
    : chromosome_(std::move(chromosome))
    , transport_modes_(std::move(transport_modes)) {
    
    // Garante que o tamanho dos modos de transporte seja consistente
    if (transport_modes_.size() != (chromosome_.size() > 0 ? chromosome_.size() - 1 : 0)) {
        transport_modes_.resize(chromosome_.size() > 0 ? chromosome_.size() - 1 : 0, utils::TransportMode::CAR);
    }
}

void NSGA2::Individual::evaluate(const NSGA2& algorithm) {
    determineTransportModes(algorithm);
    Route route = constructRoute(algorithm);
    
    // Se a rota estiver vazia, penaliza fortemente
    if (route.getAttractions().empty()) {
        objectives_ = {
            10000.0,                          // Custo máximo
            utils::Config::DAILY_TIME_LIMIT,  // Tempo máximo
            0.0                               // Nenhuma atração visitada
        };
        return;
    }
    
    double total_time = route.getTotalTime();
    double time_penalty = 0.0;
    
    // Aplica penalidade se ultrapassar o limite diário
    if (total_time > utils::Config::DAILY_TIME_LIMIT) {
        time_penalty = (total_time - utils::Config::DAILY_TIME_LIMIT) * 10.0;
    }
    
    objectives_ = {
        route.getTotalCost(),
        total_time + time_penalty,
        -static_cast<double>(route.getNumAttractions())
    };
}

void NSGA2::Individual::determineTransportModes(const NSGA2& algorithm) {
    if (chromosome_.size() <= 1) return;
    
    // Redimensiona o vetor de modos de transporte se necessário
    transport_modes_.resize(chromosome_.size() - 1);
    
    // Para cada par adjacente de atrações no cromossomo
    for (size_t i = 0; i < chromosome_.size() - 1; ++i) {
        if (chromosome_[i] >= 0 && static_cast<size_t>(chromosome_[i]) < algorithm.attractions_.size() &&
            chromosome_[i+1] >= 0 && static_cast<size_t>(chromosome_[i+1]) < algorithm.attractions_.size()) {
            
            // Obtém os nomes das atrações
            const std::string& from = algorithm.attractions_[chromosome_[i]].getName();
            const std::string& to = algorithm.attractions_[chromosome_[i+1]].getName();
            
            // Determina o modo de transporte preferencial
            transport_modes_[i] = utils::Transport::determinePreferredMode(from, to);
        } else {
            // Em caso de índices inválidos, usa modo padrão
            transport_modes_[i] = utils::TransportMode::CAR;
        }
    }
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
    Route route;
    if (algorithm.attractions_.empty()) {
        throw std::runtime_error("No attractions available");
    }
    
    // Cria uma lista de índices válidos a partir do cromossomo
    std::vector<int> valid_indices;
    for (int idx : chromosome_) {
        if (idx >= 0 && static_cast<size_t>(idx) < algorithm.attractions_.size()) {
            valid_indices.push_back(idx);
        }
    }
    
    // Se não houver índices válidos, retorna rota vazia
    if (valid_indices.empty()) {
        return route;
    }
    
    // Adiciona a primeira atração
    route.addAttraction(algorithm.attractions_[valid_indices[0]]);
    
    // Adiciona as atrações subsequentes com seus modos de transporte
    for (size_t i = 1; i < valid_indices.size(); ++i) {
        // Calcula o índice no vetor de modos de transporte
        size_t mode_idx = i - 1;
        utils::TransportMode mode = (mode_idx < transport_modes_.size()) ? 
                                   transport_modes_[mode_idx] : utils::TransportMode::CAR;
        
        // Verifica se adicionar esta atração ainda mantém o tempo dentro do limite
        Route temp_route = route;
        temp_route.addAttraction(algorithm.attractions_[valid_indices[i]], mode);
        double new_total_time = temp_route.getTotalTime();
        
        // Adiciona a atração se a rota ainda for válida (ou se estivermos ignorando restrições)
        if (new_total_time <= utils::Config::DAILY_TIME_LIMIT * 1.1) { // Tolerância de 10%
            route.addAttraction(algorithm.attractions_[valid_indices[i]], mode);
        }
        // Caso contrário, ignora esta atração e tenta a próxima no cromossomo
    }
    
    return route;
}

// Implementação do NSGA-II
NSGA2::NSGA2(const std::vector<Attraction>& attractions, Parameters params)
    : attractions_(attractions)
    , params_(std::move(params))
    , rng_(std::random_device{}()) {
    params_.validate();
    if (attractions.empty()) throw std::runtime_error("No attractions provided");
    
    // Verifica se os dados de transporte foram carregados
    if (!utils::TransportMatrices::matrices_loaded) {
        throw std::runtime_error("Transport matrices must be loaded before initializing NSGA-II");
    }
}

void NSGA2::initializePopulation() {
    population_.clear();
    population_.reserve(params_.population_size);
    
    // Criar um cromossomo base com índices sequenciais
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
        // Cópia direta do pai 1
        return std::make_shared<Individual>(parent1->chromosome_, parent1->transport_modes_);
    }

    const size_t n = attractions_.size();
    size_t point1 = point_dist(rng_);
    size_t point2 = point_dist(rng_);
    if (point1 > point2) std::swap(point1, point2);

    // Crossover de cromossomo (sequência de atrações)
    std::vector<int> child_chrom(n, -1);
    std::vector<bool> used(n, false);

    // Copia o segmento do primeiro pai
    for (size_t i = point1; i <= point2; i++) {
        child_chrom[i] = parent1->chromosome_[i];
        used[parent1->chromosome_[i]] = true;
    }

    // Preenche o resto com genes do segundo pai
    size_t fill_pos = (point2 + 1) % n;
    for (const auto& gene : parent2->chromosome_) {
        if (!used[gene]) {
            child_chrom[fill_pos] = gene;
            fill_pos = (fill_pos + 1) % n;
            used[gene] = true;
        }
    }

    // Garante que todos os genes estão preenchidos
    for (size_t i = 0; i < n; ++i) {
        if (child_chrom[i] == -1) {
            for (size_t j = 0; j < n; ++j) {
                if (!used[j]) {
                    child_chrom[i] = j;
                    used[j] = true;
                    break;
                }
            }
        }
    }

    // Crossover de modos de transporte (combinação dos dois pais)
    std::vector<utils::TransportMode> child_modes;
    child_modes.reserve(n - 1);
    
    for (size_t i = 0; i < n - 1; ++i) {
        // 50% de chance de pegar o modo de cada pai
        bool from_parent1 = prob_dist(rng_) < 0.5;
        
        if (from_parent1 && i < parent1->transport_modes_.size()) {
            child_modes.push_back(parent1->transport_modes_[i]);
        } else if (!from_parent1 && i < parent2->transport_modes_.size()) {
            child_modes.push_back(parent2->transport_modes_[i]);
        } else {
            // Fallback: escolhe modo preferencial
            child_modes.push_back(utils::TransportMode::CAR); // Será determinado na avaliação
        }
    }

    auto offspring = std::make_shared<Individual>(std::move(child_chrom), std::move(child_modes));
    offspring->evaluate(*this);
    return offspring;
}

void NSGA2::mutate(IndividualPtr individual) {
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    if (prob_dist(rng_) > params_.mutation_rate) return;

    // Mutação de sequência: troca aleatória de duas posições
    std::uniform_int_distribution<size_t> pos_dist(0, attractions_.size() - 1);
    size_t pos1 = pos_dist(rng_);
    size_t pos2 = pos_dist(rng_);
    
    std::swap(individual->chromosome_[pos1], individual->chromosome_[pos2]);
    
    // Também aplica mutação nos modos de transporte
    mutateTransportModes(individual);
    
    // Reavalia o indivíduo após a mutação
    individual->evaluate(*this);
}

void NSGA2::mutateTransportModes(IndividualPtr individual) {
    if (individual->transport_modes_.empty()) return;
    
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> pos_dist(0, individual->transport_modes_.size() - 1);
    
    // Chance de 20% de mutar um modo de transporte
    if (prob_dist(rng_) < 0.2) {
        size_t pos = pos_dist(rng_);
        
        // Inverte o modo (de CAR para WALK ou vice-versa)
        if (individual->transport_modes_[pos] == utils::TransportMode::CAR) {
            individual->transport_modes_[pos] = utils::TransportMode::WALK;
        } else {
            individual->transport_modes_[pos] = utils::TransportMode::CAR;
        }
    }
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

    // Filtrar soluções válidas, mas com critérios mais flexíveis
    std::vector<Solution> solutions;
    auto final_front = fastNonDominatedSort(population_)[0];
    
    // Adicionar soluções da primeira fronteira
    for (const auto& ind : final_front) {
        Route route = ind->constructRoute(*this);
        solutions.push_back(Solution(route));
    }
    
    // Filtrar soluções com tempo total muito acima do limite
    solutions.erase(
        std::remove_if(solutions.begin(), solutions.end(), 
            [](const Solution& sol) {
                double total_time = sol.getObjectives()[1];
                // Aceita rotas que estão dentro ou ligeiramente acima do limite (10% de tolerância)
                return total_time > utils::Config::DAILY_TIME_LIMIT * 1.1;
            }), 
        solutions.end()
    );
    
    // Ordenar as soluções por número de atrações (descendente)
    std::sort(solutions.begin(), solutions.end(), 
              [](const Solution& a, const Solution& b) {
                  return a.getObjectives()[2] < b.getObjectives()[2];
              });
    
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
        Route route = ind->constructRoute(*this);
        if (route.getTotalTime() <= utils::Config::DAILY_TIME_LIMIT * 1.1) { // Tolerância de 10%
            solutions.push_back(Solution(route));
        }
    }
    
    std::vector<double> reference_point = {10000.0, utils::Config::DAILY_TIME_LIMIT * 2, 0.0};
    return utils::Metrics::calculateHypervolume(solutions, reference_point);
}

bool NSGA2::compareByRankAndCrowding(const IndividualPtr& a, const IndividualPtr& b) {
    if (a->getRank() != b->getRank()) return a->getRank() < b->getRank();
    return a->getCrowdingDistance() > b->getCrowdingDistance();
}

bool NSGA2::isValidChromosome(const std::vector<int>& chrom) const {
    if (chrom.size() != attractions_.size()) return false;
    
    std::vector<bool> used(attractions_.size(), false);
    for (int idx : chrom) {
        if (idx < 0 || static_cast<size_t>(idx) >= attractions_.size()) return false;
        if (used[idx]) return false;
        used[idx] = true;
    }
    
    return std::all_of(used.begin(), used.end(), [](bool b) { return b; });
}

std::vector<int> NSGA2::repairChromosome(std::vector<int>& chrom) const {
    std::vector<int> result(attractions_.size());
    std::vector<bool> used(attractions_.size(), false);
    
    // Mantém os genes válidos
    size_t valid_count = 0;
    for (int idx : chrom) {
        if (idx >= 0 && static_cast<size_t>(idx) < attractions_.size() && !used[idx]) {
            result[valid_count++] = idx;
            used[idx] = true;
        }
    }
    
    // Adiciona índices faltantes
    for (size_t i = 0; i < attractions_.size(); ++i) {
        if (!used[i]) {
            if (valid_count < attractions_.size()) {
                result[valid_count++] = i;
                used[i] = true;
            }
        }
    }
    
    return result;
}

} // namespace tourist