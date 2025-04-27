#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include "orta.h"
#include "std.h"
#include "config.h"
#include "asm.h"

#ifdef WIN32 
#include <windows.h>
#else 
#include <unistd.h> 
#endif

typedef struct {
    bool help;
    bool no_preproc;
    bool disable_compile;
    bool show_version;
    bool debug;
    bool notdeletepreprocessed;
    bool only_compile;
    const char* input_file;
} ProgramOptions;

void print_usage(const char *program_name) {
    printf("%s%s%s%s\n", COLOR_BOLD, COLOR_CYAN, LOGO, COLOR_RESET);
    printf("\n%s%sUSAGE: %s %s<program.x|program.xbin>%s [options]\n", 
           COLOR_BOLD, COLOR_MAGENTA, program_name, COLOR_BLUE, COLOR_RESET);
    
    printf("\n%s%sOPTIONS:%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("  %s-h, --help%s           Display this help message\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--nopreproc%s          Disable source file preprocessing\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--notdeletepreprocessed%s          Disable preprocessed file deletion\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--disable-compile%s    Disable bytecode creation\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--only-compile%s       Only compiles no run\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--version%s            Display version information\n", COLOR_BLUE, COLOR_RESET);
    printf("  %s--debug%s              Show detailed execution information\n", COLOR_BLUE, COLOR_RESET);
    
    printf("\n%s%sEXAMPLES:%s\n", COLOR_BOLD, COLOR_MAGENTA, COLOR_RESET);
    printf("  %s example.x           %s# Run source with preprocessing\n", program_name, COLOR_GREEN);
    printf("  %s example.xbin        %s# Run pre-compiled bytecode\n", program_name, COLOR_GREEN);
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

ProgramOptions parse_arguments(int argc, char **argv) {
    ProgramOptions options = {
        .help = false,
        .no_preproc = false,
        .disable_compile = false,
        .show_version = false,
        .debug = false,
        .notdeletepreprocessed = false,
        .only_compile = false,
        .input_file = NULL
    };
    
    if (argc < 2) {
        options.help = true;
        return options;
    }
    
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        options.help = true;
        return options;
    } else if (argv[1][0] != '-') {
        options.input_file = argv[1];
    }
    
    for (int i = (options.input_file ? 2 : 1); i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            options.help = true;
        } else if (strcmp(argv[i], "--nopreproc") == 0) {
            options.no_preproc = true;
        } else if (strcmp(argv[i], "--notdeletepreprocessed") == 0) {
            options.notdeletepreprocessed = true;
        } else if (strcmp(argv[i], "--disable-compile") == 0) {
            options.disable_compile = true;
        } else if (strcmp(argv[i], "--only-compile") == 0) {
            options.only_compile = true;
        } else if (strcmp(argv[i], "--version") == 0) {
            options.show_version = true;
        } else if (strcmp(argv[i], "--debug") == 0) {
            options.debug = true;
        } else if (argv[i][0] != '-' && options.input_file == NULL) {
            options.input_file = argv[i];
        }
    }
    
    return options;
}

int main(int argc, char **argv) {
    if (install_std()) {
        return 1;
    }

    ProgramOptions options = parse_arguments(argc, argv);
    
    if (options.help) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    
    if (options.show_version) {
        printf("Orta v%.1f\n", _VERSION);
        return EXIT_SUCCESS;
    }
    
    if (options.input_file == NULL) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    if (options.debug) {
        print_progress("INIT", "Initializing virtual machine");
        print_info(options.input_file);
    }
    
    OrtaVM vm = ortavm_create(options.input_file);
    
    const char *filename = options.input_file;
    size_t len = strlen(filename);
    bool is_bytecode = false;
    
    if (len > 5 && strcmp(filename + len - 5, ".xbin") == 0) {
        is_bytecode = true;
        if (options.debug) {
            print_progress("LOAD", "Loading compiled bytecode");
        }
        
        if (!load_bytecode(&vm, filename)) {
            print_error("Failed to load bytecode file");
            ortavm_free(&vm);
            return EXIT_FAILURE;
        }
    } else if (len > 2 && strcmp(filename + len - 2, ".x") == 0) {
        if (!options.no_preproc) {
            if (options.debug) {
                print_progress("PREPROC", "Preprocessing source file");
            }
            
            char preprocessed_filename[256];
            snprintf(preprocessed_filename, sizeof(preprocessed_filename), "%.*s.pre.x", (int)(len - 2), filename);
            
            if (!orta_preprocess((char*)filename, preprocessed_filename)) {
                print_error("Preprocessing failed");
                ortavm_free(&vm);
                return EXIT_FAILURE;
            }
            
            if (options.debug) {
                print_progress("PARSE", "Parsing preprocessed source");
            }
            
            if (!parse_program(&vm, preprocessed_filename)) {
                print_error("Failed to parse preprocessed source");
                ortavm_free(&vm);
                return EXIT_FAILURE;
            }
            if (!options.notdeletepreprocessed) {
#ifdef WIN32
                char *resolved = malloc(PATH_MAX);
                if (resolved && _fullpath(resolved, preprocessed_filename, PATH_MAX)) {
                    _unlink(resolved);
                }
#else 
                char *resolved = realpath(preprocessed_filename, NULL);
                unlink(resolved);
#endif
                free(resolved);
            }
        } else {
            if (options.debug) {
                print_progress("PARSE", "Parsing source program (preprocessing disabled)");
            }
            
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
    
    if (options.debug) {
        print_progress("EXEC", "Executing program instructions");
    }
    if (!options.only_compile) {
        time_t start = time(NULL);
        execute_program(&vm);
        time_t end = time(NULL);
    
        printf("EXECUTION COMPLETED IN %ds\n", (int)(end - start));

        if (options.debug) {
            printf("\nSTACK STATE\n");
            print_stack(&vm.xpu);
        
            printf("\nREGISTERS STATE\n");
            print_registers(&vm.xpu);
        }
    }
    if (!is_bytecode && !options.disable_compile) {
        char bytecode_filename[256];
        snprintf(bytecode_filename, sizeof(bytecode_filename), "%.*s.xbin", (int)(len - 2), filename);
        
        if (options.debug) {
            print_progress("COMPILE", "Creating bytecode file");
            printf(" %s%s%s\n", COLOR_BLUE, bytecode_filename, COLOR_RESET);
        }
        
        if (create_bytecode(&vm, bytecode_filename)) {
            if (options.debug) {
                print_success("Bytecode created successfully");
            }
        } else {
            print_error("Failed to create bytecode file");
        }
    }
    int exit_code = vm.program.exit_code;
    ortavm_free(&vm);
    
    return exit_code;
}
