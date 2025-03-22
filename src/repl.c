#include "orta.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT_LENGTH 1024

char* readline(const char* prompt) {
    printf("%s", prompt);
    fflush(stdout);
    
    char* buffer = malloc(MAX_INPUT_LENGTH);
    if (!buffer) return NULL;
    
    if (fgets(buffer, MAX_INPUT_LENGTH, stdin) == NULL) {
        free(buffer);
        return NULL;
    }
    
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    
    return buffer;
}

#define HISTORY_SIZE 100
static char* history[HISTORY_SIZE] = {0};
static int history_count = 0;

void add_to_history(const char* line) {
    if (!line || strlen(line) == 0) return;
    
    if (history_count > 0 && strcmp(history[history_count-1], line) == 0) {
        return;
    }
    
    if (history_count == HISTORY_SIZE) {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i-1] = history[i];
        }
        history_count--;
    }
    
    history[history_count++] = strdup(line);
}

void free_history() {
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }
    history_count = 0;
}

bool parse_line(OrtaVM *vm, const char *line) {
    char *trimmed = trim_left(line);
    if (trimmed == NULL || strlen(trimmed) == 0 || starts_with(";", trimmed)) {
        free(trimmed);
        return true;
    }
    Vector tokens = split_to_vector(trimmed, " ");
    free(trimmed);
    
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
        return true;
    }
    char *first_token = vector_get_str(&tokens, 0);
    if (is_label_declaration(first_token)) {
        size_t label_len = strlen(first_token);
        char *label_name = strdup(first_token);
        label_name[label_len - 1] = '\0';
        add_label(&vm->program, label_name, vm->program.instructions_count);
        free(label_name);
        vector_remove(&tokens, 0);
        if (tokens.size == 0) {
            vector_free(&tokens);
            return true;
        }
        first_token = vector_get_str(&tokens, 0);
    }
    if (first_token == NULL) {
        vector_free(&tokens);
        return true;
    }
    char *instruction_name = strdup(first_token);
    vector_remove(&tokens, 0);
    Instruction parsed_instruction = parse_instruction(instruction_name);
    free(instruction_name);
    if (parsed_instruction == -1) {
        fprintf(stderr, "Error: Unknown instruction '%s'\n", first_token);
        vector_free(&tokens);
        return false;
    }
    ArgRequirement expected_args = instruction_expected_args(parsed_instruction);
    if (!validateArgCount(expected_args, tokens.size)) {
        fprintf(stderr, "Error: Expected %d arguments for '%s', got %zu\n",
                expected_args.value, instruction_to_string(parsed_instruction), tokens.size);
        vector_free(&tokens);
        return false;
    }
    InstructionData instr;
    instr.opcode = parsed_instruction;
    instr.operands = tokens;
    add_instruction(&vm->program, instr);
    return true;
}

void display_help() {
    printf("Available commands:\n"
           "  exit      - Quit the REPL\n"
           "  stack     - Display the current stack\n"
           "  registers - Display register values\n"
           "  history   - Show command history\n"
           "  help      - Show this help message\n");
}

int main() {
    OrtaVM vm = ortavm_create("repl");
    char *line;
    printf("%s", LOGO); 
    printf("OrtaVM REPL (type 'help' for commands)\n");
    
    while ((line = readline("> ")) != NULL) {
        if (strcmp(line, "exit") == 0) {
            free(line);
            break;
        }
        
        if (strcmp(line, "stack") == 0) {
            print_stack(&vm.xpu);
        }
        else if (strcmp(line, "registers") == 0) {
            print_registers(&vm.xpu);
        }
        else if (strcmp(line, "history") == 0) {
            for (int i = 0; i < history_count; i++) {
                printf("%3d: %s\n", i + 1, history[i]);
            }
        }
        else if (strcmp(line, "help") == 0) {
            display_help();
        }
        else if (strlen(line) > 0) {
            if (!parse_line(&vm, line)) {
                fprintf(stderr, "Syntax error.\n");
            } else {
                size_t current_ip = vm.program.instructions_count - 1;
                vm.xpu.ip = current_ip;
                execute_instruction(&vm, &vm.program.instructions[current_ip]);
            }
            add_to_history(line);
        }
        
        free(line);
    }
    
    free_history();
    ortavm_free(&vm);
    return 0;
}
