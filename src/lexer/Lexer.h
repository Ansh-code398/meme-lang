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
    ELIF,
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

    // Memory Safety & Borrowing (Rust-style)
    UDHAAR,
    UDHAAR_BADAL,
    NAKAL,

    // Error Handling
    PANGA,
    LAFDA,
    PAKKA,
    CHILLAO,

    // Async & Concurrency
    ASYNC,
    SABAR,

    // Module Imports
    IMPORT,
    AS,

    // Enums & Pattern Matching
    AVTAR,
    JAANCH,
    BAAKI,

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
    ARROW,
    THIN_ARROW,

    // Punctuation
    SEMICOLON,
    COMMA,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    DOT,
    ELLIPSIS,
    OPEN_SQUARE_BRACKET,
    CLOSE_SQUARE_BRACKET,
    COLON,
    QUESTION,
    // Literals
    INT_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    FSTRING_LITERAL,

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
            {"let", TokenType::LE},
            {"var", TokenType::LE},
            {"jugaad", TokenType::KAR},
            {"kar", TokenType::KAR},
            {"kaam", TokenType::KAR},
            {"def", TokenType::KAR},
            {"fn", TokenType::KAR},
            {"ye_le", TokenType::WAPAS},
            {"wapas", TokenType::WAPAS},
            {"de", TokenType::WAPAS},
            {"return", TokenType::WAPAS},

            // Conditionals
            {"dekh", TokenType::DEKH},
            {"agar", TokenType::DEKH},
            {"if", TokenType::DEKH},
            {"warna", TokenType::WARNA},
            {"nahi_toh", TokenType::WARNA},
            {"else", TokenType::WARNA},
            {"elif", TokenType::ELIF},

            // Loops & Flow Control
            {"jabtak", TokenType::JABTAK},
            {"while", TokenType::JABTAK},
            {"ghuma", TokenType::GHOOM},
            {"ghoom", TokenType::GHOOM},
            {"har", TokenType::GHOOM},
            {"for", TokenType::GHOOM},
            {"se", TokenType::SE},
            {"from", TokenType::SE},
            {"mein", TokenType::MEIN},
            {"andar", TokenType::MEIN},
            {"in", TokenType::MEIN},
            {"bas_kar", TokenType::BAS},
            {"bas", TokenType::BAS},
            {"break", TokenType::BAS},
            {"aage_badh", TokenType::CHAL},
            {"chal", TokenType::CHAL},
            {"continue", TokenType::CHAL},

            // OOP
            {"biradari", TokenType::SANCHA},
            {"vibe", TokenType::SANCHA},
            {"class", TokenType::SANCHA},
            {"sancha", TokenType::SANCHA},
            {"roop", TokenType::SANCHA},
            {"nepo", TokenType::NEPO},

            // Input / Output
            {"dikha", TokenType::DIKHA},
            {"flex", TokenType::DIKHA},
            {"bol", TokenType::DIKHA},
            {"print", TokenType::DIKHA},
            {"bata", TokenType::POOCH},
            {"pooch", TokenType::POOCH},
            {"puch", TokenType::POOCH},
            {"input", TokenType::POOCH},

            // Logic
            {"aur", TokenType::AUR},
            {"and", TokenType::AUR},
            {"ya", TokenType::YA},
            {"or", TokenType::YA},
            {"not", TokenType::NOT},

            // Literals
            {"han", TokenType::HAN},
            {"haan", TokenType::HAN},
            {"sach", TokenType::HAN},
            {"real", TokenType::HAN},
            {"true", TokenType::HAN},
            {"nhi", TokenType::NHI},
            {"nahi", TokenType::NHI},
            {"na", TokenType::NHI},
            {"jhooth", TokenType::NHI},
            {"cap", TokenType::NHI},
            {"false", TokenType::NHI},
            {"nalla", TokenType::KUCH_NHI},
            {"khali", TokenType::KUCH_NHI},
            {"kuch_nhi", TokenType::KUCH_NHI},
            {"kuch_nahi", TokenType::KUCH_NHI},
            {"null", TokenType::KUCH_NHI},
            {"none", TokenType::KUCH_NHI},
            {"nil", TokenType::KUCH_NHI},

            // Memory Safety & Borrowing
            {"udhaar", TokenType::UDHAAR},
            {"borrow", TokenType::UDHAAR},
            {"ref", TokenType::UDHAAR},
            {"udhaar_badal", TokenType::UDHAAR_BADAL},
            {"udhaar_badlo", TokenType::UDHAAR_BADAL},
            {"borrow_mut", TokenType::UDHAAR_BADAL},
            {"mut_ref", TokenType::UDHAAR_BADAL},
            {"nakal", TokenType::NAKAL},
            {"clone", TokenType::NAKAL},
            {"copy", TokenType::NAKAL},

            // Error Handling
            {"panga", TokenType::PANGA},
            {"koshish", TokenType::PANGA},
            {"try", TokenType::PANGA},
            {"lafda", TokenType::LAFDA},
            {"sambhal", TokenType::LAFDA},
            {"catch", TokenType::LAFDA},
            {"except", TokenType::LAFDA},
            {"pakka", TokenType::PAKKA},
            {"hamesha", TokenType::PAKKA},
            {"finally", TokenType::PAKKA},
            {"chillao", TokenType::CHILLAO},
            {"fek", TokenType::CHILLAO},
            {"throw", TokenType::CHILLAO},
            {"raise", TokenType::CHILLAO},

            // Async & Concurrency
            {"async", TokenType::ASYNC},
            {"side_mein", TokenType::ASYNC},
            {"asar", TokenType::ASYNC},
            {"sabar", TokenType::SABAR},
            {"await", TokenType::SABAR},

            // Module Imports
            {"bula", TokenType::IMPORT},
            {"chura", TokenType::IMPORT},
            {"lao", TokenType::IMPORT},
            {"shamil", TokenType::IMPORT},
            {"import", TokenType::IMPORT},
            {"jaise", TokenType::AS},
            {"as", TokenType::AS},

            // Enums & Pattern Matching
            {"avtar", TokenType::AVTAR},
            {"kism", TokenType::AVTAR},
            {"enum", TokenType::AVTAR},
            {"jaanch", TokenType::JAANCH},
            {"parakh", TokenType::JAANCH},
            {"match", TokenType::JAANCH},
            {"baaki", TokenType::BAAKI},
            {"default", TokenType::BAAKI}};

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
            // F-String literals (f"..." or f'...' or F"..." or F'...')
            // --------------------------------
            if ((currentChar == 'f' || currentChar == 'F') &&
                (peekNext() == '"' || peekNext() == '\''))
            {
                advance(); // consume 'f' or 'F'
                char quote = advance(); // consume opening quote

                std::string value;
                bool terminated = false;

                while (!isAtEnd())
                {
                    char c = peek();

                    if (c == quote)
                    {
                        advance();
                        terminated = true;
                        break;
                    }

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

                if (!terminated)
                {
                    tokens.push_back({TokenType::ERROR, "😒 F-string khol ke chhod diya... band tera baap karega?", tokenLine, tokenColumn});
                }
                else
                {
                    tokens.push_back({TokenType::FSTRING_LITERAL, value, tokenLine, tokenColumn});
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

            else if (currentChar == '=' && peekNext() == '>')
            {
                advance();
                advance();

                tokens.push_back({TokenType::ARROW, "=>", tokenLine, tokenColumn});

                continue;
            }

            else if (currentChar == '-' && peekNext() == '>')
            {
                advance();
                advance();

                tokens.push_back({TokenType::THIN_ARROW, "->", tokenLine, tokenColumn});

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
                if (peekNext() == '.' && position + 2 < source.length() && source[position + 2] == '.')
                {
                    advance(); // 1st dot
                    advance(); // 2nd dot
                    advance(); // 3rd dot
                    tokens.push_back({TokenType::ELLIPSIS, "...", tokenLine, tokenColumn});
                }
                else
                {
                    advance();
                    tokens.push_back({TokenType::DOT, ".", tokenLine, tokenColumn});
                }
                break;

            case '&':
                if (peekNext() == '&')
                {
                    advance(); // 1st &
                    advance(); // 2nd &
                    tokens.push_back({TokenType::AUR, "&&", tokenLine, tokenColumn});
                }
                else
                {
                    advance();
                    tokens.push_back({TokenType::ERROR, "Akela '&' kya kar raha hai? '&&' (aur) likhna chahte ho kya?", tokenLine, tokenColumn});
                }
                break;

            case '|':
                if (peekNext() == '|')
                {
                    advance(); // 1st |
                    advance(); // 2nd |
                    tokens.push_back({TokenType::YA, "||", tokenLine, tokenColumn});
                }
                else
                {
                    advance();
                    tokens.push_back({TokenType::ERROR, "Akela '|' kya kar raha hai? '||' (ya) likhna chahte ho kya?", tokenLine, tokenColumn});
                }
                break;

            case ':':
                advance();
                tokens.push_back({TokenType::COLON, ":", tokenLine, tokenColumn});
                break;

            case '?':
                advance();
                tokens.push_back({TokenType::QUESTION, "?", tokenLine, tokenColumn});
                break;

                // --------------------------------
                // Unknown character
                // --------------------------------

            default:
                if (static_cast<unsigned char>(currentChar) >= 0x80)
                {
                    std::string utfVal = "";
                    while (!isAtEnd() && static_cast<unsigned char>(peek()) >= 0x80)
                    {
                        utfVal += advance();
                    }
                    while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_' || static_cast<unsigned char>(peek()) >= 0x80))
                    {
                        utfVal += advance();
                    }
                    tokens.push_back({TokenType::IDENTIFIER, utfVal, tokenLine, tokenColumn});
                    break;
                }
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
        case TokenType::ELIF:
            return "ELIF";
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
        case TokenType::ASYNC:
            return "ASYNC";
        case TokenType::SABAR:
            return "SABAR";

        // Literals / identifiers
        case TokenType::IDENTIFIER:
            return "IDENTIFIER";
        case TokenType::INT_LITERAL:
            return "INT_LITERAL";
        case TokenType::FLOAT_LITERAL:
            return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:
            return "STRING_LITERAL";
        case TokenType::FSTRING_LITERAL:
            return "FSTRING_LITERAL";

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
        case TokenType::ARROW:
            return "ARROW";
        case TokenType::THIN_ARROW:
            return "THIN_ARROW";

        // Punctuation
        case TokenType::SEMICOLON:
            return "SEMICOLON";
        case TokenType::COMMA:
            return "COMMA";
        case TokenType::DOT:
            return "DOT";
        case TokenType::ELLIPSIS:
            return "ELLIPSIS";

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
        case TokenType::COLON:
            return "COLON";
        case TokenType::QUESTION:
            return "QUESTION";

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