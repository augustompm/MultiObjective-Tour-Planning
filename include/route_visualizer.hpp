#pragma once

#include "models.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <fstream>       
#include <filesystem>    

namespace tourist {

class RouteVisualizer {
public:
    // Gera uma visualização da linha do tempo do roteiro em formato texto
    static std::string generateTimelineText(const Route& route) {
        std::stringstream ss;
        const auto& attractions = route.getAttractions();
        const auto& transport_modes = route.getTransportModes();
        const auto& time_info = route.getTimeInfo();
        
        if (attractions.empty()) {
            return "Empty route";
        }
        
        // Definir escala de tempo (minutos por caractere)
        const int scale = 5; // 5 minutos por caractere
        const int day_start = 9 * 60; // 9:00 AM
        const int day_end = day_start + utils::Config::DAILY_TIME_LIMIT;
        
        // Gerar linha de tempo
        int timeline_length = (day_end - day_start) / scale;
        ss << "Timeline (each character = " << scale << " minutes):\n";
        ss << "      ";
        
        // Marcadores de hora
        for (int hour = day_start / 60; hour <= day_end / 60; hour++) {
            int pos = (hour * 60 - day_start) / scale;
            if (pos >= 0 && pos < timeline_length) {
                ss << std::setw(pos - ss.str().length() + 6) << hour;
            }
        }
        ss << "\n";
        
        // Linha base
        ss << "Time: ";
        for (int i = 0; i < timeline_length; i++) {
            ss << "-";
        }
        ss << "\n";
        
        // Gerar linha para cada atração
        double current_time = day_start;
        
        for (size_t i = 0; i < attractions.size(); i++) {
            const auto& attraction = attractions[i];
            std::string name = attraction->getName();
            
            // Limitar tamanho do nome
            if (name.length() > 15) {
                name = name.substr(0, 12) + "...";
            }
            
            ss << std::left << std::setw(6) << name;
            
            // Gerar representação visual
            int start_pos = 0;
            if (i < time_info.size()) {
                double arrival = time_info[i].arrival_time;
                double departure = time_info[i].departure_time;
                
                // Espera antes da visita
                if (time_info[i].wait_time > 0) {
                    start_pos = static_cast<int>((arrival - time_info[i].wait_time - day_start) / scale);
                    int wait_length = static_cast<int>(time_info[i].wait_time / scale);
                    
                    for (int j = 0; j < start_pos; j++) {
                        ss << " ";
                    }
                    for (int j = 0; j < wait_length; j++) {
                        ss << "w"; // w para espera
                    }
                    
                    start_pos += wait_length;
                } else {
                    start_pos = static_cast<int>((arrival - day_start) / scale);
                    for (int j = 0; j < start_pos; j++) {
                        ss << " ";
                    }
                }
                
                // Visita
                int visit_length = static_cast<int>(attraction->getVisitTime() / scale);
                for (int j = 0; j < visit_length; j++) {
                    ss << "V"; // V para visita
                }
                
                // Transporte para próxima atração
                if (i < attractions.size() - 1 && i < transport_modes.size()) {
                    int travel_time = static_cast<int>(utils::Transport::getTravelTime(
                        attraction->getName(),
                        attractions[i+1]->getName(),
                        transport_modes[i]
                    ));
                    
                    int travel_length = travel_time / scale;
                    char travel_char = (transport_modes[i] == utils::TransportMode::WALK) ? 'W' : 'D';
                    
                    for (int j = 0; j < travel_length; j++) {
                        ss << travel_char; // W para caminhada, D para dirigir
                    }
                }
            }
            
            ss << "\n";
        }
        
        // Adicionar legenda
        ss << "\nLegenda:\n";
        ss << "V = Visitando atração\n";
        ss << "W = Caminhando\n";
        ss << "D = Dirigindo\n";
        ss << "w = Aguardando abertura\n";
        
        return ss.str();
    }
    
    // Gera uma visualização da linha do tempo em HTML (pode ser salva em arquivo)
    static std::string generateTimelineHTML(const Route& route) {
        std::stringstream ss;
        const auto& attractions = route.getAttractions();
        const auto& transport_modes = route.getTransportModes();
        const auto& time_info = route.getTimeInfo();
        
        if (attractions.empty()) {
            return "<p>Empty route</p>";
        }
        
        // Definir parâmetros de visualização
        const int day_start = 9 * 60; // 9:00 AM
        const int day_end = day_start + utils::Config::DAILY_TIME_LIMIT;
        
        // Iniciar documento HTML
        ss << "<!DOCTYPE html>\n";
        ss << "<html>\n<head>\n";
        ss << "<title>Route Timeline</title>\n";
        ss << "<style>\n";
        ss << "body { font-family: Arial, sans-serif; }\n";
        ss << ".timeline { position: relative; height: " << (attractions.size() * 50 + 50) << "px; }\n";
        ss << ".time-axis { position: absolute; left: 150px; right: 20px; top: 20px; height: 30px; }\n";
        ss << ".time-axis .line { position: absolute; top: 15px; left: 0; right: 0; height: 1px; background: #000; }\n";
        ss << ".time-axis .hour { position: absolute; top: 0; width: 1px; height: 10px; background: #000; }\n";
        ss << ".time-axis .hour-label { position: absolute; top: -20px; width: 40px; text-align: center; font-size: 12px; margin-left: -20px; }\n";
        ss << ".attraction { position: absolute; height: 40px; left: 0; font-size: 14px; }\n";
        ss << ".attraction .name { position: absolute; width: 140px; text-align: right; padding-right: 10px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }\n";
        ss << ".activity { position: absolute; height: 30px; top: 5px; border-radius: 4px; text-align: center; font-size: 12px; color: white; overflow: hidden; }\n";
        ss << ".visit { background-color: #3498db; }\n";
        ss << ".wait { background-color: #f39c12; }\n";
        ss << ".walk { background-color: #2ecc71; }\n";
        ss << ".drive { background-color: #e74c3c; }\n";
        ss << ".legend { margin-top: 20px; font-size: 14px; }\n";
        ss << ".legend div { display: inline-block; width: 20px; height: 20px; margin-right: 5px; vertical-align: middle; border-radius: 3px; }\n";
        ss << "</style>\n";
        ss << "</head>\n<body>\n";
        
        // Criar linha do tempo
        ss << "<h1>Route Timeline</h1>\n";
        ss << "<div class=\"timeline\">\n";
        
        // Eixo do tempo
        ss << "<div class=\"time-axis\">\n";
        ss << "<div class=\"line\"></div>\n";
        
        // Marcar horas
        int timeline_width = day_end - day_start;
        for (int hour = day_start / 60; hour <= day_end / 60; hour++) {
            double percent = (hour * 60 - day_start) * 100.0 / timeline_width;
            ss << "<div class=\"hour\" style=\"left: " << percent << "%;\"></div>\n";
            ss << "<div class=\"hour-label\" style=\"left: " << percent << "%\">"
               << hour << ":00</div>\n";
        }
        ss << "</div>\n";
        
        // Criar barras para cada atração
        for (size_t i = 0; i < attractions.size(); i++) {
            const auto& attraction = attractions[i];
            double top = 50 + i * 50;
            
            ss << "<div class=\"attraction\" style=\"top: " << top << "px;\">\n";
            ss << "<div class=\"name\">" << attraction->getName() << "</div>\n";
            
            if (i < time_info.size()) {
                double arrival = time_info[i].arrival_time;
                double departure = time_info[i].departure_time;
                
                // Espera antes da visita
                if (time_info[i].wait_time > 0) {
                    double wait_start = arrival - time_info[i].wait_time;
                    double wait_percent = (wait_start - day_start) * 100.0 / timeline_width;
                    double wait_width = time_info[i].wait_time * 100.0 / timeline_width;
                    
                    ss << "<div class=\"activity wait\" style=\"left: " << (wait_percent + 150) 
                       << "px; width: " << wait_width << "%; line-height: 30px;\">Aguardando</div>\n";
                }
                
                // Visita
                double visit_percent = (arrival - day_start) * 100.0 / timeline_width;
                double visit_width = attraction->getVisitTime() * 100.0 / timeline_width;
                
                ss << "<div class=\"activity visit\" style=\"left: " << (visit_percent + 150) 
                   << "px; width: " << visit_width << "%; line-height: 30px;\">"
                   << attraction->getName() << "</div>\n";
                
                // Transporte para próxima atração
                if (i < attractions.size() - 1 && i < transport_modes.size()) {
                    double travel_time = utils::Transport::getTravelTime(
                        attraction->getName(),
                        attractions[i+1]->getName(),
                        transport_modes[i]
                    );
                    
                    double travel_percent = (departure - day_start) * 100.0 / timeline_width;
                    double travel_width = travel_time * 100.0 / timeline_width;
                    
                    std::string mode_class = (transport_modes[i] == utils::TransportMode::WALK) ? "walk" : "drive";
                    std::string mode_name = (transport_modes[i] == utils::TransportMode::WALK) ? "Caminhando" : "Dirigindo";
                    
                    ss << "<div class=\"activity " << mode_class << "\" style=\"left: " 
                       << (travel_percent + 150) << "px; width: " << travel_width 
                       << "%; line-height: 30px;\">" << mode_name << "</div>\n";
                }
            }
            
            ss << "</div>\n";
        }
        
        ss << "</div>\n";
        
        // Legenda
        ss << "<div class=\"legend\">\n";
        ss << "<div class=\"visit\"></div> Visitando atração\n";
        ss << "<div class=\"wait\"></div> Aguardando abertura\n";
        ss << "<div class=\"walk\"></div> Caminhando\n";
        ss << "<div class=\"drive\"></div> Dirigindo\n";
        ss << "</div>\n";
        
        // Informações adicionais da rota
        ss << "<h2>Route Details</h2>\n";
        ss << "<p>Total cost: R$ " << std::fixed << std::setprecision(2) << route.getTotalCost() << "</p>\n";
        ss << "<p>Total time: " << route.getTotalTime() << " minutes</p>\n";
        ss << "<p>Number of attractions: " << route.getNumAttractions() << "</p>\n";
        
        ss << "<h3>Attraction Schedule</h3>\n";
        ss << "<table border=\"1\" cellpadding=\"5\">\n";
        ss << "<tr><th>Attraction</th><th>Arrival</th><th>Wait</th><th>Visit Duration</th><th>Departure</th></tr>\n";
        
        for (size_t i = 0; i < attractions.size(); i++) {
            const auto& attraction = attractions[i];
            
            ss << "<tr>\n";
            ss << "<td>" << attraction->getName() << "</td>\n";
            
            if (i < time_info.size()) {
                ss << "<td>" << utils::Transport::formatTime(time_info[i].arrival_time) << "</td>\n";
                ss << "<td>" << time_info[i].wait_time << " min</td>\n";
                ss << "<td>" << attraction->getVisitTime() << " min</td>\n";
                ss << "<td>" << utils::Transport::formatTime(time_info[i].departure_time) << "</td>\n";
            } else {
                ss << "<td>-</td><td>-</td><td>-</td><td>-</td>\n";
            }
            
            ss << "</tr>\n";
        }
        
        ss << "</table>\n";
        
        // Finalizar documento HTML
        ss << "</body>\n</html>";
        
        return ss.str();
    }
    
    // Salva a visualização HTML em um arquivo
    static void saveTimelineHTML(const Route& route, const std::string& filename) {
        std::string html = generateTimelineHTML(route);
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Erro ao criar arquivo HTML: " + filename);
        }
        
        file << html;
        file.close();
    }
    
    // Salva visualizações HTML para um conjunto de soluções
    static void saveAllTimelineHTML(const std::vector<Solution>& solutions, const std::string& directory) {
        // Criar diretório se não existir
        std::filesystem::create_directories(directory);
        
        for (size_t i = 0; i < solutions.size(); ++i) {
            std::string filename = directory + "/route_" + std::to_string(i+1) + ".html";
            saveTimelineHTML(solutions[i].getRoute(), filename);
        }
        
        // Criar um índice HTML
        std::ofstream index(directory + "/index.html");
        if (!index.is_open()) {
            throw std::runtime_error("Erro ao criar arquivo de índice");
        }
        
        index << "<!DOCTYPE html>\n";
        index << "<html>\n<head>\n";
        index << "<title>Route Solutions</title>\n";
        index << "<style>body { font-family: Arial, sans-serif; }</style>\n";
        index << "</head>\n<body>\n";
        
        index << "<h1>Route Solutions</h1>\n";
        index << "<ul>\n";
        
        for (size_t i = 0; i < solutions.size(); ++i) {
            const auto& solution = solutions[i];
            const auto& objectives = solution.getObjectives();
            
            index << "<li><a href=\"route_" << (i+1) << ".html\">Solution " << (i+1) 
                  << "</a> - Cost: R$" << std::fixed << std::setprecision(2) << objectives[0]
                  << ", Time: " << objectives[1] << " min, Attractions: " 
                  << std::abs(static_cast<int>(objectives[2])) << "</li>\n";
        }
        
        index << "</ul>\n";
        index << "</body>\n</html>";
        
        index.close();
    }
};

} // namespace tourist