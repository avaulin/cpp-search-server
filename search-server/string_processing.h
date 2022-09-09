#pragma once

#include <string>
#include <vector>
#include <set>
#include <stdexcept>

using namespace std;


std::string ReadLine();
int ReadLineWithNumber();
std::vector<std::string> SplitIntoWords(const std::string& text);
bool IsValidWord(const std::string& word);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!IsValidWord(str)) {
            throw invalid_argument("stop word '" + str + "' got unacceptable symbols"s);
        }
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}