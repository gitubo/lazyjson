#include "tokenizer.hpp"
#include <iostream>

int main() {
    std::string json_data = R"({"is_a_person": true, "name": "Alice", "age": 30, "address": {"city":"Rome", "country": "Italy"}, "hobbies":["swimming", "running"]})";
    lazyjson::Tokenizer tokenizer = lazyjson::Tokenizer();
    lazyjson::TokenizerError tokenizerError;
    if(tokenizer.tokenize(json_data, tokenizerError)){
        std::cerr << "[Error] tokenizer returned error code: " << tokenizerError << std::endl;
    } else {
        std::cout << tokenizer.toString() << std::endl;
    }

    return 0;
}