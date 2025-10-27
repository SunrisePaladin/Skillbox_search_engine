//
// Created by ArtSolo on 26.10.2025.
//

#ifndef SEARCH_ENGINE_CONVERTERJSON_H
#define SEARCH_ENGINE_CONVERTERJSON_H

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "nlohmann_json\include\nlohmann\json.hpp"

class RelativeIndex; //предварительная декларация

using json = nlohmann::json;
namespace fs = std::filesystem;

struct DocumentRank {
    int docid;      //док_айди
    double rank;    //ранг
};

struct RequestAnswer {
    std::string request_id;              // "request001", "request002" и т.п.
    bool result = false;                 // если найден файл
    std::vector<DocumentRank> matches;   // Список совпадений
};

class ConverterJSON {
    public:
        ConverterJSON(
            const std::string& config_path = "config.json",
            const std::string& requests_path = "requests.json",
            const std::string& answers_path = "answers.json"
        );

    std::vector<std::string> GetTextDocuments();

    int GetResponsesLimit();

    std::vector<std::string> GetRequests();

    void putAnswers(const std::vector<std::vector<RelativeIndex>>& search_results);
private:
    void system_load_config();

    static const std::string APP_VERSION;

    const std::string APPLICATION_VERSION = "1.0";
    const std::string CONFIG_PATH = "config.json";
    const std::string REQUESTS_PATH = "requests.json";

    nlohmann::json m_config_data;
    // Пути к файлам
    std::string m_config_path;
    std::string m_requests_path;
    std::string m_answers_path;

    // Данные из конфигурации
    std::string m_name;
    std::string m_version;
    int m_max_responses;
    std::vector<std::string> m_file_paths; // Только пути к файлам, а не их содержимое
};

#endif //SEARCH_ENGINE_CONVERTERJSON_H