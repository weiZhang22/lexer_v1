#include <iostream>

//读入字符流
#include <fstream>
#include <sstream>

//智能指针
#include <memory>

// 引入目的：构建符号表.
#include <map>

//
#include <cstring>
#include <vector>  //每次扩容百分之五十

// 运算符、表达式、变量等的类型(其中，除Type_array外，皆是基础类型，即base-type, 或builtin-type).
typedef enum Type {
    Type_int,
    Type_float,
    Type_bool,
    Type_void,
    Type_char,
    Type_array,
} Type;

Type type_returned_of_current_function;

// 字符流，是Lexer的输入. main()函数负责把源代码读入input_string.
static std::string input_string;
static std::string current_break_label;
static std::string current_continue_label;
static std::map<std::string, std::string> print_format_strings; //for assembly code

// 编译过程中，将错误信息打印出来
static void print_line(int line_no, int column_no, const std::string &msg) {
    std::stringstream input_string_stream(input_string);
    std::string line_str;

    for (long i = 0; i <= line_no; i++) std::getline(input_string_stream, line_str, '\n');

    fprintf(stderr, "error at %d:%d\n", line_no + 1, column_no + 1);
    fprintf(stderr, "%s\n", line_str.c_str());
    for (int i = 0; i < column_no; i++)
        fprintf(stderr, " ");
    fprintf(stderr, "^ %s\n", msg.c_str());
}


//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// 用不同的整数值表示token的不同类型;枚举类型
typedef enum TOKEN_TYPE {
    TK_INT_LITERAL,                  // [0-9]+
    TK_FLOAT_LITERAL,                // [0-9]+.[0-9]+
    TK_BOOL_LITERAL,                 // true or false
    TK_CHAR_LITERAL,                 // 'a'
    TK_IDENTIFIER,                   // identifier

    // keywords
    TK_INT,                          // int
    TK_FLOAT,                        // float
    TK_BOOL,                         // bool
    TK_VOID,                         // void
    TK_CHAR,                         // char
    TK_RETURN,                       // return
    TK_IF,                           // if
    TK_ELSE,                         // else
    TK_WHILE,                        // while
    TK_FOR,                          // for
    TK_BREAK,                        // break
    TK_CONTINUE,                     // continue
    TK_PRINT,                        // print

    //logical operators
    TK_PLUS,                         // "+"
    TK_MINUS,                        // "-"
    TK_MULorDEREF,                   // "*"
    TK_DIV,                          // "/"
    TK_ADDRESS,                      // "&"
    TK_NOT,                          // "!"
    TK_LPAREN,                       // "("
    TK_RPAREN,                       // ")"
    TK_LBRACE,                       // "{"
    TK_RBRACE,                       // "}"
    TK_LBRACK,                       // "["
    TK_RBRACK,                       // "]"
    TK_ASSIGN,                       // "="
    TK_COMMA,                        // ","
    TK_SEMICOLON,                    // ";"

    // comparison operators
    TK_AND,                     // "&&"
    TK_OR,                      // "||"
    TK_EQ,                      // "=="
    TK_NE,                      // "!="
    TK_LT,                      // "<"
    TK_LE,                      // "<="
    TK_GT,                      // ">"
    TK_GE,                      // ">="

    // 人为引入的一个token，用以标识字符流（亦即源代码）的结尾.
    TK_EOF = -1,
} TOKEN_TYPE;

// TOKEN结构体.
struct TOKEN {
    TOKEN_TYPE type = TK_EOF;
    std::string lexeme;
    // 用于定位编译错误在源程序中的位置.
    int lineNo = -1;
    int columnNo = -1;
};

class Lexer {
private:
    // 字符流指针.
    long pos = 0;

    int lineNo = 0;
    int columnNo = 0;

    // 读取当前字符，并将指针指向下一个字符.
    char getChar() {
        columnNo++;
        return input_string[pos++];
    }

    // 回退一个字符.
    void put_backChar() {
        pos--;
        columnNo--;
    }

    // 获得下一个token.
    TOKEN getNextToken();

public:
    // token流，词法分析结果保存于此列表.
    std::vector<TOKEN> tokenList;

    // 构建整个token流.
    std::vector<TOKEN> constructTokenStream();
};

// getNextToken - 读取字符流(input_string)直至分析出一个token，并将此token返回.
TOKEN Lexer::getNextToken() {
    char CurrentChar = getChar();
    char NextChar;
    // 忽略空白.
    while (isspace(CurrentChar)) {
        if (CurrentChar == '\n') {
            lineNo++;
            columnNo = 0;
        }
        CurrentChar = getChar();
    }

    // 忽略注释.
    while (CurrentChar == '/') {// '/'可能是除运算符，也可能是行注释的开头

        if ((CurrentChar = getChar()) == '/') {       // 行注释
            while ((CurrentChar = getChar()) != '\n' && input_string.length() >= pos) //continue 意味着继续分析
                continue;
        } else if (CurrentChar == '*') {         // 块注释
            if ((CurrentChar = getChar()) == '/') {
                print_line(lineNo, columnNo - 1, "missing '*' before '/'");
                exit(1);
            } else{
                while(input_string.length() > pos && (!(CurrentChar == '*' & (NextChar = getChar()) =='/'))){
                    if (CurrentChar == '\n' ){
                        lineNo++;
                        columnNo = 0;
                    }
                    CurrentChar = NextChar;
                }
                if (CurrentChar == '*' && NextChar =='/'){
                    CurrentChar = getChar();
                } else{
                    print_line(lineNo, columnNo , " missing '*/'");
                    exit(1);
                }
            }
        } else{
            print_line(lineNo, columnNo - 1, "missing '/' or '*'");
            exit(1);
        }


//                put_backChar();
//                if ((CurrentChar = getChar()) != '/') {
//                    print_line(lineNo, columnNo - 2, "missing '*' before '/'");
//                    exit(1);
//                      CurrentChar = getChar();

//                } else {
//                    getChar();
//                    CurrentChar = getChar();
//                }
//            }
//        } else {
//            print_line(lineNo, columnNo - 3, "missing '*' or '/'");
//            exit(1);
//        }
        // 忽略注释后的空白符.
        while (isspace(CurrentChar)) {
            if (CurrentChar == '\n' || CurrentChar == '\r') {
                lineNo++;
                columnNo = 1;
            }
            CurrentChar = getChar();
        }
    }

    // 整数或浮点数
    if (isdigit(CurrentChar) || CurrentChar == '.') { // 有可能是数值
        std::string NumberString;

        if (CurrentChar == '.') {                 // 浮点数: .[0-9]+
            do {
                NumberString += CurrentChar;
                CurrentChar = getChar();
            } while (isdigit(CurrentChar));
            put_backChar();
            return TOKEN{TK_FLOAT_LITERAL, NumberString, lineNo, columnNo};
        } else {
            do { // 数值的开头: [0-9]+
                NumberString += CurrentChar;
                CurrentChar = getChar();
            } while (isdigit(CurrentChar));

            if (CurrentChar == '.' ) {
                if (isdigit(getChar())){
                    put_backChar();
                    // 若出现小数点，是浮点数: [0-9]+.[0-9]+
                    do {
                        NumberString += CurrentChar;
                        CurrentChar = getChar();
                    } while (isdigit(CurrentChar));
                    put_backChar();
                    return TOKEN{TK_FLOAT_LITERAL, NumberString, lineNo, columnNo};
                }else{
                    print_line(lineNo , columnNo - 1, "数值格式错误，小数点后不是数字");
                    exit(1);
                }
            }else{                  // 若未出现小数点，是整数: [0-9]+
                put_backChar();
                return TOKEN{TK_INT_LITERAL, NumberString, lineNo, columnNo};
            }
        }
    }

    // 标识符或关键字
    if (isalpha(CurrentChar) || CurrentChar == '_') { // 标识符以字母或_开头
        std::string IdentifierString;
        do {
            IdentifierString += CurrentChar;
            CurrentChar = getChar();
        } while (isalnum(CurrentChar) || CurrentChar == '_');
        put_backChar();

        // 1.关键字
        if (IdentifierString == "int")
            return TOKEN{TK_INT, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "float")
            return TOKEN{TK_FLOAT, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "bool")
            return TOKEN{TK_BOOL, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "void")
            return TOKEN{TK_VOID, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "char")
            return TOKEN{TK_CHAR, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "true" || IdentifierString == "false")
            return TOKEN{TK_BOOL_LITERAL, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "return")
            return TOKEN{TK_RETURN, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "if" )
            return TOKEN{TK_IF, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "else")
            return TOKEN{TK_ELSE, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "while")
            return TOKEN{TK_WHILE, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "for")
            return TOKEN{TK_FOR, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "break")
            return TOKEN{TK_BREAK, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "continue")
            return TOKEN{TK_CONTINUE, IdentifierString, lineNo, columnNo};
        if (IdentifierString == "print")
            return TOKEN{TK_PRINT, IdentifierString, lineNo, columnNo};

        // 2.普通标识符
        return TOKEN{TK_IDENTIFIER, IdentifierString, lineNo, columnNo};
    }

    // 运算符或分割符
    switch (CurrentChar) {
        case '+': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_PLUS, s, lineNo, columnNo};
        }
        case '-': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_MINUS, s, lineNo, columnNo};
        }
        case '*': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_MULorDEREF, s, lineNo, columnNo};
        }
        case '/': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_DIV, s, lineNo, columnNo};
        }
        case '(': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_LPAREN, s, lineNo, columnNo};
        }
        case ')': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_RPAREN, s, lineNo, columnNo};
        }
        case '{': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_LBRACE, s, lineNo, columnNo};
        }
        case '}': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_RBRACE, s, lineNo, columnNo};
        }
        case '[': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_LBRACK, s, lineNo, columnNo};
        }
        case ']': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_RBRACK, s, lineNo, columnNo};
        }
        case '&': {
            std::string s(1, CurrentChar);
            CurrentChar = getChar();
            if (CurrentChar == '&') {
                s += CurrentChar;
                return TOKEN{TK_AND, s, lineNo, columnNo};
            } else put_backChar();
            return TOKEN{TK_ADDRESS, s, lineNo, columnNo};
        }
        case '|': {
            std::string s(1, CurrentChar);
            CurrentChar = getChar();
            if (CurrentChar == '|') {
                s += CurrentChar;
                return TOKEN{TK_OR, s, lineNo, columnNo};
            } else put_backChar();
            break;
        }
        case '=': {
            std::string s(1, CurrentChar);
            CurrentChar = getChar();
            if (CurrentChar == '=') {
                s += CurrentChar;
                return TOKEN{TK_EQ, s, lineNo, columnNo};
            } else put_backChar();
            return TOKEN{TK_ASSIGN, s, lineNo, columnNo};
        }
        case '!': {
            std::string s(1, CurrentChar);
            CurrentChar = getChar();
            if (CurrentChar == '=') {
                s += CurrentChar;
                return TOKEN{TK_NE, s, lineNo, columnNo};
            } else put_backChar();
            return TOKEN{TK_NOT, s, lineNo, columnNo};
        }
        case '<': {
            std::string s(1, CurrentChar);
            CurrentChar = getChar();
            if (CurrentChar == '=') {
                s += CurrentChar;
                return TOKEN{TK_LE, s, lineNo, columnNo};
            } else put_backChar();
            return TOKEN{TK_LT, s, lineNo, columnNo};
        }
        case '>': {
            std::string s(1, CurrentChar);
            CurrentChar = getChar();
            if (CurrentChar == '=') {
                s += CurrentChar;
                return TOKEN{TK_GE, s, lineNo, columnNo};
            } else put_backChar();
            return TOKEN{TK_GT, s, lineNo, columnNo};
        }
        case ',': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_COMMA, s, lineNo, columnNo};
        }
        case ';': {
            std::string s(1, CurrentChar);
            return TOKEN{TK_SEMICOLON, s, lineNo, columnNo};
        }
        case '\'': {                                                                        // char character ref. https://github.com/rui314/chibicc/blob/aa0accc75e9358d313fef0a6d4005103e2ce25f5/tokenize.c
            if ((CurrentChar = getChar()) == '\0')
                print_line(lineNo, columnNo, "unclosed char literal");
            char c;
            if (CurrentChar == '\\') {
                CurrentChar = getChar();
                switch (CurrentChar) {
                    case 'a':
                        c = '\a';
                        break;
                    case 'b':
                        c = '\b';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case 'n':
                        c = '\n';
                        break;
                    case 'v':
                        c = '\v';
                        break;
                    case 'f':
                        c = '\f';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                    default:
                        c = CurrentChar;
                }
            } else c = CurrentChar;

            if ((CurrentChar = getChar()) != '\'')
                print_line(lineNo, columnNo, "unclosed char literal");
            std::string s(1, c);
            return TOKEN{TK_CHAR_LITERAL, s, lineNo, columnNo};
        }
        default:
            break;
    }
    // 检查是否是字符流结尾：
    // 1.若是，则返回EOF.
    if (input_string.length() <= pos) {
        return TOKEN{TK_EOF, "EOF"};
    }
    // 2.若不是，则报错
    fprintf(stderr, "%c is an invalid token\n", CurrentChar);
    exit(1);
}

// 持续调用getNextToken()，获得整个token流，存于tokenList
std::vector<TOKEN> Lexer::constructTokenStream() {
    TOKEN currentToken;
    do {
        currentToken = getNextToken();
        tokenList.push_back(currentToken);
    } while (currentToken.type != TK_EOF);
    return std::move(tokenList);
}






int main(int argc, char *argv[]) {
    if (argc == 2) {
        /* source code can be read from one of two places:
         * 1. standard input terminal. As in test.sh, it's represented by "-".
         *    This approach is convenient for testing a large number of cases
         *    by running "make test" command.
         * 2. a file. This approach is convenient for debugging a single test case.
        */
        // for convenience, source code will be read into a string
        // ref: https://www.zhihu.com/question/426117879/answer/2618969836?utm_source=qq&utm_medium=social&utm_oi=867698515231522816


        std::ostringstream out;  // this var is auxiliary
        if (strcmp(argv[1], "-") == 0) {  // "-" is exactly the string in argv[1]
            out << std::cin.rdbuf();
        } else {   // argv[1] is the name of test file
            std::ifstream fin(argv[1]);
            out << fin.rdbuf();
            fin.close();
        }
        input_string = out.str(); // with the help of out, source code is read into a string
    } else {
        std::cout << "Usage: ./code input_source_code missing.\n";
        return 1;
    }


// 词法分析.
    Lexer lexer;
    // 打印所有的tokens
    for (auto &currentToken: lexer.constructTokenStream()) {
        //if()
        fprintf(stderr, "%s : type %d\n", currentToken.lexeme.c_str(), currentToken.type);
    }
}