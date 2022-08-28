#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <cassert>

using namespace std;


template <typename T>
void RunTestImpl(const T func, const string& func_name) {
    func();
    cerr << func_name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl((func), (#func))



template <typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    out << "[";
    bool fst = true;
    for (const T& element : container) {
        if (fst) {
            out << element;
            fst = false;
        } else {
            out << ", "s << element;
        }
    }
    out << "]";
    return out;
}
template <typename T>
ostream& operator<<(ostream& out, const set<T>& container) {
    out << "{";
    bool fst = true;
    for (const T& element : container) {
        if (fst) {
            out << element;
            fst = false;
        } else {
            out << ", "s << element;
        }
    }
    out << "}";
    return out;
}
template <typename T, typename U>
ostream& operator<<(ostream& out, const map<T, U>& container) {
    out << "{";
    bool fst = true;
    for (const auto& [key, value] : container) {
        if (fst) {
            out << key << ": " << value;
            fst = false;
        } else {
            out << ", "s << key << ": " << value;
        }
    }
    out << "}";
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))



const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double FLOAT_COMPARE_THRESHOLD = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template <typename Filter>
    vector<Document> FindTopDocuments(const string& raw_query, Filter filter) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < FLOAT_COMPARE_THRESHOLD) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus filter_status) const {
        return FindTopDocuments(raw_query,
                                [filter_status](int document_id, DocumentStatus status, int rating) {
                                    return status == filter_status;
                                });
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
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
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Filter>
    vector<Document> FindAllDocuments(const Query& query, Filter filter) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const DocumentData& document_data = documents_.at(document_id);
                if (filter(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestAddDocument() {
	// Проверим, что добавляемый документ находится по слову из него
	{
		SearchServer server;
		server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("dog"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 43);
	}
	// 2 docs
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("city"s);
		ASSERT_EQUAL(found_docs.size(),2);
	}
	// 5 out of 6
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(44, "pig in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(45, "lost in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(46, "rain in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(47, "ghost in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("city"s);
		ASSERT_EQUAL(found_docs.size(), 5);
	}
	// same id
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(42, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("city"s);
		ASSERT_EQUAL(found_docs.size(), 1);
	}
}

void TestMinusWords() {
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("city -dog"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 42);
	}
}

void TestDocumentMatching() {
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto matching_result = server.MatchDocument("in city", 42);
		const auto matched_words = get<0>(matching_result);
		ASSERT_EQUAL(matched_words.size(),2);
		vector<string> estimated_words = {"city"s, "in"s};
		ASSERT_EQUAL(matched_words, estimated_words);
	}
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto matching_result = server.MatchDocument("in city", 42);
		const auto matched_words = get<0>(matching_result);
		ASSERT_EQUAL(matched_words.size(),2);
		vector<string> estimated_words = {"city"s, "in"s};
		ASSERT_EQUAL(matched_words, estimated_words);
	}
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto matching_result = server.MatchDocument("in city -cat", 42);
		const auto matched_words = get<0>(matching_result);
		ASSERT_EQUAL(matched_words.size(), 0);
	}
}

void TestDocsSortByRelevance() {
	{
		SearchServer server;
		server.AddDocument(42, "dog run"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(45, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(46, "dog in box"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(44, "dog in the city and pants"s, DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("dog city pants"s);
		ASSERT_EQUAL(found_docs.size(), 4);
		ASSERT_EQUAL(found_docs[0].id, 44);
		ASSERT_EQUAL(found_docs[1].id, 45);
		ASSERT_EQUAL(found_docs[2].id, 42);
	}
}

void TestDocsRating() {
	{
		SearchServer server;
		server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {100, 10, 100});
		const auto found_docs = server.FindTopDocuments("city"s);
		ASSERT_EQUAL(found_docs[0].rating, 70);
		ASSERT_EQUAL(found_docs[1].rating, 2);
	}
}

void TestSearchWithPredicate() {
	{
		SearchServer server;
		server.AddDocument(54, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(55, "dog in the city"s, DocumentStatus::ACTUAL, {100, 10, 100});
		server.AddDocument(297, "dog in the small town"s, DocumentStatus::ACTUAL, {100, 10, 100});
		const auto found_docs = server.FindTopDocuments("dog"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 27 == 0; });
		ASSERT_EQUAL(found_docs.size(), 2);
		ASSERT_EQUAL(found_docs[0].id, 297);
		ASSERT_EQUAL(found_docs[1].id, 54);
	}
}

void TestSearchWithStatus() {
	{
		SearchServer server;
		server.AddDocument(54, "dog in the city"s, DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(55, "dog in the city"s, DocumentStatus::IRRELEVANT, {100, 10, 100});
		server.AddDocument(55, "dog in the city"s, DocumentStatus::REMOVED, {100, 10, 100});
		server.AddDocument(297, "dog in the small town"s, DocumentStatus::BANNED, {100, 10, 100});
		const auto found_docs = server.FindTopDocuments("dog"s, DocumentStatus::IRRELEVANT);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 55);
	}
}

void TestRalavanceAlgo() {
	{
		SearchServer search_server;
		search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
		search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
		search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
		const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
		ASSERT_EQUAL(found_docs.size(), 3);
		ASSERT(found_docs[0].relevance - 0.866434 < FLOAT_COMPARE_THRESHOLD);
		ASSERT(found_docs[1].relevance - 0.173287 < FLOAT_COMPARE_THRESHOLD);
		ASSERT(found_docs[2].relevance - 0.138629 < FLOAT_COMPARE_THRESHOLD);
	}
}

void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestAddDocument);
	RUN_TEST(TestMinusWords);
	RUN_TEST(TestDocumentMatching);
	RUN_TEST(TestDocsSortByRelevance);
	RUN_TEST(TestDocsRating);
	RUN_TEST(TestSearchWithPredicate);
	RUN_TEST(TestSearchWithStatus);
	RUN_TEST(TestRalavanceAlgo);
}


int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}




