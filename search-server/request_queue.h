#pragma once

#include <vector>
#include <string>
#include <deque>

#include "search_server.h"
#include "document.h"


class RequestQueue {
public:
    RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        auto res = server_.FindTopDocuments(raw_query, document_predicate);
        AddRequest(raw_query, res);
        return res;
    }

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
    vector<Document> AddFindRequest(const string& raw_query);
    void AddRequest(const string& raw_query, const vector<Document>& response);
    int GetNoResultRequests() const;
    void UpdateDeque();

private:
    struct QueryResult {
        string raw_query;
        vector<Document> response;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    int EmptyRequestCnt_ = 0;
}; 