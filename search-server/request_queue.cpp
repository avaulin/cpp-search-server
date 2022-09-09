#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : server_(search_server) {}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    auto res = server_.FindTopDocuments(raw_query, status);
    AddRequest(raw_query, res);
    return res;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    auto res = server_.FindTopDocuments(raw_query);
    AddRequest(raw_query, res);
    return res;
}

void RequestQueue::AddRequest(const string& raw_query, const vector<Document>& response) {
    requests_.push_back({raw_query, response});
    if (response.empty()) {
        ++EmptyRequestCnt_;
    }
    UpdateDeque();
}

int RequestQueue::GetNoResultRequests() const{
    return EmptyRequestCnt_;
}



void RequestQueue::UpdateDeque() {
    if (requests_.size() > min_in_day_) {
        auto del_req = requests_.front();
        if (del_req.response.empty()) {
            --EmptyRequestCnt_;
        }
        requests_.pop_front();
        UpdateDeque();
    }
}