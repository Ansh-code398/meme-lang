#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>

enum class TokenType
{
    // Keywords
    LE,
    KAR,
    WAPAS,
    DEKH,
    WARNA,
    JABTAK,
    GHOOM,
    SE,
    MEIN,
    BAS,
    CHAL,
    DIKHA,
    POOCH,
    AUR,
    YA,
    HAN,
    NHI,
    KUCH_NHI,
    SANCHA,
    NEPO,

    // Operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MOD,
    ASSIGN,
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    GREATER_THAN,
    LESS_THAN_EQUAL,
    GREATER_THAN_EQUAL,
    NOT,

    // Punctuation
    SEMICOLON,
    COMMA,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    DOT,
    OPEN_SQUARE_BRACKET,
    CLOSE_SQUARE_BRACKET,
    // Literals
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    // Identifiers
    IDENTIFIER,

    // End of file
    END_OF_FILE,

    // Error
    ERROR
};

struct Token
{
    TokenType type;
    std::string value;
    int line;
    int column;
};

class Lexer
{
private:
    std::string source;
    size_t position;
    int line;
    int column;

    bool isAtEnd() const
    {
        return position >= source.length();
    }

    char peek() const
    {
        if (isAtEnd())
            return '\0';
        return source[position];
    };
    char advance()
    {
        if (!isAtEnd())
        {
            char currentChar = source[position++];
            if (currentChar == '\n')
            {
                line++;
                column = 1;
            }
            else
            {
                column++;
            }
            return currentChar;
        }
        return '\0';
    }

    char peekNext() const
    {
        if (position + 1 >= source.length())
            return '\0';
        return source[position + 1];
    }

public:
    Lexer(const std::string &source) : source(source), position(0), line(1), column(1) {}

    std::vector<Token> tokenize()
    {
        std::vector<Token> tokens;

        // KEYWORDS
        std::unordered_map<std::string, TokenType> keywords = {
            // Variables & Functions
            {"pakad", TokenType::LE},
            {"le", TokenType::LE},
            {"jugaad", TokenType::KAR},
            {"kar", TokenType::KAR},
            {"ye_le", TokenType::WAPAS},
            {"wapas", TokenType::WAPAS},

            // Conditionals
            {"dekh", TokenType::DEKH},
            {"agar", TokenType::DEKH},
            {"warna", TokenType::WARNA},
            {"nahi_toh", TokenType::WARNA},

            // Loops & Flow Control
            {"jabtak", TokenType::JABTAK},
            {"ghuma", TokenType::GHOOM},
            {"ghoom", TokenType::GHOOM},
            {"se", TokenType::SE},
            {"mein", TokenType::MEIN},
            {"andar", TokenType::MEIN},
            {"bas_kar", TokenType::BAS},
            {"bas", TokenType::BAS},
            {"aage_badh", TokenType::CHAL},
            {"chal", TokenType::CHAL},

            // OOP
            {"biradari", TokenType::SANCHA},
            {"vibe", TokenType::SANCHA},
            {"class", TokenType::SANCHA},
            {"sancha", TokenType::SANCHA},
            {"nepo", TokenType::NEPO},

            // Input / Output
            {"dikha", TokenType::DIKHA},
            {"flex", TokenType::DIKHA},
            {"bol", TokenType::DIKHA},
            {"bata", TokenType::POOCH},
            {"pooch", TokenType::POOCH},
            {"puch", TokenType::POOCH},

            // Logic
            {"aur", TokenType::AUR},
            {"ya", TokenType::YA},

            // Literals
            {"han", TokenType::HAN},
            {"haan", TokenType::HAN},
            {"real", TokenType::HAN},
            {"nhi", TokenType::NHI},
            {"nahi", TokenType::NHI},
            {"cap", TokenType::NHI},
            {"nalla", TokenType::KUCH_NHI},
            {"khali", TokenType::KUCH_NHI},
            {"kuch_nhi", TokenType::KUCH_NHI},
            {"kuch_nahi", TokenType::KUCH_NHI}};

        while (!isAtEnd())
        {
            char currentChar = peek();

            // --------------------------------
            // Whitespace handling
            // --------------------------------
            if (currentChar == ' ' ||
                currentChar == '\t' ||
                currentChar == '\r' ||
                currentChar == '\n')
            {
                advance();
                continue;
            }

            int tokenLine = line;
            int tokenColumn = column;

            // --------------------------------
            // Comment handling
            // --------------------------------

            // Single-line comment //
            if (currentChar == '/' && peekNext() == '/')
            {
                while (!isAtEnd() && peek() != '\n')
                {
                    advance();
                }

                continue;
            }

            // Multi-line comment /*
            else if (currentChar == '/' && peekNext() == '*')
            {
                advance(); // consume '/'
                advance(); // consume '*'

                bool terminated = false;

                while (!isAtEnd())
                {
                    if (peek() == '*' && peekNext() == '/')
                    {
                        advance(); // consume '*'
                        advance(); // consume '/'
                        terminated = true;
                        break;
                    }

                    advance();
                }

                // Unterminated block comment
                if (!terminated)
                {
                    tokens.push_back({TokenType::ERROR,
                                      "💤 Comment likhte-likhte so gaye kya? '*/' lagana bhool gaye!",
                                      tokenLine,
                                      tokenColumn});
                }

                continue;
            }

            // --------------------------------
            // Identifiers / Keywords
            // --------------------------------
            if (std::isalpha(currentChar) || currentChar == '_')
            {
                std::string value;

                while (!isAtEnd() &&
                       (std::isalnum(peek()) || peek() == '_'))
                {
                    value += advance();
                }

                auto it = keywords.find(value);

                if (it != keywords.end())
                {
                    tokens.push_back({it->second,
                                      value,
                                      tokenLine,
                                      tokenColumn});
                }
                else
                {
                    tokens.push_back({TokenType::IDENTIFIER,
                                      value,
                                      tokenLine,
                                      tokenColumn});
                }

                continue;
            }

            // --------------------------------
            // Number literals
            // --------------------------------
            if (std::isdigit(currentChar))
            {
                std::string value;
                bool isFloat = false;

                while (!isAtEnd() && std::isdigit(peek()))
                {
                    value += advance();
                }

                // Check for decimal point
                if (peek() == '.' && std::isdigit(peekNext()))
                {
                    isFloat = true;

                    value += advance(); // consume '.'

                    while (!isAtEnd() && std::isdigit(peek()))
                    {
                        value += advance();
                    }
                }

                tokens.push_back({isFloat ? TokenType::FLOAT_LITERAL
                                          : TokenType::INT_LITERAL,
                                  value,
                                  tokenLine,
                                  tokenColumn});

                continue;
            }

            // --------------------------------
            // String literals
            // Supports both "..." and '...'
            // --------------------------------
            if (currentChar == '"' || currentChar == '\'')
            {
                char quote = currentChar;

                advance(); // consume opening quote

                std::string value;
                bool terminated = false;

                while (!isAtEnd())
                {
                    char c = peek();

                    // Closing quote
                    if (c == quote)
                    {
                        advance();
                        terminated = true;
                        break;
                    }

                    // Escape characters
                    if (c == '\\')
                    {
                        advance();

                        if (isAtEnd())
                            break;

                        char escaped = advance();

                        switch (escaped)
                        {
                        case 'n':
                            value += '\n';
                            break;

                        case 't':
                            value += '\t';
                            break;

                        case '"':
                            value += '"';
                            break;

                        case '\'':
                            value += '\'';
                            break;

                        case '\\':
                            value += '\\';
                            break;

                        default:
                            value += escaped;
                            break;
                        }
                    }
                    else
                    {
                        value += advance();
                    }
                }

                // Unterminated string
                if (!terminated)
                {
                    tokens.push_back({TokenType::ERROR, "😒 String khol ke chhod diya... band tera baap karega?", tokenLine, tokenColumn});
                }
                else
                {
                    tokens.push_back({TokenType::STRING_LITERAL, value, tokenLine, tokenColumn});
                }

                continue;
            }

            // --------------------------------
            // Double Character Operators
            // --------------------------------

            if (currentChar == '=' && peekNext() == '=')
            {
                advance();
                advance();

                tokens.push_back({TokenType::EQUAL, "==", tokenLine, tokenColumn});

                continue;
            }

            else if (currentChar == '!' && peekNext() == '=')
            {
                advance();
                advance();

                tokens.push_back({TokenType::NOT_EQUAL, "!=", tokenLine, tokenColumn});

                continue;
            }

            else if (currentChar == '<' && peekNext() == '=')
            {
                advance();
                advance();

                tokens.push_back({TokenType::LESS_THAN_EQUAL, "<=", tokenLine, tokenColumn});

                continue;
            }

            else if (currentChar == '>' && peekNext() == '=')
            {
                advance();
                advance();

                tokens.push_back({TokenType::GREATER_THAN_EQUAL, ">=", tokenLine, tokenColumn});

                continue;
            }

            // --------------------------------
            // Single Character Operators
            // --------------------------------

            switch (currentChar)
            {
            case '+':
                advance();
                tokens.push_back({TokenType::PLUS, "+", tokenLine, tokenColumn});
                break;

            case '-':
                advance();
                tokens.push_back({TokenType::MINUS, "-", tokenLine, tokenColumn});
                break;

            case '*':
                advance();
                tokens.push_back({TokenType::MULTIPLY, "*", tokenLine, tokenColumn});
                break;

            case '/':
                advance();
                tokens.push_back({TokenType::DIVIDE, "/", tokenLine, tokenColumn});
                break;

            case '%':
                advance();
                tokens.push_back({TokenType::MOD, "%", tokenLine, tokenColumn});
                break;

            case '=':
                advance();
                tokens.push_back({TokenType::ASSIGN, "=", tokenLine, tokenColumn});
                break;

            case '<':
                advance();
                tokens.push_back({TokenType::LESS_THAN, "<", tokenLine, tokenColumn});
                break;

            case '>':
                advance();
                tokens.push_back({TokenType::GREATER_THAN, ">", tokenLine, tokenColumn});
                break;

            case '!':
                advance();
                tokens.push_back({TokenType::NOT, "!", tokenLine, tokenColumn});
                break;

                // --------------------------------
                // Punctuation
                // --------------------------------

            case ';':
                advance();
                  tokens.push_back({TokenType::SEMICOLON, ";",tokenLine,tokenColumn});
                break;

            case ',':
                advance();
                tokens.push_back({TokenType::COMMA, ",", tokenLine, tokenColumn});
                break;

            case '(':
                advance();
                tokens.push_back({TokenType::OPEN_PAREN, "(", tokenLine, tokenColumn});
                break;

            case ')':
                advance();
                tokens.push_back({TokenType::CLOSE_PAREN, ")", tokenLine, tokenColumn});
                break;

            case '{':
                advance();
                tokens.push_back({TokenType::OPEN_BRACE, "{", tokenLine, tokenColumn});
                break;

            case '}':
                advance();
                tokens.push_back({TokenType::CLOSE_BRACE, "}", tokenLine, tokenColumn});
                break;

            case '[':
                advance();
                tokens.push_back({TokenType::OPEN_SQUARE_BRACKET, "[", tokenLine, tokenColumn});
                break;

            case ']':
                advance();
                  tokens.push_back({TokenType::CLOSE_SQUARE_BRACKET, "]",tokenLine,tokenColumn});
                break;

            case '.':
                advance();
                tokens.push_back({TokenType::DOT, ".", tokenLine, tokenColumn});
                break;

                // --------------------------------
                // Unknown character
                // --------------------------------

            default:
                advance();

                tokens.push_back({TokenType::ERROR, std::string("😭 Chi sasura! Ye kaisa alien symbol ghusa diye ho code mein: ") + currentChar, tokenLine, tokenColumn});

                break;
            }
        }

        // --------------------------------
        // EOF token
        // --------------------------------

          tokens.push_back({TokenType::END_OF_FILE, "", line, column});

        return tokens;
    }

    std::string tokenTypeToString(TokenType type)
    {
        switch (type)
        {
        // Keywords
        case TokenType::LE:
            return "LE";
        case TokenType::KAR:
            return "KAR";
        case TokenType::WAPAS:
            return "WAPAS";
        case TokenType::DEKH:
            return "DEKH";
        case TokenType::WARNA:
            return "WARNA";
        case TokenType::JABTAK:
            return "JABTAK";
        case TokenType::GHOOM:
            return "GHOOM";
        case TokenType::SE:
            return "SE";
        case TokenType::MEIN:
            return "MEIN";
        case TokenType::BAS:
            return "BAS";
        case TokenType::CHAL:
            return "CHAL";
        case TokenType::DIKHA:
            return "DIKHA";
        case TokenType::POOCH:
            return "POOCH";
        case TokenType::AUR:
            return "AUR";
        case TokenType::YA:
            return "YA";
        case TokenType::HAN:
            return "HAN";
        case TokenType::NHI:
            return "NHI";
        case TokenType::KUCH_NHI:
            return "KUCH_NHI";
        case TokenType::SANCHA:
            return "SANCHA";
        case TokenType::NEPO:
            return "NEPO";

        // Literals / identifiers
        case TokenType::IDENTIFIER:
            return "IDENTIFIER";
        case TokenType::INT_LITERAL:
            return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:
            return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:
            return "STRING_LITERAL";

        // Operators
        case TokenType::PLUS:
            return "PLUS";
        case TokenType::MINUS:
            return "MINUS";
        case TokenType::MULTIPLY:
            return "MULTIPLY";
        case TokenType::DIVIDE:
            return "DIVIDE";
        case TokenType::MOD:
            return "MOD";

        case TokenType::ASSIGN:
            return "ASSIGN";
        case TokenType::EQUAL:
            return "EQUAL";
        case TokenType::NOT_EQUAL:
            return "NOT_EQUAL";
        case TokenType::LESS_THAN:
            return "LESS_THAN";
        case TokenType::LESS_THAN_EQUAL:
            return "LESS_THAN_EQUAL";
        case TokenType::GREATER_THAN:
            return "GREATER_THAN";
        case TokenType::GREATER_THAN_EQUAL:
            return "GREATER_THAN_EQUAL";
        case TokenType::NOT:
            return "NOT";

        // Punctuation
        case TokenType::SEMICOLON:
            return "SEMICOLON";
        case TokenType::COMMA:
            return "COMMA";
        case TokenType::DOT:
            return "DOT";

        case TokenType::OPEN_PAREN:
            return "OPEN_PAREN";
        case TokenType::CLOSE_PAREN:
            return "CLOSE_PAREN";

        case TokenType::OPEN_BRACE:
            return "OPEN_BRACE";
        case TokenType::CLOSE_BRACE:
            return "CLOSE_BRACE";

        case TokenType::OPEN_SQUARE_BRACKET:
            return "OPEN_SQUARE_BRACKET";
        case TokenType::CLOSE_SQUARE_BRACKET:
            return "CLOSE_SQUARE_BRACKET";

        // Special
        case TokenType::ERROR:
            return "ERROR";
        case TokenType::END_OF_FILE:
            return "END_OF_FILE";

        default:
            return "UNKNOWN_TOKEN";
        }
    }
};

#endif