#ifndef ASM_H
#define ASM_H
#include "orta.h"

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_LABEL,
    TOKEN_LOCAL_LABEL,
    TOKEN_INSTRUCTION,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_COMMENT,
    TOKEN_DIRECTIVE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    size_t line;
    size_t column;
} Token;

typedef struct {
    char *input;
    size_t pos;
    size_t line;
    size_t column;
    size_t length;
} Lexer;

typedef struct {
    Token *tokens;
    size_t pos;
    size_t count;
    size_t capacity;
} TokenStream;

typedef struct {
    char name[64];
    char value[256];
    int is_function_macro;
    int argc;
    char args[16][32];
} Define;

typedef struct {
    Define *defines;
    size_t count;
    size_t capacity;
    char **include_paths;
    size_t include_count;
    size_t include_capacity;
} Preprocessor;

static Lexer* lexer_create(const char *input) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->input = strdup(input);
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->length = strlen(input);
    return lexer;
}

static void lexer_free(Lexer *lexer) {
    if (lexer) {
        free(lexer->input);
        free(lexer);
    }
}

static char lexer_peek(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    return lexer->input[lexer->pos];
}

static char lexer_advance(Lexer *lexer) {
    if (lexer->pos >= lexer->length) return '\0';
    char c = lexer->input[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (isspace(lexer_peek(lexer)) && lexer_peek(lexer) != '\n') {
        lexer_advance(lexer);
    }
}

static Token* token_create(TokenType type, const char *value, size_t line, size_t column) {
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->column = column;
    return token;
}

static void token_free(Token *token) {
    if (token) {
        free(token->value);
        free(token);
    }
}

static int is_identifier_start(char c) {
    return isalpha(c) || c == '_';
}

static int is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

static int is_digit(char c) {
    return isdigit(c);
}

static Token* lexer_read_identifier(Lexer *lexer) {
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    char buffer[256];
    size_t i = 0;
    
    while (is_identifier_char(lexer_peek(lexer)) && i < sizeof(buffer) - 1) {
        buffer[i++] = lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    return token_create(TOKEN_IDENTIFIER, buffer, start_line, start_column);
}

static Token* lexer_read_number(Lexer *lexer) {
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    char buffer[256];
    size_t i = 0;
    
    if (lexer_peek(lexer) == '0' && (lexer->input[lexer->pos + 1] == 'x' || lexer->input[lexer->pos + 1] == 'X')) {
        buffer[i++] = lexer_advance(lexer);
        buffer[i++] = lexer_advance(lexer);
        while (isxdigit(lexer_peek(lexer)) && i < sizeof(buffer) - 1) {
            buffer[i++] = lexer_advance(lexer);
        }
    } else {
        while (is_digit(lexer_peek(lexer)) && i < sizeof(buffer) - 1) {
            buffer[i++] = lexer_advance(lexer);
        }
    }
    buffer[i] = '\0';
    
    return token_create(TOKEN_NUMBER, buffer, start_line, start_column);
}

static Token* lexer_read_string(Lexer *lexer) {
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    char buffer[512];
    size_t i = 0;
    
    buffer[i++] = lexer_advance(lexer);
    
    while (lexer_peek(lexer) != '"' && lexer_peek(lexer) != '\0' && i < sizeof(buffer) - 2) {
        if (lexer_peek(lexer) == '\\') {
            buffer[i++] = lexer_advance(lexer);
            char escaped = lexer_peek(lexer);
            if (escaped != '\0') {
                buffer[i++] = lexer_advance(lexer);
            }
        } else {
            buffer[i++] = lexer_advance(lexer);
        }
    }
    
    if (lexer_peek(lexer) == '"') {
        buffer[i++] = lexer_advance(lexer);
    }
    
    buffer[i] = '\0';
    return token_create(TOKEN_STRING, buffer, start_line, start_column);
}

static Token* lexer_read_comment(Lexer *lexer) {
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    char buffer[256];
    size_t i = 0;
    
    while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0' && i < sizeof(buffer) - 1) {
        buffer[i++] = lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    return token_create(TOKEN_COMMENT, buffer, start_line, start_column);
}

static Token* lexer_read_directive(Lexer *lexer) {
    size_t start_line = lexer->line;
    size_t start_column = lexer->column;
    char buffer[256];
    size_t i = 0;
    
    while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0' && i < sizeof(buffer) - 1) {
        buffer[i++] = lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    return token_create(TOKEN_DIRECTIVE, buffer, start_line, start_column);
}

static Token* lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);
    
    char c = lexer_peek(lexer);
    size_t line = lexer->line;
    size_t column = lexer->column;
    
    if (c == '\0') {
        return token_create(TOKEN_EOF, NULL, line, column);
    }
    
    if (c == '\n') {
        lexer_advance(lexer);
        return token_create(TOKEN_NEWLINE, "\n", line, column);
    }
    
    if (c == ';') {
        return lexer_read_comment(lexer);
    }
    
    if (c == '#') {
        return lexer_read_directive(lexer);
    }
    
    if (c == '"') {
        return lexer_read_string(lexer);
    }
    
    if (c == ',') {
        lexer_advance(lexer);
        return token_create(TOKEN_COMMA, ",", line, column);
    }
    
    if (c == '(') {
        lexer_advance(lexer);
        return token_create(TOKEN_LPAREN, "(", line, column);
    }
    
    if (c == ')') {
        lexer_advance(lexer);
        return token_create(TOKEN_RPAREN, ")", line, column);
    }
    
    if (is_digit(c)) {
        return lexer_read_number(lexer);
    }
    
    if (is_identifier_start(c)) {
        Token *token = lexer_read_identifier(lexer);
        
        if (lexer_peek(lexer) == ':') {
            lexer_advance(lexer);
            if (token->value[0] == '.') {
                token->type = TOKEN_LOCAL_LABEL;
            } else {
                token->type = TOKEN_LABEL;
            }
        }
        
        return token;
    }
    
    if (c == '.') {
        Token *token = lexer_read_identifier(lexer);
        if (lexer_peek(lexer) == ':') {
            lexer_advance(lexer);
            token->type = TOKEN_LOCAL_LABEL;
        } else {
            token->type = TOKEN_LOCAL_LABEL;
        }
        return token;
    }
    
    lexer_advance(lexer);
    return token_create(TOKEN_ERROR, NULL, line, column);
}

static TokenStream* tokenize(const char *input) {
    TokenStream *stream = malloc(sizeof(TokenStream));
    stream->capacity = 1024;
    stream->tokens = malloc(sizeof(Token) * stream->capacity);
    stream->count = 0;
    stream->pos = 0;
    
    Lexer *lexer = lexer_create(input);
    
    while (1) {
        Token *token = lexer_next_token(lexer);
        
        if (stream->count >= stream->capacity) {
            stream->capacity *= 2;
            stream->tokens = realloc(stream->tokens, sizeof(Token) * stream->capacity);
        }
        
        stream->tokens[stream->count++] = *token;
        free(token);
        
        if (stream->tokens[stream->count - 1].type == TOKEN_EOF) {
            break;
        }
    }
    
    lexer_free(lexer);
    return stream;
}

static void token_stream_free(TokenStream *stream) {
    if (stream) {
        for (size_t i = 0; i < stream->count; i++) {
            free(stream->tokens[i].value);
        }
        free(stream->tokens);
        free(stream);
    }
}

static Token* token_stream_peek(TokenStream *stream) {
    if (stream->pos >= stream->count) return NULL;
    return &stream->tokens[stream->pos];
}

static Token* token_stream_advance(TokenStream *stream) {
    if (stream->pos >= stream->count) return NULL;
    return &stream->tokens[stream->pos++];
}

static int token_stream_match(TokenStream *stream, TokenType type) {
    Token *token = token_stream_peek(stream);
    return token && token->type == type;
}

static void token_stream_skip_newlines(TokenStream *stream) {
    while (token_stream_match(stream, TOKEN_NEWLINE) || token_stream_match(stream, TOKEN_COMMENT)) {
        token_stream_advance(stream);
    }
}

static Preprocessor* preprocessor_create() {
    Preprocessor *pp = malloc(sizeof(Preprocessor));
    pp->defines = malloc(sizeof(Define) * 64);
    pp->count = 0;
    pp->capacity = 64;
    pp->include_paths = malloc(sizeof(char*) * 16);
    pp->include_count = 0;
    pp->include_capacity = 16;
    return pp;
}

static void preprocessor_free(Preprocessor *pp) {
    if (pp) {
        free(pp->defines);
        for (size_t i = 0; i < pp->include_count; i++) {
            free(pp->include_paths[i]);
        }
        free(pp->include_paths);
        free(pp);
    }
}

static void preprocessor_add_define(Preprocessor *pp, const char *name, const char *value) {
    if (pp->count >= pp->capacity) {
        pp->capacity *= 2;
        pp->defines = realloc(pp->defines, sizeof(Define) * pp->capacity);
    }
    
    Define *def = &pp->defines[pp->count++];
    strncpy(def->name, name, sizeof(def->name) - 1);
    def->name[sizeof(def->name) - 1] = '\0';
    strncpy(def->value, value, sizeof(def->value) - 1);
    def->value[sizeof(def->value) - 1] = '\0';
    def->is_function_macro = 0;
    def->argc = 0;
}

static void preprocessor_add_include_path(Preprocessor *pp, const char *path) {
    if (pp->include_count >= pp->include_capacity) {
        pp->include_capacity *= 2;
        pp->include_paths = realloc(pp->include_paths, sizeof(char*) * pp->include_capacity);
    }
    
    pp->include_paths[pp->include_count++] = strdup(path);
}

static char* preprocessor_expand_defines(Preprocessor *pp, const char *text) {
    static char result[4096];
    const char *p = text;
    size_t i = 0;
    
    while (*p && i < sizeof(result) - 1) {
        int replaced = 0;
        
        for (size_t j = 0; j < pp->count; j++) {
            Define *def = &pp->defines[j];
            size_t name_len = strlen(def->name);
            
            if (strncmp(p, def->name, name_len) == 0) {
                char next_char = p[name_len];
                if (!is_identifier_char(next_char)) {
                    size_t value_len = strlen(def->value);
                    if (i + value_len < sizeof(result) - 1) {
                        strcpy(result + i, def->value);
                        i += value_len;
                        p += name_len;
                        replaced = 1;
                        break;
                    }
                }
            }
        }
        
        if (!replaced) {
            result[i++] = *p++;
        }
    }
    
    result[i] = '\0';
    return result;
}

static char* read_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *content = malloc(size + 1);
    fread(content, 1, size, fp);
    content[size] = '\0';
    
    fclose(fp);
    return content;
}

static char* preprocess_file(Preprocessor *pp, const char *filename) {
    char *content = read_file(filename);
    if (!content) return NULL;
    
    TokenStream *stream = tokenize(content);
    free(content);
    
    static char result[8192];
    size_t pos = 0;
    int on_new_line = 1;
    
    for (size_t i = 0; i < stream->count; i++) {
        Token *token = &stream->tokens[i];
        
        if (token->type == TOKEN_DIRECTIVE) {
            if (strncmp(token->value, "#define", 7) == 0) {
                char name[64], value[256];
                if (sscanf(token->value + 7, " %63s %255[^\n]", name, value) == 2) {
                    preprocessor_add_define(pp, name, value);
                } else if (sscanf(token->value + 7, " %63s", name) == 1) {
                    preprocessor_add_define(pp, name, "1");
                }
            } else if (strncmp(token->value, "#include", 8) == 0) {
                char include_file[256];
                if (sscanf(token->value + 8, " \"%255[^\"]\"", include_file) == 1 ||
                    sscanf(token->value + 8, " <%255[^>]>", include_file) == 1) {
                    char *included = preprocess_file(pp, include_file);
                    if (included && pos + strlen(included) < sizeof(result) - 1) {
                        strcpy(result + pos, included);
                        pos += strlen(included);
                        free(included);
                    }
                }
            }
            on_new_line = 1;
        } else if (token->type == TOKEN_COMMENT) {
            continue;
        } else if (token->type == TOKEN_NEWLINE) {
            if (pos < sizeof(result) - 1) {
                result[pos++] = '\n';
            }
            on_new_line = 1;
        } else {
            if (token->value) {
                char *expanded = preprocessor_expand_defines(pp, token->value);
                if (!on_new_line && pos < sizeof(result) - 1) {
                    result[pos++] = ' ';
                }
                if (pos + strlen(expanded) < sizeof(result) - 1) {
                    strcpy(result + pos, expanded);
                    pos += strlen(expanded);
                }
            }
            
            if (token->type == TOKEN_LABEL || token->type == TOKEN_LOCAL_LABEL) {
                if (pos < sizeof(result) - 1) {
                    result[pos++] = ':';
                }
            }
            
            on_new_line = 0;
        }
    }
    
    result[pos] = '\0';
    token_stream_free(stream);
    
    return strdup(result);
}

Instruction parse_instruction(const char *instruction) {
    for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
        if (strcmp(instructions[i].name, instruction) == 0) {
            return instructions[i].instruction;
        }
    }
    return (Instruction)-1;
}

static int parse_operands(TokenStream *stream, Vector *operands) {
    while (!token_stream_match(stream, TOKEN_NEWLINE) && 
           !token_stream_match(stream, TOKEN_EOF) &&
           !token_stream_match(stream, TOKEN_COMMENT)) {
        
        Token *token = token_stream_advance(stream);
        if (!token) break;
        
        if (token->type == TOKEN_COMMA) {
            continue;
        }
        
        char *operand = strdup(token->value);
        vector_push(operands, &operand);
    }
    
    return 1;
}

int parse_program(OrtaVM *vm, const char *filename) {
    Preprocessor *pp = preprocessor_create();
    preprocessor_add_include_path(pp, ".");
    preprocessor_add_include_path(pp, "~/.orta/");
    
    char *preprocessed = preprocess_file(pp, filename);
    if (!preprocessed) {
        fprintf(stderr, "Error: Failed to preprocess file '%s'\n", filename);
        preprocessor_free(pp);
        return 0;
    }
    
    TokenStream *stream = tokenize(preprocessed);
    free(preprocessed);
    preprocessor_free(pp);
    
    size_t current_line = 1;
    
    while (!token_stream_match(stream, TOKEN_EOF)) {
        token_stream_skip_newlines(stream);
        
        Token *token = token_stream_peek(stream);
        if (!token || token->type == TOKEN_EOF) break;
        
        current_line = token->line;
        
        if (token->type == TOKEN_LABEL) {
            char *label_name = strdup(token->value);
            if (label_name[0] != '.') {
                free(vm->program.last_non_local_label);
                vm->program.last_non_local_label = strdup(label_name);
            }
            add_label(&vm->program, label_name, vm->program.instructions_count);
            free(label_name);
            token_stream_advance(stream);
            continue;
        }
        
        if (token->type == TOKEN_LOCAL_LABEL) {
            if (!vm->program.last_non_local_label) {
                fprintf(stderr, "Error: Local label '%s' has no context\n", token->value);
                token_stream_free(stream);
                return 0;
            }
            
            char resolved[256];
            snprintf(resolved, sizeof(resolved), "%s_%s", 
                    vm->program.last_non_local_label, token->value + 1);
            
            add_label(&vm->program, resolved, vm->program.instructions_count);
            token_stream_advance(stream);
            continue;
        }
        
        if (token->type == TOKEN_IDENTIFIER) {
            Instruction parsed_instruction = parse_instruction(token->value);
            if (parsed_instruction == (Instruction)-1) {
                fprintf(stderr, "Error: Unknown instruction '%s' at line %zu\n", 
                       token->value, token->line);
                token_stream_free(stream);
                return 0;
            }
            
            token_stream_advance(stream);
            
            Vector operands;
            vector_init(&operands, 5, sizeof(char*));
            
            if (!parse_operands(stream, &operands)) {
                vector_free(&operands);
                token_stream_free(stream);
                return 0;
            }
            
            ArgRequirement expected_args = instruction_expected_args(parsed_instruction);
            if (!validateArgCount(expected_args, operands.size)) {
                fprintf(stderr, "Error: Expected %d args for '%s', got %zu at line %zu\n",
                       expected_args.value, instruction_to_string(parsed_instruction), 
                       operands.size, current_line);
                vector_free(&operands);
                token_stream_free(stream);
                return 0;
            }
            
            InstructionData instr;
            instr.opcode = parsed_instruction;
            instr.operands = operands;
            instr.line = current_line;
            add_instruction(&vm->program, instr);
            continue;
        }
        
        token_stream_advance(stream);
    }
    
    token_stream_free(stream);
    return 1;
}

int orta_preprocess(char *filename, char *output_file) {
    Preprocessor *pp = preprocessor_create();
    preprocessor_add_include_path(pp, ".");
    preprocessor_add_include_path(pp, "~/.orta/");
    
    char *preprocessed = preprocess_file(pp, filename);
    if (!preprocessed) {
        fprintf(stderr, "Error: Failed to preprocess file '%s'\n", filename);
        preprocessor_free(pp);
        return 0;
    }
    
    FILE *output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file %s\n", output_file);
        free(preprocessed);
        preprocessor_free(pp);
        return 0;
    }
    
    fputs(preprocessed, output);
    fclose(output);
    
    free(preprocessed);
    preprocessor_free(pp);
    return 1;
}

#endif
