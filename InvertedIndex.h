//
// Created by Артём on 27.10.2025.
//

#ifndef SEARCH_ENGINE_INVERTEDINDEX_H
#define SEARCH_ENGINE_INVERTEDINDEX_H


#pragma once

#include <vector>
#include <string>
#include <map>
#include <cstddef>
#include <mutex>         // Для std::mutex и std::lock_guard (или shared_mutex)
#include <shared_mutex>

struct Entry {
    size_t doc_id;
    size_t count;

    bool operator== (const Entry& other) const {
        return (doc_id == other.doc_id) && (count == other.count);
    }

    Entry(size_t id, size_t c) : doc_id(id), count(c) {}
};

class InvertedIndex {
public:
    InvertedIndex() = default;

    //добавил указатель
    void UpdateDocumentBase(const std::vector<std::string>& input_docs);

    //словарь частоты слов
    const std::map<std::string, std::vector<Entry>>& GetFrequencyDictionary() const;

    std::vector<std::string> GetDocuments() const;

    std::vector<Entry> GetWordCount(const std::string& word);

private:

    //void system_index_documents();

    void _index_one_document(size_t doc_id);

    //разбиение на слова
    std::vector<std::string> _split_text(const std::string& text) const;

    std::vector<std::string> docs;

    std::map<std::string, std::vector<Entry>> freq_dictionary;

    /*Мьютекс для безопасного доступа к freq_dictionary.
    * Использую shared_mutex, чтобы разрешить одновременное чтение (GetWordCount)
    * и эксклюзивную запись (во время индексации).
    */
    mutable std::shared_mutex rw_mutex;
};


#endif //SEARCH_ENGINE_INVERTEDINDEX_H