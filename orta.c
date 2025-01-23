#include "orta.h"

void usage(char *program_name) {
    fprintf(stderr, "| OrtaVM\n", program_name);
    fprintf(stderr, "| Usage: %s <orta_code_file.orta/orta_vm_file.ovm> <ovm_output.ovm/deovm_output.orta> [flags]\n", program_name);
    fprintf(stderr, "|> Flags:\n");
    fprintf(stderr, "|    --build-only          Build only without execution.\n");
    fprintf(stderr, "|    --show-warnings       Show warnings (sets warning level to 1).\n");
    fprintf(stderr, "|    --dump                Dumps the stack\n");
    fprintf(stderr, "|    -l <limit>            Set limit value. (Default: 1000)\n");
    fprintf(stderr, "|    -i <path>             Add a include path (U can use ~ for home)\n");
    fprintf(stderr, "|    --target <target>     Set an compilation target (windows|posix)\n");
    fprintf(stderr, "|    --support=<feature>   Enables a feature\n");
    fprintf(stderr, "|> Features:");
    fprintf(stderr, "|    uppercase             Enables uppercase Instructions\n");
}

int main(int argc, char **argv) {
    OrtaVM vm = {0};
    int build_only = 0;
    char orta_args[20][256] = {0};
    int orta_argc = 0;
    int is_orta_args = 0;
    int dump = 0;
    int test = 0;
    int target = 0;
    if (argc < 2) {
        usage(argv[0]);
        fprintf(stderr, "ERROR: No input file provided.\n");
        return 1;
    }

    for (int i = 3; i < argc; i++) {
         if (strcmp(argv[i], "--") == 0) {
             is_orta_args = 1;
             continue;
         }
         if (is_orta_args) {
            strcpy(orta_args[orta_argc++], argv[i]);
            continue;
         } else if (strcmp(argv[i], "--build-only") == 0) {
             build_only = 1;
         } else if (strcmp(argv[i], "--show-warnings") == 0) {
             level = 1;
         } else if (strcmp(argv[i], "-l") == 0) {
             if (i + 1 < argc) {
                 limit = atoi(argv[++i]);
             } else {
                 fprintf(stderr, "ERROR: Missing value for -l option.\n");
                 return 1;
             }
         } else if (strcmp(argv[i], "--dump") == 0) {
            dump = 1;
         } else if (strcmp(argv[i], "--test") == 0) {
            test = 1;
         } else if (strncmp(argv[i], "--support=", 10) == 0) {
            char *word = strchr(argv[i], '=') + 1;
            if (strcmp(word, "uppercase") == 0) {
                support_uppercase = 1;
            } else {
                fprintf(stderr, "ERROR: Invalid value for -support option.\n");
                return 1;
            }
         } else if (strcmp(argv[i], "-i") == 0) {
            char *path;
            if (i + 1 < argc) {
                path = strdup(argv[++i]);
            } else {
                fprintf(stderr, "ERROR: Missing path for -i option.\n");
                return 1;
            }
            OrtaVM_add_include_path(path);
         } else if (strcmp(argv[i], "--target") == 0) {
            if (i + 1 < argc) {
                if (strcmp(argv[++i], "windows") == 0) {
                    target = 1;
                } else if (strcmp(argv[i], "posix") == 0) {
                    target = 2;
                }
            } else {
                fprintf(stderr, "ERROR: Missing target for --target option.\n");
                return 1;
            }
         } else {
             fprintf(stderr, "ERROR: Unknown option '%s'.\n", argv[i]);
             return 1;
         }
    }

    if (ends_with(argv[1], ".ovm")) {
        if (OrtaVM_load_program_from_file(&vm, argv[1]) != RTS_OK) {
            fprintf(stderr, "ERROR: Failed to load .ovm file: %s\n", argv[1]);
            return 1;
        }
    } else if (ends_with(argv[1], ".orta")) {
        if (argc < 3) {
            fprintf(stderr, "ERROR: Missing output file for .orta input.\n");
            return 1;
        }
        if (OrtaVM_preprocess_file(argv[1], orta_argc, orta_args, target) != RTS_OK) {
            fprintf(stderr, "ERROR: Failed to preprocess file: %s", argv[1]);
            return 1;
        }

        char filename[256];
        snprintf(filename, sizeof(filename), "%s.ppd", argv[1]);

        if (OrtaVM_parse_program(&vm, filename) != RTS_OK) {
            fprintf(stderr, "ERROR: Failed to parse .orta file: %s\n", argv[1]);
            return 1;
        }
        // DONE: Make it os independent
        #ifdef _WIN32
            system("del *.ppd");
        #else
            system("rm *.ppd");
        #endif
        if (OrtaVM_save_program_to_file(&vm, argv[2]) != RTS_OK) {
            fprintf(stderr, "ERROR: Failed to save .ovm file: %s\n", argv[2]);
            return 1;
        }
        if (build_only) {
            return 0;
        }
    } else if (ends_with(argv[1], ".horta")) {
        fprintf(stderr, "ERROR: .horta files cannot be executen!\nNOTE: .horta files can be included: #include <%s>\n", argv[1]);
        return 1;
    } else {
        fprintf(stderr, "ERROR: Invalid file extension. Expected .orta or .ovm.\n");
        return 1;
    }


    RTStatus status = OrtaVM_execute(&vm);
    if (status != RTS_OK) {
        fprintf(stderr, "Execution Error: %s\n", error_to_string(status));
        return 1;
    }

    // if (test) ;

    if (dump) OrtaVM_dump(&vm);

    return 0;
}