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


//===----------------------------------------------------------------------===//
// Symbols, Symboltable
//===----------------------------------------------------------------------===//

class Symbol {
public:
    std::string name;
    Type type;

    Symbol(std::string sym_name, Type sym_type) :
            name(std::move(sym_name)), type(sym_type) {}

    // 为了使用std::dynamic_pointer_cast，此virtual析构函数不可少
    virtual ~Symbol() = default;
};

class Variable_Symbol : public Symbol {
public:
    int offset;  // 针对局部变量，包括数组
    Type baseType;

    Variable_Symbol(std::string sym_name, Type sym_type, Type sym_base_type, int sym_offset) :
            Symbol(std::move(sym_name), sym_type), baseType(sym_base_type), offset(sym_offset) {}
};

class Function_Symbol : public Symbol {
public:
    Function_Symbol(std::string sym_name, Type sym_type) :
            Symbol(std::move(sym_name), sym_type) {}
};

// 作用域 = ScopedSymbolTable
class ScopedSymbolTable {
public:
    std::string name;      // 作用域名称
    std::string category;  // 如：全局作用域、函数作用域、block作用域等

    // map是基于红黑树的数据结构，也可以用基于哈希表的unordered_map
    std::map<std::string, std::shared_ptr<Symbol>> symbols;
    int level;
    std::shared_ptr<ScopedSymbolTable> enclosing_scope;
    std::vector<std::shared_ptr<ScopedSymbolTable>> sub_scopes;

    ScopedSymbolTable(std::map<std::string, std::shared_ptr<Symbol>> syms,
                      int scope_level,
                      std::shared_ptr<ScopedSymbolTable> parent_scope
    ) : symbols(std::move(syms)),
        level(scope_level),
        enclosing_scope(std::move(parent_scope)) {}

    void insert(std::shared_ptr<Symbol> symbol) {
        symbols[symbol->name] = std::move(symbol);
    }

    std::shared_ptr<Symbol> lookup(std::string &sym_name, bool current_scope_only) {
        auto i = symbols.find(sym_name);
        if (i != symbols.end()) {
            // 注意：此处不能用std::move(i->second),否则首次找到该符号表项后，会引起删除
            return i->second;
        } else if (!current_scope_only && enclosing_scope != nullptr)
            return enclosing_scope->lookup(sym_name, false);
        else return nullptr;
    }
};

std::shared_ptr<ScopedSymbolTable> global_scope;

//===----------------------------------------------------------------------===//
// AST nodes and abstract visitor
//===----------------------------------------------------------------------===//

// Visitor类会引用各个AST结点类，故在此声明，其定义紧跟Visitor类定义之后
class NumLiteral_AST_Node;

class Assignment_AST_Node;

class Return_AST_Node;

class BinaryOperator_AST_Node;

class UnaryOperator_AST_Node;//一元运算

class Variable_AST_Node;

class BaseType_AST_Node;

class Block_AST_Node;

class If_AST_Node;

class While_AST_Node;

class For_AST_Node;

class Break_AST_Node;

class Continue_AST_Node;

class Print_AST_Node;

class SingleVariableDeclaration_AST_Node;

class VariableDeclarations_AST_Node;

class Parameter_AST_Node;

class FunctionDeclaration_AST_Node;

class FunctionCall_AST_Node;

class Program_AST_Node;

// Visitor类定义. (Visitor类是SemanticVisitor和CodeGenerator的基类，这两个派生类的定义在各个AST_Node派生类的定义之后)
class Visitor {
public:
    virtual void visit(NumLiteral_AST_Node &node) = 0;

    virtual void visit(Assignment_AST_Node &node) = 0;

    virtual void visit(Return_AST_Node &node) = 0;

    virtual void visit(BinaryOperator_AST_Node &node) = 0;

    virtual void visit(UnaryOperator_AST_Node &node) = 0;

    virtual void visit(Variable_AST_Node &node) = 0;

    virtual void visit(BaseType_AST_Node &node) = 0;

    virtual void visit(Block_AST_Node &node) = 0;

    virtual void visit(If_AST_Node &node) = 0;

    virtual void visit(While_AST_Node &node) = 0;

    virtual void visit(For_AST_Node &node) = 0;

    virtual void visit(Break_AST_Node &node) = 0;

    virtual void visit(Continue_AST_Node &node) = 0;

    virtual void visit(Print_AST_Node &node) = 0;

    virtual void visit(SingleVariableDeclaration_AST_Node &node) = 0;

    virtual void visit(Parameter_AST_Node &node) = 0;

    virtual void visit(VariableDeclarations_AST_Node &node) = 0;

    virtual void visit(FunctionDeclaration_AST_Node &node) = 0;

    virtual void visit(FunctionCall_AST_Node &node) = 0;

    virtual void visit(Program_AST_Node &node) = 0;
};

// 注意：各种AST_Node的定义中，若出现TOKEN是不太合适的
/// AST_Node - 各个AST_Node派生类的基类.
class AST_Node {
public:
    // 为某些结点(比如变量、表达式等对应的结点)设置数据类型属性，将在Visitor访问该结点时的相关处理带来方便.
    // 当然，这些属性信息也可以保存在符号表中，至于如何选择看喜好.
    Type type = Type_void;
    Type baseType = Type_void;

    virtual ~AST_Node() = default;

    virtual void accept(Visitor &v) = 0;
};

/// NumLiteral_AST_Node - 整数、浮点数、布尔常量的字面量
class NumLiteral_AST_Node : public AST_Node {
public:
    std::string literal;

    explicit NumLiteral_AST_Node(TOKEN &tok) : literal(tok.lexeme) {
        if (tok.type == TK_INT_LITERAL) type = Type_int;
        else if (tok.type == TK_FLOAT_LITERAL) type = Type_float;
        else if (tok.type == TK_BOOL_LITERAL) type = Type_bool;
        else if (tok.type == TK_CHAR_LITERAL) type = Type_char;
    }

    void accept(Visitor &v) override { v.visit(*this); }
};

/// 赋值语句对应的结点
class Assignment_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> left;
    std::string operation; // 此处必然是“=”
    std::shared_ptr<AST_Node> right;

    Assignment_AST_Node(std::shared_ptr<AST_Node> leftvalue, std::string op, std::shared_ptr<AST_Node> rightvalue)
            : left(std::move(leftvalue)), operation(std::move(op)), right(std::move(rightvalue)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// return语句对应的结点
class Return_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> expression; // 返回值表达式
    std::string functionName;             // 是哪个函数体里面的return？

    explicit Return_AST_Node(std::shared_ptr<AST_Node> expr) :
            expression(std::move(expr)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// if语句对应的结点
class If_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> condition;
    std::shared_ptr<AST_Node> then_statement;
    std::shared_ptr<AST_Node> else_statement;

    If_AST_Node(std::shared_ptr<AST_Node> con, std::shared_ptr<AST_Node> then_stmt, std::shared_ptr<AST_Node> else_stmt)
            : condition(std::move(con)), then_statement(std::move(then_stmt)), else_statement(std::move(else_stmt)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// while语句对应的结点
class While_AST_Node : public AST_Node {
public:

    std::shared_ptr<AST_Node> condition;
    std::shared_ptr<AST_Node> statement;

    While_AST_Node(std::shared_ptr<AST_Node> con, std::shared_ptr<AST_Node> stmt)
            : condition(std::move(con)), statement(std::move(stmt)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// for语句对应的结点
class For_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> initialization;  // for (expression? ;;)
    std::shared_ptr<AST_Node> condition;       // for (;expression;)
    std::shared_ptr<AST_Node> increment;       // for (;;expression)
    std::shared_ptr<AST_Node> statement;       // for () statement

    For_AST_Node(std::shared_ptr<AST_Node> init, std::shared_ptr<AST_Node> con, std::shared_ptr<AST_Node> inc,
                 std::shared_ptr<AST_Node> stmt)
            : initialization(std::move(init)), condition(std::move(con)), increment(std::move(inc)),
              statement(std::move(stmt)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// break语句对应的结点
class Break_AST_Node : public AST_Node {
public:
    std::string break_label;

    void accept(Visitor &v) override { v.visit(*this); }
};

/// continue语句对应的结点
class Continue_AST_Node : public AST_Node {
public:
    std::string continue_label;

    void accept(Visitor &v) override { v.visit(*this); }
};

/// print语句对应的结点
class Print_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> value;
    explicit Print_AST_Node(std::shared_ptr<AST_Node> v) : value(std::move(v)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// 二元运算对应的结点
class BinaryOperator_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> left;
    TOKEN_TYPE op;
    std::shared_ptr<AST_Node> right;

    BinaryOperator_AST_Node(std::shared_ptr<AST_Node> Left, TOKEN_TYPE op, std::shared_ptr<AST_Node> Right)
            : left(std::move(Left)), op(op), right(std::move(Right)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// 单个变量声明对应的结点
class SingleVariableDeclaration_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> type; // type node
    std::shared_ptr<AST_Node> var;  // variable node
    std::vector<std::shared_ptr<AST_Node>> inits; // assignment nodes
    SingleVariableDeclaration_AST_Node(std::shared_ptr<AST_Node> typeNode, std::shared_ptr<AST_Node> varNode,
                                       std::vector<std::shared_ptr<AST_Node>> assignNodes)
            : type(std::move(typeNode)), var(std::move(varNode)), inits(std::move(assignNodes)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// 变量声明语句对应的结点，注意：一个变量声明语句可能同时声明多个变量
class VariableDeclarations_AST_Node : public AST_Node {
public:
    std::vector<std::shared_ptr<AST_Node>> varDeclarations;

    explicit VariableDeclarations_AST_Node(std::vector<std::shared_ptr<AST_Node>> newVarDeclarations)
            : varDeclarations(std::move(newVarDeclarations)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// 一元运算对应的结点
class UnaryOperator_AST_Node : public AST_Node {
public:
    TOKEN_TYPE op;
    std::shared_ptr<AST_Node> right;

    UnaryOperator_AST_Node(TOKEN_TYPE op, std::shared_ptr<AST_Node> Right)
            : op(op), right(std::move(Right)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

/// 变量声明时的类型或函数定义中返回值的类型所对应的结点
class BaseType_AST_Node : public AST_Node {
public:

    void accept(Visitor &v) override { v.visit(*this); }
};

/// 变量结点，变量声明和每次变量引用时都对应单独的变量结点，但对应的符号表项只有一个
class Variable_AST_Node : public AST_Node {
public:
    std::string var_name;
    // 针对数组
    int length = -1;      // 数组长度(声明数组变量时用到)
    std::shared_ptr<AST_Node> indexExpression = nullptr; // 数组单个元素下标表达式

    std::shared_ptr<Variable_Symbol> symbol;

    explicit Variable_AST_Node(std::string var) : var_name(std::move(var)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

class Parameter_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> parameterType;  // 参数类型结点
    std::shared_ptr<AST_Node> parameterVar;   // 参数名称结点
    std::shared_ptr<Variable_Symbol> symbol;

    Parameter_AST_Node(std::shared_ptr<AST_Node> typeNode, std::shared_ptr<AST_Node> varNode)
            : parameterType(std::move(typeNode)), parameterVar(std::move(varNode)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

class Block_AST_Node : public AST_Node {
public:
    std::vector<std::shared_ptr<AST_Node>> statements;

    explicit Block_AST_Node(std::vector<std::shared_ptr<AST_Node>> newStatements)
            : statements(std::move(newStatements)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

class FunctionDeclaration_AST_Node : public AST_Node {
public:
    std::shared_ptr<AST_Node> funcType;   // 函数返回值类型结点
    std::string funcName;                 // 函数名
    std::vector<std::shared_ptr<AST_Node>> formalParams; // 形参列表，即formal parameters
    std::shared_ptr<AST_Node> funcBlock;  // 函数体

    int offset = -1;                       // 偏移量

    FunctionDeclaration_AST_Node(std::shared_ptr<AST_Node> t, std::string n, std::vector<std::shared_ptr<AST_Node>> &&p,
                                 std::shared_ptr<AST_Node> b) :
            funcType(std::move(t)), funcName(std::move(n)), formalParams(p), funcBlock(std::move(b)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

class FunctionCall_AST_Node : public AST_Node {
public:
    std::string funcName;  // 函数名
    std::vector<std::shared_ptr<AST_Node>> arguments; // 实参列表，即arguments

    FunctionCall_AST_Node(std::string name, std::vector<std::shared_ptr<AST_Node>> args) :
            funcName(std::move(name)), arguments(std::move(args)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};

class Program_AST_Node : public AST_Node {
public:
    // 全局变量List
    std::vector<std::shared_ptr<VariableDeclarations_AST_Node>> gvarDeclarationsList;
    // 函数定义List
    std::vector<std::shared_ptr<FunctionDeclaration_AST_Node>> funcDeclarationList;

    Program_AST_Node(std::vector<std::shared_ptr<VariableDeclarations_AST_Node>> newVarDeclList,
                     std::vector<std::shared_ptr<FunctionDeclaration_AST_Node>> newFuncDeclList) :
            gvarDeclarationsList(std::move(newVarDeclList)), funcDeclarationList(std::move(newFuncDeclList)) {}

    void accept(Visitor &v) override { v.visit(*this); }
};
//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

// 如果想在.data段列出所有float常量，则用之，否则删掉就行。
std::vector<std::shared_ptr<NumLiteral_AST_Node>> floats;

class Parser {
    Lexer lexer;
    TOKEN CurrentToken;
    std::vector<TOKEN> tokenList;
    long pos = 0; // Token流指针
public:
    explicit Parser(Lexer lex) : lexer(std::move(lex)) {
        tokenList = std::move(lexer.constructTokenStream());
        CurrentToken = tokenList[pos++];
    }

    std::shared_ptr<AST_Node> parse();

private:
    void eatCurrentToken() { CurrentToken = tokenList[pos++]; };

    void put_backToken() { CurrentToken = tokenList[--pos - 1]; }

    std::shared_ptr<AST_Node> primary();

    std::shared_ptr<AST_Node> unary();

    std::shared_ptr<AST_Node> mul_div();

    std::shared_ptr<AST_Node> add_sub();

    std::shared_ptr<AST_Node> relational();

    std::shared_ptr<AST_Node> equality();

    std::shared_ptr<AST_Node> expression();

    std::shared_ptr<AST_Node> expression_statement();

    std::shared_ptr<AST_Node> block();

    std::shared_ptr<AST_Node> declarator();

    std::shared_ptr<AST_Node> type_specification();

    std::shared_ptr<AST_Node> variable_declaration();

    std::shared_ptr<AST_Node> statement();

    std::shared_ptr<AST_Node> formal_parameter();

    std::shared_ptr<AST_Node> function_declaration();

    std::shared_ptr<AST_Node> program();
};

// primary := num_literal
//              | "(" expression ")"
//              | identifier func_args?
//              | identifier "[" expression "]"
// func_args = "(" (expression ("," expression)*)? ")"
std::shared_ptr<AST_Node> Parser::primary() {
    // num_literal
    if (CurrentToken.type == TK_INT_LITERAL ||
        CurrentToken.type == TK_FLOAT_LITERAL ||
        CurrentToken.type == TK_BOOL_LITERAL ||
        CurrentToken.type == TK_CHAR_LITERAL) {
        auto node = std::make_shared<NumLiteral_AST_Node>(CurrentToken);
        eatCurrentToken();
        return node;
    }

    // "(" expression ")"
    if (CurrentToken.type == TK_LPAREN) {
        eatCurrentToken(); // eat "("
        auto node = std::move(expression());
        if (CurrentToken.type == TK_RPAREN)
            eatCurrentToken(); // eat ")"
        return node;
    }

    // identifier func_args?
    //            | "[" expression "]"
    // func_args = "(" (expression ("," expression)*)? ")"
    if (CurrentToken.type == TK_IDENTIFIER) {
        std::string name = CurrentToken.lexeme;  // 函数或变量名称
        eatCurrentToken();
        // 函数调用
        if (CurrentToken.type == TK_LPAREN) {
            eatCurrentToken();  // eat "("
            std::string functionName = name;
            std::vector<std::shared_ptr<AST_Node>> actualParamNodes;
            while (CurrentToken.type != TK_RPAREN) {
                auto paramNode = expression();
                actualParamNodes.push_back(paramNode);
                if (CurrentToken.type == TK_COMMA) eatCurrentToken(); // eat ","
            }
            eatCurrentToken();   // eat ")"
            return std::make_shared<FunctionCall_AST_Node>(name, actualParamNodes);
        }

        // 变量
        auto variableNode = std::make_shared<Variable_AST_Node>(name);
        // 变量1：数组元素
        if (CurrentToken.type == TK_LBRACK) {
            eatCurrentToken();  // eat "["
            variableNode->indexExpression = expression();
            variableNode->type = Type_array;
            eatCurrentToken();   // eat "]"
        }
        // 变量2：基本变量
        return variableNode;
    }

    // error!
    {
        fprintf(stderr, "error!:  %s\n", CurrentToken.lexeme.c_str());
        exit(1);
    }

}

// unary :=	(“+” | “-” | “!” | “*” | “&”) unary | primary
std::shared_ptr<AST_Node> Parser::unary() {
    if (CurrentToken.type == TK_PLUS ||
        CurrentToken.type == TK_MINUS ||
        CurrentToken.type == TK_NOT ||
        CurrentToken.type == TK_MULorDEREF ||
        CurrentToken.type == TK_ADDRESS) {
        TOKEN tok = CurrentToken;
        eatCurrentToken();
        return std::make_shared<UnaryOperator_AST_Node>(tok.type, std::move(unary()));
    } else return primary();
}


// mul_div := unary ("*" unary | "/" unary)*
std::shared_ptr<AST_Node> Parser::mul_div() {
    std::shared_ptr<AST_Node> node = unary();
    while (true) {
        if (CurrentToken.type == TK_MULorDEREF || CurrentToken.type == TK_DIV) {
            TOKEN op_token = CurrentToken;
            eatCurrentToken();
            auto left = std::move(node);
            node = std::make_shared<BinaryOperator_AST_Node>(std::move(left), op_token.type,
                                                             std::move(unary()));
            continue;
        }
        return node;
    }
}

// add_sub  :=  mul_div ("+" mul_div | "-" mul_div)*
std::shared_ptr<AST_Node> Parser::add_sub() {
    auto node = mul_div();
    while (true) {
        if (CurrentToken.type == TK_PLUS || CurrentToken.type == TK_MINUS) {
            TOKEN op_token = CurrentToken;
            eatCurrentToken();
            auto left = std::move(node);
            node = std::make_shared<BinaryOperator_AST_Node>(std::move(left), op_token.type,
                                                             std::move(mul_div()));
            continue;
        }
        return node;
    }
}

// relational  :=  add_sub ("<" add_sub | "<=" add_sub | ">" add_sub | ">=" add_sub)*
std::shared_ptr<AST_Node> Parser::relational() {
    auto node = add_sub();
    while (true) {
        if (CurrentToken.type == TK_LT ||
            CurrentToken.type == TK_LE ||
            CurrentToken.type == TK_GT ||
            CurrentToken.type == TK_GE) {
            TOKEN op_token = CurrentToken;
            eatCurrentToken();
            auto left = std::move(node);
            node = std::make_shared<BinaryOperator_AST_Node>(std::move(left), op_token.type,
                                                             std::move(add_sub()));
            continue;
        }
        return node;
    }
}

// equality  :=  relational ("==" relational | "!=" relational)*
std::shared_ptr<AST_Node> Parser::equality() {
    auto node = relational();
    while (true) {
        if (CurrentToken.type == TK_EQ || CurrentToken.type == TK_NE) {
            TOKEN op_token = CurrentToken;
            eatCurrentToken();
            auto left = std::move(node);
            node = std::make_shared<BinaryOperator_AST_Node>(std::move(left), op_token.type,
                                                             std::move(relational()));
            continue;
        }
        return node;
    }
}

// expression  :=  equality ("=" expression)?
std::shared_ptr<AST_Node> Parser::expression() {
    auto node = equality();
    if (CurrentToken.type == TK_ASSIGN) {
        TOKEN assign_token = CurrentToken;
        eatCurrentToken();
        auto left = std::move(node);
        node = std::make_shared<Assignment_AST_Node>(std::move(left), std::move(assign_token.lexeme),
                                                     std::move(expression()));
    }
    return node;
}

// expression_statement  :=  expression? ";"
std::shared_ptr<AST_Node> Parser::expression_statement() {
    std::shared_ptr<AST_Node> node = nullptr;
    if (CurrentToken.type == TK_SEMICOLON) eatCurrentToken();
    else {
        node = expression();
        if (CurrentToken.type == TK_SEMICOLON) eatCurrentToken();
        else {
            fprintf(stderr, "error:  %s\n", CurrentToken.lexeme.c_str());
            exit(1);
        }
    }
    return node;
}

// declarator  := identifier type-suffix
// type-suffix  :=  ϵ | ("[" num_literal "]")?
std::shared_ptr<AST_Node> Parser::declarator() {
    if (CurrentToken.type == TK_IDENTIFIER) {
        auto node = std::make_shared<Variable_AST_Node>(std::move(CurrentToken.lexeme));
        eatCurrentToken();
        if (CurrentToken.type == TK_LBRACK) {
            eatCurrentToken(); // eat "["
            node->length = std::stoi(CurrentToken.lexeme);
            node->type = Type_array;
            eatCurrentToken(); // eat length
            eatCurrentToken(); // eat "]"
        }
        return node;
    }
    return nullptr;
}

// type_specification := "int" | "float" | "bool" | "void" | "char"
std::shared_ptr<AST_Node> Parser::type_specification() {
    auto node = std::make_shared<BaseType_AST_Node>();
    switch (CurrentToken.type) {
        case TK_INT:
            node->type = Type_int;
            break;
        case TK_FLOAT:
            node->type = Type_float;
            break;
        case TK_CHAR:
            node->type = Type_char;
            break;
        case TK_VOID:
            node->type = Type_void;
            break;
        case TK_BOOL:
            node->type = Type_bool;
            break;
        default:
            break;
    }
    eatCurrentToken();
    return node;
}

// variable_declaration	 :=  type_specification declarator ("=" expression)? ("," declarator ("=" expression)?)* ";"
//                         | type_specification declarator ("=" "{" (expression)? ("," expression)* "}")? ("," declarator ("=" expression)?)* ";"
std::shared_ptr<AST_Node> Parser::variable_declaration() {
    std::vector<std::shared_ptr<AST_Node>> VarDeclarations;
    std::shared_ptr<BaseType_AST_Node> basetypeNode = std::dynamic_pointer_cast<BaseType_AST_Node>(
            type_specification());
    while (CurrentToken.type != TK_SEMICOLON) {
        auto variableNode = std::dynamic_pointer_cast<Variable_AST_Node>(declarator());
        // 学习chibicc的做法，将变量初始化视为一个(或多个，对数组)赋值语句
        std::vector<std::shared_ptr<AST_Node>> initNodes;
        if (CurrentToken.type == TK_ASSIGN) {
            eatCurrentToken(); // eat "="
            if (CurrentToken.type == TK_LBRACE) { // 数组初始化
                eatCurrentToken(); // eat "{"
                int i = 0;   // 数组的下标从0开始
                while (CurrentToken.type != TK_RBRACE) {
                    // newVarNode是赋值语句左边的数组元素对应的变量
                    auto newVarNode = std::make_shared<Variable_AST_Node>(variableNode->var_name);
                    newVarNode->type = Type_array;
                    newVarNode->baseType = basetypeNode->type;
                    // 构建一个与数组元素下标对应的表达式，即primary := identifier "[" expression "]"中的expression.
                    TOKEN tok = TOKEN{TK_INT_LITERAL, std::to_string(i), 0, 0};
                    auto indexNode = std::make_shared<NumLiteral_AST_Node>(tok);
                    indexNode->type = Type_int;
                    newVarNode->indexExpression = indexNode;

                    initNodes.push_back(
                            std::make_shared<Assignment_AST_Node>(std::move(newVarNode), "=", std::move(expression())));
                    if (CurrentToken.type == TK_COMMA) eatCurrentToken(); // eat ","
                    i++;
                }
                eatCurrentToken(); // eat "}"
            } else {                              // 普通变量，即基本类型变量初始化
                initNodes.push_back(std::make_shared<Assignment_AST_Node>(variableNode, "=", std::move(expression())));
            }
        }
        auto node = std::make_shared<SingleVariableDeclaration_AST_Node>(basetypeNode, variableNode,
                                                                         std::move(initNodes));
        VarDeclarations.push_back(node);
        if (CurrentToken.type == TK_COMMA) eatCurrentToken(); // eat ","
    }
    eatCurrentToken();  // eat ";"
    return std::make_shared<VariableDeclarations_AST_Node>(VarDeclarations);
}

// statement  := expression_statement
//                   | block
//                   | variable_declaration
//                   | "return" expression-statement
//                   | "if" "(" expression ")" statement ("else" statement)?
//                   | "while" "(" expression ")" statement
//                   | for" "(" expression? ";" expression ";" expression? ")" statement
//                   | "break" ";"
//                   | "continue" ";"
//                   | "print" "(" expression ")" ";"
std::shared_ptr<AST_Node> Parser::statement() {
    // block
    if (CurrentToken.type == TK_LBRACE) return block();

    // variable_declaration
    if (CurrentToken.type == TK_INT ||
        CurrentToken.type == TK_FLOAT ||
        CurrentToken.type == TK_BOOL ||
        CurrentToken.type == TK_CHAR)
        return variable_declaration();

    // "return" expression-statement
    if (CurrentToken.type == TK_RETURN) {
        eatCurrentToken();  // eat "return"
        return std::make_shared<Return_AST_Node>(std::move(expression_statement()));
    }

    // "if" "(" expression ")" statement ("else" statement)?
    if (CurrentToken.type == TK_IF) {
        eatCurrentToken(); // eat "if"
        std::shared_ptr<AST_Node> condition, then_stmt, else_stmt;
        if (CurrentToken.type == TK_LPAREN) {
            eatCurrentToken(); // eat "("
            condition = expression();
            if (CurrentToken.type == TK_RPAREN)
                eatCurrentToken(); // eat ")"
        }
        then_stmt = statement();
        if (CurrentToken.type == TK_ELSE) {
            eatCurrentToken();  // eat "else"
            else_stmt = statement();
        }
        return std::make_shared<If_AST_Node>(condition, then_stmt, else_stmt);
    }

    // "while" "(" expression ")" statement
    if (CurrentToken.type == TK_WHILE) {
        eatCurrentToken(); // eat "while"
        std::shared_ptr<AST_Node> condition, stmt;
        if (CurrentToken.type == TK_LPAREN) {
            eatCurrentToken(); // eat "("
            condition = expression();
            if (CurrentToken.type == TK_RPAREN)
                eatCurrentToken(); // eat ")"
        }
        stmt = statement();
        return std::make_shared<While_AST_Node>(condition, stmt);
    }

    // "for" "(" expression? ";" expression ";" expression? ")" statement
    if (CurrentToken.type == TK_FOR) {
        eatCurrentToken(); // eat "for"
        std::shared_ptr<AST_Node> initialization, condition, increment, stmt;
        if (CurrentToken.type == TK_LPAREN) {
            eatCurrentToken(); // eat "("
            if (CurrentToken.type != TK_SEMICOLON)
                initialization = expression();
            eatCurrentToken(); // eat ";"
            condition = expression();
            eatCurrentToken(); // eat ";"
            if (CurrentToken.type != TK_RPAREN)
                increment = expression();
            eatCurrentToken(); // eat ")"
        }
        stmt = statement();
        return std::make_shared<For_AST_Node>(initialization, condition, increment, stmt);
    }

    // "break" ";"
    if (CurrentToken.type == TK_BREAK) {
        eatCurrentToken(); // eat "break"
        auto breakNode = std::make_shared<Break_AST_Node>();
        eatCurrentToken(); // eat ";"
        return breakNode;
    }

    // "continue" ";"
    if (CurrentToken.type == TK_CONTINUE) {
        eatCurrentToken(); // eat "continue"
        auto continueNode = std::make_shared<Continue_AST_Node>();
        eatCurrentToken(); // eat ";"
        return continueNode;
    }

    //                   | "print" "(" expression ")" ";"
    if (CurrentToken.type == TK_PRINT) {
        eatCurrentToken(); // eat "print"
        eatCurrentToken(); // eat "("
        auto valueNode = expression();
        eatCurrentToken(); // eat ")"
        eatCurrentToken(); // eat ";"
        return std::make_shared<Print_AST_Node>(valueNode);
    }

    // expression_statement
    return expression_statement();
}

// block  := "{" statement* "}"
std::shared_ptr<AST_Node> Parser::block() {
    std::vector<std::shared_ptr<AST_Node>> newStatements;
    eatCurrentToken();   // eat "{"
    // construct newStatements
    while (CurrentToken.type != TK_RBRACE && CurrentToken.type != TK_EOF) {
        std::shared_ptr<AST_Node> newStatement = statement();
        if (newStatement != nullptr)
            newStatements.push_back(std::move(newStatement));
    }
    std::shared_ptr<AST_Node> node = std::make_shared<Block_AST_Node>(std::move(newStatements));
    eatCurrentToken();   // eat "}"
    return node;
}

// formal_parameter  :=  type_specification identifier
std::shared_ptr<AST_Node> Parser::formal_parameter() {
    std::shared_ptr<BaseType_AST_Node> typeNode = std::dynamic_pointer_cast<BaseType_AST_Node>(type_specification());
    if (CurrentToken.type == TK_IDENTIFIER) {
        auto varNode = std::make_shared<Variable_AST_Node>(std::move(CurrentToken.lexeme));
        eatCurrentToken();  // eat parameter name
        auto node = std::make_shared<Parameter_AST_Node>(typeNode, varNode);
        return node;
    } else return nullptr;

}

// function_declaration  :=  type_specification identifier "(" formal_parameters? ")" block
// formal_parameters  :=  formal_parameter ("," formal_parameter)*
std::shared_ptr<AST_Node> Parser::function_declaration() {
    // 函数类型
    std::shared_ptr<BaseType_AST_Node> typeNode = std::dynamic_pointer_cast<BaseType_AST_Node>(type_specification());

    // 函数名称
    std::string funcName;
    if (CurrentToken.type == TK_IDENTIFIER) {
        funcName = CurrentToken.lexeme;
        eatCurrentToken();
    }
    // 函数形参列表
    if (CurrentToken.type == TK_LPAREN) eatCurrentToken(); // eat "("
    else {
        fprintf(stderr, "missing (");
        exit(1);
    }
    std::vector<std::shared_ptr<AST_Node>> formalParams;
    if (CurrentToken.type != TK_RPAREN) {
        while (CurrentToken.type != TK_COMMA && CurrentToken.type != TK_RPAREN) {
            auto newFormalParam = formal_parameter();
            formalParams.push_back(newFormalParam);
            if (CurrentToken.type == TK_COMMA) eatCurrentToken();
        }
    }
    if (CurrentToken.type == TK_RPAREN) eatCurrentToken(); // eat ")"
    // 函数body
    auto body = block();

    return std::make_shared<FunctionDeclaration_AST_Node>(std::move(typeNode), std::move(funcName),
                                                          std::move(formalParams), std::move(body));
}

// program  :=  (variable_declaration | function_declaration)*
std::shared_ptr<AST_Node> Parser::program() {
    std::vector<std::shared_ptr<VariableDeclarations_AST_Node>> variableDeclarationsList;
    std::vector<std::shared_ptr<FunctionDeclaration_AST_Node>> functionDeclarationList;
    while (CurrentToken.type != TK_EOF) {
        if (CurrentToken.type == TK_INT || CurrentToken.type == TK_FLOAT ||
            CurrentToken.type == TK_BOOL || CurrentToken.type == TK_VOID ||
            CurrentToken.type == TK_CHAR) {
            eatCurrentToken();
            if (CurrentToken.type == TK_IDENTIFIER) {
                eatCurrentToken();
                if (CurrentToken.type == TK_LPAREN) {
                    put_backToken();
                    put_backToken();
                    auto newFunction = std::dynamic_pointer_cast<FunctionDeclaration_AST_Node>(function_declaration());
                    functionDeclarationList.push_back(newFunction);
                } else {
                    put_backToken();
                    put_backToken();
                    auto newVariableDeclarations = std::dynamic_pointer_cast<VariableDeclarations_AST_Node>(
                            variable_declaration());
                    variableDeclarationsList.push_back(newVariableDeclarations);
                }
            }
        }
    }
    return std::make_shared<Program_AST_Node>(std::move(variableDeclarationsList), std::move(functionDeclarationList));
}

std::shared_ptr<AST_Node> Parser::parse() {
    return program();
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
        //if(currentToken.type == 5){
            fprintf(stderr, "%s : type %d\n", currentToken.lexeme.c_str(), currentToken.type);
        }
    std::ofstream outfile;
    outfile.open("token.txt");
    for(int i=0;i < lexer.tokenList.size();i++){
        outfile << i << "  " <<lexer.tokenList[i].lexeme + " : "<<"type"<< lexer.tokenList[i].type<< std::endl;
    }
    // 语法分析.
    Parser parser(lexer);
    std::shared_ptr<AST_Node> tree = parser.parse();


}