#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <random>
#include <ctime>
#include "ConverterJSON.h" // Используем новый заголовочный файл
#include "gtest/gtest.h"

TEST(sample_test_case, sample_test) {
    EXPECT_EQ(1, 0);
}

// Пространство имен для работы с файловой системой
namespace fs = std::filesystem;

// Функция для создания тестовых файлов (остается прежней)
void setup_test_environment() {
    // Создадим директорию 'resources'
    fs::create_directory("resources");

    // Создадим файлы
    std::ofstream("resources/file001.txt") << "это первый тестовый файл";
    std::ofstream("resources/file002.txt") << "это второй тестовый файл";

    // 1. Создадим пример config.json
    json config_content = {
        {"config", {
            {"name", "SkillboxSearchEngine"},
            {"version", "1.0"},
            {"max_responses", 5}
        }},
        {"files", {
            "resources/file001.txt",
            "resources/file002.txt"
            //"resources/file_does_not_exist.txt" // Несуществующий файл
        }}
    };
    std::ofstream("config.json") << std::setw(4) << config_content;

    // 2. Создадим пример requests.json
    json requests_content = {
        {"requests", {
            "первый тестовый",
            "второй файл"
        }}
    };
    std::ofstream("requests.json") << std::setw(4) << requests_content;
}

int main() {
    setlocale(LC_ALL, "RUS");
    srand(unsigned(time(NULL)));
    //system("chcp 1251");

    setup_test_environment();
    std::cout << "\n--- Запуск теста ConverterJSON ---" << std::endl;

    try {
        // Инициализация класса ConverterJSON
        ConverterJSON converter;

        std::cout << "\n--- Тест GetResponsesLimit ---" << std::endl;
        int limit = converter.GetResponsesLimit();
        std::cout << "Max Responses Limit: " << limit << std::endl; // Ожидается 5

        std::cout << "\n--- Тест GetTextDocuments ---" << std::endl;
        std::vector<std::string> docs = converter.GetTextDocuments();
        std::cout << "Loaded " << docs.size() << " document(s) content:" << std::endl;
        for (size_t i = 0; i < docs.size(); ++i) {
            std::cout << "  [Doc ID " << i << "]: \"" << docs[i].substr(0, 30) << "...\"" << std::endl;
        }

        std::cout << "\n--- Тест GetRequests ---" << std::endl;
        std::vector<std::string> requests = converter.GetRequests();
        std::cout << "Loaded requests:" << std::endl;
        for (size_t i = 0; i < requests.size(); ++i) {
            std::cout << "  [Request ID " << i + 1 << "]: " << requests[i] << std::endl;
        }

        std::cout << "\n--- Тест putAnswers ---" << std::endl;

        // Создание ответов для putAnswers
        RequestAnswer req1;
        req1.request_id = "request001";
        req1.result = true;
        req1.matches = {{0, 0.989}, {1, 0.897}}; // Найдены два документа

        RequestAnswer req2;
        req2.request_id = "request002";
        req2.result = false; // Ничего не найдено

        std::vector<RequestAnswer> answers_to_write = {req1, req2};
        converter.putAnswers(answers_to_write);

    } catch (const std::exception& e) {
        std::cerr << "\n--- ПРОИЗОШЛА КРИТИЧЕСКАЯ ОШИБКА ---" << std::endl;
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    //system("pause");
    return 0;
}