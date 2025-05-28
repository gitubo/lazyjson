#include "tokenizer.hpp"
#include <stdexcept>
#include <cctype>
#include <sstream>

namespace lazyjson {

    std::string Token::toString() const {
        switch (type) {
            case TokenType::TOKEN_SOF:          return "<SOF> ";
            case TokenType::TOKEN_OBJECT_START: return "OBJ_START ";
            case TokenType::TOKEN_OBJECT_END:   return "OBJ_END ";
            case TokenType::TOKEN_ARRAY_START:  return "ARR_START ";
            case TokenType::TOKEN_ARRAY_END:    return "ARR_END ";
            case TokenType::TOKEN_COLON:        return "COLON ";
            case TokenType::TOKEN_COMMA:        return "COMMA ";
            case TokenType::TOKEN_STRING:       return "STRING(" + std::string(value) + ") ";
            case TokenType::TOKEN_NUMBER:       return "NUMBER(" + std::string(value) + ") ";
            case TokenType::TOKEN_BOOLEAN:      return "BOOLEAN(" + std::string(value) + ") ";
            case TokenType::TOKEN_NULL:         return "NULL ";
            case TokenType::TOKEN_EOF:          return "<EOF> ";
            case TokenType::TOKEN_ERROR:        return "<ERROR(" + std::string(value) + ")> ";
            default:                            return "<Invalid token type> ";
        }
    }

    void Token::dump(std::ostringstream& oss) const {
        switch (type) {
            case TokenType::TOKEN_SOF:          oss << "<SOF>"; break;
            case TokenType::TOKEN_OBJECT_START: oss <<  "{ "; break;
            case TokenType::TOKEN_OBJECT_END:   oss <<  " }"; break;
            case TokenType::TOKEN_ARRAY_START:  oss <<  "[ "; break;
            case TokenType::TOKEN_ARRAY_END:    oss <<  " ]"; break;
            case TokenType::TOKEN_COLON:        oss <<  " : "; break;
            case TokenType::TOKEN_COMMA:        oss << ", "; break;
            case TokenType::TOKEN_STRING:       oss << "\"" << value << "\""; break;
            case TokenType::TOKEN_NUMBER:       oss << value; break;
            case TokenType::TOKEN_BOOLEAN:      oss << value; break;
            case TokenType::TOKEN_NULL:         oss << "null"; break;
            case TokenType::TOKEN_EOF:          oss << "<EOF>"; break;
            case TokenType::TOKEN_ERROR:        oss << "<ERROR(" << value << ")> "; break;
            default:                            oss << "<Invalid token type> "; break;
        }
    }
    
    int Tokenizer::tokenize(std::string_view jsonString_, TokenizerError& errorOut) {
        tokens_.clear();
        errorOut = TokenizerError::NONE;
        tokens_.push_back({TokenType::TOKEN_SOF, {jsonString_.begin(), 0}});
        auto it = jsonString_.begin();
        while (it != jsonString_.end()) {
            // Skip initial whitespace
            while (it != jsonString_.end() && std::isspace(*it)) {
                ++it;
            }

            if (it == jsonString_.end()) break;

            switch (*it) {
                case '{': tokens_.push_back({TokenType::TOKEN_OBJECT_START, {it++, 1}}); break;
                case '}': tokens_.push_back({TokenType::TOKEN_OBJECT_END, {it++, 1}}); break;
                case '[': tokens_.push_back({TokenType::TOKEN_ARRAY_START, {it++, 1}}); break;
                case ']': tokens_.push_back({TokenType::TOKEN_ARRAY_END, {it++, 1}}); break;
                case ':': tokens_.push_back({TokenType::TOKEN_COLON, {it++, 1}}); break;
                case ',': tokens_.push_back({TokenType::TOKEN_COMMA, {it++, 1}}); break;
                case '"': {
                    auto start = it + 1;
                    auto end = findStringEnd(it);
                    if (end == jsonString_.end()){
                        errorOut = TokenizerError::UNTERMINATED_STRING;
                        return 1;
                    }
                    tokens_.push_back({TokenType::TOKEN_STRING, {start, static_cast<size_t>(end - start)}});
                    it = end + 1;
                    break;
                }
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '-': {
                    auto start = it;
                    while (it != jsonString_.end() && (std::isdigit(*it) || *it == '.' || *it == 'e' || *it == 'E' || *it == '+' || *it == '-')) {
                        ++it;
                    }
                    tokens_.push_back({TokenType::TOKEN_NUMBER, {start, static_cast<size_t>(it - start)}});
                    break;
                }
                case 'n':
                    if (jsonString_.substr(it - jsonString_.begin(), 4) == "null") {
                        tokens_.push_back({TokenType::TOKEN_NULL, {it, 4}});
                        it += 4;
                        break;
                    }
                    errorOut = TokenizerError::UNEXPECTED_CHARACTER;
                    return 1;
                case 't':
                    if (jsonString_.substr(it - jsonString_.begin(), 4) == "true") {
                        tokens_.push_back({TokenType::TOKEN_BOOLEAN, {it, 4}});
                        it += 4;
                        break;
                    }
                    errorOut = TokenizerError::UNEXPECTED_CHARACTER;
                    return 1;
                case 'f':
                    if (jsonString_.substr(it - jsonString_.begin(), 5) == "false") {
                        tokens_.push_back({TokenType::TOKEN_BOOLEAN, {it, 5}});
                        it += 5;
                        break;
                    }
                    errorOut = TokenizerError::UNEXPECTED_CHARACTER;
                    return 1;
                default:
                    if (!std::isspace(*it)) {
                        errorOut = TokenizerError::UNEXPECTED_CHARACTER;
                        return 1;
                    } else {
                        ++it; // Skip whitespace
                    }
                    break;
            }
        }
        tokens_.push_back({TokenType::TOKEN_EOF, {jsonString_.end(), 0}});
        return 0;
    }

    std::string_view::iterator Tokenizer::findStringEnd(std::string_view::iterator start) {
        auto it = start + 1;
        while (it != jsonString_.end()) {
            if (*it == '"' && *(it - 1) != '\\') {
                return it;
            }
            ++it;
        }
        return jsonString_.end();
    }

    std::vector<Token> Tokenizer::getTokens(){
        return tokens_;
    }

    std::string Tokenizer::toString() const {
        std::ostringstream oss;
        for (const auto& token : tokens_) {
            oss << token.toString();
        }
        return oss.str();
    }

    std::ostream& operator<<(std::ostream& os, const TokenType& type) {
        switch (type) {
            case TokenType::TOKEN_SOF:
                os << "TOKEN_SOF";
                break;
            case TokenType::TOKEN_OBJECT_START:
                os << "TOKEN_OBJECT_START";
                break;
            case TokenType::TOKEN_OBJECT_END:
                os << "TOKEN_OBJECT_END";
                break;
            case TokenType::TOKEN_ARRAY_START:
                os << "TOKEN_ARRAY_START";
                break;
            case TokenType::TOKEN_ARRAY_END:
                os << "TOKEN_ARRAY_END";
                break;
            case TokenType::TOKEN_COLON:
                os << "TOKEN_COLON";
                break;
            case TokenType::TOKEN_COMMA:
                os << "TOKEN_COMMA";
                break;
            case TokenType::TOKEN_STRING:
                os << "TOKEN_STRING";
                break;
            case TokenType::TOKEN_NUMBER:
                os << "TOKEN_NUMBER";
                break;
            case TokenType::TOKEN_BOOLEAN:
                os << "TOKEN_BOOLEAN";
                break;
            case TokenType::TOKEN_NULL:
                os << "TOKEN_NULL";
                break;            
            case TokenType::TOKEN_EOF:
                os << "TOKEN_EOF";
                break;
            case TokenType::TOKEN_ERROR:
                os << "TOKEN_ERROR";
                break;
            default:
                os << "TOKEN_ERROR";
                break;
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const TokenizerError& error) {
        switch (error) {
            case TokenizerError::UNTERMINATED_STRING:
                os << "UNTERMINATED_STRING";
                break;
            case TokenizerError::UNEXPECTED_CHARACTER:
                os << "UNEXPECTED_CHARACTER";
                break;
            case TokenizerError::INVALID_PATH_FORMAT:
                os << "Invalid path format";
                break;
            default:
                os << "UNKNOWN_ERROR";
                break;
        }
        return os;
    }
} // namespace lazyjson