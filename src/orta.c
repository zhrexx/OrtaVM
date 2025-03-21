#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "orta.h"
#include "std.h"
#include "config.h"

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

char *expand_path(const char *path) {
    if (path[0] == '~') {
        
        #ifndef _WIN32
        const char *home = getenv("HOME");
        #else 
        const char *home = getenv("USERPROFILE");
        #endif
        if (!home) return NULL;

        size_t len = strlen(home) + strlen(path);
        char *full_path = malloc(len);
        if (!full_path) return NULL;
        snprintf(full_path, len, "%s%s", home, path + 1);
        return full_path;
    }
    return strdup(path);
}

int check_directory(const char *path) {
    char *full_path = expand_path(path);
    if (!full_path) return 0;

    struct stat st;
    int exists = (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode));
    free(full_path);
    return exists;
}

void ensure_directory(const char *path) {
    char *full_path = expand_path(path);
    if (!full_path) {
        print_error("Failed to expand path");
        exit(1);
    }

    struct stat st;
    if (stat(full_path, &st) != 0) {
        mkdir(full_path, 0755);
    }

    free(full_path);
}

int install_std() {
    ensure_directory("~/.orta/");

    char *std_x_path = expand_path("~/.orta/std.x");
    if (!std_x_path) {
        print_error("Could not allocate memory for path");
        return 1;
    }

    FILE *fp = fopen(std_x_path, "wb");
    free(std_x_path);

    if (!fp) {
        print_error("Could not open file: ~/.orta/std.x");
        return 1;
    }

    fwrite(std_x, 1, std_x_len, fp);
    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    if (install_std()) {
        return 1;
    }

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
        
        if (!orta_disable_compile) { 
            print_progress("COMPILE", "Creating bytecode file");
            printf(" %s%s%s\n", COLOR_BLUE, bytecode_filename, COLOR_RESET);
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
