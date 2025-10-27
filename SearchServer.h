//
// Created by Артём on 27.10.2025.
//

#ifndef SEARCH_ENGINE_SEARCHSERVER_H
#define SEARCH_ENGINE_SEARCHSERVER_H
#define float_eps std::numeric_limits<float>::epsilon()

#pragma once

#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <cmath>
#include "InvertedIndex.h"
#include "ConverterJSON.h"

struct RelativeIndex {
    size_t doc_id;
    float rank;
    bool operator ==(const RelativeIndex& other) const {
        return (doc_id == other.doc_id && (fabs(rank - other.rank) < float_eps));
    }
};

class SearchServer {
public:
    SearchServer(InvertedIndex& idx) : _index(idx) {};

    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string>& queries_input);

private:

    std::map<size_t, size_t> _calculate_absolute_relevance(const std::vector<std::string>& unique_words) const;

    std::vector<std::string> _split_text(const std::string& text) const;

    // Преобразует абсолютную релевантность в относительную (rank).
    std::vector<RelativeIndex> _get_ranked_results(const std::map<size_t, size_t>& absolute_relevance) const;

    InvertedIndex& _index;
};

#endif //SEARCH_ENGINE_SEARCHSERVER_H