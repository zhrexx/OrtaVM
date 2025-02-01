// FCfO - Flag Checker for Orta
#include "orta.h"

int main(int argc, char **argv) {
    // TODO: Implement -f [-f <max>]
    if (argc < 2) {
        printf("Usage: %s <input.ovm> [output.txt]\n", argv[0]);
        return 1;
    }
    FILE *output = stdout;
    if (argc == 3) {
        FILE *output = fopen(argv[2], "w");
    }

    OrtaVM vm = {0};
    RTStatus lstatus = OrtaVM_load_program_from_file(&vm, argv[1]);
    if (lstatus != RTS_OK) {
        error_occurred("fcfo", 0, "LOADING_OVM", "Could not load binary");
    }
    for (int i = 0; i < vm.meta.flags_size; i++) {
        fprintf(output, "%d\n", vm.meta.flags[i]);
    }

    return 0;
}
