// TODO: Implement all new instructions
// Assert is updated but them dont added

// TODO: second argument dont works. PS is 0
// TODO: Optimize the generated assembly

#include "orta.h"
#ifdef _WIN32
    fprintf("Error: Please use linux.\n");
    exit(1);
#endif

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s <file.ovm>\n", argv[0]);
        exit(1);
    }
    OrtaVM vm = {0};
    if (OrtaVM_load_program_from_file(&vm, argv[1]) != RTS_OK) {
        fprintf(stderr, "ERROR: Cannot load this .ovm file\n");
        exit(1);
    }

    // ASM Header files
    char *header = ";; OVM RUNTIME\n"
                   "%include \"helpers/runtime.asm\"\n"
                   ";; OVM START\n"
                   //"global _start\n" // Uncomment to use ld | whe are using gcc as the linker
                   "global main\n"
                   "global _entry\n"
                   "_start: ;; Does nothing only jumps to entry point | Because of elf shit convention with _start\n"
                   "\tjmp _entry\n"
                   "main:\n"
                   "\tjmp _entry\n\n";
    FILE *output = fopen("output.asm", "w");
    if (output == NULL) {
        fclose(output);
        fprintf(stderr, "Failed to open file: output.asm!\n");
        return 1;
    }
    fprintf(output, "%s", header);

    // Actual Instruction Handling
    Program program = vm.program;
    // TODO: char **strings = malloc(program.size * sizeof(char *));
    // TODO: if (strings == NULL) {
    // TODO:     fprintf(stderr, "Memory allocation for strings failed.\n");
    // TODO:     exit(1);
    // TODO: }
    // TODO: int string_count = 0;

    assert(INSTRUCTION_COUNT == 44);
    for (size_t i = 0; i < program.size; i++) {
        Token token = program.tokens[i];
        vm.program.current_token = i;

        fprintf(output, "label_%zu:\n", i);
        if (i == vm.meta.entry_pos) {
            fprintf(output, "_entry: ;; Entry point\n");
            continue;
        }

        switch (token.inst) {
            case I_NOP:
				break;
            case I_IGNORE:
				fprintf(output, "\t;; inst::ignore\n");
				fprintf(output, "\t;; jmp %d\n", i+2);
				break;
            case I_IGNORE_IF:
				fprintf(output, "\t;; inst::ignore_if\n");
				fprintf(output, "\tpop rax\n");
				fprintf(output, "\tcmp rax, %d\n", token.word.as_i64);
				fprintf(output, "\tje label_%d\n", i+2);
				break;
            case I_PUSH:
				fprintf(output, "\t;; stack::push | Integer\n");
				fprintf(output, "\tpush %d\n", token.word.as_i64);
				break;
            case I_PUSH_STR: // TODO: Implement
				fprintf(output, "\t;; TODO: stack::push | String\n");
				break;
            case I_DROP:
				fprintf(output, "\t;; stack::drop\n");
                fprintf(output, "\tadd rsp, 8\n");
				break;
            case I_JMP:
				fprintf(output, "\t;; inst::jmp \n");
				fprintf(output, "\tjmp label_%d\n", token.word.as_i64);
				break;
            case I_JMP_IF:
				fprintf(output, "\t;; inst::jmp_if\n");
				fprintf(output, "\tpop rax\n");
				fprintf(output, "\tcmp rax, %d\n", token.jump_if_args.cond.as_i64);
				fprintf(output, "\tje label_%d\n", token.jump_if_args.dest.as_i64);
				break;
            case I_JMP_IFN:
				fprintf(output, "\t;; inst::jmp_ifn\n");
				fprintf(output, "\tpop rax\n");
                fprintf(output, "\tcmp rax, %d\n", token.jump_if_args.cond.as_i64);
                fprintf(output, "\tjne label_%d\n", token.jump_if_args.dest.as_i64);
				break;
            case I_DUP:
				fprintf(output, "\t;; DUP\n");
                fprintf(output, "\tmov rax, [rsp]\n");
                fprintf(output, "\tsub rsp, 8\n");
                fprintf(output, "\tmov [rsp], rax\n");
				break;
            case I_SWAP:
				fprintf(output, "\t;; SWAP\n");
				fprintf(output, "\tpop rax\n");
				fprintf(output, "\tpop rbx\n");
				fprintf(output, "\tpush rax\n");
				fprintf(output, "\tpush rbx\n");
				break;
            case I_EQ:
                fprintf(output, "\t;; EQ\n");
                fprintf(output, "\tpop rax\n");
                fprintf(output, "\tpop rbx\n");
                fprintf(output, "\tcmp rax, rbx\n");
                fprintf(output, "\tmov rcx, 0\n");
                fprintf(output, "\tsete cl\n");
                fprintf(output, "\tpush rcx\n");
				break;
            case I_LT:
                fprintf(output, "\t;; LT\n");
                fprintf(output, "\tpop rax\n");
                fprintf(output, "\tpop rbx\n");
                fprintf(output, "\tcmp rbx, rax\n");
                fprintf(output, "\tmov rcx, 0\n");
                fprintf(output, "\tsetl cl\n");
                fprintf(output, "\tpush rcx\n");
				break;
            case I_GT:
                fprintf(output, "\t;; GT\n");
                fprintf(output, "\tpop rax\n");
                fprintf(output, "\tpop rbx\n");
                fprintf(output, "\tcmp rbx, rax\n");
                fprintf(output, "\tmov rcx, 0\n");
                fprintf(output, "\tsetg cl\n");
                fprintf(output, "\tpush rcx\n");
				break;
            case I_GET:
                fprintf(output, "\t;; GET\n");
                fprintf(output, "\tmov rdi, %d\n", token.word.as_i64);
                fprintf(output, "\tcall ovm_get\n");
				break;
            case I_CLEAR_STACK:
				fprintf(output, "\t;; stack::clear\n");
				fprintf(output, "\tmov rsp, 0\n");
				break;
            case I_ADD:
                fprintf(output, "\t;; math::add\n");
                fprintf(output, "\tpop rax\n");
                fprintf(output, "\tpop rbx\n");
                fprintf(output, "\tadd rax, rbx\n");
                fprintf(output, "\tpush rax\n");
				break;
            case I_SUB:
                fprintf(output, "\t;; math::sub\n");
                fprintf(output, "\tpop rax\n");
                fprintf(output, "\tpop rbx\n");
                fprintf(output, "\tsub rax, rbx\n");
                fprintf(output, "\tpush rax\n");
				break;
            case I_MUL:
                fprintf(output, "\t;; math::mul\n");
                fprintf(output, "\tpop rbx\n");
                fprintf(output, "\tpop rax\n");
                fprintf(output, "\tmul rbx\n");
                fprintf(output, "\tpush rax\n");
				break;
            case I_DIV:
                fprintf(output, "\t;; math::div\n");
                fprintf(output, "\tpop rbx\n");
                fprintf(output, "\tpop rax\n");
                fprintf(output, "\txor rdx, rdx\n");
                fprintf(output, "\tdiv rbx\n");
                fprintf(output, "\tpush rax\n");
				break;
            case I_EXIT:
                fprintf(output, "\t;; exit\n");
                fprintf(output, "\tovm_exit %d\n", token.word.as_i64);
				break;
            case I_EXIT_IF: // TODO: The second argument(condition) is every time 0
				fprintf(output, "\t;; FIX: exit_if %d %d\n", token.exit_if.exit_code.as_i64, token.exit_if.cond.as_i64);
				fprintf(output, "\tpop rax\n");
				fprintf(output, "\tcmp rax, %ld\n", token.exit_if.cond.as_i64);
				fprintf(output, "\tje .skip_exit\n");
				fprintf(output, "\tovm_exit %ld\n", token.exit_if.exit_code.as_i64);
				fprintf(output, "\t.skip_exit:\n");
				break;
            case I_STR_LEN: // TODO: Implement
				fprintf(output, "\t;; TODO: stack::strlen\n");
				break;
            case I_STR_CMP: // TODO: Implement
				fprintf(output, "\t;; TODO: stack::strcmp\n");
				break;
            case I_INPUT: // TODO: Fix
				fprintf(output, "\t;; FIX: io::input\n");
                fprintf(output, "\tcall orta_input\n");
				break;
            case I_INPUT_STR: // TODO: Implement
                fprintf(output, "\t;; TODO: io::input_str\n");
				break;
            case I_PRINT:
                fprintf(output, "\t;; io::print\n");
                fprintf(output, "\tpop rdi\n");
                fprintf(output, "\tcall ifc_orta_print_int\n");
                break;
            case I_CAST_AND_PRINT: // TODO: Implement
				fprintf(output, "\t;; TODO: io::cprint\n");
				break;
            case I_PRINT_STR: // TODO: Implement
				fprintf(output, "\t;; TODO: io::print_str\n");
				break;
            case I_COLOR: // TODO: Implement
                fprintf(output, "\t;; TODO: io::color\n");
				break;
            case I_CLEAR: // TODO: Implement
				fprintf(output, "\t;; TODO: io::clear\n");
				break;
            case I_SLEEP:
				fprintf(output, "\t;; sleep\n");
				fprintf(output, "\tmov edx, %d\n", token.word.as_i64);
				fprintf(output, "\tcall orta_sleep\n");
				break;
            case I_EXECUTE_CMD: // TODO: Implement
				break;
            case I_FILE_OPEN: // TODO: Implement
				break;
            case I_FILE_WRITE: // TODO: Implement
				break;
            case I_FILE_READ: // TODO: Implement
				break;
            case I_RANDOM_INT:
                fprintf(output, "\t;; math::random_int \n");
                fprintf(output, "\tmov dx, %d\n", token.random_int.min.as_i64);
                fprintf(output, "\tmov cx, %d\n", token.random_int.max.as_i64);
                fprintf(output, "\tcall orta_random_int\n");
				break;
            case I_PUSH_CURRENT_INST:
				fprintf(output, "\t;; inst::current\n");
				fprintf(output, "\tpush %d\n", i);
				break;
            case I_JMP_FS:
				fprintf(output, "\t;; inst::jmp_fs\n");
				fprintf(output, "\tpop rax\n");
				fprintf(output, "\tjmp rax\n");
				break;
            case I_BFUNC:
				break;
            case I_CAST: // TODO: Implement
				break;
            case I_GET_DATE: // TODO: Implement
				break;
        }
    }

    fprintf(output, "\n\n;; OVM END\n");
    fprintf(output, "ovm_exit 0\n");

    fclose(output);
    return 0;
}