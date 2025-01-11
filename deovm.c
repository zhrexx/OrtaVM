#include "orta.h"

int main(int argc, char **argv) {
    OrtaVM vm = {0};

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <orta_vm_file.ovm> <output.orta>\n", argv[0]);
        fprintf(stderr, "ERROR: No input file provided.\n");
        return 1;
    }
    if (OrtaVM_load_program_from_file(&vm, argv[1]) != RTS_OK) {
        fprintf(stderr, "ERROR: Failed to load .ovm file: %s\n", argv[1]);
        return 1;
    }
    char *output = argv[2];
    FILE *f = fopen(output, "w");
    for (size_t i = 0; i < vm.program.size; i++) {
        Token token = vm.program.tokens[i];
        switch (token.inst) {
           case I_NOP:
                fprintf(f, "NOP\n");
                break;
           case I_PUSH:
                fprintf(f, "PUSH %d\n", token.int_value);
                break;
           case I_DROP:
                fprintf(f, "DROP\n");
                break;
           case I_JMP:
                fprintf(f, "JMP %d\n", token.int_value);
                break;
           case I_JMP_IF:
                fprintf(f, "JMP_IF %d %d\n", token.jump_if_args.dest, token.jump_if_args.cond);
                break;
           case I_DUP:
                fprintf(f, "DUP\n");
                break;
           case I_SWAP:
                fprintf(f, "SWAP\n");
                break;
           case I_EQ:
                fprintf(f, "EQ\n");
                break;
           case I_CLEAR_STACK:
                fprintf(f, "CLEAR_STACK\n");
                break;
           case I_ADD:
                fprintf(f, "ADD\n");
                break;
           case I_SUB:
                fprintf(f, "SUB\n");
                break;
           case I_MUL:
                fprintf(f, "MUL\n");
                break;
           case I_DIV:
                fprintf(f, "DIV\n");
                break;
           case I_EXIT:
                fprintf(f, "EXIT %d\n", token.int_value);
                break;
           case I_INPUT:
                fprintf(f, "INPUT\n");
                break;
           case I_PRINT:
                fprintf(f, "PRINT\n");
                break;
           case I_CAST_AND_PRINT:
                fprintf(f, "CAST_AND_PRINT\n");
                break;
           case I_PRINT_STR:
                fprintf(f, "PRINT_STR \"%s\"\n", token.str_value);
                break;
           case I_COLOR:
                fprintf(f, "COLOR \"%s\"\n", token.str_value);
                break;
           case I_CLEAR:
                fprintf(f, "CLEAR\n");
                break;
           case I_SLEEP:
                fprintf(f, "SLEEP %d\n", token.int_value);
                break;
           case I_FILE_OPEN:
                fprintf(f, "FILE_OPEN \"%s\"\n", token.str_value);
                break;
           case I_FILE_WRITE:
                fprintf(f, "FILE_WRITE \"%s\"\n", token.str_value);
                break;
           case I_FILE_READ:
                fprintf(f, "FILE_READ\n");
                break;
           case I_RANDOM_INT:
                fprintf(f, "RANDOM_INT %d %d\n", token.random_int.min, token.random_int.max);
                break;
        }

    }
    fclose(f);
    return 0;
}
