#include "document.h"
#include "paginator.h"
#include "string_processing.h"
#include "search_server.h"
#include "log_duration.h"


SearchServer::SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {}


void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
                               const vector<int>& ratings) {
    if (document_id < 0) {
        throw invalid_argument("id must be greater then 0"s);
    }
    if (documents_.count(document_id) > 0) {
        throw invalid_argument("not unique id"s);
    }

    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

// int SearchServer::GetDocumentId(int index) const{
//     return document_ids_.at(index);
// }

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    }
    const map<string, double>& empty_map = {};
    return empty_map;
}

vector<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

vector<int>::iterator SearchServer::end() {
    return document_ids_.end();
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    LOG_DURATION_STREAM("?????????????? ???????????????????? ???? ??????????????: "s + raw_query);
    auto query = ParseQuery(raw_query);
    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return make_tuple(matched_words, documents_.at(document_id).status);
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Invalid word '"s + word + "'"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw invalid_argument("Invalid query"s);
    }

    return QueryWord{text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
    Query result;
    for (const string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}