#include "config.h"
#include "orta.h"

static OrtaVM vm;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: xd <file.xbin>\n");
        return 1;
    }

    vm = ortavm_create("debug");
    if (!load_xbin(&vm, argv[1])) {
        fprintf(stderr, "%sError:%s Failed to load bytecode\n", COLOR_RED, COLOR_RESET);
        return 1;
    }

    for (size_t i = 0; i < vm.program.instructions_count; i++) {
        if (get_label_at_pos(&vm, i)) {
            printf("%s: \n", get_label_at_pos(&vm, i));
            continue;
        }
        InstructionData *instr = &vm.program.instructions[i];

        printf("    %s ", instruction_to_string(instr->opcode));

        VECTOR_FOR_EACH(char*, op, &instr->operands) {
            printf("%s ", *op);
        }
        printf("\n");
    }
    printf("\n");
}
