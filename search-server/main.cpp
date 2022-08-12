#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        vector<string> words = SplitIntoWordsNoStop(document);
        map<string, double> word_counts;
        for (string& word : words) {
            ++word_counts[word];
        }
        for (auto& [word, count] : word_counts) {
            word_to_document_freqs_[word][document_id] = count / words.size();
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct Query {
        set<string> plus;
        set<string> minus;
    };

    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    Query ParseQuery(const string& text) const {
        Query q;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                q.minus.insert(word);
            } else {
                q.plus.insert(word);
            }
        }
        return q;
    }

    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> matched_documents;
        map<string, int> words_freq;
        map<int, double> doc_freqs;
        
        for (const string& word : query.plus) {
            if (word_to_document_freqs_.count(word) > 0) {
                words_freq[word] = word_to_document_freqs_.at(word).size();
            }
        }
        
        for (auto& [term, term_freq] : words_freq) {
            double idf = log(static_cast<double>(document_count_) / term_freq);
            //cout << term << " "s << term_freq << " "s << idf << endl;
            if (word_to_document_freqs_.count(term) > 0) {
                for (auto& [doc, doc_freq] : word_to_document_freqs_.at(term)) {
                    doc_freqs[doc] += (idf * doc_freq);
                    //cout << doc << " "s << doc_freq << endl;
                }
            }
        }
        for (auto& [doc, freq] : doc_freqs) {
            matched_documents.push_back({doc, freq});
        }
        
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}
