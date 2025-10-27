//
// Created by ArtSolo on 26.10.2025.
//

#ifndef SEARCH_ENGINE_CONVERTERJSON_H
#define SEARCH_ENGINE_CONVERTERJSON_H

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <iomanip>
#include <algorithm>

#include "nlohmann_json\include\nlohmann\json.hpp"

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

    void putAnswers(const std::vector<RequestAnswer>& answers);
private:
    void system_load_config();

    static const std::string APP_VERSION;

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