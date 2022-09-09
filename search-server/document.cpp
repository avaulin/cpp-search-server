#include "document.h"

using namespace std;

Document::Document() = default;

Document::Document(int id, double relevance, int rating): id(id), relevance(relevance), rating(rating) {}

std::ostream& operator << (std::ostream& out, const Document& doc) {
    return out << "{ document_id = "s << doc.id
        << ", relevance = "s << doc.relevance
        << ", rating = "s << doc.rating << " }"s;
}