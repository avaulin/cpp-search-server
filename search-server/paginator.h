#pragma once

#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end, size_t size) :begin_(begin), end_(end), size_(size) {}

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template< typename T>
std::ostream& operator<<(std::ostream& out, const IteratorRange<T>& ItRange) {
    for (auto it = ItRange.begin(); it != ItRange.end(); ++it) {
        out << *it;
    }
    return out;
}



template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        auto dist = distance(begin, end);
        size_ = dist / page_size;
        if (dist % page_size != 0) {
            ++size_;
        }

        if (begin > end) {
            throw invalid_argument("begin > end"s);
        } else if (page_size == 0) {
            throw invalid_argument("page size must be greater then 0"s);
        }

        for (int i=0; i < size_; ++i) {
            auto p_begin = next(begin, page_size * i);
            auto p_end = next(p_begin, page_size);
            if (p_end > end) {
                p_end = end;
            }
            pages_.push_back({p_begin, p_end, static_cast<size_t>(distance(p_begin, p_end))});
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return size_;
    }

private:
    vector<IteratorRange<Iterator>> pages_;
    size_t size_;
}; 


template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}