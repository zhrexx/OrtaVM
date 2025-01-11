#include "orta.h"

int main(int argc, char **argv) {
    OrtaVM vm = {0};

    int build_only = 0;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <orta_code_file.orta/orta_vm_file.ovm> [ovm_output.ovm]\n", argv[0]);
        fprintf(stderr, "ERROR: No input file provided.\n");
        return 1;
    }
    if (argc > 3 && strcmp(argv[3], "--build-only") == 0) {
        build_only = 1;
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
        if (OrtaVM_parse_program(&vm, argv[1]) != RTS_OK) {
            fprintf(stderr, "ERROR: Failed to parse .orta file: %s\n", argv[1]);
            return 1;
        }
        if (OrtaVM_save_program_to_file(&vm, argv[2]) != RTS_OK) {
            fprintf(stderr, "ERROR: Failed to save .ovm file: %s\n", argv[2]);
            return 1;
        }
        if (build_only) {
            return 0;
        }
    } else {
        fprintf(stderr, "ERROR: Invalid file extension. Expected .orta or .ovm.\n");
        return 1;
    }


    RTStatus status = OrtaVM_execute(&vm);
    if (status != RTS_OK) {
        fprintf(stderr, "Execution Error: %d\n", status);
        return 1;
    }

    if (argc > 3 && strcmp(argv[3], "--dump") == 0) {
        OrtaVM_dump(&vm);
    }

    return 0;
}