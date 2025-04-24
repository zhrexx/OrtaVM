#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_PRINT,
    TOKEN_PARSE,
    TOKEN_EQUAL,       // =
    TOKEN_PLUS,        // +
    TOKEN_MINUS,       // -
    TOKEN_MULTIPLY,    // *
    TOKEN_DIVIDE,      // /
    TOKEN_GT,          // >
    TOKEN_LT,          // <
    TOKEN_EQ,          // ==
    TOKEN_LPAREN,      // (
    TOKEN_RPAREN,      // )
    TOKEN_LBRACE,      // {
    TOKEN_RBRACE,      // }
    TOKEN_COMMA        // ,
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
    NODE_PRINT_STATEMENT,
    NODE_PARSE_STATEMENT,
    NODE_BINARY_EXPRESSION,
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_STRING,
    NODE_FUNCTION_CALL,
    NODE_ARGUMENT_LIST
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
        } if_statement;
        
        struct {
            char *value;
        } print_statement;
        
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
    {"if", TOKEN_IF},
    {"print", TOKEN_PRINT},
    {"parse", TOKEN_PARSE},
    {NULL, 0}
};

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

void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->current_char != '\0' && isspace(lexer->current_char)) {
        lexer_advance(lexer);
    }
}

char *lexer_collect_identifier(Lexer *lexer) {
    char *buffer = malloc(256);
    int i = 0;
    
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
    
    while (lexer->current_char != '\0' && lexer->current_char != '"') {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    
    lexer_advance(lexer);
    
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
    
    if (lexer->current_char == '\0') {
        token.type = TOKEN_EOF;
        token.value = NULL;
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
            
        default:
            fprintf(stderr, "Unexpected character: %c at line %d, column %d\n", 
                    lexer->current_char, lexer->line, lexer->column);
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
    if (op_type == TOKEN_GT || op_type == TOKEN_LT || op_type == TOKEN_EQ) {
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
    
    return node;
}

ASTNode *parser_parse_print_statement(Parser *parser) {
    parser_expect(parser, TOKEN_PRINT);
    parser_expect(parser, TOKEN_LPAREN);
    
    Token string = parser_current_token(parser);
    parser_expect(parser, TOKEN_STRING);
    
    parser_expect(parser, TOKEN_RPAREN);
    
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_PRINT_STATEMENT;
    node->data.print_statement.value = strdup(string.value);
    
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

ASTNode *parser_parse_statement(Parser *parser) {
    Token token = parser_current_token(parser);
    
    if (token.type == TOKEN_VAR) {
        return parser_parse_var_declaration(parser);
    } else if (token.type == TOKEN_IF) {
        return parser_parse_if_statement(parser);
    } else if (token.type == TOKEN_PRINT) {
        return parser_parse_print_statement(parser);
    } else if (token.type == TOKEN_PARSE) {
        return parser_parse_parse_statement(parser);
    } else if (token.type == TOKEN_IDENTIFIER) {
        if (parser->position + 1 < parser->count && 
            parser->tokens[parser->position + 1].type == TOKEN_EQUAL) {
            return parser_parse_assignment(parser);
        } else {
            ASTNode *expr = parser_parse_expression(parser);
            return expr;
        }
    } else {
        fprintf(stderr, "Unexpected token in statement: %s\n", token.value);
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
    va_end(args);
    fprintf(gen->out, "\n");
}

char *codegen_create_label(CodeGenerator *gen, const char *prefix) {
    char *label = malloc(64);
    sprintf(label, "__%s_%d", prefix, gen->label_counter++);
    return label;
}

void codegen_generate_expression(CodeGenerator *gen, ASTNode *node) {
    if (node->type == NODE_NUMBER) {
        codegen_emit(gen, "push %d", node->data.number.value);
    } else if (node->type == NODE_STRING) {
        codegen_emit(gen, "push \"%s\"", node->data.string.value);
    } else if (node->type == NODE_IDENTIFIER) {
        codegen_emit(gen, "getvar %s", node->data.identifier.name);
    } else if (node->type == NODE_BINARY_EXPRESSION) {
        codegen_generate_expression(gen, node->data.binary_expression.left);
        codegen_generate_expression(gen, node->data.binary_expression.right);
        
        switch (node->data.binary_expression.operator) {
            case TOKEN_PLUS:
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
                break;
            default:
                fprintf(stderr, "Unknown operator in code generation\n");
                exit(1);
        }
    } else if (node->type == NODE_FUNCTION_CALL) {
        for (int i = 0; i < node->data.function_call.args->data.argument_list.count; i++) {
            codegen_generate_expression(gen, node->data.function_call.args->data.argument_list.args[i]);
        }
        codegen_emit(gen, "call %s %d", node->data.function_call.name, 
                     node->data.function_call.args->data.argument_list.count);
    }
}

void codegen_generate_var_declaration(CodeGenerator *gen, ASTNode *node) {
    if (node->data.var_declaration.value != NULL) {
        codegen_generate_expression(gen, node->data.var_declaration.value);
        codegen_emit(gen, "setvar %s", node->data.var_declaration.name);
    } else {
        codegen_emit(gen, "var %s", node->data.var_declaration.name);
    }
}

void codegen_generate_assignment(CodeGenerator *gen, ASTNode *node) {
    codegen_generate_expression(gen, node->data.assignment.value);
    codegen_emit(gen, "setvar %s", node->data.assignment.name);
}

void codegen_generate_statement(CodeGenerator *gen, ASTNode *node);

void codegen_generate_if_statement(CodeGenerator *gen, ASTNode *node) {
    char *end_label = codegen_create_label(gen, "if_end");
    
    codegen_generate_expression(gen, node->data.if_statement.condition);
    codegen_emit(gen, "jmpif %s", end_label);
    
    for (int i = 0; i < node->data.if_statement.body_count; i++) {
        codegen_generate_statement(gen, node->data.if_statement.body[i]);
    }
    
    codegen_emit(gen, "%s:", end_label);
    free(end_label);
}

void codegen_generate_print_statement(CodeGenerator *gen, ASTNode *node) {
    codegen_emit(gen, "push \"%s\"", node->data.print_statement.value);
    codegen_emit(gen, "print");
}

void codegen_generate_parse_statement(CodeGenerator *gen, ASTNode *node) {
    for (int i = 0; i < node->data.parse_statement.args->data.argument_list.count; i++) {
        codegen_generate_expression(gen, node->data.parse_statement.args->data.argument_list.args[i]);
    }
    codegen_emit(gen, "parse %d", node->data.parse_statement.args->data.argument_list.count);
}

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
            
        case NODE_PRINT_STATEMENT:
            codegen_generate_print_statement(gen, node);
            break;
            
        case NODE_PARSE_STATEMENT:
            codegen_generate_parse_statement(gen, node);
            break;
            
        case NODE_FUNCTION_CALL:
            codegen_generate_expression(gen, node);
            codegen_emit(gen, "pop");
            break;
            
        default:
            fprintf(stderr, "Unknown node type in code generation\n");
            exit(1);
    }
}

void codegen_generate_program(CodeGenerator *gen, ASTNode *program) {
    for (int i = 0; i < program->data.program.count; i++) {
        codegen_generate_statement(gen, program->data.program.statements[i]);
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
            break;
            
        case NODE_PRINT_STATEMENT:
            free(node->data.print_statement.value);
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

void compile(const char *input_file, const char *output_file) {
    char *source = read_file(input_file);
    
    Token *tokens = tokenize(source);
    ASTNode *ast = parse(tokens);
    
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

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        return 1;
    }
    
    compile(argv[1], argv[2]);
    return 0;
}


