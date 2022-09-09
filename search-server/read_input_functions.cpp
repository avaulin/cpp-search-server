#include "read_input_functions.h"

#include <iostream>

using namespace std;

string ReadLine() {
    string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}