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
    assert(INSTRUCTION_COUNT == 45);
    for (size_t i = 0; i < vm.program.size; i++) {
        Token token = vm.program.tokens[i];
        switch (token.inst) {
           case I_NOP:
                fprintf(f, "nop\n");
                break;
           case I_PUSH:
                fprintf(f, "stack::push %d\n", token.word.as_i64);
                break;
           case I_DROP:
                fprintf(f, "stack::drop\n");
                break;
           case I_JMP:
                fprintf(f, "inst::jmp %d\n", token.word.as_i64);
                break;
           case I_JMP_IF:
                fprintf(f, "inst::jmp_if %d %d\n", token.jump_if_args.dest, token.jump_if_args.cond);
                break;
           case I_JMP_IFN:
                fprintf(f, "inst::jmp_ifn %d %d\n", token.jump_if_args.dest, token.jump_if_args.cond);
                break;
           case I_DUP:
                fprintf(f, "stack::dup\n");
                break;
           case I_SWAP:
                fprintf(f, "stack::swap\n");
                break;
           case I_EQ:
                fprintf(f, "stack::eq\n");
                break;
           case I_CLEAR_STACK:
                fprintf(f, "stack::clear\n");
                break;
           case I_ADD:
                fprintf(f, "math::add\n");
                break;
           case I_SUB:
                fprintf(f, "math::sub\n");
                break;
           case I_MUL:
                fprintf(f, "math::mul\n");
                break;
           case I_DIV:
                fprintf(f, "math::div\n");
                break;
           case I_EXIT:
                fprintf(f, "exit %d\n", token.word.as_i64);
                break;
           case I_INPUT:
                fprintf(f, "io::input\n");
                break;
           case I_PRINT:
                fprintf(f, "io::print\n");
                break;
           case I_CAST_AND_PRINT:
                fprintf(f, "io::cprint\n");
                break;
           case I_PRINT_STR:
                fprintf(f, "io::print \"%s\"\n", token.word.as_str);
                break;
           case I_COLOR:
                fprintf(f, "io::color \"%s\"\n", token.word.as_str);
                break;
           case I_CLEAR:
                fprintf(f, "io::clear\n");
                break;
           case I_SLEEP:
                fprintf(f, "io::sleep %d\n", token.word.as_i64);
                break;
           case I_FILE_OPEN:
                fprintf(f, "file::open \"%s\"\n", token.word.as_str);
                break;
           case I_FILE_WRITE:
                fprintf(f, "file::write \"%s\"\n", token.word.as_str);
                break;
           case I_FILE_READ:
                fprintf(f, "file::read\n");
                break;
           case I_RANDOM_INT:
                fprintf(f, "math::random_int %d %d\n", token.random_int.min, token.random_int.max);
                break;
           case I_LT:
                fprintf(f, "stack::lt\n");
                break;
           case I_GT:
                fprintf(f, "stack::gt\n");
                break;
           case I_PUSH_STR:
                fprintf(f, "io::push \"%s\"\n", token.word.as_str);
                break;
           case I_EXIT_IF:
                fprintf(f, "exit_if %d %d\n", token.exit_if.exit_code, token.exit_if.cond);
                break;
           case I_STR_LEN:
                fprintf(f, "stack::strlen\n");
                break;
           case I_INPUT_STR:
                fprintf(f, "io::input_str\n");
                break;
           case I_EXECUTE_CMD:
                fprintf(f, "io::execute \"%s\"\n", token.word.as_str);
                break;
           case I_IGNORE:
                fprintf(f, "inst::ignore\n");
                break;
           case I_IGNORE_IF:
                fprintf(f, "inst::ignore_if %d\n", token.word.as_i64);
                break;
           case I_GET:
                fprintf(f, "stack::get %d\n", token.word.as_i64);
                break;
           case I_STR_CMP:
                fprintf(f, "stack::strcmp \"%s\"\n", token.word.as_str);
                break;
           case I_PUSH_CURRENT_INST:
                fprintf(f, "inst::current\n");
                break;
           case I_JMP_FS:
                fprintf(f, "inst::jmp_fs\n");
                break;
           case I_BFUNC:
                fprintf(f, "c::bfunc %d\n", token.word.as_i64);
                break;
           case I_CAST:
                fprintf(f, "stack::cast %s\n", token.word.as_str);
                break;
           case I_SEND:
                fprintf(f, "net::send \"%s\"\n", token.word.as_str);
                break;
           case I_GET_DATE:
                fprintf(f, "time::today\n");
                break;
           case I_CALL:
                fprintf(f, "call %d\n", token.word.as_i64);
           case I_RET:
                fprintf(f, "ret\n");
        }
    }
    fclose(f);
    return 0;
}
