//
// Created by ArtSolo on 26.10.2025.
//

#include "ConverterJSON.h"

const std::string ConverterJSON::APP_VERSION = "1.0";

ConverterJSON::ConverterJSON(
    const std::string& config_path,
    const std::string& requests_path,
    const std::string& answers_path)
    : m_config_path(config_path),
      m_requests_path(requests_path),
      m_answers_path(answers_path),
      m_max_responses(5)
{
    // Загрузка и проверка конфигурации происходит при создании объекта
    system_load_config();
}

void ConverterJSON::system_load_config() {
    if (!fs::exists(m_config_path)) {
        throw std::runtime_error("config file is missing");
    }

    std::ifstream config_file(m_config_path);
    json data;
    try {
        data = json::parse(config_file);
    } catch (json::parse_error& e) {
        throw std::runtime_error("Error decoding JSON from " + m_config_path);
    }

    if (!data.contains("config") || data["config"].is_null()) {
        throw std::runtime_error("config file is empty");
    }

    auto& config_data = data["config"];

    m_name = config_data.value("name", "");
    m_version = config_data.value("version", "");
    m_max_responses = config_data.value("max_responses", 5);

    if (!m_name.empty()) {
        std::cout << "Starting " << m_name << "..." << std::endl;
    } else {
        std::cout << "Warning: 'name' field is missing in config. Starting unnamed engine..." << std::endl;
    }

    if (m_version != APP_VERSION) {
        throw std::runtime_error(
            "config.json has incorrect file version. App version: " + APP_VERSION +
            ", Config version: " + m_version
        );
    }

    // Загрузка и проверка путей к файлам
    if (data.contains("files") && data["files"].is_array()) {
        for (const auto& file_path_json : data["files"]) {
            std::string file_path = file_path_json.get<std::string>();
            if (fs::exists(file_path)) {
                m_file_paths.push_back(file_path);
            } else {
                std::cerr << "Error: File not found (skipping): " << file_path << std::endl;
            }
        }
    } else {
        std::cout << "Warning: 'files' field is missing or not a list. No files to index." << std::endl;
    }

    std::cout << "Configuration loaded. " << m_file_paths.size() << " file paths stored for indexing." << std::endl;
}

//Метод получения содержимого файлов
std::vector<std::string> ConverterJSON::GetTextDocuments() {
    std::vector<std::string> documents_content;

    // Итерируемся по уже проверенным и сохраненным путям
    for (const auto& file_path : m_file_paths) {
        std::ifstream file_stream(file_path);

        // Читаем все содержимое файла в строку
        if (file_stream.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file_stream)),
                                 std::istreambuf_iterator<char>());
            documents_content.push_back(content);
        } else {
            std::cerr << "Error: Could not open file to read content: " << file_path << std::endl;
        }
    }

    return documents_content;
}

int ConverterJSON::GetResponsesLimit() {
    // Возвращаю значение config.json
    return m_max_responses;
}

//Метод возвращает список запросов из файла requests.json
std::vector<std::string> ConverterJSON::GetRequests() {
    std::vector<std::string> requests_list;

    if (!fs::exists(m_requests_path)) {
        std::cerr << "Error: " << m_requests_path << " not found." << std::endl;
        return requests_list;
    }

    std::ifstream requests_file(m_requests_path);
    json data;
    try {
        data = json::parse(requests_file);
    } catch (json::parse_error& e) {
        std::cerr << "Error decoding JSON from " << m_requests_path << ": " << e.what() << std::endl;
        return requests_list;
    }

    if (data.contains("requests") && data["requests"].is_array()) {
        try {
            requests_list = data["requests"].get<std::vector<std::string>>();
        } catch (const std::exception& e) {
            std::cerr << "Error: 'requests' field in " << m_requests_path
                      << " contains non-string elements. Skipping processing." << std::endl;
        }
    } else {
        std::cerr << "Warning: " << m_requests_path
                  << " is missing the 'requests' array." << std::endl;
    }

    std::cout << "Loaded " << requests_list.size() << " request(s) from " << m_requests_path << std::endl;
    return requests_list;
}

void ConverterJSON::putAnswers(const std::vector<RequestAnswer>& answers_data) {
    json root_answers;
    json answers_obj;

    for (const auto& answer : answers_data) {
        json request_entry;

        request_entry["result"] = answer.result ? "true" : "false";

        if (!answer.result) {
            answers_obj[answer.request_id] = request_entry;
            continue;
        }

        if (answer.matches.size() > 1) {
            json relevance_array = json::array();

            // Копируем и сортируем по убыванию ранга
            std::vector<DocumentRank> sorted_matches = answer.matches;
            std::sort(sorted_matches.begin(), sorted_matches.end(), [](const DocumentRank& a, const DocumentRank& b) {
                return a.rank > b.rank; // Сортируем по убыванию rank
            });

            for (const auto& match : sorted_matches) {
                relevance_array.push_back({
                    {"docid", match.docid},
                    {"rank", match.rank}
                });
            }
            request_entry["relevance"] = relevance_array;
        }
        else if (answer.matches.size() == 1) {
            const auto& match = answer.matches[0];
            request_entry["docid"] = match.docid;
            request_entry["rank"] = match.rank;
        }

        answers_obj[answer.request_id] = request_entry;
    }

    root_answers["answers"] = answers_obj;

    // Создание/перезапись файла answers.json
    try {
        std::ofstream answers_file(m_answers_path);
        answers_file << std::setw(4) << root_answers << std::endl;
        std::cout << "Answers successfully written to " << m_answers_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error writing file " << m_answers_path << ": " << e.what() << std::endl;
    }
}