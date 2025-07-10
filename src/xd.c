#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "orta.h"

#define MAX_BREAKPOINTS 64
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_BOLD    "\x1b[1m"
#define COLOR_DIM     "\x1b[2m"

static OrtaVM vm;
static size_t breakpoints[MAX_BREAKPOINTS];
static int bp_count = 0;
static int running = 1;

void print_header() {
    printf("\n%s XBin Debugger v1.0 - Orta VM %s\n\n", COLOR_CYAN, COLOR_RESET);
    printf("Type %shelp%s for a list of commands\n\n", COLOR_YELLOW, COLOR_RESET);
}

void print_prompt() {
    printf("%s⟫%s ", COLOR_MAGENTA, COLOR_RESET);
    fflush(stdout);
}

char **split_args(const char *input, int *count) {
    Vector v;
    vector_init(&v, 8, sizeof(char*));
    char *token = strtok((char*)input, " ");
    while (token) {
        char *copy = strdup(token);
        vector_push(&v, &copy);
        token = strtok(NULL, " ");
    }
    *count = v.size;
    return (char**)v.data;
}

void add_breakpoint(size_t address) {
    if (bp_count >= MAX_BREAKPOINTS) {
        printf("%sError:%s Too many breakpoints (max: %d)\n", COLOR_RED, COLOR_RESET, MAX_BREAKPOINTS);
        return;
    }
    breakpoints[bp_count++] = address;
    printf("%sBreakpoint set at 0x%zx%s\n", COLOR_GREEN, address, COLOR_RESET);
}

int is_breakpoint(size_t ip) {
    for (int i = 0; i < bp_count; i++) {
        if (breakpoints[i] == ip) return 1;
    }
    return 0;
}

void list_breakpoints() {
    if (bp_count == 0) {
        printf("No breakpoints set\n");
        return;
    }
    
    printf("%sBreakpoints:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("Idx | Address\n");
    printf("----------------\n");
    
    for (int i = 0; i < bp_count; i++) {
        printf("%s%3d%s | 0x%-8zx\n", 
               is_breakpoint(vm.xpu.ip) && breakpoints[i] == vm.xpu.ip ? COLOR_RED : "", 
               i, COLOR_RESET, breakpoints[i]);
    }
    printf("\n");
}

void delete_breakpoint(int index) {
    if (index < 0 || index >= bp_count) {
        printf("%sError:%s Invalid breakpoint index\n", COLOR_RED, COLOR_RESET);
        return;
    }
    memmove(&breakpoints[index], &breakpoints[index+1], 
            (bp_count - index - 1) * sizeof(size_t));
    bp_count--;
    printf("%sBreakpoint %d removed%s\n", COLOR_GREEN, index, COLOR_RESET);
}

void cmd_break(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %sbreak <address>%s\n", COLOR_YELLOW, COLOR_RESET);
        return;
    }
    size_t addr = strtoul(argv[1], NULL, 0);
    add_breakpoint(addr);
}

void cmd_delete(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %sdelete <index>%s\n", COLOR_YELLOW, COLOR_RESET);
        return;
    }
    int index = atoi(argv[1]);
    delete_breakpoint(index);
}

void cmd_step() {
    if (vm.xpu.ip >= vm.program.instructions_count) {
        printf("%sProgram finished%s\n", COLOR_GREEN, COLOR_RESET);
        running = 0;
        return;
    }
    
    printf("\n%s→ ", COLOR_BLUE);
    InstructionData *instr = &vm.program.instructions[vm.xpu.ip];
    printf("%s%s%s ", COLOR_YELLOW, instruction_to_string(instr->opcode), COLOR_RESET);
    VECTOR_FOR_EACH(char*, op, &instr->operands) {
        printf("%s ", *op);
    }
    printf("%s\n", COLOR_RESET);
    
    execute_instruction(&vm, instr);
    
    if (vm.xpu.ip >= vm.program.instructions_count) {
        printf("%sProgram finished%s\n", COLOR_GREEN, COLOR_RESET);
        running = 0;
    }
}

void cmd_continue() {
    while (running && !is_breakpoint(vm.xpu.ip)) {
        if (vm.xpu.ip >= vm.program.instructions_count) {
            printf("%sProgram finished%s\n", COLOR_GREEN, COLOR_RESET);
            running = 0;
            break;
        }
        execute_instruction(&vm, &vm.program.instructions[vm.xpu.ip]);
    }
}

void cmd_registers() {
    printf("%sRegisters:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("Register | Value\n");
    printf("------------------\n");
    
    printf("IP       | 0x%-12zx\n", vm.xpu.ip);
    printf("SP       | 0x%-12zx\n", vm.xpu.stack.count);
    
    for (int i = 0; i < REG_COUNT; i++) {
        Word w = vm.xpu.registers[i].reg_value;
        printf("%-8s | ", register_table[i].name);
        
        switch (w.type) {
            case WINT:
                printf("INT: %-10d\n", w.as_int);
                break;
            case WFLOAT:
                printf("FLT: %-10.2f\n", w.as_float);
                break;
            case WCHARP:
                printf("STR: %-10s\n", w.as_string);
                break;
            case W_CHAR:
                printf("CHR: %-10c\n", w.as_char);
                break;
            case WPOINTER:
                printf("PTR: %-10p\n", w.as_pointer);
                break;
            case WBOOL:
                printf("BOOL: %-9s\n", (w.as_bool) ? "true" : "false");
                break;
            default:
                printf("%-14s\n", "EMPTY");
        }
    }
    printf("\n");
}

void cmd_stack(int argc, char **argv) {
    int n = 5;
    if (argc >= 2) n = atoi(argv[1]);
    
    if (vm.xpu.stack.count == 0) {
        printf("Stack is empty\n");
        return;
    }
    
    printf("%sStack (top %d):%s\n", COLOR_BOLD, n, COLOR_RESET);
    printf("Index | Value\n");
    printf("----------------\n");
    
    for (int i = 0; i < n && i < vm.xpu.stack.count; i++) {
        printf("%s%5d%s | ", 
               i == 0 ? COLOR_GREEN : "", 
               vm.xpu.stack.count - 1 - i,
               COLOR_RESET);
        printf("[%d] ", i);
        print_word(vm.xpu.stack.stack[vm.xpu.stack.count - 1 - i]);
        printf("\n");
    }
    printf("\n");
}

void cmd_memory(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %smemory <address> <size>%s\n", COLOR_YELLOW, COLOR_RESET);
        return;
    }
    void *addr = (void*)strtoul(argv[1], NULL, 0);
    int size = atoi(argv[2]);
    
    printf("%sMemory at %p:%s\n", COLOR_BOLD, addr, COLOR_RESET);
    
    for (int i = 0; i < size; i++) {
        if (i % 16 == 0) {
            if (i > 0) printf("\n");
            printf("%s%04x:%s ", COLOR_BLUE, i, COLOR_RESET);
        }
        
        unsigned char byte = ((unsigned char*)addr)[i];
        printf("%02x%s", byte, (i % 2 == 1) ? " " : "");
        
        if (i % 16 == 15) {
            printf(" %s|%s ", COLOR_DIM, COLOR_RESET);
            for (int j = i - 15; j <= i; j++) {
                unsigned char c = ((unsigned char*)addr)[j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
        }
    }
    printf("\n");
}

void cmd_disassemble() {
    printf("%sCurrent IP: 0x%zx%s\n", COLOR_BOLD, vm.xpu.ip, COLOR_RESET);
    printf("Curr | Address   | Instruction\n");
    printf("-------------------------------\n");
    
    for (int i = -2; i <= 2; i++) {
        size_t ip = vm.xpu.ip + i;
        if (ip >= vm.program.instructions_count) continue;
        
        InstructionData *instr = &vm.program.instructions[ip];
        
        printf("%s%s%s | 0x%-7zx | %s%-10s%s ", 
               (i == 0) ? COLOR_GREEN : "",
               (i == 0) ? "→" : " ",
               COLOR_RESET,
               ip,
               (i == 0) ? COLOR_YELLOW : "",
               instruction_to_string(instr->opcode),
               COLOR_RESET);
        
        VECTOR_FOR_EACH(char*, op, &instr->operands) {
            printf("%s ", *op);
        }
        printf("\n");
    }
    printf("\n");
}

void cmd_help() {
    printf("%sAvailable commands:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("Command        | Description\n");
    printf("----------------------------------------------------\n");
    printf("%sbreak <addr>%s   | Set breakpoint at specified address\n", COLOR_YELLOW, COLOR_RESET);
    printf("%slist%s           | List all breakpoints\n", COLOR_YELLOW, COLOR_RESET);
    printf("%sdelete <idx>%s   | Delete breakpoint by index\n", COLOR_YELLOW, COLOR_RESET);
    printf("%sstep%s           | Execute next instruction\n", COLOR_YELLOW, COLOR_RESET);
    printf("%scontinue%s       | Continue execution until breakpoint or end\n", COLOR_YELLOW, COLOR_RESET);
    printf("%sregisters%s      | Display registers\n", COLOR_YELLOW, COLOR_RESET);
    printf("%sstack [n]%s      | Display top n stack values (default: 5)\n", COLOR_YELLOW, COLOR_RESET);
    printf("%smemory <a> <s>%s | Display s bytes of memory starting at addr a\n", COLOR_YELLOW, COLOR_RESET);
    printf("%sdisassemble%s    | Show current instruction and surroundings\n", COLOR_YELLOW, COLOR_RESET);
    printf("%shelp%s           | Show this help message\n", COLOR_YELLOW, COLOR_RESET);
    printf("%squit%s           | Exit the debugger\n", COLOR_YELLOW, COLOR_RESET);
    printf("\n");
}

void handle_command(char *input) {
    int argc;
    char **argv = split_args(input, &argc);
    if (argc == 0) return;

    if (strcmp(argv[0], "break") == 0 || strcmp(argv[0], "b") == 0) {
        cmd_break(argc, argv);
    } else if (strcmp(argv[0], "list") == 0 || strcmp(argv[0], "l") == 0) {
        list_breakpoints();
    } else if (strcmp(argv[0], "delete") == 0 || strcmp(argv[0], "d") == 0) {
        cmd_delete(argc, argv);
    } else if (strcmp(argv[0], "step") == 0 || strcmp(argv[0], "s") == 0) {
        cmd_step();
    } else if (strcmp(argv[0], "continue") == 0 || strcmp(argv[0], "c") == 0) {
        cmd_continue();
    } else if (strcmp(argv[0], "registers") == 0 || strcmp(argv[0], "r") == 0) {
        cmd_registers();
    } else if (strcmp(argv[0], "stack") == 0) {
        cmd_stack(argc, argv);
    } else if (strcmp(argv[0], "memory") == 0 || strcmp(argv[0], "m") == 0) {
        cmd_memory(argc, argv);
    } else if (strcmp(argv[0], "disassemble") == 0 || strcmp(argv[0], "dis") == 0) {
        cmd_disassemble();
    } else if (strcmp(argv[0], "help") == 0 || strcmp(argv[0], "h") == 0) {
        cmd_help();
    } else if (strcmp(argv[0], "quit") == 0 || strcmp(argv[0], "q") == 0) {
        running = 0;
    } else {
        printf("%sUnknown command: %s%s\n", COLOR_RED, argv[0], COLOR_RESET);
        printf("Type %shelp%s for a list of commands\n", COLOR_YELLOW, COLOR_RESET);
    }

    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
}

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
    find_label(&vm.program, OENTRY, &vm.xpu.ip);

    print_header();
    printf("%sLoaded '%s' with %zu instructions%s\n", 
           COLOR_GREEN, vm.program.filename, vm.program.instructions_count, COLOR_RESET);

    char input[256];
    while (running) {
        if (is_breakpoint(vm.xpu.ip)) {
            printf("\n%s● Breakpoint hit at 0x%zx%s\n", COLOR_RED, vm.xpu.ip, COLOR_RESET);
            cmd_disassemble();
        }
        
        print_prompt();
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        handle_command(input);
    }

    printf("%sExiting debugger%s\n", COLOR_CYAN, COLOR_RESET);
    ortavm_free(&vm);
    return 0;
}
