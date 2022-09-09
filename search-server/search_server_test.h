#pragma once

#include "search_server.h"
#include "request_queue.h"

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




// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server("and"s);
        server.AddDocument(doc_id, content, Document::DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, Document::DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestAddDocument() {
	// Проверим, что добавляемый документ находится по слову из него
	{
		SearchServer server("and"s);
		server.AddDocument(43, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("dog"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 43);
	}
	// 2 docs
	{
		SearchServer server("and"s);
		ASSERT_EQUAL(server.GetDocumentCount(), 0);
		server.AddDocument(42, "cat in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		ASSERT_EQUAL(server.GetDocumentCount(), 2);
	}
}

void TestExcludedDocumentsWithMinusWords() {
	{
		SearchServer server("and"s);
		server.AddDocument(42, "cat in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("city -dog"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 42);
	}
}

void TestDocumentMatching() {
	{
		SearchServer server("and"s);
		server.AddDocument(42, "cat in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		const auto [matched_words, status] = server.MatchDocument("in city", 42);
		//const auto matched_words = get<0>(matching_result);
		ASSERT_EQUAL(matched_words.size(),2);
		vector<string> estimated_words = {"city"s, "in"s};
		ASSERT_EQUAL(matched_words, estimated_words);
		ASSERT_EQUAL(static_cast<int>(status), 0);
	}
	{
		SearchServer server("and"s);
		server.AddDocument(42, "cat in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		const auto matching_result = server.MatchDocument("in city", 42);
		const auto matched_words = get<0>(matching_result);
		ASSERT_EQUAL(matched_words.size(),2);
		vector<string> estimated_words = {"city"s, "in"s};
		ASSERT_EQUAL(matched_words, estimated_words);
	}
	{
		SearchServer server("and"s);
		server.AddDocument(42, "cat in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		const auto matching_result = server.MatchDocument("in city -cat", 42);
		const auto matched_words = get<0>(matching_result);
		ASSERT_EQUAL(matched_words.size(), 0);
	}
//	5 out of 6
	{
		SearchServer server("and"s);
		server.AddDocument(42, "cat in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(44, "pig in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(45, "lost in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(46, "rain in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(47, "ghost in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("city"s);
		ASSERT_EQUAL(found_docs.size(), 5);
	}
}

void TestDocsSortByRelevance() {
	{
		SearchServer server("and"s);
		server.AddDocument(42, "dog run"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(45, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(46, "dog in box"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(44, "dog in the city and pants"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		const auto found_docs = server.FindTopDocuments("dog city pants"s);
		ASSERT_EQUAL(found_docs.size(), 4);
		ASSERT_EQUAL(found_docs[0].id, 44);
		ASSERT_EQUAL(found_docs[1].id, 45);
		ASSERT_EQUAL(found_docs[2].id, 42);
	}
}

void TestDocsRating() {
	{
		SearchServer server("and"s);
		server.AddDocument(42, "cat in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(43, "dog in the city"s, Document::DocumentStatus::ACTUAL, {100, 10, 100});
		const auto found_docs = server.FindTopDocuments("city"s);
		ASSERT_EQUAL(found_docs[0].rating, 70); // (100 + 10 + 100) / 3 = 70
		ASSERT_EQUAL(found_docs[1].rating, 2); // (1 + 2 + 3) / 3 = 2
	}
}

void TestSearchWithPredicate() {
	{
		SearchServer server("and"s);
		server.AddDocument(54, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(55, "dog in the city"s, Document::DocumentStatus::ACTUAL, {100, 10, 100});
		server.AddDocument(297, "dog in the small town"s, Document::DocumentStatus::ACTUAL, {100, 10, 100});
		const auto found_docs = server.FindTopDocuments("dog"s, [](int document_id, Document::DocumentStatus status, int rating) { return document_id % 27 == 0; });
		ASSERT_EQUAL(found_docs.size(), 2);
		ASSERT_EQUAL(found_docs[0].id, 297);
		ASSERT_EQUAL(found_docs[1].id, 54);
	}
}

void TestSearchWithStatus() {
	{
		SearchServer server("and"s);
		server.AddDocument(54, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(55, "dog in the city"s, Document::DocumentStatus::IRRELEVANT, {100, 10, 100});
		server.AddDocument(297, "dog in the small town"s, Document::DocumentStatus::BANNED, {100, 10, 100});
		const auto found_docs = server.FindTopDocuments("dog"s, Document::DocumentStatus::IRRELEVANT);
		ASSERT_EQUAL(found_docs.size(), 1);
		ASSERT_EQUAL(found_docs[0].id, 55);
	}
	{
		SearchServer server("and"s);
		server.AddDocument(54, "dog in the city"s, Document::DocumentStatus::ACTUAL, {1,2,3});
		server.AddDocument(55, "dog in the city"s, Document::DocumentStatus::IRRELEVANT, {100, 10, 100});
		const auto found_docs = server.FindTopDocuments("dog"s, Document::DocumentStatus::REMOVED);
		ASSERT_EQUAL(found_docs.size(), 0);
	}
}

void TestCalculateRalavance() {
	{
		SearchServer search_server("and"s);
		search_server.AddDocument(11, "белый кот и модный ошейник"s,        Document::DocumentStatus::ACTUAL, {8, -3});
		search_server.AddDocument(12, "пушистый кот пушистый хвост"s,       Document::DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(13, "ухоженный пёс выразительные глаза"s, Document::DocumentStatus::ACTUAL, {5, -12, 2, 1});
		search_server.AddDocument(14, "ухоженный скворец евгений"s,         Document::DocumentStatus::BANNED, {9});
		const auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
		ASSERT_EQUAL(found_docs.size(), 3);
		ASSERT(found_docs[0].relevance - 0.866434 < FLOAT_COMPARE_THRESHOLD);
		ASSERT(found_docs[1].relevance - 0.173287 < FLOAT_COMPARE_THRESHOLD);
		ASSERT(found_docs[2].relevance - 0.138629 < FLOAT_COMPARE_THRESHOLD);
		/* Логика расчета relevance (TF-IDF)
		IDF слова = log(количество всех документов / количество документов где встречается слово)
		IDF пушистый = log(4 / 1) = 1.38629
		IDF ухоженный = log(4 / 2) = 0.693147
		IDF кот = log(4 / 2) = 0.693147

		TF слова в документе = количество этого слова в документе / количество слов в документе

						док 11	док 12	док 13	док 14
		TF пушистый      0       2/4     0        0
		TF ухоженный     0        0     1/4      1/3
		TF кот          1/4      1/4     0        0

		TF-IDF документа = сумма произведений TF и IDF всех слов запроса
		TF-IDF 11 = 0 + 0 + 0.25 *0.693147 = 0.173287
		TF-IDF 12 = 0.5 * 1.38629 + 0 + 0.25 * 0.693147 = 0,693145 + 0,173287 = 0,866432
		TF-IDF 13 = 0 + 0.25 * 0.693147 + 0 = 0,173287
		TF-IDF 14 = 0 + 0.33333 * 0.693147 + 0 = 0,231049
		 */
	}
}

void TestResultPagination() {
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, Document::DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, Document::DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, Document::DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, Document::DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, Document::DocumentStatus::ACTUAL, {1, 1, 1});
	search_server.AddDocument(6, "big dog hamster Vasya"s, Document::DocumentStatus::ACTUAL, {1, 1, 1});
	search_server.AddDocument(7, "big dog hamster Varya"s, Document::DocumentStatus::ACTUAL, {1, 1, 1});

    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
	
	// 1 документ со словом "curly" и 4 документа со словом "dog", всего 5.
	// размер страницы 5, значит 5 документов распологаются на 3 страницах
	ASSERT_EQUAL(pages.size(), 3);
}

void TestRequestQueueStore() {
	SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, Document::DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, Document::DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, Document::DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, Document::DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, Document::DocumentStatus::ACTUAL, {1, 1, 1});

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);

    ASSERT_EQUAL(request_queue.GetNoResultRequests(), 1437);
}

void TestSearchServer() {
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestAddDocument);
	RUN_TEST(TestExcludedDocumentsWithMinusWords);
	RUN_TEST(TestDocumentMatching);
	RUN_TEST(TestDocsSortByRelevance);
	RUN_TEST(TestDocsRating);
	RUN_TEST(TestSearchWithPredicate);
	RUN_TEST(TestSearchWithStatus);
	RUN_TEST(TestCalculateRalavance);
    RUN_TEST(TestResultPagination);
	RUN_TEST(TestRequestQueueStore);
}