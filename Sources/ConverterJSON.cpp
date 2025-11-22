//
// Created by ArtSolo on 26.10.2025.
//

#include "../Headers/ConverterJSON.h"
#include "../Headers/SearchServer.h"
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
    nlohmann::json data;
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

    m_config_data = data;

    std::cout << "Configuration loaded. " << m_file_paths.size() << " file paths stored for indexing." << std::endl;
}

//Метод получения содержимого файлов
std::vector<std::string> ConverterJSON::GetTextDocuments() {
    std::vector<std::string> documents;

    if (!m_config_data.contains("files")) {
        std::cerr << "Ошибка: Ключ 'files' отсутствует в конфигурации." << std::endl;
        return documents;
    }

    // Используем m_config_data, загруженную и проверенную в конструкторе
    const auto& files_array = m_config_data.at("files");

    if (!files_array.is_array()) {
        std::cerr << "Ошибка: Значение по ключу 'files' не является массивом." << std::endl;
        return documents;
    }

    for (const auto& file_path_json : files_array) {
        std::string file_path = file_path_json.get<std::string>();
        std::ifstream document_file(file_path);

        // Проверка существования файла
        if (!document_file.is_open()) {
            std::cerr << "Warning: File not found at path: " << file_path
                      << ". Skipping this document." << std::endl;
            continue; // Переходим к следующему файлу
        }

        // Чтение содержимого
        std::stringstream ss;
        ss << document_file.rdbuf();
        documents.push_back(ss.str());
    }

    return documents;
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

void ConverterJSON::putAnswers(const std::vector<std::vector<RelativeIndex>>& search_results) {
    json root_answers;
    json answers_obj;
    size_t request_id_counter = 1;

    int max_responses = GetResponsesLimit();

    for (const auto& query_results : search_results) {
        std::string request_id = "request";
        request_id+= (request_id_counter < 10 ? "00" : (request_id_counter < 100 ? "0" : "")) + std::to_string(request_id_counter++);

        json request_entry;

        if (query_results.empty()) {
            request_entry["result"] = "false";
        } else {
            request_entry["result"] = "true";

            // Ограничиваем количество ответов
            size_t limit = std::min((size_t)max_responses, query_results.size());
            if (limit == 1) {
                const auto& match = query_results[0];
                request_entry["docid"] = match.doc_id;
                request_entry["rank"] = std::stod(std::to_string(match.rank).substr(0, std::to_string(match.rank).find('.') + 4));
            }
            // Если найдено > 1 ответа, используем "relevance"
            else if (limit > 1) {
                json relevance_array = json::array();

                for (size_t i = 0; i < limit; ++i) {
                    const auto& match = query_results[i];
                    std::string rank_str = std::to_string(match.rank);
                    double formatted_rank = std::stod(rank_str.substr(0, rank_str.find('.') + 4));

                    relevance_array.push_back({
                        {"docid", match.doc_id},
                        {"rank", formatted_rank}
                    });
                }
                request_entry["relevance"] = relevance_array;
            }
        }

        answers_obj[request_id] = request_entry;
    }

    root_answers["answers"] = answers_obj;

    try {
        std::ofstream answers_file(m_answers_path);
        answers_file << std::setw(4) << root_answers << std::endl;
        std::cout << "Answers successfully written to " << m_answers_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error writing file " << m_answers_path << ": " << e.what() << std::endl;
    }
}