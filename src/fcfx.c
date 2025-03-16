#include "orta.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#define VERSION "1.0.0"
#define PROGRAM_NAME "fcfx"

typedef struct {
    bool minimize;
    bool verbose;
    char* input_file;
} ProgramOptions;

const char* short_to_flag(short flag) {
    switch (flag) {
        case FLAG_MEMORY:  return "MEMORY";
        case FLAG_STACK:   return "STACK";
        case FLAG_CMD:     return "CMD";
        default:           return "INVALID";
    }
}

const char* short_to_flag_minimized(short flag) {
    switch (flag) {
        case FLAG_NOTHING: return "U";
        case FLAG_MEMORY:  return "M";
        case FLAG_STACK:   return "S";
        case FLAG_CMD:     return "C";
        default:           return "I";
    }
}

void print_usage(FILE* stream) {
    fprintf(stream, "Usage: %s [OPTIONS] <input-file>\n\n", PROGRAM_NAME);
    fprintf(stream, "Options:\n");
    fprintf(stream, "  -m, --minimize     Output minimized flag format\n");
    fprintf(stream, "  -v, --verbose      Enable verbose output\n");
    fprintf(stream, "  -h, --help         Display this help message\n");
    fprintf(stream, "  -V, --version      Display version information\n");
}

ProgramOptions parse_args(int argc, char *argv[]) {
    ProgramOptions options = {false, false, NULL};
    int opt;
    
    static struct option long_options[] = {
        {"minimize", no_argument, 0, 'm'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "mvhV", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                options.minimize = true;
                break;
            case 'v':
                options.verbose = true;
                break;
            case 'h':
                print_usage(stdout);
                exit(EXIT_SUCCESS);
            case 'V':
                printf("%s version %s\n", PROGRAM_NAME, VERSION);
                exit(EXIT_SUCCESS);
            default:
                print_usage(stderr);
                exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(stderr);
        exit(EXIT_FAILURE);
    }
    
    options.input_file = argv[optind];
    return options;
}

int main(int argc, char *argv[]) {
    ProgramOptions options = parse_args(argc, argv);
    
    if (options.verbose) {
        printf("Analyzing file: %s\n", options.input_file);
    }
    
    OrtaVM vm = ortavm_create(options.input_file);
    if (!load_bytecode(&vm, options.input_file)) {
        fprintf(stderr, "Error: Failed to load xbin file '%s'\n", options.input_file);
        exit(EXIT_FAILURE);
    }

    if (options.verbose) {
        printf("Found %d flags\n", vm.meta.flags_count);
    }

    if (vm.meta.flags_count == 0) {
        printf("No flags found in the file\n");
        return EXIT_SUCCESS;
    }

    for (int i = 0; i < vm.meta.flags_count; i++) {
        if (options.minimize) {
            printf("%s", short_to_flag_minimized(vm.meta.flags[i]));
        } else {
            printf("%s ", short_to_flag(vm.meta.flags[i]));
            if (options.verbose) {
                printf("\n");
            }
        }
    }
    if (!options.verbose) printf("\n");
    
    ortavm_free(&vm);
    return EXIT_SUCCESS;
}

