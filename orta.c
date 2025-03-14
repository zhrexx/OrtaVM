#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "orta.h"

#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"

void print_error(const char *message) {
    fprintf(stderr, "%s%s ERROR %s %s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET, message);
}

void print_success(const char *message) {
    printf("%s%s SUCCESS %s %s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET, message);
}

void print_usage(const char *program_name) {
    printf("\n%s%s╔════════════════════════════════════════╗%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s║            ORTAVM LAUNCHER             ║%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s╚════════════════════════════════════════╝%s\n\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    
    printf("%sUSAGE:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s %s<program.x>%s\n\n", program_name, COLOR_BLUE, COLOR_RESET);
    
    printf("%sDESCRIPTION:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  Executes OrtaVM bytecode programs and displays the stack state\n\n");
    
    printf("%sEXAMPLE:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s example.x\n\n", program_name);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    
    printf("%s%s┌─ OrtaVM ─┐%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s│ STARTING │%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s└──────────┘%s\n\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    
    printf("Initializing VM for %s%s%s...\n", COLOR_BLUE, argv[1], COLOR_RESET);
    OrtaVM vm = ortavm_create(argv[1]);
    
    printf("Parsing program...\n");
    if (!parse_program(&vm, argv[1])) {
        print_error("Failed to parse program");
        ortavm_free(&vm);
        return EXIT_FAILURE;
    }
    
    printf("Executing program...\n");
    execute_program(&vm);
    
    printf("\n%s%s┌─ EXECUTION COMPLETE ─┐%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s│     STACK STATE      │%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s└──────────────────────┘%s\n\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    
    print_stack(&vm.xpu);
    
    printf("\n%s%s┌──────────────────────┐%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s│  REGISTERS STATE     │%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s└──────────────────────┘%s\n\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    print_registers(&vm.xpu);
    ortavm_free(&vm);
    print_success("Program execution finished");
    
    return EXIT_SUCCESS;
}
