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
           case I_JMP_IFN:
                fprintf(f, "JMP_IFN %d %d\n", token.jump_if_args.dest, token.jump_if_args.cond);
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
           case I_LT:
                fprintf(f, "LT\n");
                break;
           case I_GT:
                fprintf(f, "GT\n");
                break;
           case I_PUSH_STR:
                fprintf(f, "PUSH_STR \"%s\"\n", token.str_value);
                break;
           case I_EXIT_IF:
                fprintf(f, "EXIT_IF %d %d\n", token.exit_if.exit_code, token.exit_if.cond);
                break;
           case I_STR_LEN:
                fprintf(f, "STRLEN\n");
                break;
           case I_INPUT_STR:
                fprintf(f, "INPUT_STR\n");
                break;
           case I_EXECUTE_CMD:
                fprintf(f, "EXECUTE_CMD \"%s\"\n", token.str_value);
                break;
           case I_IGNORE:
                fprintf(f, "IGNORE\n");
                break;
           case I_IGNORE_IF:
                fprintf(f, "IGNORE_IF %d\n", token.int_value);
                break;
           case I_GET:
                fprintf(f, "GET %d\n", token.int_value);
                break;
           case I_STR_CMP:
                fprintf(f, "STRCMP \"%s\"\n", token.str_value);
                break;
           case I_PUSH_CURRENT_INST:
                fprintf(f, "PUSH_CURRENT\n");
           case I_JMP_FS:
                fprintf(f, "JMP_FS\n");
           case I_BFUNC:
                fprintf(f, "BFUNC %d\n", token.int_value);
           case I_CAST:
                fprintf(f, "CAST %s\n", token.str_value);
           case I_SEND:
                fprintf(f, "SEND \"%s\"\n", token.str_value);
        }

    }
    fclose(f);
    return 0;
}
