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

    // Contains orta asm runtime | helpers/runtime.asm
    char orta_runtime_buffer[O_DEFAULT_BUFFER_CAPACITY*8];

    // ASM Header files
    char *header = ";; OVM RUNTIME\n"
                   "%include \"helpers/runtime.asm\"\n"
                   ";; OVM START\n"
                   //"global _start\n" // Uncomment to use ld | where using gcc as the linker
                   "global main\n"
                   "global _entry\n"
                   "_start: ;; Does nothing only jumps to entry point | Because of elf shit convention with _start\n"
                   "\tjmp _entry\n"
                   "main:\n"
                   "\tjmp _entry\n\n"
                   "_entry: ;; Entry point\n";
    FILE *output = fopen("output.asm", "w");
    if (output == NULL) {
        fclose(output);
        fprintf(stderr, "Failed to open file: output.asm!\n");
        return 1;
    }
    fprintf(output, "%s", header);

    // Actual Instruction Handling
    Program program = vm.program;
    assert(INSTRUCTION_COUNT == 43);
    for (size_t i = 0; i < program.size; i++) {
        Token token = program.tokens[i];
        vm.program.current_token = i;
        fprintf(output, "label_%zu:\n", i);
        switch (token.inst) {
            case I_NOP:
				break;
            case I_IGNORE: // TODO: Implement
				fprintf(output, "\t;; TODO: inst::ignore\n");
				break;
            case I_IGNORE_IF: // TODO: Implement
				fprintf(output, "\t;; TODO: inst::ignore_if\n");
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
            case I_JMP_IF: // TODO: Implement
				fprintf(output, "\t;; TODO: inst::jmp_if\n");
				break;
            case I_JMP_IFN: // TODO: Implement
				fprintf(output, "\t;; TODO: inst::jmp_ifn\n");
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
            case I_CLEAR_STACK: // TODO: Implement
				fprintf(output, "\t;; TODO: stack::clear\n");
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
            case I_EXIT_IF: // TODO: Implement
				fprintf(output, "\t;; TODO: exit_if\n");
				break;
            case I_STR_LEN: // TODO: Implement
				fprintf(output, "\t;; TODO: stack::strlen\n");
				break;
            case I_STR_CMP: // TODO: Implement
				fprintf(output, "\t;; TODO: stack::strcmp\n");
				break;
            case I_INPUT: // TODO: Implement
				fprintf(output, "\t;; TODO: io::input\n");
                fprintf(output, "\tcall ifc_orta_input\n");
				break;
            case I_INPUT_STR: // TODO: Implement
                fprintf(output, "\t;; TODO: io::input_str\n");
				break;
            case I_PRINT:
                fprintf(output, "\t;; io::print\n\tpop rdi\n\tcall ifc_orta_print_int\n");
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
            case I_SLEEP: // TODO: Implement
				break;
            case I_EXECUTE_CMD: // TODO: Implement
				break;
            case I_FILE_OPEN: // TODO: Implement
				break;
            case I_FILE_WRITE: // TODO: Implement
				break;
            case I_FILE_READ: // TODO: Implement
				break;
            case I_RANDOM_INT: // TODO: Implement
				break;
            case I_PUSH_CURRENT_INST: // TODO: Implement
				break;
            case I_JMP_FS: // TODO: Implement
				break;
            case I_BFUNC:
				break;
            case I_CAST: // TODO: Implement
				break;
            case I_SEND:
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