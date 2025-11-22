//
// Created by Артём on 27.10.2025.
//

#include "../Headers/SearchServer.h"
#include <sstream>
#include <cmath>
#include <limits>
#include <set>

std::vector<std::string> SearchServer::_split_text(const std::string& text) const {
    std::vector<std::string> words;
    std::stringstream ss(text);
    std::string word;
    while (ss >> word) {
        words.push_back(word);
    }
    return words;
}

std::vector<std::vector<RelativeIndex>> SearchServer::search(const std::vector<std::string>& queries_input) {
    std::vector<std::vector<RelativeIndex>> final_results;

    for (const std::string& query : queries_input) {
        // 1 и 2. Разбиение и формирование уникального списка слов
        std::vector<std::string> words = _split_text(query);
        std::map<std::string, bool> unique_map;
        for (const std::string& word : words) {
            unique_map[word] = true;
        }

        std::vector<std::string> unique_words;
        for (const auto& pair : unique_map) {
            unique_words.push_back(pair.first);
        }

        // 3. Сортировка по частоте (самые редкие - первые)
        auto get_total_word_count = [this](const std::string& word) -> size_t {
            size_t total_count = 0;
            for (const auto& entry : this->_index.GetWordCount(word)) {
                total_count += entry.count;
            }
            return total_count;
        };

        std::sort(unique_words.begin(), unique_words.end(),
            [&](const std::string& a, const std::string& b) {
                return get_total_word_count(a) < get_total_word_count(b);
            });

        // 4, 5 и 6. Расчет абсолютной релевантности и фильтрация
        // Если по итогу не осталось ни одного документа, abs_relevance будет пустой.
        std::map<size_t, size_t> abs_relevance = _calculate_absolute_relevance(unique_words);

        if (abs_relevance.empty()) {
            final_results.emplace_back();
        }
        else {
            // 7, 8. Расчет относительной релевантности и сортировка
            std::vector<RelativeIndex> ranked_results = _get_ranked_results(abs_relevance);
            final_results.push_back(std::move(ranked_results));
        }
    }
    return final_results;
}

std::map<size_t, size_t> SearchServer::_calculate_absolute_relevance(const std::vector<std::string>& unique_words) const {
    std::map<size_t, size_t> final_doc_relevance;

    if (unique_words.empty()) {
        return final_doc_relevance;
    }

    // 1. Инициализация (Шаг 4): По первому, самому редкому слову находим все документы.
    const std::string& rare_word = unique_words[0];

    std::set<size_t> common_doc_ids;

    std::vector<Entry> rare_word_entries = _index.GetWordCount(rare_word);

    // Если самое редкое слово не найдено, нет смысла продолжать (Требование 6)
    if (rare_word_entries.empty()) {
        return final_doc_relevance;
    }

    for (const Entry& entry : rare_word_entries) {
        final_doc_relevance[entry.doc_id] = entry.count;
        common_doc_ids.insert(entry.doc_id);
    }

    // 2. Итеративное сужение и расчет (Шаг 5): По каждому следующему слову.
    for (size_t i = 1; i < unique_words.size(); ++i) {
        const std::string& current_word = unique_words[i];

        std::set<size_t> current_word_doc_ids;
        std::vector<Entry> current_entries = _index.GetWordCount(current_word);

        // 2.1. Формируем doc_ids для текущего слова, а также обновляем релевантность
        for (const Entry& entry : current_entries) {
            current_word_doc_ids.insert(entry.doc_id);

            if (common_doc_ids.count(entry.doc_id)) {
                final_doc_relevance[entry.doc_id] += entry.count;
            }
        }

        // 2.2. Фильтрация: Находим пересечение (новые общие документы)
        std::set<size_t> intersection_doc_ids;
        for (size_t doc_id : common_doc_ids) {
            if (current_word_doc_ids.count(doc_id)) {
                intersection_doc_ids.insert(doc_id);
            }
        }
        common_doc_ids = std::move(intersection_doc_ids);

        // 2.3. Удаляем из final_doc_relevance те, которые больше не соответствуют (не содержат текущее слово)
        // Итерируемся по копии keys, чтобы безопасно удалить из map
        std::vector<size_t> docs_to_remove;
        for (const auto& pair : final_doc_relevance) {
            if (common_doc_ids.find(pair.first) == common_doc_ids.end()) {
                docs_to_remove.push_back(pair.first);
            }
        }
        for (size_t doc_id : docs_to_remove) {
            final_doc_relevance.erase(doc_id);
        }

        if (common_doc_ids.empty()) {
            return {};
        }
    }

    // 3. Возвращаем абсолютную релевантность только для тех документов,
    return final_doc_relevance;
}

std::vector<RelativeIndex> SearchServer::_get_ranked_results(const std::map<size_t, size_t>& absolute_relevance) const {
    std::vector<RelativeIndex> ranked_results;

    // 1. Находим максимальную абсолютную релевантность
    size_t max_abs_relevance = 0;
    for (const auto& pair : absolute_relevance) {
        if (pair.second > max_abs_relevance) {
            max_abs_relevance = pair.second;
        }
    }

    if (max_abs_relevance == 0) {
        return ranked_results;
    }

    // 2. Расчет относительной релевантности
    for (const auto& pair : absolute_relevance) {
        float rank = (float)pair.second / max_abs_relevance;
        ranked_results.push_back({
            pair.first, // doc_id
            rank
        });
    }

    // 3. Сортировка
    std::sort(ranked_results.begin(), ranked_results.end(),
        [](const RelativeIndex& a, const RelativeIndex& b) {

            if (fabs(a.rank - b.rank) > float_eps) {
                return a.rank > b.rank;
            }
            return a.doc_id < b.doc_id;
        });

    return ranked_results;
}