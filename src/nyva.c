#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

typedef enum {
    TARGET_ORTA,
    TARGET_JS,
} Target;

Target current_target = TARGET_ORTA;

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_PARSE,
    TOKEN_EQUAL,       // =
    TOKEN_PLUS,        // +
    TOKEN_MINUS,       // -
    TOKEN_MULTIPLY,    // *
    TOKEN_DIVIDE,      // /
    TOKEN_GT,          // >
    TOKEN_LT,          // <
    TOKEN_EQ,          // ==
    TOKEN_NEQ,         // !=
    TOKEN_LPAREN,      // (
    TOKEN_RPAREN,      // )
    TOKEN_LBRACE,      // {
    TOKEN_RBRACE,      // }
    TOKEN_COMMA,       // ,
    TOKEN_SEMICOLON,   // ;
    TOKEN_COLON,       // :
    TOKEN_AND,         // &
    TOKEN_BACKSLASH,   // \

    TOKEN_FN,
    TOKEN_RETURN,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_AT,          // @
    TOKEN_IMPORT,
    TOKEN_BREAK,
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

typedef struct {
    char *input;
    size_t input_len;
    int pos;
    int line;
    int column;
    char current_char;
} Lexer;

typedef struct {
    Token *tokens;
    int count;
    int position;
} Parser;

typedef enum {
    NODE_PROGRAM,
    NODE_VAR_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_IF_STATEMENT,
    NODE_PARSE_STATEMENT,
    NODE_BINARY_EXPRESSION,
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_STRING,
    NODE_FUNCTION_CALL,
    NODE_ARGUMENT_LIST,
    NODE_FUNCTION_DEFINITION,
    NODE_PARAMETER_LIST,
    NODE_RETURN_STATEMENT,
    NODE_FOR_STATEMENT,
    NODE_IMPORT,
    NODE_BREAK_STATEMENT,
    NODE_WHILE_STATEMENT,
} NodeType;

typedef struct ASTNode {
    NodeType type;
    union {
        struct {
            char *name;
            struct ASTNode *value;
        } var_declaration;

        struct {
            char *name;
            struct ASTNode *value;
        } assignment;

        struct {
            struct ASTNode *condition;
            struct ASTNode **body;
            int body_count;
            struct ASTNode **else_body;
            int else_body_count;
        } if_statement;

        struct {
            struct ASTNode *args;
        } parse_statement;

        struct {
            struct ASTNode *left;
            TokenType operator;
            struct ASTNode *right;
        } binary_expression;

        struct {
            char *name;
        } identifier;

        struct {
            int value;
        } number;

        struct {
            char *value;
        } string;

        struct {
            char *name;
            struct ASTNode *args;
        } function_call;

        struct {
            struct ASTNode **args;
            int count;
        } argument_list;

        struct {
            struct ASTNode **statements;
            int count;
        } program;

        struct {
            char *name;
            struct ASTNode *params;
            struct ASTNode **body;
            int body_count;
        } function_definition;

        struct {
            struct ASTNode **params;
            int count;
        } parameter_list;

        struct {
            struct ASTNode *value;
        } return_statement;

        struct {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *update;
            struct ASTNode **body;
            int body_count;
        } for_statement;
        struct {
            char *file;
        } import_statement;
        struct {
            struct ASTNode *condition;
            struct ASTNode **body;
            int body_count;
        } while_statement;

    } data;
} ASTNode;

typedef struct {
    FILE *out;
    int label_counter;
} CodeGenerator;

typedef struct {
    char *name;
    TokenType token;
} Keyword;

Keyword keywords[] = {
    {"var", TOKEN_VAR},
    {"const", TOKEN_VAR},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"parse", TOKEN_PARSE},
    {"fn", TOKEN_FN},
    {"return", TOKEN_RETURN},
    {"for", TOKEN_FOR},
    {"import", TOKEN_IMPORT},
    {"break", TOKEN_BREAK},
    {"while", TOKEN_WHILE},
    {NULL, 0}
};

typedef struct {
    char **files;
    int count;
} ImportTracker;

ImportTracker imported_files = {NULL, 0};

int is_file_imported(const char *filename) {
    for (int i = 0; i < imported_files.count; i++) {
        if (strcmp(imported_files.files[i], filename) == 0) {
            return 1;
        }
    }
    return 0;
}

void add_imported_file(const char *filename) {
    if (imported_files.files == NULL) {
        imported_files.files = malloc(sizeof(char*) * 64);
    }
    imported_files.files[imported_files.count++] = strdup(filename);
}

char* escape_string(const char* src) {
    if (src == NULL) {
        return NULL;
    }

    size_t src_len = strlen(src);

    char* dest = (char*)malloc(src_len * 2 + 1);
    if (dest == NULL) {
        return NULL;
    }

    size_t j = 0;
    for (size_t i = 0; i < src_len; i++) {
        switch (src[i]) {
            case '\\':
                dest[j++] = '\\';
                dest[j++] = '\\';
                break;
            case '"':
                dest[j++] = '\\';
                dest[j++] = '"';
                break;
            case '\n':
                dest[j++] = '\\';
                dest[j++] = 'n';
                break;
            case '\r':
                dest[j++] = '\\';
                dest[j++] = 'r';
                break;
            case '\t':
                dest[j++] = '\\';
                dest[j++] = 't';
                break;
            default:
                dest[j++] = src[i];
                break;
        }
    }

    dest[j] = '\0';

    char* result = (char*)realloc(dest, j + 1);
    return result == NULL ? dest : result;
}

void lexer_error(Lexer *lexer, char *msg, ...) {
    va_list args;
    va_start(args, msg);

    fprintf(stderr, "%d:%d ERROR: ", lexer->line, lexer->column);
    vfprintf(stderr, msg, args);
    va_end(args);
    exit(1);
}


void lexer_advance(Lexer *lexer) {
    if (lexer->pos < lexer->input_len) {
        lexer->current_char = lexer->input[lexer->pos++];
        lexer->column++;
        if (lexer->current_char == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
    } else {
        lexer->current_char = '\0';
    }
}

char lexer_get_previous_char(Lexer *lexer) {
    if (lexer->pos > 0) {
        return lexer->input[lexer->pos - 1];
    }
    return '\0';
}

void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->current_char != '\0' && isspace(lexer->current_char)) {
        lexer_advance(lexer);
    }
}

char *lexer_collect_identifier(Lexer *lexer) {
    char *buffer = malloc(256);
    int i = 0;

    if (lexer->current_char == '@') {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }

    while (lexer->current_char != '\0' && (isalnum(lexer->current_char) || lexer->current_char == '_')) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    buffer[i] = '\0';
    return buffer;
}

char *lexer_collect_number(Lexer *lexer) {
    char *buffer = malloc(256);
    int i = 0;

    if (lexer->current_char == '-') {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }

    while (lexer->current_char != '\0' && isdigit(lexer->current_char)) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    buffer[i] = '\0';
    return buffer;
}

char *lexer_collect_string(Lexer *lexer) {
    char *buffer = malloc(256);
    int i = 0;

    lexer_advance(lexer);

    bool escaped = false;

    while (lexer->current_char != '\0' && (escaped || lexer->current_char != '"')) {
        if (escaped) {
            switch (lexer->current_char) {
                case 'n': buffer[i++] = '\n'; break;
                case 't': buffer[i++] = '\t'; break;
                case 'r': buffer[i++] = '\r'; break;
                case '\\': buffer[i++] = '\\'; break;
                case '"': buffer[i++] = '\\'; buffer[i++] = '"'; break;
                default: buffer[i++] = lexer->current_char; break;
            }
            escaped = false;
        } else if (lexer->current_char == '\\') {
            escaped = true;
        } else {
            buffer[i++] = lexer->current_char;
        }

        lexer_advance(lexer);
    }

    if (lexer->current_char == '"') {
        lexer_advance(lexer);
    }

    buffer[i] = '\0';
    return buffer;
}

TokenType get_keyword_token(const char *identifier) {
    for (int i = 0; keywords[i].name != NULL; i++) {
        if (strcmp(identifier, keywords[i].name) == 0) {
            return keywords[i].token;
        }
    }
    return TOKEN_IDENTIFIER;
}

Token lexer_get_next_token(Lexer *lexer) {
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;

    lexer_skip_whitespace(lexer);

    if (lexer->current_char == '-' &&
        lexer->pos < lexer->input_len &&
        isdigit(lexer->input[lexer->pos])) {
        token.type = TOKEN_NUMBER;
        token.value = lexer_collect_number(lexer);
        return token;
    }

    if (lexer->current_char == '\0') {
        token.type = TOKEN_EOF;
        token.value = NULL;
        return token;
    }

    if (lexer->current_char == '@') {
        char *identifier = lexer_collect_identifier(lexer);
        token.type = TOKEN_IDENTIFIER;
        token.value = identifier;
        return token;
    }

    if (isalpha(lexer->current_char) || lexer->current_char == '_') {
        char *identifier = lexer_collect_identifier(lexer);
        token.type = get_keyword_token(identifier);
        token.value = identifier;
        return token;
    }

    if (isdigit(lexer->current_char)) {
        token.type = TOKEN_NUMBER;
        token.value = lexer_collect_number(lexer);
        return token;
    }

    if (lexer->current_char == '"') {
        token.type = TOKEN_STRING;
        token.value = lexer_collect_string(lexer);
        return token;
    }

    switch (lexer->current_char) {
        case '=':
            lexer_advance(lexer);
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                token.type = TOKEN_EQ;
                token.value = strdup("==");
            } else {
                token.type = TOKEN_EQUAL;
                token.value = strdup("=");
            }
            return token;
        case '!':
            lexer_advance(lexer);
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                token.type = TOKEN_NEQ;
                token.value = strdup("!=");
            } else {
               lexer_error(lexer, "expected '=' got '%c'\n", lexer->current_char);
            }
            return token;
        case '+':
            lexer_advance(lexer);
            token.type = TOKEN_PLUS;
            token.value = strdup("+");
            return token;

        case '-':
            lexer_advance(lexer);
            token.type = TOKEN_MINUS;
            token.value = strdup("-");
            return token;

        case '*':
            lexer_advance(lexer);
            token.type = TOKEN_MULTIPLY;
            token.value = strdup("*");
            return token;

        case '/':
            lexer_advance(lexer);
            token.type = TOKEN_DIVIDE;
            token.value = strdup("/");
            return token;

        case '>':
            lexer_advance(lexer);
            token.type = TOKEN_GT;
            token.value = strdup(">");
            return token;

        case '<':
            lexer_advance(lexer);
            token.type = TOKEN_LT;
            token.value = strdup("<");
            return token;

        case '(':
            lexer_advance(lexer);
            token.type = TOKEN_LPAREN;
            token.value = strdup("(");
            return token;

        case ')':
            lexer_advance(lexer);
            token.type = TOKEN_RPAREN;
            token.value = strdup(")");
            return token;

        case '{':
            lexer_advance(lexer);
            token.type = TOKEN_LBRACE;
            token.value = strdup("{");
            return token;

        case '}':
            lexer_advance(lexer);
            token.type = TOKEN_RBRACE;
            token.value = strdup("}");
            return token;

        case ',':
            lexer_advance(lexer);
            token.type = TOKEN_COMMA;
            token.value = strdup(",");
            return token;
        case ';':
            lexer_advance(lexer);
            token.type = TOKEN_SEMICOLON;
            token.value = strdup(";");
            return token;
        case ':':
            lexer_advance(lexer);
            token.type = TOKEN_COLON;
            token.value = strdup(":");
            return token;
        case '@':
            lexer_advance(lexer);
            token.type = TOKEN_AT;
            token.value = strdup("@");
            return token;
        case '&':
            lexer_advance(lexer);
            token.type = TOKEN_AND;
            token.value = strdup("&");
        case '\\':
            lexer_advance(lexer);
            token.type = TOKEN_BACKSLASH;
            token.value = strdup("\\");
        default:
            lexer_error(lexer, "Unexpected character: %c\n",
                    lexer->current_char);
            exit(1);
    }
}

Token *tokenize(const char *input) {
    Lexer lexer = {
        .input = strdup(input),
        .input_len = strlen(input),
        .pos = 0,
        .line = 1,
        .column = 0
    };
    lexer_advance(&lexer);

    Token *tokens = malloc(sizeof(Token) * 1024);
    int count = 0;

    Token token;
    do {
        token = lexer_get_next_token(&lexer);
        tokens[count++] = token;
    } while (token.type != TOKEN_EOF);

    free(lexer.input);
    return tokens;
}

Parser parser_create(Token *tokens) {
    Parser parser = {
        .tokens = tokens,
        .position = 0,
        .count = 0
    };

    while (tokens[parser.count].type != TOKEN_EOF) {
        parser.count++;
    }
    parser.count++;

    return parser;
}

Token parser_current_token(Parser *parser) {
    return parser->tokens[parser->position];
}

void parser_advance(Parser *parser) {
    if (parser->position < parser->count) {
        parser->position++;
    }
}

bool parser_expect(Parser *parser, TokenType type) {
    if (parser_current_token(parser).type == type) {
        parser_advance(parser);
        return true;
    }

    Token token = parser_current_token(parser);
    fprintf(stderr, "Expected token type %d but got %d with value '%s' at line %d, column %d\n",
            type, token.type, token.value, token.line, token.column);
    exit(1);
}

ASTNode *parser_parse_expression(Parser *parser);

ASTNode *parser_parse_argument_list(Parser *parser) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_ARGUMENT_LIST;
    node->data.argument_list.args = malloc(sizeof(ASTNode*) * 64);
    node->data.argument_list.count = 0;

    if (parser_current_token(parser).type != TOKEN_RPAREN) {
        node->data.argument_list.args[node->data.argument_list.count++] = parser_parse_expression(parser);

        while (parser_current_token(parser).type == TOKEN_COMMA) {
            parser_advance(parser);
            node->data.argument_list.args[node->data.argument_list.count++] = parser_parse_expression(parser);
        }
    }

    return node;
}

ASTNode *parser_parse_function_call(Parser *parser, char *name) {
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *args = parser_parse_argument_list(parser);

    parser_expect(parser, TOKEN_RPAREN);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_FUNCTION_CALL;
    node->data.function_call.name = name;
    node->data.function_call.args = args;

    return node;
}

ASTNode *parser_parse_factor(Parser *parser) {
    Token token = parser_current_token(parser);

    if (token.type == TOKEN_NUMBER) {
        parser_advance(parser);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_NUMBER;
        node->data.number.value = atoi(token.value);
        return node;
    } else if (token.type == TOKEN_STRING) {
        parser_advance(parser);
        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_STRING;
        node->data.string.value = strdup(token.value);
        return node;
    } else if (token.type == TOKEN_IDENTIFIER) {
        parser_advance(parser);

        if (parser_current_token(parser).type == TOKEN_LPAREN) {
            return parser_parse_function_call(parser, strdup(token.value));
        } else {
            ASTNode *node = malloc(sizeof(ASTNode));
            node->type = NODE_IDENTIFIER;
            node->data.identifier.name = strdup(token.value);
            return node;
        }
    } else if (token.type == TOKEN_LPAREN) {
        parser_advance(parser);
        ASTNode *node = parser_parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return node;
    } else {
        fprintf(stderr, "Unexpected token: %s\n", token.value);
        exit(1);
    }
}

ASTNode *parser_parse_term(Parser *parser) {
    ASTNode *left = parser_parse_factor(parser);

    while (parser_current_token(parser).type == TOKEN_MULTIPLY ||
           parser_current_token(parser).type == TOKEN_DIVIDE) {
        TokenType op_type = parser_current_token(parser).type;
        parser_advance(parser);

        ASTNode *right = parser_parse_factor(parser);

        ASTNode *new_node = malloc(sizeof(ASTNode));
        new_node->type = NODE_BINARY_EXPRESSION;
        new_node->data.binary_expression.left = left;
        new_node->data.binary_expression.operator = op_type;
        new_node->data.binary_expression.right = right;

        left = new_node;
    }

    return left;
}

ASTNode *parser_parse_expression(Parser *parser) {
    ASTNode *left = parser_parse_term(parser);

    while (parser_current_token(parser).type == TOKEN_PLUS ||
           parser_current_token(parser).type == TOKEN_MINUS) {
        TokenType op_type = parser_current_token(parser).type;
        parser_advance(parser);

        ASTNode *right = parser_parse_term(parser);

        ASTNode *new_node = malloc(sizeof(ASTNode));
        new_node->type = NODE_BINARY_EXPRESSION;
        new_node->data.binary_expression.left = left;
        new_node->data.binary_expression.operator = op_type;
        new_node->data.binary_expression.right = right;

        left = new_node;
    }

    return left;
}

ASTNode *parser_parse_condition(Parser *parser) {
    ASTNode *left = parser_parse_expression(parser);

    TokenType op_type = parser_current_token(parser).type;
    if (op_type == TOKEN_GT || op_type == TOKEN_LT || op_type == TOKEN_EQ || op_type == TOKEN_NEQ) {
        parser_advance(parser);
        ASTNode *right = parser_parse_expression(parser);

        ASTNode *node = malloc(sizeof(ASTNode));
        node->type = NODE_BINARY_EXPRESSION;
        node->data.binary_expression.left = left;
        node->data.binary_expression.operator = op_type;
        node->data.binary_expression.right = right;

        return node;
    } else {
        return left;
    }
}

ASTNode *parser_parse_var_declaration(Parser *parser) {
    parser_expect(parser, TOKEN_VAR);

    Token identifier = parser_current_token(parser);
    parser_expect(parser, TOKEN_IDENTIFIER);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_VAR_DECLARATION;
    node->data.var_declaration.name = strdup(identifier.value);

    if (parser_current_token(parser).type == TOKEN_EQUAL) {
        parser_advance(parser);
        node->data.var_declaration.value = parser_parse_expression(parser);
    } else {
        node->data.var_declaration.value = NULL;
    }

    return node;
}

ASTNode *parser_parse_assignment(Parser *parser) {
    Token identifier = parser_current_token(parser);
    parser_advance(parser);

    parser_expect(parser, TOKEN_EQUAL);

    ASTNode *value = parser_parse_expression(parser);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_ASSIGNMENT;
    node->data.assignment.name = strdup(identifier.value);
    node->data.assignment.value = value;

    return node;
}

ASTNode *parser_parse_statement(Parser *parser);

ASTNode *parser_parse_if_statement(Parser *parser) {
    parser_expect(parser, TOKEN_IF);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *condition = parser_parse_condition(parser);

    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_LBRACE);

    ASTNode **body = malloc(sizeof(ASTNode*) * 64);
    int body_count = 0;

    while (parser_current_token(parser).type != TOKEN_RBRACE &&
           parser_current_token(parser).type != TOKEN_EOF) {
        body[body_count++] = parser_parse_statement(parser);
    }

    parser_expect(parser, TOKEN_RBRACE);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_IF_STATEMENT;
    node->data.if_statement.condition = condition;
    node->data.if_statement.body = body;
    node->data.if_statement.body_count = body_count;
    node->data.if_statement.else_body = NULL;
    node->data.if_statement.else_body_count = 0;

    if (parser_current_token(parser).type == TOKEN_ELSE) {
        parser_advance(parser);
        parser_expect(parser, TOKEN_LBRACE);

        ASTNode **else_body = malloc(sizeof(ASTNode*) * 64);
        int else_body_count = 0;

        while (parser_current_token(parser).type != TOKEN_RBRACE &&
               parser_current_token(parser).type != TOKEN_EOF) {
            else_body[else_body_count++] = parser_parse_statement(parser);
        }

        parser_expect(parser, TOKEN_RBRACE);

        node->data.if_statement.else_body = else_body;
        node->data.if_statement.else_body_count = else_body_count;
    }

    return node;
}

ASTNode *parser_parse_parse_statement(Parser *parser) {
    parser_expect(parser, TOKEN_PARSE);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *args = parser_parse_argument_list(parser);

    parser_expect(parser, TOKEN_RPAREN);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_PARSE_STATEMENT;
    node->data.parse_statement.args = args;

    return node;
}

ASTNode *parser_parse_parameter_list(Parser *parser) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_PARAMETER_LIST;
    node->data.parameter_list.params = malloc(sizeof(ASTNode*) * 64);
    node->data.parameter_list.count = 0;

    if (parser_current_token(parser).type != TOKEN_RPAREN) {
        Token param = parser_current_token(parser);
        parser_expect(parser, TOKEN_IDENTIFIER);

        ASTNode *param_node = malloc(sizeof(ASTNode));
        param_node->type = NODE_IDENTIFIER;
        param_node->data.identifier.name = strdup(param.value);

        node->data.parameter_list.params[node->data.parameter_list.count++] = param_node;

        while (parser_current_token(parser).type == TOKEN_COMMA) {
            parser_advance(parser);

            param = parser_current_token(parser);
            parser_expect(parser, TOKEN_IDENTIFIER);

            param_node = malloc(sizeof(ASTNode));
            param_node->type = NODE_IDENTIFIER;
            param_node->data.identifier.name = strdup(param.value);

            node->data.parameter_list.params[node->data.parameter_list.count++] = param_node;
        }
    }

    return node;
}

ASTNode *parser_parse_function_definition(Parser *parser) {
    parser_expect(parser, TOKEN_FN);

    Token name = parser_current_token(parser);
    parser_expect(parser, TOKEN_IDENTIFIER);

    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *params = parser_parse_parameter_list(parser);
    parser_expect(parser, TOKEN_RPAREN);

    parser_expect(parser, TOKEN_LBRACE);

    ASTNode **body = malloc(sizeof(ASTNode*) * 256);
    int body_count = 0;

    while (parser_current_token(parser).type != TOKEN_RBRACE &&
           parser_current_token(parser).type != TOKEN_EOF) {
        body[body_count++] = parser_parse_statement(parser);
    }

    parser_expect(parser, TOKEN_RBRACE);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_FUNCTION_DEFINITION;
    node->data.function_definition.name = strdup(name.value);
    node->data.function_definition.params = params;
    node->data.function_definition.body = body;
    node->data.function_definition.body_count = body_count;

    return node;
}

ASTNode *parser_parse_return_statement(Parser *parser) {
    parser_expect(parser, TOKEN_RETURN);

    ASTNode *value = NULL;
    if (parser_current_token(parser).type != TOKEN_RBRACE) {
        value = parser_parse_expression(parser);
    }

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_RETURN_STATEMENT;
    node->data.return_statement.value = value;

    return node;
}

ASTNode *parser_parse_for_statement(Parser *parser) {
    parser_expect(parser, TOKEN_FOR);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *init = NULL;
    if (parser_current_token(parser).type == TOKEN_VAR) {
        init = parser_parse_var_declaration(parser);
    } else if (parser_current_token(parser).type == TOKEN_IDENTIFIER) {
        init = parser_parse_assignment(parser);
    } else {
        fprintf(stderr, "Expected var declaration or assignment in for loop initialization\n");
        exit(1);
    }
    parser_expect(parser, TOKEN_SEMICOLON);

    ASTNode *condition = parser_parse_condition(parser);

    parser_expect(parser, TOKEN_SEMICOLON);

    ASTNode *update = parser_parse_assignment(parser);

    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_LBRACE);

    ASTNode **body = malloc(sizeof(ASTNode*) * 64);
    int body_count = 0;

    while (parser_current_token(parser).type != TOKEN_RBRACE &&
           parser_current_token(parser).type != TOKEN_EOF) {
        body[body_count++] = parser_parse_statement(parser);
    }

    parser_expect(parser, TOKEN_RBRACE);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_FOR_STATEMENT;
    node->data.for_statement.init = init;
    node->data.for_statement.condition = condition;
    node->data.for_statement.update = update;
    node->data.for_statement.body = body;
    node->data.for_statement.body_count = body_count;

    return node;
}

ASTNode *parser_parse_while_statement(Parser *parser) {
    parser_expect(parser, TOKEN_WHILE);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *condition = parser_parse_condition(parser);

    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_LBRACE);

    ASTNode **body = malloc(sizeof(ASTNode*) * 64);
    int body_count = 0;

    while (parser_current_token(parser).type != TOKEN_RBRACE &&
           parser_current_token(parser).type != TOKEN_EOF) {
        body[body_count++] = parser_parse_statement(parser);
    }

    parser_expect(parser, TOKEN_RBRACE);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_WHILE_STATEMENT;
    node->data.while_statement.condition = condition;
    node->data.while_statement.body = body;
    node->data.while_statement.body_count = body_count;

    return node;
}


ASTNode *parser_parse_import_statement(Parser *parser) {
    parser_expect(parser, TOKEN_IMPORT);
    parser_expect(parser, TOKEN_LPAREN);
    char *file = strdup(parser_current_token(parser).value);
    parser_expect(parser, TOKEN_STRING);
    parser_expect(parser, TOKEN_RPAREN);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_IMPORT;
    node->data.import_statement.file = file;

    return node;
}

ASTNode *parser_parse_break_statement(Parser *parser) {
    parser_expect(parser, TOKEN_BREAK);

    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_BREAK_STATEMENT;

    return node;
}

ASTNode *parser_parse_statement(Parser *parser) {
    Token token = parser_current_token(parser);

    if (token.type == TOKEN_VAR) {
        return parser_parse_var_declaration(parser);
    } else if (token.type == TOKEN_IF) {
        return parser_parse_if_statement(parser);
    } else if (token.type == TOKEN_PARSE) {
        return parser_parse_parse_statement(parser);
    } else if (token.type == TOKEN_FN) {
        return parser_parse_function_definition(parser);
    } else if (token.type == TOKEN_RETURN) {
        return parser_parse_return_statement(parser);
    } else if (token.type == TOKEN_FOR) {
        return parser_parse_for_statement(parser);
    } else if (token.type == TOKEN_WHILE) {
        return parser_parse_while_statement(parser);
    } else if (token.type == TOKEN_IDENTIFIER) {
        if (parser->position + 1 < parser->count &&
            parser->tokens[parser->position + 1].type == TOKEN_EQUAL) {
            return parser_parse_assignment(parser);
        } else {
            ASTNode *expr = parser_parse_expression(parser);
            return expr;
        }
    } else if (token.type == TOKEN_IMPORT) {
      return parser_parse_import_statement(parser);
    } else if (token.type == TOKEN_BREAK) {
        return parser_parse_break_statement(parser);
    } else {
        fprintf(stderr, "Unexpected token in statement: %s at line %d, column %d\n",
                token.value, token.line, token.column);
        exit(1);
    }
}

ASTNode *parser_parse_program(Parser *parser) {
    ASTNode **statements = malloc(sizeof(ASTNode*) * 1024);
    int count = 0;

    while (parser_current_token(parser).type != TOKEN_EOF) {
        statements[count++] = parser_parse_statement(parser);
    }

    ASTNode *program = malloc(sizeof(ASTNode));
    program->type = NODE_PROGRAM;
    program->data.program.statements = statements;
    program->data.program.count = count;

    return program;
}

ASTNode *parse(Token *tokens) {
    Parser parser = parser_create(tokens);
    return parser_parse_program(&parser);
}

void codegen_emit(CodeGenerator *gen, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(gen->out, fmt, args);
    if (current_target == TARGET_ORTA) fprintf(gen->out, "\n");
    va_end(args);
}

char *codegen_create_label(CodeGenerator *gen, const char *prefix) {
    char *label = malloc(64);
    sprintf(label, "__%s_%d", prefix, gen->label_counter++);
    return label;
}

char* itoa(int value) {
    int base = 10;
    if (value == 0) {
        char* result = (char*)malloc(2);
        if (result) {
            result[0] = '0';
            result[1] = '\0';
        }
        return result;
    }

    int isNegative = 0;
    if (base == 10 && value < 0) {
        isNegative = 1;
        value = -value;
    }

    int len = 0;
    int temp = value;
    while (temp) {
        len++;
        temp /= base;
    }

    int resultLength = len + (isNegative && base == 10 ? 1 : 0) + 1;
    char* result = (char*)malloc(resultLength);
    if (!result) {
        return NULL;
    }

    result[resultLength - 1] = '\0';

    int i = resultLength - 2;
    while (value) {
        int digit = value % base;
        result[i--] = (digit < 10) ? ('0' + digit) : ('a' + (digit - 10));
        value /= base;
    }

    if (isNegative) {
        result[0] = '-';
    }

    return result;
}

void codegen_generate_expression_orta(CodeGenerator *gen, ASTNode *node) {
    if (node->type == NODE_NUMBER) {
        codegen_emit(gen, "push %d", node->data.number.value);
    } else if (node->type == NODE_STRING) {
        codegen_emit(gen, "push \"%s\"", node->data.string.value);
    } else if (node->type == NODE_IDENTIFIER) {
        codegen_emit(gen, "getvar %s", node->data.identifier.name);
    } else if (node->type == NODE_BINARY_EXPRESSION) {
        codegen_generate_expression_orta(gen, node->data.binary_expression.left);
        codegen_generate_expression_orta(gen, node->data.binary_expression.right);

        switch (node->data.binary_expression.operator) {
            case TOKEN_PLUS:
                if (node->data.binary_expression.left->type == NODE_STRING && node->data.binary_expression.right->type == NODE_STRING
                    || node->data.binary_expression.right->type == NODE_IDENTIFIER && node->data.binary_expression.left->type == NODE_IDENTIFIER
                    || node->data.binary_expression.right->type == NODE_STRING && node->data.binary_expression.left->type == NODE_IDENTIFIER
                    || node->data.binary_expression.right->type == NODE_IDENTIFIER && node->data.binary_expression.left->type == NODE_STRING)
                    codegen_emit(gen, "merge");
                else
                    codegen_emit(gen, "add");
                break;
            case TOKEN_MINUS:
                codegen_emit(gen, "sub");
                break;
            case TOKEN_MULTIPLY:
                codegen_emit(gen, "mul");
                break;
            case TOKEN_DIVIDE:
                codegen_emit(gen, "div");
                break;
            case TOKEN_GT:
                codegen_emit(gen, "gt");
                break;
            case TOKEN_LT:
                codegen_emit(gen, "lt");
                break;
            case TOKEN_EQ:
                codegen_emit(gen, "eq");
                codegen_emit(gen, "not");
                break;
            case TOKEN_NEQ:
                codegen_emit(gen, "eq");
                break;
            default:
                fprintf(stderr, "Unknown operator in code generation\n");
                exit(1);
        }
    } else if (node->type == NODE_FUNCTION_CALL) {
        char *func_name = node->data.function_call.name;
        if (strcmp(func_name, "print") == 0) {
            if (node->data.function_call.args->data.argument_list.count == 0) {
                codegen_emit(gen, "print");
            } else {
                for (int i = 0; i < node->data.function_call.args->data.argument_list.count; i++) {
                   codegen_generate_expression_orta(gen, node->data.function_call.args->data.argument_list.args[i]);
                }
                codegen_emit(gen, "print");
            }
        } else if (strcmp(func_name, "@inline") == 0) {
            for (int i = 0; i < node->data.function_call.args->data.argument_list.count; i++) codegen_emit(gen, node->data.function_call.args->data.argument_list.args[i]->data.string.value);
        } else {
            codegen_emit(gen, "; ------- Function call %s -------", func_name);
            for (int i = 0; i < node->data.function_call.args->data.argument_list.count; i++) {
                codegen_generate_expression_orta(gen, node->data.function_call.args->data.argument_list.args[i]);
            }
            codegen_emit(gen, "push %d ; args count", node->data.function_call.args->data.argument_list.count);
            codegen_emit(gen, "call %s", func_name);
            codegen_emit(gen, "; ------- Function call %s end -------", func_name);
        }
    }
}

void codegen_generate_expression_js(CodeGenerator *gen, ASTNode *node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_NUMBER:
            codegen_emit(gen, "%d", node->data.number.value);
            break;
        case NODE_STRING: {
            char *escaped = escape_string(node->data.string.value);
            codegen_emit(gen, "\"%s\"", escaped);
            free(escaped);
            break;
        }
        case NODE_IDENTIFIER:
            codegen_emit(gen, "%s", node->data.identifier.name);
            break;
        case NODE_BINARY_EXPRESSION: {
            codegen_emit(gen, "(");
            codegen_generate_expression_js(gen, node->data.binary_expression.left);
            const char *op;
            switch (node->data.binary_expression.operator) {
                case TOKEN_PLUS: op = "+"; break;
                case TOKEN_MINUS: op = "-"; break;
                case TOKEN_MULTIPLY: op = "*"; break;
                case TOKEN_DIVIDE: op = "/"; break;
                case TOKEN_GT: op = ">"; break;
                case TOKEN_LT: op = "<"; break;
                case TOKEN_EQ: op = "==="; break;
                case TOKEN_NEQ: op = "!=="; break;
                default:
                    fprintf(stderr, "Unsupported operator in JS codegen: %d\n",
                            node->data.binary_expression.operator);
                    exit(1);
            }
            codegen_emit(gen, " %s ", op);
            codegen_generate_expression_js(gen, node->data.binary_expression.right);
            codegen_emit(gen, ")");
            break;
        }
        case NODE_FUNCTION_CALL: {
            char *func_name = node->data.function_call.name;
            if (strcmp(func_name, "print") == 0) {
                codegen_emit(gen, "console.log(");
                for (int i = 0; i < node->data.function_call.args->data.argument_list.count; i++) {
                    if (i > 0) codegen_emit(gen, ", ");
                    codegen_generate_expression_js(gen, node->data.function_call.args->data.argument_list.args[i]);
                }
                codegen_emit(gen, ")\n");
            } else if (strcmp(func_name, "@inline") == 0) {
                for (int i = 0; i < node->data.function_call.args->data.argument_list.count; i++) {
                    codegen_emit(gen, node->data.function_call.args->data.argument_list.args[i]->data.string.value);
                    codegen_emit(gen, "\n");
                }
            } else {
                codegen_emit(gen, "%s(", func_name);
                for (int i = 0; i < node->data.function_call.args->data.argument_list.count; i++) {
                    if (i > 0) codegen_emit(gen, ", ");
                    codegen_generate_expression_js(gen, node->data.function_call.args->data.argument_list.args[i]);
                }
                codegen_emit(gen, ")");
            }
            break;
        }
        default:
            fprintf(stderr, "Unsupported node type in JS expression: %d\n", node->type);
            exit(1);
    }
}

void codegen_generate_expression(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_expression_orta(gen, node);
    } else {
        codegen_generate_expression_js(gen, node);
    } 
}

void codegen_generate_var_declaration_orta(CodeGenerator *gen, ASTNode *node) {
    if (node->data.var_declaration.value != NULL) {
        codegen_generate_expression(gen, node->data.var_declaration.value);
        codegen_emit(gen, "setvar %s", node->data.var_declaration.name);
    } else {
        codegen_emit(gen, "var %s", node->data.var_declaration.name);
    }
}

void codegen_generate_var_declaration_js(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "let %s", node->data.var_declaration.name);
    if (node->data.var_declaration.value != NULL) {
        codegen_emit(gen, " = ");
        codegen_generate_expression_js(gen, node->data.var_declaration.value);
    }
    codegen_emit(gen, ";\n");
}

void codegen_generate_var_declaration(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_var_declaration_orta(gen, node); 
    } else {
        codegen_generate_var_declaration_js(gen, node);
    }
}

void codegen_generate_assignment_orta(CodeGenerator *gen, ASTNode *node) {
    codegen_generate_expression(gen, node->data.assignment.value);
    codegen_emit(gen, "setvar %s", node->data.assignment.name);
}

void codegen_generate_assignment_js(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "%s = ", node->data.assignment.name);
    codegen_generate_expression_js(gen, node->data.assignment.value);
    codegen_emit(gen, ";\n");
}

void codegen_generate_assignment(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_assignment_orta(gen, node);
    } else {
        codegen_generate_assignment_js(gen, node);
    }
}

void codegen_generate_statement(CodeGenerator *gen, ASTNode *node);
void codegen_generate_statement_with_break(CodeGenerator *gen, ASTNode *node, char *break_label);

char *current_break_label = NULL;

void codegen_generate_if_statement_orta(CodeGenerator *gen, ASTNode *node) {
    char *else_label = NULL;
    char *end_label = codegen_create_label(gen, "if_end");

    codegen_generate_expression(gen, node->data.if_statement.condition);

    if (node->data.if_statement.else_body != NULL) {
        else_label = codegen_create_label(gen, "else");
        codegen_emit(gen, "jmpif %s", else_label);
    } else {
        codegen_emit(gen, "jmpif %s", end_label);
    }

    for (int i = 0; i < node->data.if_statement.body_count; i++) {
        codegen_generate_statement_with_break(gen, node->data.if_statement.body[i], current_break_label);
    }

    if (node->data.if_statement.else_body != NULL) {
        codegen_emit(gen, "jmp %s", end_label);
        codegen_emit(gen, "%s:", else_label);

        for (int i = 0; i < node->data.if_statement.else_body_count; i++) {
            codegen_generate_statement(gen, node->data.if_statement.else_body[i]);
        }

        free(else_label);
    }

    codegen_emit(gen, "%s:", end_label);
    free(end_label);
}

void codegen_generate_if_statement_js(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "if (");
    codegen_generate_expression_js(gen, node->data.if_statement.condition);
    codegen_emit(gen, ") {\n");
    for (int i = 0; i < node->data.if_statement.body_count; i++) {
        codegen_generate_statement(gen, node->data.if_statement.body[i]);
    }
    codegen_emit(gen, "}");
    if (node->data.if_statement.else_body != NULL) {
        codegen_emit(gen, " else {\n");
        for (int i = 0; i < node->data.if_statement.else_body_count; i++) {
            codegen_generate_statement(gen, node->data.if_statement.else_body[i]);
        }
        codegen_emit(gen, "}\n");
    } else {
        codegen_emit(gen, "\n");
    }
}

void codegen_generate_if_statement(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_if_statement_orta(gen, node);
    } else {
        codegen_generate_if_statement_js(gen, node);
    }
}

void codegen_generate_parse_statement_orta(CodeGenerator *gen, ASTNode *node) {
    for (int i = 0; i < node->data.parse_statement.args->data.argument_list.count; i++) {
        codegen_generate_expression(gen, node->data.parse_statement.args->data.argument_list.args[i]);
    }
    codegen_emit(gen, "eval %d", node->data.parse_statement.args->data.argument_list.count);
}

void codegen_generate_parse_statement(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_parse_statement_orta(gen, node);
    } else {
        assert(0 && "Not Implemented\n");
    }
}

void codegen_generate_parameter_list(CodeGenerator *gen, ASTNode *node) {}

char *error_not_enough_args = NULL;

void codegen_generate_function_definition_orta(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "%s:", node->data.function_definition.name);
    if (!(strcmp(node->data.function_definition.name, "__entry") == 0)) {
        codegen_emit(gen, "push %d", node->data.function_definition.params->data.parameter_list.count);
        codegen_emit(gen, "eq");
        codegen_emit(gen, "not");
        codegen_emit(gen, "jmpif %s", error_not_enough_args);
    }
    codegen_emit(gen, "togglelocalscope");
    ASTNode *params = node->data.function_definition.params;
    for (int i = params->data.parameter_list.count - 1; i >= 0; i--) {
        ASTNode *param = params->data.parameter_list.params[i];
        codegen_emit(gen, "setvar %s", param->data.identifier.name);
    }

    for (int i = 0; i < node->data.function_definition.body_count; i++) {
        codegen_generate_statement(gen, node->data.function_definition.body[i]);
    }

    codegen_emit(gen, "togglelocalscope");
    if (!(strcmp(node->data.function_definition.name, "__entry") == 0)) {
        codegen_emit(gen, "push 0");
        codegen_emit(gen, "ret");
    } else {
        codegen_emit(gen, "halt");
    }
}

void codegen_generate_function_definition_js(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "function %s(", node->data.function_definition.name);
    ASTNode *params = node->data.function_definition.params;
    for (int i = 0; i < params->data.parameter_list.count; i++) {
        if (i > 0) codegen_emit(gen, ", ");
        codegen_emit(gen, "%s", params->data.parameter_list.params[i]->data.identifier.name);
    }
    codegen_emit(gen, ") {\n");
    for (int i = 0; i < node->data.function_definition.body_count; i++) {
        codegen_generate_statement(gen, node->data.function_definition.body[i]);
    }
    codegen_emit(gen, "}\n");
}

void codegen_generate_function_definition(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_function_definition_orta(gen, node);
    } else {
        codegen_generate_function_definition_js(gen, node);
    }
}

void codegen_generate_return_statement_orta(CodeGenerator *gen, ASTNode *node) {
    if (node->data.return_statement.value != NULL) {
        codegen_generate_expression(gen, node->data.return_statement.value);
    } else {
        codegen_emit(gen, "push 0");
    }
    codegen_emit(gen, "togglelocalscope");
    codegen_emit(gen, "ret");
}

void codegen_generate_return_statement_js(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "return");
    if (node->data.return_statement.value != NULL) {
        codegen_emit(gen, " ");
        codegen_generate_expression_js(gen, node->data.return_statement.value);
    }
    codegen_emit(gen, ";\n");
}

void codegen_generate_return_statement(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_return_statement_orta(gen, node);
    } else {
        codegen_generate_return_statement_js(gen, node);
    }
}

void codegen_generate_for_statement_orta(CodeGenerator *gen, ASTNode *node) {
    char *start_label = codegen_create_label(gen, "for_start");
    char *end_label = codegen_create_label(gen, "for_end");

    codegen_generate_statement(gen, node->data.for_statement.init);

    codegen_emit(gen, "%s:", start_label);

    codegen_generate_expression(gen, node->data.for_statement.condition);
    codegen_emit(gen, "jmpif %s", end_label);

    current_break_label = end_label;
    for (int i = 0; i < node->data.for_statement.body_count; i++) {
        codegen_generate_statement_with_break(gen, node->data.for_statement.body[i], current_break_label);
    }

    codegen_generate_statement(gen, node->data.for_statement.update);

    codegen_emit(gen, "jmp %s", start_label);

    codegen_emit(gen, "%s:", end_label);

    free(start_label);
    free(end_label);
}

void codegen_generate_for_statement_js(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "for (");
    // Init
    if (node->data.for_statement.init->type == NODE_VAR_DECLARATION) {
        codegen_generate_var_declaration_js(gen, node->data.for_statement.init);
    } else if (node->data.for_statement.init->type == NODE_ASSIGNMENT) {
        codegen_generate_assignment_js(gen, node->data.for_statement.init);
    }
    codegen_emit(gen, "; ");
    codegen_generate_expression_js(gen, node->data.for_statement.condition);
    codegen_emit(gen, "; ");
    codegen_generate_expression_js(gen, node->data.for_statement.update);
    codegen_emit(gen, ") {\n");
    for (int i = 0; i < node->data.for_statement.body_count; i++) {
        codegen_generate_statement(gen, node->data.for_statement.body[i]);
    }
    codegen_emit(gen, "}\n");
}


void codegen_generate_for_statement(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_for_statement_orta(gen, node);
    } else {
        codegen_generate_for_statement_js(gen, node);
    }
}

void codegen_generate_while_statement_orta(CodeGenerator *gen, ASTNode *node) {
    char *start_label = codegen_create_label(gen, "while_start");
    char *end_label = codegen_create_label(gen, "while_end");

    codegen_emit(gen, "%s:", start_label);

    codegen_generate_expression(gen, node->data.while_statement.condition);
    codegen_emit(gen, "jmpif %s", end_label);

    current_break_label = end_label;
    for (int i = 0; i < node->data.while_statement.body_count; i++) {
        codegen_generate_statement_with_break(gen, node->data.while_statement.body[i], current_break_label);
    }

    codegen_emit(gen, "jmp %s", start_label);

    codegen_emit(gen, "%s:", end_label);

    free(start_label);
    free(end_label);
}

void codegen_generate_while_statement_js(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "while (");
    codegen_generate_expression_js(gen, node->data.while_statement.condition);
    codegen_emit(gen, ") {\n");
    for (int i = 0; i < node->data.while_statement.body_count; i++) {
        codegen_generate_statement(gen, node->data.while_statement.body[i]);
    }
    codegen_emit(gen, "}\n");
}


void codegen_generate_while_statement(CodeGenerator *gen, ASTNode *node) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_while_statement_orta(gen, node);
    } else {
        codegen_generate_while_statement_js(gen, node);
    }
}

void codegen_generate_import_statement(CodeGenerator *gen, ASTNode *node);

void codegen_generate_statement(CodeGenerator *gen, ASTNode *node) {
    switch (node->type) {
        case NODE_VAR_DECLARATION:
            codegen_generate_var_declaration(gen, node);
            break;

        case NODE_ASSIGNMENT:
            codegen_generate_assignment(gen, node);
            break;

        case NODE_IF_STATEMENT:
            codegen_generate_if_statement(gen, node);
            break;

        case NODE_PARSE_STATEMENT:
            codegen_generate_parse_statement(gen, node);
            break;

        case NODE_FUNCTION_DEFINITION:
            codegen_generate_function_definition(gen, node);
            break;

        case NODE_RETURN_STATEMENT:
            codegen_generate_return_statement(gen, node);
            break;

        case NODE_FUNCTION_CALL:
            codegen_generate_expression(gen, node);
            // codegen_emit(gen, "pop");
            break;
        case NODE_FOR_STATEMENT:
            codegen_generate_for_statement(gen, node);
            break;
        case NODE_WHILE_STATEMENT:
            codegen_generate_while_statement(gen, node);
            break;
        case NODE_IMPORT:
            codegen_generate_import_statement(gen, node);
            break;
        case NODE_BREAK_STATEMENT: break;
        default:
            fprintf(stderr, "Unknown node type in code generation: %d\n", node->type);
            exit(1);
    }
}

void codegen_generate_statement_with_break_orta(CodeGenerator *gen, ASTNode *node, char *break_label) {
    if (node->type == NODE_BREAK_STATEMENT) {
        if (break_label == NULL) {
            fprintf(stderr, "Error: 'break' statement outside of loop\n");
            exit(1);
        }
        codegen_emit(gen, "jmp %s", break_label);
    } else {
        codegen_generate_statement(gen, node);
    }
}

void codegen_generate_statement_with_break_js(CodeGenerator *gen, ASTNode *node, char *break_label) {
    if (node->type == NODE_BREAK_STATEMENT) {
        if (break_label == NULL) {
            fprintf(stderr, "Error: 'break' statement outside of loop\n");
            exit(1);
        }
        codegen_emit(gen, "break;");
    } else {
        codegen_generate_statement(gen, node);
    }
}

void codegen_generate_statement_with_break(CodeGenerator *gen, ASTNode *node, char *break_label) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_statement_with_break_orta(gen, node, break_label);
    } else {
        codegen_generate_statement_with_break_js(gen, node, break_label);
    }
}

void codegen_generate_program_orta(CodeGenerator *gen, ASTNode *program) {
    error_not_enough_args = codegen_create_label(gen, "error_not_enough_args");
    for (int i = 0; i < program->data.program.count; i++) {
        codegen_generate_statement(gen, program->data.program.statements[i]);
    }

    codegen_emit(gen, "%s:", error_not_enough_args);
    codegen_emit(gen, "push \"ERROR: not enough arguments\"");
    codegen_emit(gen, "print");
    codegen_emit(gen, "halt 1");

    free(error_not_enough_args);
}

void codegen_generate_program_js(CodeGenerator *gen, ASTNode *program) {
    for (int i = 0; i < program->data.program.count; i++) {
        codegen_generate_statement(gen, program->data.program.statements[i]);
    }
    codegen_emit(gen, "__entry();\n");
}

void codegen_generate_program(CodeGenerator *gen, ASTNode *program) {
    if (current_target == TARGET_ORTA) {
        codegen_generate_program_orta(gen, program);
    } else {
        codegen_generate_program_js(gen, program);
    }
}

void free_ast(ASTNode *node) {
    if (node == NULL) return;

    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->data.program.count; i++) {
                free_ast(node->data.program.statements[i]);
            }
            free(node->data.program.statements);
            break;

        case NODE_VAR_DECLARATION:
            free(node->data.var_declaration.name);
            free_ast(node->data.var_declaration.value);
            break;

        case NODE_ASSIGNMENT:
            free(node->data.assignment.name);
            free_ast(node->data.assignment.value);
            break;

        case NODE_IF_STATEMENT:
            free_ast(node->data.if_statement.condition);
            for (int i = 0; i < node->data.if_statement.body_count; i++) {
                free_ast(node->data.if_statement.body[i]);
            }
            free(node->data.if_statement.body);
            if (node->data.if_statement.else_body != NULL) {
                for (int i = 0; i < node->data.if_statement.else_body_count; i++) {
                    free_ast(node->data.if_statement.else_body[i]);
                }
                free(node->data.if_statement.else_body);
            }
            break;

        case NODE_PARSE_STATEMENT:
            free_ast(node->data.parse_statement.args);
            break;

        case NODE_BINARY_EXPRESSION:
            free_ast(node->data.binary_expression.left);
            free_ast(node->data.binary_expression.right);
            break;

        case NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;

        case NODE_FUNCTION_CALL:
            free(node->data.function_call.name);
            free_ast(node->data.function_call.args);
            break;

        case NODE_ARGUMENT_LIST:
            for (int i = 0; i < node->data.argument_list.count; i++) {
                free_ast(node->data.argument_list.args[i]);
            }
            free(node->data.argument_list.args);
            break;

        case NODE_NUMBER:
            break;

        case NODE_STRING:
            free(node->data.string.value);
            break;
        case NODE_FUNCTION_DEFINITION:
            free(node->data.function_definition.name);
            free_ast(node->data.function_definition.params);
            for (int i = 0; i < node->data.function_definition.body_count; i++) {
                free_ast(node->data.function_definition.body[i]);
            }
            free(node->data.function_definition.body);
            break;

        case NODE_PARAMETER_LIST:
            for (int i = 0; i < node->data.parameter_list.count; i++) {
                free_ast(node->data.parameter_list.params[i]);
            }
            free(node->data.parameter_list.params);
            break;

        case NODE_RETURN_STATEMENT:
            free_ast(node->data.return_statement.value);
            break;
        case NODE_FOR_STATEMENT:
            free_ast(node->data.for_statement.init);
            free_ast(node->data.for_statement.condition);
            free_ast(node->data.for_statement.update);
            for (int i = 0; i < node->data.for_statement.body_count; i++) {
                free_ast(node->data.for_statement.body[i]);
            }
            free(node->data.for_statement.body);
            break;
        case NODE_WHILE_STATEMENT:
            free_ast(node->data.while_statement.condition);
            for (int i = 0; i < node->data.for_statement.body_count; i++) {
                free_ast(node->data.for_statement.body[i]);
            }
            free(node->data.for_statement.body);
            break;
        case NODE_IMPORT:
            free(node->data.import_statement.file);
            break;
    }

    free(node);
}

void free_tokens(Token *tokens) {
    int i = 0;
    while (tokens[i].type != TOKEN_EOF) {
        free(tokens[i].value);
        i++;
    }
    free(tokens);
}

char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Could not open file %s\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);
    return buffer;
}

void codegen_generate_import_statement(CodeGenerator *gen, ASTNode *node) {
    char *filename = node->data.import_statement.file;

    if (filename[0] == '"' && filename[strlen(filename)-1] == '"') {
        filename = strndup(filename + 1, strlen(filename) - 2);
    }

    if (is_file_imported(filename)) {
        codegen_emit(gen, "; Skipping already imported file: %s", filename);
        return;
    }

    add_imported_file(filename);

    char *source = read_file(filename);
    if (!source) {
        fprintf(stderr, "Error: Could not import file %s\n", filename);
        exit(1);
    }

    char temp_filename[256];
    sprintf(temp_filename, "%s.tmp", filename);

    Token *tokens = tokenize(source);
    ASTNode *imported_ast = parse(tokens);

    codegen_emit(gen, "; --- Begin imported file: %s ---", filename);

    for (int i = 0; i < imported_ast->data.program.count; i++) {
        ASTNode *stmt = imported_ast->data.program.statements[i];

        if (stmt->type == NODE_FUNCTION_DEFINITION &&
            strcmp(stmt->data.function_definition.name, "__entry") == 0) {
            continue;
        }

        codegen_generate_statement(gen, stmt);
    }

    codegen_emit(gen, "; --- End imported file: %s ---", filename);

    free_ast(imported_ast);
    free_tokens(tokens);
    free(source);
}

char* preprocess(const char* source) {
    if (source == NULL) {
        return NULL;
    }

    size_t source_len = strlen(source);
    char* result = malloc(source_len + 1);
    if (result == NULL) {
        return NULL;
    }

    size_t i = 0;
    size_t j = 0;

    while (i < source_len) {
        if (source[i] == '/' && i + 1 < source_len && source[i + 1] == '/') {
            while (i < source_len && source[i] != '\n') {
                i++;
            }
            if (i < source_len && source[i] == '\n') {
                result[j++] = source[i++];
            }
            continue;
        }

        if (i + 1 < source_len &&
            source[i] == '-' &&
            source[i + 1] == '>') {

            i += 2;

            while (i < source_len && isspace(source[i])) {
                i++;
            }

            while (i < source_len &&
                  (isalnum(source[i]) || source[i] == '_' ||
                   source[i] == '<' || source[i] == '>' ||
                   source[i] == '|' || source[i] == '[' ||
                   source[i] == ']' || source[i] == '.')) {
                i++;
            }

            continue;
        }

        if (source[i] == ':' && i + 1 < source_len && isspace(source[i + 1])) {
            size_t temp_i = i + 1;

            while (temp_i < source_len && isspace(source[temp_i])) {
                temp_i++;
            }

            if (temp_i < source_len &&
                (isalnum(source[temp_i]) || source[temp_i] == '_' ||
                 source[temp_i] == '<' || source[temp_i] == '>')) {

                i = temp_i;

                while (i < source_len &&
                      (isalnum(source[i]) || source[i] == '_' ||
                       source[i] == '<' || source[i] == '>' ||
                       source[i] == '|' || source[i] == '[' ||
                       source[i] == ']' || source[i] == '.')) {
                    i++;
                }

                continue;
            }
        }

        result[j++] = source[i++];
    }

    result[j] = '\0';

    char* final_result = realloc(result, j + 1);
    return final_result ? final_result : result;
}

bool is_lib = false;

void compile(const char *input_file, const char *output_file) {
    char *source = preprocess(read_file(input_file));

    Token *tokens = tokenize(source);
    ASTNode *ast = parse(tokens);

    int has_entry = 0;
    for (int i = 0; i < ast->data.program.count; i++) {
        ASTNode *stmt = ast->data.program.statements[i];
        if (stmt->type == NODE_FUNCTION_DEFINITION &&
            strcmp(stmt->data.function_definition.name, "__entry") == 0) {
            has_entry = 1;
            break;
        }
    }
    if (!is_lib && !has_entry) {
        fprintf(stderr, "Error: __entry function not defined\n");
        free_ast(ast);
        free_tokens(tokens);
        free(source);
        exit(1);
    }

    FILE *output = fopen(output_file, "w");
    if (output == NULL) {
        fprintf(stderr, "Could not open output file %s\n", output_file);
        exit(1);
    }

    CodeGenerator gen = {
        .out = output,
        .label_counter = 0
    };

    codegen_generate_program(&gen, ast);

    fclose(output);
    free_ast(ast);
    free_tokens(tokens);
    free(source);
}

void cleanup_imports() {
    for (int i = 0; i < imported_files.count; i++) {
        free(imported_files.files[i]);
    }
    free(imported_files.files);
}

int main(int argc, char **argv) {
    if (argc == 4 && strncmp(argv[3], "--target=", 9) == 0) {
        if (strcmp(argv[3]+9, "js") == 0) {
            current_target = TARGET_JS;
        }
    } else if (argc == 4 && strcmp(argv[3], "--library") == 0) {
        is_lib = true;
    } else if (argc == 3) {} else {
        fprintf(stderr, "%s <input_file> <output_file> [--target=js|--library]\n", argv[0]);
        exit(1);
    }

    compile(argv[1], argv[2]);
    cleanup_imports();
    return 0;
}
