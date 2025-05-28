#ifndef LAZYJSON_TOKENIZER_HPP
#define LAZYJSON_TOKENIZER_HPP

#include <string_view>
#include <vector>

namespace lazyjson {

    enum class TokenType {
        TOKEN_SOF,
        TOKEN_OBJECT_START,
        TOKEN_OBJECT_END,
        TOKEN_ARRAY_START,
        TOKEN_ARRAY_END,
        TOKEN_COLON,
        TOKEN_COMMA,
        TOKEN_STRING,
        TOKEN_NUMBER,
        TOKEN_BOOLEAN,
        TOKEN_NULL,
        TOKEN_EOF,
        TOKEN_ERROR
    };
    std::ostream& operator<<(std::ostream& os, const TokenType& error);

    struct Token {
        TokenType type;
        std::string_view value;
        std::string toString() const;
        void dump(std::ostringstream&) const;
    };

    enum class TokenizerError {
        UNTERMINATED_STRING,
        UNEXPECTED_CHARACTER,
        INVALID_PATH_FORMAT,
        NONE
    };
    std::ostream& operator<<(std::ostream& os, const TokenizerError& error);

    class Tokenizer {
    public:
        Tokenizer(){}
        int tokenize(std::string_view, TokenizerError&);
        std::vector<Token> getTokens();
        std::string toString() const;

    private:
        void skipWhitespace(std::string_view::iterator& it);
        std::string_view::iterator findStringEnd(std::string_view::iterator start);

        std::vector<Token> tokens_;
        std::string_view jsonString_;
    };

} // namespace lazyjson

#endif // LAZYJSON_TOKENIZER_HPP