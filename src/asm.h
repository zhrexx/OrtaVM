#ifndef ASM_H
#define ASM_H
#include "orta.h"

Instruction parse_instruction(const char *instruction) {
    for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
        if (strcmp(instructions[i].name, instruction) == 0) {
            return instructions[i].instruction;
        }
    }
    return (Instruction)-1;
}

int parse_program(OrtaVM *vm, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        return 0;
    }
    
    char buffer[8192];
    size_t bytes_read = fread(buffer, sizeof(char), 8191, fp);
    buffer[bytes_read] = '\0';
    fclose(fp);
    
    Vector lines = split_to_vector(buffer, "\n");
    
    VECTOR_FOR_EACH(char *, line, &lines) {
        if (*line == NULL) continue;
        
        char *trimmed = trim_left(*line);
        if (trimmed == NULL || strlen(trimmed) == 0 || starts_with(";", trimmed)) {
            free(trimmed);
            continue;
        }
        
        Vector tokens = split_to_vector(trimmed, " ");
        free(trimmed);
        if (tokens.size < 1) {
            vector_free(&tokens);
            continue;
        }
        
        for (size_t i = 0; i < tokens.size; i++) {
            char *token = vector_get_str(&tokens, i);
            if (token != NULL && starts_with(";", token)) {
                while (tokens.size > i) {
                    vector_remove(&tokens, i);
                }
                break;
            }
        }
        
        if (tokens.size == 0) {
            vector_free(&tokens);
            continue;
        }
        
        char *first_token = vector_get_str(&tokens, 0);
        if (first_token != NULL && is_label_declaration(first_token)) {
            size_t label_len = strlen(first_token);
            char *label_name = strdup(first_token);
            label_name[label_len - 1] = '\0';

            add_label(&vm->program, label_name, vm->program.instructions_count);
            //printf("Adding label: '%s' at address %zu\n", label_name, vm->program.instructions_count);
            free(label_name);
            
            vector_remove(&tokens, 0);
            
            if (tokens.size == 0) {
                vector_free(&tokens);
                continue;
            }
            
            first_token = vector_get_str(&tokens, 0);
        }
        
        if (first_token == NULL) {
            vector_free(&tokens);
            continue;
        }
        
        char *instruction_name = strdup(first_token);
        vector_remove(&tokens, 0);
        
        Instruction parsed_instruction = parse_instruction(instruction_name);
        free(instruction_name);
        
        if (parsed_instruction == -1) {
            fprintf(stderr, "Error: Unknown instruction '%s'\n", first_token);
            vector_free(&tokens);
            vector_free(&lines);
            return 0;
        }
        
        ArgRequirement expected_args = instruction_expected_args(parsed_instruction);
        if (!validateArgCount(expected_args, tokens.size)) {
            fprintf(stderr, "Error: Expected %d arguments for instruction '%s', but got %zu\n",
                    expected_args.value, instruction_to_string(parsed_instruction), tokens.size);
            vector_free(&tokens);
            vector_free(&lines);
            return 0;
        }
        
        InstructionData instr;
        instr.opcode = parsed_instruction;
        instr.operands = tokens;
        add_instruction(&vm->program, instr);
    }
    
    vector_free(&lines);
    return 1;
}


typedef struct {
    char name[MAX_DEFINE_NAME];
    char value[MAX_DEFINE_VALUE];
    int is_function_macro;
    int argc;
    char args[MAX_ARGS][MAX_ARG_LEN];
} Define;

Define defines[MAX_DEFINES];
int define_count = 0;

char include_paths[MAX_INCLUDE_PATHS][MAX_LINE_LENGTH];
int include_path_count = 0;


void add_include_path(const char *path) {
    if (include_path_count >= MAX_INCLUDE_PATHS) {
        fprintf(stderr, "Warning: Too many include paths\n");
        return;
    }

    char resolved_path[MAX_LINE_LENGTH];
    if (path[0] == '~') {
        const char *home = NULL;

        #if defined(_WIN32) || defined(_WIN64)
        home = getenv("USERPROFILE");
        #else
        home = getenv("HOME");
        #endif

        if (home) {
            snprintf(resolved_path, sizeof(resolved_path), "%s%s", home, path + 1);
        } else {
            strncpy(resolved_path, path, sizeof(resolved_path) - 1);
            resolved_path[sizeof(resolved_path) - 1] = '\0';
        }
    } else {
        strncpy(resolved_path, path, sizeof(resolved_path) - 1);
        resolved_path[sizeof(resolved_path) - 1] = '\0';
    }

    strncpy(include_paths[include_path_count], resolved_path, MAX_LINE_LENGTH - 1);
    include_paths[include_path_count][MAX_LINE_LENGTH - 1] = '\0';
    include_path_count++;
}

void add_simple_define(const char *name, const char *value) {
    if (define_count >= MAX_DEFINES) {
        fprintf(stderr, "Warning: Too many #define directives\n");
        return;
    }
    
    strncpy(defines[define_count].name, name, MAX_DEFINE_NAME - 1);
    defines[define_count].name[MAX_DEFINE_NAME - 1] = '\0';
    
    strncpy(defines[define_count].value, value, MAX_DEFINE_VALUE - 1);
    defines[define_count].value[MAX_DEFINE_VALUE - 1] = '\0';
    
    defines[define_count].is_function_macro = 0;
    defines[define_count].argc = 0;
    
    define_count++;
}

void add_function_macro(const char *name, const char *args_str, const char *value) {
    if (define_count >= MAX_DEFINES) {
        fprintf(stderr, "Warning: Too many #define directives\n");
        return;
    }
    
    Define *d = &defines[define_count];
    
    strncpy(d->name, name, MAX_DEFINE_NAME - 1);
    d->name[MAX_DEFINE_NAME - 1] = '\0';
    
    strncpy(d->value, value, MAX_DEFINE_VALUE - 1);
    d->value[MAX_DEFINE_VALUE - 1] = '\0';
    
    d->is_function_macro = 1;
    d->argc = 0;
    
    char arg_buffer[MAX_LINE_LENGTH];
    strncpy(arg_buffer, args_str, MAX_LINE_LENGTH - 1);
    arg_buffer[MAX_LINE_LENGTH - 1] = '\0';
    
    char *token = strtok(arg_buffer, ",");
    while (token && d->argc < MAX_ARGS) {
        while (isspace(*token)) token++;
        
        char *end = token + strlen(token) - 1;
        while (end > token && isspace(*end)) end--;
        *(end + 1) = '\0';
        
        strncpy(d->args[d->argc], token, MAX_ARG_LEN - 1);
        d->args[d->argc][MAX_ARG_LEN - 1] = '\0';
        d->argc++;
        
        token = strtok(NULL, ",");
    }
    
    define_count++;
}

char *replace_args_in_macro(Define *d, char args_values[MAX_ARGS][MAX_ARG_LEN]) {
    static char result[MAX_DEFINE_VALUE];
    char *p = d->value;
    int i = 0;
    
    while (*p && i < MAX_DEFINE_VALUE - 1) {
        int replaced = 0;
        
        for (int j = 0; j < d->argc; j++) {
            if (strncmp(p, d->args[j], strlen(d->args[j])) == 0 &&
                (!isalnum(p[strlen(d->args[j])]) && p[strlen(d->args[j])] != '_')) {
                
                strcpy(result + i, args_values[j]);
                i += strlen(args_values[j]);
                p += strlen(d->args[j]);
                replaced = 1;
                break;
            }
        }
        
        if (!replaced) {
            result[i++] = *p++;
        }
    }
    
    result[i] = '\0';
    return result;
}

char *parse_macro_args(char *p, Define *d, char args_values[MAX_ARGS][MAX_ARG_LEN]) {
    int arg_count = 0;
    int paren_depth = 1;
    char arg_buffer[MAX_ARG_LEN];
    int buffer_idx = 0;
    
    p++;
    
    while (*p && paren_depth > 0 && arg_count < MAX_ARGS) {
        if (*p == '(' && *(p-1) != '\\') {
            paren_depth++;
            arg_buffer[buffer_idx++] = *p++;
        } else if (*p == ')' && *(p-1) != '\\') {
            paren_depth--;
            if (paren_depth == 0) {
                if (buffer_idx > 0) {
                    arg_buffer[buffer_idx] = '\0';
                    strncpy(args_values[arg_count], arg_buffer, MAX_ARG_LEN - 1);
                    args_values[arg_count][MAX_ARG_LEN - 1] = '\0';
                    arg_count++;
                }
                p++;
                break;
            } else {
                arg_buffer[buffer_idx++] = *p++;
            }
        } else if (*p == ',' && paren_depth == 1 && *(p-1) != '\\') {
            arg_buffer[buffer_idx] = '\0';
            strncpy(args_values[arg_count], arg_buffer, MAX_ARG_LEN - 1);
            args_values[arg_count][MAX_ARG_LEN - 1] = '\0';
            arg_count++;
            buffer_idx = 0;
            p++;
        } else {
            if (buffer_idx < MAX_ARG_LEN - 1) {
                arg_buffer[buffer_idx++] = *p;
            }
            p++;
        }
    }
    
    if (arg_count != d->argc) {
        fprintf(stderr, "Warning: Macro %s expects %d arguments, got %d\n", 
                d->name, d->argc, arg_count);
    }
    
    return p;
}

char *process_defines(char *line) {
    static char result[MAX_LINE_LENGTH];
    char *p = line;
    int i = 0;
    
    while (*p && i < MAX_LINE_LENGTH - 1) {
        int replaced = 0;
        
        for (int j = 0; j < define_count; j++) {
            Define *d = &defines[j];
            
            if (strncmp(p, d->name, strlen(d->name)) == 0) {
                char next_char = p[strlen(d->name)];
                
                if (d->is_function_macro && next_char == '(') {
                    char args_values[MAX_ARGS][MAX_ARG_LEN];
                    char *new_p = parse_macro_args(p + strlen(d->name), d, args_values);
                    
                    char *expanded = replace_args_in_macro(d, args_values);
                    strcpy(result + i, expanded);
                    i += strlen(expanded);
                    p = new_p;
                    replaced = 1;
                    break;
                } else if (!d->is_function_macro && (!isalnum(next_char) && next_char != '_')) {
                    strcpy(result + i, d->value);
                    i += strlen(d->value);
                    p += strlen(d->name);
                    replaced = 1;
                    break;
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

int parse_define_directive(char *line) {
    char *p = line + 7;
    while (isspace(*p)) p++;
    
    char define_name[MAX_DEFINE_NAME];
    int name_idx = 0;
    
    while (*p && !isspace(*p) && *p != '(' && name_idx < MAX_DEFINE_NAME - 1) {
        define_name[name_idx++] = *p++;
    }
    define_name[name_idx] = '\0';
    
    if (*p == '(') {
        p++;
        char args_str[MAX_LINE_LENGTH];
        int args_idx = 0;
        int paren_depth = 1;
        
        while (*p && paren_depth > 0 && args_idx < MAX_LINE_LENGTH - 1) {
            if (*p == '(') paren_depth++;
            else if (*p == ')') paren_depth--;
            
            if (paren_depth > 0) {
                args_str[args_idx++] = *p;
            }
            p++;
        }
        args_str[args_idx] = '\0';
        
        while (isspace(*p)) p++;
        
        add_function_macro(define_name, args_str, p);
    } else {
        while (isspace(*p)) p++;
        add_simple_define(define_name, p);
    }
    
    return 0;
}

FILE *find_include_file(const char *filename) {
    FILE *file = NULL;
    
    file = fopen(filename, "r");
    if (file) return file;
    
    char full_path[MAX_LINE_LENGTH];
    for (int i = 0; i < include_path_count; i++) {
        snprintf(full_path, MAX_LINE_LENGTH, "%s/%s", include_paths[i], filename);
        file = fopen(full_path, "r");
        if (file) {
            return file;
        }
    }
    
    return NULL;
}

int process_include(const char *filename, FILE *output, int depth) {
    if (depth >= MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Error: Maximum include depth exceeded for %s\n", filename);
        return 0;
    }
    
    FILE *include_fp = find_include_file(filename);
    if (!include_fp) {
        fprintf(stderr, "Warning: Cannot include file %s\n", filename);
        return 0;
    }
    
    char include_line[MAX_LINE_LENGTH];
    while (fgets(include_line, MAX_LINE_LENGTH, include_fp)) {
        char trimmed_line[MAX_LINE_LENGTH] = {0};
        int i = 0, j = 0;
        int in_string = 0;

        while (include_line[i] != '\0' && include_line[i] != '\n') {
            if (include_line[i] == '"') in_string = !in_string;
            if (!in_string && include_line[i] == ';') {
                trimmed_line[j] = '\0';
                break;
            }
            trimmed_line[j++] = include_line[i++];
        }

        while (j > 0 && (trimmed_line[j - 1] == ' ' || trimmed_line[j - 1] == '\t')) j--;
        trimmed_line[j] = '\0';

        if (j > 0) {
            if (strncmp(trimmed_line, "#include", 8) == 0) {
                char nested_include_file[MAX_LINE_LENGTH];
                if (sscanf(trimmed_line + 8, " \"%[^\"]\"", nested_include_file) == 1) {
                    process_include(nested_include_file, output, depth + 1);
                } else if (sscanf(trimmed_line + 8, " <%[^>]>", nested_include_file) == 1) {
                    process_include(nested_include_file, output, depth + 1);
                }
            } else if (strncmp(trimmed_line, "#define", 7) == 0) {
                parse_define_directive(trimmed_line);
            } else {
                char *processed = process_defines(trimmed_line);
                if (processed) {
                    fputs(processed, output);
                    fputs("\n", output);
                }
            }
        }
    }
    
    fclose(include_fp);
    return 1;
}

int orta_preprocess(char *filename, char *output_file) {
    char *dpaths[] = {
        ".",
        "~/.orta/",
    };
    int path_count = 2;
    for (int i = 0; i < path_count && i < MAX_INCLUDE_PATHS; i++) {
        add_include_path(dpaths[i]);
    }
    
    FILE *input = fopen(filename, "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file %s\n", filename);
        return 0;
    }

    FILE *output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file %s\n", output_file);
        fclose(input);
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    char *SNULL = malloc(sizeof(void*));
    sprintf(SNULL, "%p", NULL);
    add_simple_define("NULL", SNULL);

    while (fgets(line, MAX_LINE_LENGTH, input)) {
        char trimmed_line[MAX_LINE_LENGTH] = {0};
        int i = 0, j = 0;
        int in_string = 0;

        while (line[i] != '\0' && line[i] != '\n') {
            if (line[i] == '"') in_string = !in_string;
            if (!in_string && line[i] == ';') {
                trimmed_line[j] = '\0';
                break;
            }
            trimmed_line[j++] = line[i++];
        }

        while (j > 0 && (trimmed_line[j - 1] == ' ' || trimmed_line[j - 1] == '\t')) j--;
        trimmed_line[j] = '\0';

        if (j > 0) {
            if (strncmp(trimmed_line, "#include", 8) == 0) {
                char include_file[MAX_LINE_LENGTH];
                if (sscanf(trimmed_line + 8, " \"%[^\"]\"", include_file) == 1) {
                    process_include(include_file, output, 0);
                } else if (sscanf(trimmed_line + 8, " <%[^>]>", include_file) == 1) {
                    process_include(include_file, output, 0);
                }
            } else if (strncmp(trimmed_line, "#define", 7) == 0) {
                parse_define_directive(trimmed_line);
            } else {
                char *processed = process_defines(trimmed_line);
                if (processed) {
                    fputs(processed, output);
                    fputs("\n", output);
                }
            }
        }
    }
    
    free(SNULL);
    fclose(input);
    fclose(output);
    return 1;
}


#endif // ASM_H
