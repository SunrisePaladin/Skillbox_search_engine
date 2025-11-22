//
// Created by Артём on 27.10.2025.
//

#include "../Headers/InvertedIndex.h"
#include <sstream>
#include <iostream>
#include <thread>
#include <vector>

void InvertedIndex::UpdateDocumentBase(const std::vector<std::string>& input_docs) {
    // 1. Очищаем старые данные
    {
        std::unique_lock<std::shared_mutex> lock(rw_mutex);
        docs.clear();
        freq_dictionary.clear();

        // 2. Копируем новые документы в docs
        docs = input_docs;
    }
    // 3. Запускаем процесс индексации
    std::cout << "Updating document base... " << docs.size() << " documents loaded." << std::endl;

    // 4. Создаём потоки на документы
    std::vector<std::thread> threads;
    threads.reserve(docs.size()); // Резервируем место

    // 5. Запускаем по одному потоку на каждый документ
    // (Требование 1: В отдельных потоках... индексацию каждого из файлов)
    for (size_t doc_id = 0; doc_id < docs.size(); ++doc_id) {
        // emplace_back создает поток "на месте"
        threads.emplace_back(
            &InvertedIndex::_index_one_document, // Функция-член класса
            this,                                // Указатель на 'this'
            doc_id                               // Аргумент для функции
        );
    }

    // 6. Ожидаем завершения всех потоков
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::cout << "Indexing complete. " << freq_dictionary.size()
              << " unique words found." << std::endl;

    //system_index_documents();
}

void InvertedIndex::_index_one_document(size_t doc_id) {
    // 1. Получаем текст
    // Чтение из 'docs' относительно безопасно, т.к. 'docs' не меняется после UpdateDocumentBase (в теории)
    const std::string& text = docs[doc_id];

    // 2. Разбиваем на слова (Требование 2)
    std::vector<std::string> words = _split_text(text);

    // 3. Считаем локальную частоту слов (Требование 3, 4)
    std::map<std::string, size_t> local_word_counts;
    for (const std::string& word : words) {
        if (!word.empty()) {
            local_word_counts[word]++;
        }
    }

    // 4. Обновляем глобальный freq_dictionary (Требование 5)
    // Захватываем эксклюзивный доступ для записи в freq_dictionary
    std::unique_lock<std::shared_mutex> lock(rw_mutex);

    for (const auto& pair : local_word_counts) {
        const std::string& word = pair.first;
        const size_t count = pair.second;

        Entry new_entry(doc_id, count);

        freq_dictionary[word].push_back(new_entry);
    }
}

/*
void InvertedIndex::system_index_documents() {
    for (size_t d = 0; d < docs.size(); ++d) {

        std::map<std::string, size_t> local_word_counts;

        // Разделяем текст на слова
        std::vector<std::string> words = _split_text(docs[d]);

        // Считаем частоту слов в этом документе
        for (const std::string& word : words) {
            if (!word.empty()) {
                local_word_counts[word]++;
            }
        }

        // Обновляем главный freq_dictionary и переносим данные из local_word_counts в freq_dictionary
        for (const auto& pair : local_word_counts) {
            const std::string& word = pair.first;
            const size_t count = pair.second;

            Entry new_entry(d, count);
            freq_dictionary[word].push_back(new_entry);
        }
    }
    std::cout << "Indexing complete. " << freq_dictionary.size()
              << " unique words found." << std::endl;
}
*/

std::vector<std::string> InvertedIndex::_split_text(const std::string& text) const {
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;

    //выдёргиваем слова из строчного потока
    while (ss >> word) {
        words.push_back(word);
    }
    return words;
}

std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) {
    // Захватываем общий (shared) доступ для чтения
    // Несколько потоков могут выполнять этот метод одновременно
    std::shared_lock<std::shared_mutex> lock(rw_mutex);

    auto it = freq_dictionary.find(word);

    if (it == freq_dictionary.end()) {
        // Если слова нет, возвращаем пустой вектор
        return {};
    } else {
        // Возвращаем копию вектора вхождений
        return it->second;
    }
}

const std::map<std::string, std::vector<Entry>>& InvertedIndex::GetFrequencyDictionary() const {
    return freq_dictionary;
}

std::vector<std::string> InvertedIndex::GetDocuments() const {
    return docs;
}