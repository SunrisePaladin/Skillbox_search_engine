#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <random>
#include <ctime>
#include "ConverterJSON.h"
#include "InvertedIndex.h"
#include "SearchServer.h"
#include "gtest/gtest.h"
namespace fs = std::filesystem;

extern void TestWord(InvertedIndex& index, const std::string& word);

//функция для форматирования текст
void PrintIndex(const std::map<std::string, std::vector<Entry>>& index) {
    std::cout << "\n--- Inverted Index Content ---" << std::endl;
    for (const auto& pair : index) {
        std::cout << "index[\"" << pair.first << "\"] = ";

        for (size_t i = 0; i < pair.second.size(); ++i) {
            // Печатаем {doc_id, count}
            std::cout << "{" << pair.second[i].doc_id << ", "
                      << pair.second[i].count << "}";

            // Добавляем запятую, если это не последний элемент
            if (i < pair.second.size() - 1) {
                std::cout << ", ";
            }
        }
        std::cout << std::endl;
    }
    std::cout << "------------------------------" << std::endl;
}


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
    setlocale(LC_ALL, "RUS"); // Clion не дружит с локалями, так что вывод в основном на английском
    srand(unsigned(time(NULL)));

    setup_test_environment();
    std::cout << "\n--- Запуск теста ConverterJSON ---" << std::endl;

    //Старый вариант
    /*
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

    //Тест инверсного индекса
    std::vector<std::string> document_texts = {
        "milk sugar salt",                     // doc_id = 0
        "milk a milk b milk c milk d"          // doc_id = 1
    };

    // 2. Создаем экземпляр класса
    InvertedIndex index;

    // 3. Обновляем базу (это запустит индексацию)
    index.UpdateDocumentBase(document_texts);

    // 4. Получаем и печатаем результат
    // Мы используем GetFrequencyDictionary() для получения данных
    PrintIndex(index.GetFrequencyDictionary());
    */

    try {
        // 1. Инициализация и загрузка конфигурации
        ConverterJSON converter;
        std::vector<std::string> docs_content = converter.GetTextDocuments();
        std::vector<std::string> requests = converter.GetRequests();

        // 2. Индексация документов
        InvertedIndex index;
        index.UpdateDocumentBase(docs_content); // Запустит многопоточную индексацию

        // 3. Создание SearchServer
        std::cout << "\n--- ПОИСК ЗАПРОСОВ ---" << std::endl;
        SearchServer server(index);

        // 4. Запуск поиска
        std::vector<std::vector<RelativeIndex>> answers = server.search(requests);

        // 5. Запись результатов в answers.json
        converter.putAnswers(answers);

    } catch (const std::exception& e) {
        std::cerr << "\n--- КРИТИЧЕСКАЯ ОШИБКА ---" << std::endl;
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    // 1. Создаем базу документов
    std::vector<std::string> document_texts = {
        "milk sugar salt",                     // doc_id = 0
        "milk a milk b milk c milk d",         // doc_id = 1
        "salt water and sugar"                 // doc_id = 2
    };

    // 2. Создаем экземпляр класса
    InvertedIndex index;

    // 3. Обновляем базу (это запустит многопоточную индексацию)
    index.UpdateDocumentBase(document_texts);

    // 4. Тестируем новый интерфейс GetWordCount
    std::cout << "\n--- Testing GetWordCount ---" << std::endl;

    TestWord(index, "milk");   // Ожидается {0, 1}, {1, 4}
    TestWord(index, "sugar");  // Ожидается {0, 1}, {2, 1}
    TestWord(index, "a");      // Ожидается {1, 1}
    TestWord(index, "water");  // Ожидается {2, 1}
    TestWord(index, "banana"); // Ожидается "Word not found"

    // 5. Тестируем оператор == (для GTest)
    Entry e1(0, 1);
    Entry e2(0, 1);
    Entry e3(1, 4);

    if (e1 == e2) {
        std::cout << "\n(Test OK: e1 == e2)" << std::endl;
    }
    if (!(e1 == e3)) {
        std::cout << "(Test OK: e1 != e3)" << std::endl;
    }

    system("pause");
    return 0;
}