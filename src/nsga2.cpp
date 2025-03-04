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
#include <unordered_set>
#include "hypervolume.hpp"  // New include for hypervolume metrics

namespace tourist {

// Implementação de Individual
NSGA2::Individual::AttractionGene::AttractionGene(int idx)
    : attraction_index(idx)
    , arrival_time(0.0)
    , departure_time(0.0)
    , visit_duration(0.0) {}

NSGA2::Individual::TransportGene::TransportGene(utils::TransportMode m)
    : mode(m)
    , start_time(0.0)
    , end_time(0.0)
    , duration(0.0)
    , cost(0.0)
    , distance(0.0) {}

NSGA2::Individual::Individual(std::vector<int> attraction_indices) {
    // Converter índices em genes de atração
    attraction_genes_.reserve(attraction_indices.size());
    for (int idx : attraction_indices) {
        AttractionGene gene(idx);
        attraction_genes_.push_back(gene);
    }
    
    // Criar genes de transporte vazios entre as atrações
    if (attraction_genes_.size() > 1) {
        transport_genes_.resize(attraction_genes_.size() - 1, TransportGene());
    }
}

NSGA2::Individual::Individual(std::vector<AttractionGene> attraction_genes, 
                             std::vector<TransportGene> transport_genes)
    : attraction_genes_(std::move(attraction_genes))
    , transport_genes_(std::move(transport_genes)) {
    
    // Verificar se o número de genes de transporte é correto (n-1 onde n é o número de atrações)
    if (attraction_genes_.size() > 1 && transport_genes_.size() != attraction_genes_.size() - 1) {
        transport_genes_.resize(attraction_genes_.size() - 1, TransportGene());
    }
}

void NSGA2::Individual::evaluate(const NSGA2& algorithm) {
    // Calcular todos os tempos e custos ao longo da rota
    calculateTimeAndCosts(algorithm);
    
    // Se a rota for inválida ou vazia, usar penalização proporcional
    if (!is_valid_ || attraction_genes_.empty()) {
        // Penalização proporcional em vez de valores extremos
        double penalty_factor = attraction_genes_.empty() ? 2.0 : 1.5;
        objectives_ = {
            std::min(algorithm.attractions_.size() * 100.0 * penalty_factor, 10000.0),  // Penalidade razoável de custo
            std::min(utils::Config::DAILY_TIME_LIMIT * penalty_factor, 
                    utils::Config::DAILY_TIME_LIMIT * 2.0),  // Penalidade razoável de tempo
            -0.1  // Penalidade leve para atrações (permitir competição)
        };
        return;
    }
    
    // Verificar se o tempo total excede o limite
    // Abordagem de penalidade mais gradual para restrições de tempo
    double time_penalty = 0.0;
    
    if (total_time_ > utils::Config::DAILY_TIME_LIMIT) {
        // Calcular violação
        double violation = total_time_ - utils::Config::DAILY_TIME_LIMIT;
        // Aplicar penalidade proporcional à violação
        time_penalty = violation * (1.0 + violation / utils::Config::DAILY_TIME_LIMIT);
    }
    
    // Definir objetivos com penalidades mais equilibradas
    objectives_ = {
        total_cost_,                                      // Custo original
        total_time_ + time_penalty,                       // Tempo com penalidade
        -static_cast<double>(attraction_genes_.size())    // Negativo para maximização
    };
}

void NSGA2::Individual::calculateTimeAndCosts(const NSGA2& algorithm) {
    if (attraction_genes_.empty()) {
        is_valid_ = false;
        return;
    }
    
    // Inicializar valores
    total_time_ = 0.0;
    total_cost_ = 0.0;
    is_valid_ = true;
    
    // Hora de início do dia (9:00 = 540 minutos desde meia-noite)
    double current_time = 9 * 60;
    
    // Reduzir um pouco a validação estrita para permitir soluções viáveis
    bool strict_validation = false;  // Menos rígido durante a avaliação inicial
    
    // Processar cada atração e transporte
    for (size_t i = 0; i < attraction_genes_.size(); ++i) {
        auto& attr_gene = attraction_genes_[i];
        
        // Verificar se o índice da atração é válido
        if (attr_gene.attraction_index < 0 || 
            static_cast<size_t>(attr_gene.attraction_index) >= algorithm.attractions_.size()) {
            is_valid_ = false;
            return;
        }
        
        const auto& attraction = algorithm.attractions_[attr_gene.attraction_index];
        
        // Verificar se a atração está aberta na hora atual - com tolerância
        if (!attraction.isOpenAt(current_time)) {
            if (current_time < attraction.getOpeningTime()) {
                // Atração ainda não abriu, esperar até a abertura
                double wait_time = attraction.getOpeningTime() - current_time;
                current_time = attraction.getOpeningTime();
                total_time_ += wait_time;
            } else if (strict_validation) {
                // Atração já fechou, rota inválida
                is_valid_ = false;
                return;
            } else {
                // Tolerante: assumir que podemos visitar no dia seguinte ou ajustar
                total_time_ += 30; // Penalidade de tempo para ajustes
            }
        }
        
        // Atualizar hora de chegada na atração
        attr_gene.arrival_time = current_time;
        
        // Adicionar tempo de visita
        attr_gene.visit_duration = attraction.getVisitTime();
        current_time += attr_gene.visit_duration;
        attr_gene.departure_time = current_time;
        
        total_time_ += attr_gene.visit_duration;
        total_cost_ += attraction.getCost();
        
        // Se houver próxima atração, calcular transporte
        if (i < attraction_genes_.size() - 1 && i < transport_genes_.size()) {
            auto& trans_gene = transport_genes_[i];
            
            // Verificar a validade do índice da próxima atração
            if (attraction_genes_[i+1].attraction_index < 0 || 
                static_cast<size_t>(attraction_genes_[i+1].attraction_index) >= algorithm.attractions_.size()) {
                is_valid_ = false;
                return;
            }
            
            const auto& next_attraction = algorithm.attractions_[attraction_genes_[i+1].attraction_index];
            
            // Atualizar hora de início do transporte
            trans_gene.start_time = current_time;
            
            try {
                // Calcular duração e custo do transporte
                trans_gene.duration = utils::Transport::getTravelTime(
                    attraction.getName(), 
                    next_attraction.getName(), 
                    trans_gene.mode
                );
                
                trans_gene.distance = utils::Transport::getDistance(
                    attraction.getName(), 
                    next_attraction.getName(), 
                    trans_gene.mode
                );
                
                trans_gene.cost = utils::Transport::getTravelCost(
                    attraction.getName(), 
                    next_attraction.getName(), 
                    trans_gene.mode
                );
            } catch (const std::exception& e) {
                // Tratamento de erro para atrações incompatíveis
                std::cerr << "Erro ao calcular transporte: " << e.what() << std::endl;
                trans_gene.duration = 30.0;  // Valor padrão
                trans_gene.distance = 5000.0;  // 5km
                trans_gene.cost = 30.0;  // 30 reais
            }
            
            // Atualizar hora e custos totais
            current_time += trans_gene.duration;
            trans_gene.end_time = current_time;
            
            total_time_ += trans_gene.duration;
            total_cost_ += trans_gene.cost;
        }
    }
    
    // Verificar se o tempo total está dentro do limite com tolerância de 20%
    double max_time_with_tolerance = utils::Config::DAILY_TIME_LIMIT * 1.2;
    is_valid_ = (total_time_ <= max_time_with_tolerance);
}

void NSGA2::Individual::determineTransportModes(const NSGA2& algorithm) {
    if (attraction_genes_.size() <= 1) return;
    
    // Verificar cada par adjacente de atrações
    for (size_t i = 0; i < attraction_genes_.size() - 1; ++i) {
        if (i >= transport_genes_.size()) {
            transport_genes_.resize(attraction_genes_.size() - 1);
        }
        
        int from_idx = attraction_genes_[i].attraction_index;
        int to_idx = attraction_genes_[i+1].attraction_index;
        
        if (from_idx >= 0 && static_cast<size_t>(from_idx) < algorithm.attractions_.size() &&
            to_idx >= 0 && static_cast<size_t>(to_idx) < algorithm.attractions_.size()) {
            
            try {
                // Obtém os nomes das atrações
                const std::string& from = algorithm.attractions_[from_idx].getName();
                const std::string& to = algorithm.attractions_[to_idx].getName();
                
                // Usar a função determinePreferredMode diretamente
                transport_genes_[i].mode = utils::Transport::determinePreferredMode(from, to);
                
                // Adicionar log para depuração
                double walking_time = utils::Transport::getTravelTime(from, to, utils::TransportMode::WALK);
                std::cout << "From " << from << " to " << to << ", walking time: " 
                          << walking_time << " min, chosen mode: " 
                          << (transport_genes_[i].mode == utils::TransportMode::WALK ? "WALK" : "CAR") 
                          << std::endl;
            } catch (const std::exception& e) {
                // Tratamento de erro: usar modo padrão
                transport_genes_[i].mode = utils::TransportMode::CAR;
                std::cerr << "Error determining transport mode: " << e.what() << std::endl;
            }
        } else {
            // Em caso de índices inválidos, usa modo padrão
            transport_genes_[i].mode = utils::TransportMode::CAR;
        }
    }
}

bool NSGA2::Individual::dominates(const Individual& other) const {
    const auto& other_obj = other.getObjectives();
    
    // Safety check: if either objective vector is empty, cannot determine dominance
    if (objectives_.empty() || other_obj.empty()) {
        return false;
    }
    
    // Check that both vectors have the same size
    if (objectives_.size() != other_obj.size()) {
        return false;
    }
    
    // Use the objective_ranges_ptr to access the shared objective ranges
    if (!objective_ranges_ptr) {
        // Fall back to simple dominance check if no ranges are available
        bool at_least_one_better = false;
        bool all_at_least_as_good = true;
        for (size_t i = 0; i < objectives_.size(); ++i) {
            if (objectives_[i] > other_obj[i]) {
                all_at_least_as_good = false;
                break;
            }
            if (objectives_[i] < other_obj[i]) {
                at_least_one_better = true;
            }
        }
        return all_at_least_as_good && at_least_one_better;
    }
    
    // Use the full population's ranges for normalization via objective_ranges_ptr
    bool at_least_one_better = false;
    const double EPSILON = 1e-6;
    for (size_t i = 0; i < objectives_.size(); ++i) {
        double min_val = (*objective_ranges_ptr)[i].first;
        double max_val = (*objective_ranges_ptr)[i].second;
        double range = max_val - min_val;
        
        // Avoid division by zero
        if (std::abs(range) < 1e-10) {
            if (objectives_[i] > other_obj[i]) return false;
            if (objectives_[i] < other_obj[i]) at_least_one_better = true;
            continue;
        }
        
        // Normalize both solution objectives into [0,1]
        double norm_this = (objectives_[i] - min_val) / range;
        double norm_other = (other_obj[i] - min_val) / range;
        
        // If this solution is significantly worse in any objective, it does not dominate
        if (norm_this > norm_other + EPSILON) return false;
        
        // Mark if significantly better in at least one objective
        if (norm_this < norm_other - EPSILON) at_least_one_better = true;
    }
    
    return at_least_one_better;
}

Route NSGA2::Individual::constructRoute(const NSGA2& algorithm) const {
    Route route;
    
    // Verificar se o indivíduo é válido com uma tolerância maior
    // para permitir algumas violações menores na geração de soluções
    if (attraction_genes_.empty() || total_time_ > utils::Config::DAILY_TIME_LIMIT * 1.3) {
        return route;
    }
    
    try {
        // Adicionar a primeira atração à rota
        if (attraction_genes_[0].attraction_index >= 0 && 
            static_cast<size_t>(attraction_genes_[0].attraction_index) < algorithm.attractions_.size()) {
            
            const auto& first_attr = algorithm.attractions_[attraction_genes_[0].attraction_index];
            route.addAttraction(first_attr);
            
            // Adicionar as atrações subsequentes com seus modos de transporte
            for (size_t i = 1; i < attraction_genes_.size(); ++i) {
                if (attraction_genes_[i].attraction_index >= 0 && 
                    static_cast<size_t>(attraction_genes_[i].attraction_index) < algorithm.attractions_.size()) {
                    
                    const auto& attr = algorithm.attractions_[attraction_genes_[i].attraction_index];
                    utils::TransportMode mode = (i-1 < transport_genes_.size()) ? 
                                            transport_genes_[i-1].mode : utils::TransportMode::CAR;
                    
                    route.addAttraction(attr, mode);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Erro ao construir rota: " << e.what() << std::endl;
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

const std::vector<double> NSGA2::TRACKING_REFERENCE_POINT = {
    0.0,  // Will be computed as sum of top 10 attraction costs
    utils::Config::DAILY_TIME_LIMIT * (1.0 + utils::Config::TIME_TOLERANCE),  // daily limit + tau
    -1.0  // Minimum of 1 attraction, represented as -1.0
};

void NSGA2::initializeReferencePoint() {
    // Gather all attraction costs
    std::vector<double> attraction_costs;
    attraction_costs.reserve(attractions_.size());
    for (const auto& attraction : attractions_) {
        attraction_costs.push_back(attraction.getCost());
    }

    // Sort descending to get highest costs
    std::sort(attraction_costs.begin(), attraction_costs.end(), std::greater<double>());

    double top10_cost_sum = 0.0;
    for (size_t i = 0; i < std::min(size_t(10), attraction_costs.size()); i++) {
        top10_cost_sum += attraction_costs[i];
    }
    top10_cost_sum += 500.0;  // Conservative transport cost

    auto& ref_point = const_cast<std::vector<double>&>(TRACKING_REFERENCE_POINT);
    ref_point[0] = top10_cost_sum;

    std::cout << "Reference point initialized: [" << ref_point[0] << ", "
              << ref_point[1] << ", " << ref_point[2] << "]" << std::endl;
    std::cout << "Using time tolerance (tau): " << utils::Config::TIME_TOLERANCE * 100 << "%" << std::endl;
}

void NSGA2::initializePopulation() {
    population_.clear();
    population_.reserve(params_.population_size);
    
    // Criar um cromossomo base com índices sequenciais
    std::vector<int> base_chrom(attractions_.size());
    std::iota(base_chrom.begin(), base_chrom.end(), 0);

    // Adicionar indivíduos com diferentes tamanhos para maior diversidade
    for (size_t i = 0; i < params_.population_size; ++i) {
        // Escolher um tamanho aleatório entre 2 e o número total de atrações
        std::uniform_int_distribution<size_t> size_dist(2, attractions_.size());
        size_t chrom_size = (i < params_.population_size / 2) ? attractions_.size() : size_dist(rng_);
        
        auto chrom = base_chrom;
        // Embaralhar o cromossomo para gerar diversidade
        std::shuffle(chrom.begin(), chrom.end(), rng_);
        // Se necessário, truncar para o tamanho desejado
        if (chrom_size < chrom.size()) {
            chrom.resize(chrom_size);
        }
        
        auto ind = std::make_shared<Individual>(std::move(chrom));
        // Determinar os modos de transporte ideais
        ind->determineTransportModes(*this);
        population_.push_back(std::move(ind));
    }
    
    // Avaliar a população inicial
    evaluatePopulation(population_);
}

void NSGA2::evaluatePopulation(Population& pop) {
    if (pop.empty()) throw std::runtime_error("Empty population");
    for (auto& ind : pop) {
        if (!ind) throw std::runtime_error("Null individual found");
        ind->evaluate(*this);
    }
    
    // Update the objective ranges after evaluation
    updateObjectiveRanges(pop);
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

    // Verificar se algum indivíduo possui objetivos inválidos
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

    // Para cada objetivo
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
                
                // Aplica normalização conforme recomendado no artigo original
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
    
    if (prob_dist(rng_) > params_.crossover_rate) {
        // Cópia direta do pai 1
        return std::make_shared<Individual>(
            parent1->attraction_genes_, 
            parent1->transport_genes_
        );
    }

    // Crossover de uma rota para outra
    // Implementamos um crossover de ordem parcialmente mapeado (PMX)
    
    // Número de atrações em cada pai
    size_t size1 = parent1->attraction_genes_.size();
    size_t size2 = parent2->attraction_genes_.size();
    
    if (size1 == 0 || size2 == 0) {
        return std::make_shared<Individual>(
            parent1->attraction_genes_, 
            parent1->transport_genes_
        );
    }
    
    // Determinar tamanho do filho (entre o mínimo e o máximo dos pais)
    std::uniform_int_distribution<size_t> size_dist(
        std::min(size1, size2),
        std::max(size1, size2)
    );
    size_t child_size = size_dist(rng_);
    
    // Aplicar crossover de ordem (OX)
    std::vector<Individual::AttractionGene> child_attr_genes;
    std::vector<Individual::TransportGene> child_trans_genes;
    
    // Mapear quais atrações já estão incluídas
    std::vector<bool> included(attractions_.size(), false);
    
    // Escolher um segmento aleatório do primeiro pai
    std::uniform_int_distribution<size_t> point_dist1(0, size1 > 0 ? size1 - 1 : 0);
    std::uniform_int_distribution<size_t> point_dist2(0, size1 > 0 ? size1 - 1 : 0);
    
    size_t point1 = size1 > 0 ? point_dist1(rng_) : 0;
    size_t point2 = size1 > 0 ? point_dist2(rng_) : 0;
    
    if (point1 > point2) {
        std::swap(point1, point2);
    }
    
    // Ajustar os pontos para evitar índices fora dos limites
    point2 = std::min(point2, size1 - 1);
    
    // Copiar o segmento escolhido do primeiro pai
    for (size_t i = point1; i <= point2 && i < size1; i++) {
        int idx = parent1->attraction_genes_[i].attraction_index;
        if (idx >= 0 && static_cast<size_t>(idx) < attractions_.size()) {
            child_attr_genes.push_back(parent1->attraction_genes_[i]);
            included[idx] = true;
        }
    }
    
    // Preencher com genes do segundo pai que não estão incluídos
    for (size_t i = 0; i < size2; i++) {
        int idx = parent2->attraction_genes_[i].attraction_index;
        if (idx >= 0 && static_cast<size_t>(idx) < attractions_.size() && !included[idx]) {
            child_attr_genes.push_back(parent2->attraction_genes_[i]);
            included[idx] = true;
            
            // Interromper se atingimos o tamanho desejado
            if (child_attr_genes.size() >= child_size) break;
        }
    }
    
    // Garantir que temos genes suficientes - completar com genes do primeiro pai
    if (child_attr_genes.size() < child_size) {
        for (size_t i = 0; i < size1; i++) {
            // Evitar os pontos já copiados
            if (i >= point1 && i <= point2) continue;
            
            int idx = parent1->attraction_genes_[i].attraction_index;
            if (idx >= 0 && static_cast<size_t>(idx) < attractions_.size() && !included[idx]) {
                child_attr_genes.push_back(parent1->attraction_genes_[i]);
                included[idx] = true;
                
                // Interromper se atingimos o tamanho desejado
                if (child_attr_genes.size() >= child_size) break;
            }
        }
    }
    
    // Garantir que o filho tenha pelo menos uma atração
    if (child_attr_genes.empty() && !parent1->attraction_genes_.empty()) {
        child_attr_genes.push_back(parent1->attraction_genes_[0]);
    }
    
    // Gerar genes de transporte correspondentes
    if (child_attr_genes.size() > 1) {
        child_trans_genes.resize(child_attr_genes.size() - 1);
        
        // Determinar modos de transporte (50% de chance de cada pai)
        for (size_t i = 0; i < child_trans_genes.size(); i++) {
            bool from_parent1 = prob_dist(rng_) < 0.5;
            
            size_t parent1_size = parent1->transport_genes_.size();
            size_t parent2_size = parent2->transport_genes_.size();
            
            if (from_parent1 && i < parent1_size) {
                child_trans_genes[i].mode = parent1->transport_genes_[i].mode;
            } else if (!from_parent1 && i < parent2_size) {
                child_trans_genes[i].mode = parent2->transport_genes_[i].mode;
            } else {
                // Se não tiver informação, usa modo padrão (será determinado depois)
                child_trans_genes[i].mode = utils::TransportMode::CAR;
            }
        }
    }
    
    // Criar o indivíduo filho
    auto offspring = std::make_shared<Individual>(
        std::move(child_attr_genes),
        std::move(child_trans_genes)
    );
    
    // Recalcular modos de transporte ótimos e avaliar
    offspring->determineTransportModes(*this);
    offspring->evaluate(*this);
    
    return offspring;
}

void NSGA2::mutate(IndividualPtr individual) {
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    
    if (prob_dist(rng_) > params_.mutation_rate) return;
    
    // Mutação de genes de atração
    if (!individual->attraction_genes_.empty()) {
        // Possibilidade 1: Trocar duas atrações de posição
        if (individual->attraction_genes_.size() >= 2 && prob_dist(rng_) < 0.5) {
            std::uniform_int_distribution<size_t> pos_dist(0, individual->attraction_genes_.size() - 1);
            size_t pos1 = pos_dist(rng_);
            size_t pos2 = pos_dist(rng_);
            
            // Garantir que pos1 != pos2
            while (pos1 == pos2 && individual->attraction_genes_.size() > 1) {
                pos2 = pos_dist(rng_);
            }
            
            std::swap(individual->attraction_genes_[pos1], individual->attraction_genes_[pos2]);
        }
        
        // Possibilidade 2: Adicionar ou remover uma atração aleatória
        else if (prob_dist(rng_) < 0.3) {
            // 30% de chance de adicionar/remover uma atração
            if (individual->attraction_genes_.size() < attractions_.size() && prob_dist(rng_) < 0.7) {
                // Adicionar uma atração
                std::vector<bool> included(attractions_.size(), false);
                for (const auto& gene : individual->attraction_genes_) {
                    if (gene.attraction_index >= 0 && static_cast<size_t>(gene.attraction_index) < attractions_.size()) {
                        included[gene.attraction_index] = true;
                    }
                }
                
                // Encontrar atrações não incluídas
                std::vector<int> not_included;
                for (size_t i = 0; i < attractions_.size(); i++) {
                    if (!included[i]) {
                        not_included.push_back(i);
                    }
                }
                
                if (!not_included.empty()) {
                    std::uniform_int_distribution<size_t> attr_dist(0, not_included.size() - 1);
                    int new_attr_idx = not_included[attr_dist(rng_)];
                    
                    // Adicionar ao final da rota
                    Individual::AttractionGene new_gene(new_attr_idx);
                    individual->attraction_genes_.push_back(new_gene);
                    
                    // Adicionar transporte
                    if (individual->attraction_genes_.size() > 1) {
                        individual->transport_genes_.push_back(Individual::TransportGene());
                    }
                }
            } else if (individual->attraction_genes_.size() > 1) {
                // Remover uma atração (manter pelo menos uma)
                std::uniform_int_distribution<size_t> pos_dist(0, individual->attraction_genes_.size() - 1);
                size_t pos = pos_dist(rng_);
                
                individual->attraction_genes_.erase(individual->attraction_genes_.begin() + pos);
                
                // Ajustar vetor de transportes
                if (pos < individual->transport_genes_.size()) {
                    individual->transport_genes_.erase(individual->transport_genes_.begin() + pos);
                }
                else if (!individual->transport_genes_.empty()) {
                    individual->transport_genes_.pop_back();
                }
            }
        }
    }
    
    // Mutação de genes de transporte
    mutateTransportModes(individual);
    
    // Recalcular tempos, custos e objetivos
    individual->evaluate(*this);
}

void NSGA2::mutateTransportModes(IndividualPtr individual) {
    if (individual->transport_genes_.empty()) return;
    
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> pos_dist(0, individual->transport_genes_.size() - 1);
    
    // 30% chance to mutate at least one transport mode
    if (prob_dist(rng_) < 0.3) {
        size_t pos = pos_dist(rng_);
        
        // Ensure valid indices
        if (pos < individual->attraction_genes_.size() - 1 && 
            individual->attraction_genes_[pos].attraction_index >= 0 && 
            individual->attraction_genes_[pos + 1].attraction_index >= 0) {
            
            const std::string& from = attractions_[individual->attraction_genes_[pos].attraction_index].getName();
            const std::string& to = attractions_[individual->attraction_genes_[pos + 1].attraction_index].getName();
            
            // Get walking travel time
            double walk_time = utils::Transport::getTravelTime(from, to, utils::TransportMode::WALK);
            
            // Only switch to WALK if time is below the preference
            if (individual->transport_genes_[pos].mode == utils::TransportMode::CAR &&
                walk_time < utils::Config::WALK_TIME_PREFERENCE) {
                individual->transport_genes_[pos].mode = utils::TransportMode::WALK;
            } else {
                // Otherwise, always use CAR
                individual->transport_genes_[pos].mode = utils::TransportMode::CAR;
            }
            
            // Recalculate costs and times
            individual->calculateTimeAndCosts(*this);
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
    
    // Update objective ranges for the combined population
    updateObjectiveRanges(combined);
    
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
    initializeReferencePoint();
    initializePopulation();
    
    // Rastrear melhor hipervolume durante a execução
    double best_hypervolume = 0.0;
    Population best_population = population_;
    
    for (size_t gen = 0; gen < params_.max_generations; ++gen) {
        auto offspring = createOffspring(population_);
        evaluatePopulation(offspring); // Garantir que offspring seja avaliado
        population_ = selectNextGeneration(population_, offspring);
        
        // Calcular hipervolume da geração atual
        double current_hv = calculateHypervolume(population_);
        
        // Atualizar a melhor população se o hipervolume melhorou
        if (current_hv > best_hypervolume) {
            best_hypervolume = current_hv;
            best_population = population_;
        }
        
        logProgress(gen, population_);
    }
    
    // Usar a população com melhor hipervolume para gerar soluções
    auto fronts = fastNonDominatedSort(best_population);
    std::vector<Solution> solutions;
    
    // Considerar mais fronteiras para garantir diversidade
    size_t fronts_to_consider = std::min(size_t(4), fronts.size());
    
    // Relaxar progressivamente a restrição de tempo
    for (double tolerance_factor = 1.0; tolerance_factor <= 1.2; tolerance_factor += 0.05) {
        // Para cada fronteira, extrair soluções válidas
        for (size_t f = 0; f < fronts_to_consider; ++f) {
            for (const auto& ind : fronts[f]) {
                // Verificar se a rota está dentro do limite de tempo com tolerância atual
                if (ind->getTotalTime() <= utils::Config::DAILY_TIME_LIMIT * tolerance_factor) {
                    Route route = ind->constructRoute(*this);
                    if (!route.getAttractions().empty()) {
                        solutions.push_back(Solution(route));
                        
                        // Se já temos soluções suficientes, podemos parar
                        if (solutions.size() >= 50) break;
                    }
                }
            }
            
            // Se já temos soluções suficientes, podemos parar
            if (solutions.size() >= 50) break;
        }
        
        // Se temos soluções suficientes, não precisamos relaxar mais
        if (solutions.size() >= 10) break;
    }
    
    // Se mesmo com tolerância, ainda não temos soluções, aceitar as melhores disponíveis
    if (solutions.empty() && !fronts.empty()) {
        for (const auto& ind : fronts[0]) {
            Route route = ind->constructRoute(*this);
            if (!route.getAttractions().empty()) {
                solutions.push_back(Solution(route));
                
                // Limitar a 10 soluções neste caso
                if (solutions.size() >= 10) break;
            }
        }
    }
    
    // Ordenar as soluções primeiro por número de atrações (descendente)
    std::sort(solutions.begin(), solutions.end(), 
            [](const Solution& a, const Solution& b) {
                const auto& a_obj = a.getObjectives();
                const auto& b_obj = b.getObjectives();
                
                // Ordenar pelo negativo do número de atrações (objetivo [2]) 
                return a_obj[2] < b_obj[2];
            });

    // Remover soluções duplicadas com hash mais efetivo
    auto hash_solution = [](const Solution& sol) {
        const auto& route = sol.getRoute();
        const auto& attractions = route.getAttractions();
        const auto& modes = route.getTransportModes();
        
        std::string hash;
        for (const auto* attr : attractions) {
            hash += attr->getName() + "|";
        }
        hash += "::";
        for (const auto& mode : modes) {
            hash += (mode == utils::TransportMode::CAR ? "C" : "W") + std::string("|");
        }
        return hash;
    };

    std::unordered_set<std::string> solution_hashes;
    std::vector<Solution> unique_solutions;

    for (const auto& sol : solutions) {
        std::string hash = hash_solution(sol);
        if (solution_hashes.find(hash) == solution_hashes.end()) {
            solution_hashes.insert(hash);
            unique_solutions.push_back(sol);
        }
    }

    return unique_solutions;
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
        // ...existing code...
        if (ind->getTotalTime() <= utils::Config::DAILY_TIME_LIMIT * 1.1) { // Tolerância de 10%
            solutions.push_back(Solution(route));
        }
    }
    
    if (solutions.empty()) {
        std::cout << "Warning: No valid solutions for hypervolume calculation" << std::endl;
        return 0.0;
    }
    
    std::vector<double> reference_point = TRACKING_REFERENCE_POINT;
    std::cout << "Calculating hypervolume with " << solutions.size() << " solutions" << std::endl;
    std::cout << "Reference point: [" << reference_point[0] << ", "
              << reference_point[1] << ", " << reference_point[2] << "]" << std::endl;
    
    std::vector<double> min_values = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max()
    };
    std::vector<double> max_values = {
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::lowest()
    };
    
    for (const auto& sol : solutions) {
        const auto& obj = sol.getObjectives();
        for (size_t i = 0; i < obj.size() && i < 3; i++) {
            min_values[i] = std::min(min_values[i], obj[i]);
            max_values[i] = std::max(max_values[i], obj[i]);
        }
    }
    
    for (size_t i = 0; i < reference_point.size() && i < 3; i++) {
        min_values[i] = std::min(min_values[i], reference_point[i]);
        max_values[i] = std::max(max_values[i], reference_point[i]);
    }
    
    bool can_normalize = true;
    for (size_t i = 0; i < max_values.size() && i < min_values.size(); i++) {
        if (std::abs(max_values[i] - min_values[i]) < 1e-10) {
            can_normalize = false;
            break;
        }
    }
    
    if (can_normalize) {
        class NormalizedSolution : public Solution {
        public:
            NormalizedSolution(const Solution& orig,
                               const std::vector<double>& min_vals,
                               const std::vector<double>& max_vals)
                : Solution(orig.getRoute()), original_objectives_(orig.getObjectives()) {
                std::vector<double> norm_obj(original_objectives_.size());
                for (size_t i = 0; i < original_objectives_.size(); i++) {
                    double range = max_vals[i] - min_vals[i];
                    if (i == 2) {
                        norm_obj[i] = (original_objectives_[i] - min_vals[i]) / range;
                    } else {
                        norm_obj[i] = (original_objectives_[i] - min_vals[i]) / range;
                    }
                }
                normalized_objectives_ = norm_obj;
            }
            
            std::vector<double> getObjectives() const override {
                return normalized_objectives_;
            }
            
        private:
            std::vector<double> original_objectives_;
            std::vector<double> normalized_objectives_;
        };
        
        std::vector<Solution> normalized_solutions;
        normalized_solutions.reserve(solutions.size());
        for (const auto& sol : solutions) {
            normalized_solutions.push_back(NormalizedSolution(sol, min_values, max_values));
        }
        
        std::vector<double> norm_reference = {1.0, 1.0, 1.0};
        double hypervolume = utils::Metrics::calculateHypervolume(normalized_solutions, norm_reference);
        std::cout << "Normalized hypervolume: " << hypervolume << std::endl;
        return hypervolume;
    } else {
        double hypervolume = utils::Metrics::calculateHypervolume(solutions, reference_point);
        std::cout << "Unnormalized hypervolume: " << hypervolume << std::endl;
        return hypervolume;
    }
}

bool NSGA2::compareByRankAndCrowding(const IndividualPtr& a, const IndividualPtr& b) {
    if (a->getRank() != b->getRank()) return a->getRank() < b->getRank();
    return a->getCrowdingDistance() > b->getCrowdingDistance();
}

bool NSGA2::isValidChromosome(const std::vector<int>& chrom) const {
    if (chrom.empty()) return false;
    
    std::vector<bool> used(attractions_.size(), false);
    for (int idx : chrom) {
        if (idx < 0 || static_cast<size_t>(idx) >= attractions_.size()) return false;
        if (used[idx]) return false;
        used[idx] = true;
    }
    return true;
}

std::vector<int> NSGA2::repairChromosome(std::vector<int>& chrom) const {
    std::vector<int> result;
    std::vector<bool> used(attractions_.size(), false);
    
    // Mantém os genes válidos
    for (int idx : chrom) {
        if (idx >= 0 && static_cast<size_t>(idx) < attractions_.size() && !used[idx]) {
            result.push_back(idx);
            used[idx] = true;
        }
    }
    
    // Se o cromossomo estiver vazio após reparos, adicionar pelo menos uma atração aleatória
    if (result.empty() && !attractions_.empty()) {
        std::uniform_int_distribution<size_t> dist(0, attractions_.size() - 1);
        result.push_back(dist(rng_));
    }
    
    return result;
}

void NSGA2::updateObjectiveRanges(const Population& pop) {
    if (pop.empty() || pop[0]->getObjectives().empty()) {
        return;
    }
    
    size_t num_objectives = pop[0]->getObjectives().size();
    objective_ranges_.resize(num_objectives, {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest()});
    
    // Find min and max for each objective
    for (const auto& ind : pop) {
        const auto& objectives = ind->getObjectives();
        if (objectives.size() != num_objectives) continue;
        for (size_t i = 0; i < num_objectives; ++i) {
            objective_ranges_[i].first = std::min(objective_ranges_[i].first, objectives[i]);
            objective_ranges_[i].second = std::max(objective_ranges_[i].second, objectives[i]);
        }
    }
    
    // Ensure reasonable ranges (avoid division by zero)
    for (auto& range : objective_ranges_) {
        if (std::abs(range.second - range.first) < 1e-10) {
            range.second = range.first + 1.0;  // Arbitrary non-zero range
        }
    }
    
    // Set the objective ranges reference in each individual
    for (auto& ind : const_cast<Population&>(pop)) {
        ind->setObjectiveRanges(objective_ranges_);
    }
}

} // namespace tourist