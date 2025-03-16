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
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_BG_BLACK "\x1b[40m"

#define LOGO "\n\
   ██████╗ ██████╗ ████████╗ █████╗ ██╗   ██╗███╗   ███╗\n\
  ██╔═══██╗██╔══██╗╚══██╔══╝██╔══██╗██║   ██║████╗ ████║\n\
  ██║   ██║██████╔╝   ██║   ███████║██║   ██║██╔████╔██║\n\
  ██║   ██║██╔══██╗   ██║   ██╔══██║╚██╗ ██╔╝██║╚██╔╝██║\n\
  ╚██████╔╝██║  ██║   ██║   ██║  ██║ ╚████╔╝ ██║ ╚═╝ ██║\n\
   ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝  ╚═══╝  ╚═╝     ╚═╝\n"

#define BOX_TOP    "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓"
#define BOX_BOTTOM "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛"
#define SEPARATOR  "┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫"

void print_error(const char *message) {
    fprintf(stderr, "\n%s%s ⚠ ERROR %s %s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET, message);
}

void print_success(const char *message) {
    printf("\n%s%s ✓ SUCCESS %s %s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET, message);
}

void print_info(const char *message) {
    printf("%s%s ℹ INFO %s %s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET, message);
}

void print_progress(const char *stage, const char *detail) {
    printf("%s%s[%s%s%s] %s%s%s\n", 
           COLOR_BOLD, COLOR_YELLOW, 
           COLOR_CYAN, stage, COLOR_YELLOW,
           COLOR_RESET, COLOR_BOLD, detail);
    printf("%s   └─> %s", COLOR_BOLD, COLOR_RESET);
}

void print_usage(const char *program_name) {
    printf("%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, LOGO, COLOR_RESET);
    
    printf("\n%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, BOX_TOP, COLOR_RESET);
    printf("%s%s┃               ORTAVM BYTECODE LAUNCHER                 ┃%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, BOX_BOTTOM, COLOR_RESET);
    
    printf("\n%s%s USAGE: %s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("   %s %s<program.x|program.xbin>%s [options]\n", program_name, COLOR_BLUE, COLOR_RESET);
    
    printf("\n%s%s OPTIONS: %s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("   %s-h, --help%s           Display this help message\n", COLOR_BLUE, COLOR_RESET);
    printf("   %s--nopreproc%s          Disable source file preprocessing (enabled by default)\n", COLOR_BLUE, COLOR_RESET);
    printf("   %s--disable-compile%s    Disable bytecode compiling (disabled by default)\n", COLOR_BLUE, COLOR_RESET);

    printf("\n%s%s DESCRIPTION: %s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("   Executes OrtaVM bytecode programs and displays the stack state\n");
    printf("   Supports both .x (source) and .xbin (compiled bytecode) formats\n");
    
    printf("\n%s%s EXAMPLES: %s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("   %s example.x          %s# Execute source program with preprocessing and create bytecode\n", program_name, COLOR_GREEN);
    printf("   %s example.x --nopreproc %s# Execute source program without preprocessing\n", program_name, COLOR_GREEN);
    printf("   %s example.xbin       %s# Execute pre-compiled bytecode\n\n", program_name, COLOR_GREEN);
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
    
    int orta_preprocess_flag    = 1;
    int orta_disable_compile    = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--nopreproc") == 0) {
            orta_preprocess_flag = 0;
        } else if (strcmp(argv[i], "--disable-compile") == 0) {
            orta_disable_compile = 1; 
        }
    }
    
    time_t start = time(NULL);
    printf("%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, LOGO, COLOR_RESET);
    printf("%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, BOX_TOP, COLOR_RESET);
    printf("%s%s┃                     EXECUTION STARTED                  ┃%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%s%s%s\n\n", COLOR_BOLD, COLOR_CYAN, BOX_BOTTOM, COLOR_RESET);
    
    print_progress("INIT", "Initializing virtual machine");
    print_info(argv[1]);
    OrtaVM vm = ortavm_create(argv[1]);
    
    const char *filename = argv[1];
    size_t len = strlen(filename);
    bool is_bytecode = false;
    
    if (len > 5 && strcmp(filename + len - 5, ".xbin") == 0) {
        is_bytecode = true;
        print_progress("LOAD", "Loading compiled bytecode");
        
        if (!load_bytecode(&vm, filename)) {
            print_error("Failed to load bytecode file");
            ortavm_free(&vm);
            return EXIT_FAILURE;
        }
    } else if (len > 2 && strcmp(filename + len - 2, ".x") == 0) {
        if (orta_preprocess_flag) {
            print_progress("PREPROC", "Preprocessing source file");
            
            char preprocessed_filename[256];
            snprintf(preprocessed_filename, sizeof(preprocessed_filename), "%.*s.pre.x", (int)(len - 2), filename);
            
            if (!orta_preprocess((char*)filename, preprocessed_filename)) {
                print_error("Preprocessing failed");
                ortavm_free(&vm);
                return EXIT_FAILURE;
            }
            
            print_progress("PARSE", "Parsing preprocessed source");
            
            if (!parse_program(&vm, preprocessed_filename)) {
                print_error("Failed to parse preprocessed source");
                ortavm_free(&vm);
                return EXIT_FAILURE;
            }
        } else {
            print_progress("PARSE", "Parsing source program (preprocessing disabled)");
            
            if (!parse_program(&vm, filename)) {
                print_error("Failed to parse source program");
                ortavm_free(&vm);
                return EXIT_FAILURE;
            }
        }
    } else {
        print_error("Unsupported file format. Please use .x or .xbin files");
        ortavm_free(&vm);
        return EXIT_FAILURE;
    }
    
    print_progress("EXEC", "Executing program instructions");
    execute_program(&vm);
    time_t end = time(NULL); 
    printf("\n%s%s%s%s\n", COLOR_BOLD, COLOR_GREEN, BOX_TOP, COLOR_RESET);
    printf("%s%s┃            EXECUTION COMPLETED IN %03ds                 ┃%s\n", COLOR_BOLD, COLOR_GREEN, end - start, COLOR_RESET);
    printf("%s%s%s%s\n", COLOR_BOLD, COLOR_GREEN, SEPARATOR, COLOR_RESET);
    printf("%s%s┃                      STACK STATE                       ┃%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s%s%s\n\n", COLOR_BOLD, COLOR_GREEN, BOX_BOTTOM, COLOR_RESET);
    
    print_stack(&vm.xpu);
    
    printf("\n%s%s%s%s\n", COLOR_BOLD, COLOR_GREEN, BOX_TOP, COLOR_RESET);
    printf("%s%s┃                    REGISTERS STATE                     ┃%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s%s%s\n\n", COLOR_BOLD, COLOR_GREEN, BOX_BOTTOM, COLOR_RESET);
    
    print_registers(&vm.xpu);
    
    if (!is_bytecode) {
        char bytecode_filename[256];
        snprintf(bytecode_filename, sizeof(bytecode_filename), "%.*s.xbin", (int)(len - 2), filename);
        
        print_progress("COMPILE", "Creating bytecode file");
        printf(" %s%s%s\n", COLOR_BLUE, bytecode_filename, COLOR_RESET);
        if (!orta_disable_compile) { 
            if (create_bytecode(&vm, bytecode_filename)) {
                print_success("Bytecode created successfully");
            } else {
                print_error("Failed to create bytecode file");
            }
        }
        ortavm_free(&vm);
    }
    
    printf("\n%s%s%s%s\n", COLOR_BOLD, COLOR_GREEN, BOX_TOP, COLOR_RESET);
    printf("%s%s┃                  PROGRAM TERMINATED                    ┃%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("%s%s%s%s\n\n", COLOR_BOLD, COLOR_GREEN, BOX_BOTTOM, COLOR_RESET);
    
    return EXIT_SUCCESS;
}
