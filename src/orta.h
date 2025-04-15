#ifndef ORTA_H 
#define ORTA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "xlib.h"

#ifdef _WIN32
    #include <windows.h>
    #include <dirent.h> 
    #include <sys/stat.h>
    #define mkdir(path, skip) mkdir(path);
    #define stat _stat
    #include <io.h>
    #include <process.h>
    #define open _open
    #define read _read
    #define write _write
    #define close _close
    #define dup _dup
    #define dup2 _dup2
    #define pipe _pipe
    #define fork _fork
    #define execvp _execvp
    #define waitpid _cwait
    #define WEXITSTATUS(status) ((status) & 0xFF)
#else
    #include <unistd.h>
    #include <time.h>
    #include <sys/stat.h> 
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

#include "libs/vector.h"
#include "libs/str.h"

#define ODEFAULT_STACK_SIZE 16384
#define ODEFAULT_ENTRY "__entry"
#define MEMORY_CAPACITY 8192
#define MAX_LINE_LENGTH 1024
#define MAX_INCLUDE_DEPTH 16
#define MAX_DEFINES 100
#define MAX_DEFINE_NAME 64
#define MAX_DEFINE_VALUE 1024
#define MAX_ARGS 16
#define MAX_ARG_LEN 256
#define MAX_INCLUDE_PATHS 16

static size_t OSTACK_SIZE = ODEFAULT_STACK_SIZE;
static char *OENTRY = ODEFAULT_ENTRY;

typedef enum {
    WINT,
    WFLOAT,
    WCHARP, 
    W_CHAR,
    WPOINTER,
    WBOOL,
} WordType;

typedef struct {
    WordType type;
    union {
        int as_int;
        float as_float;
        char as_char;
        char *as_string;
        void *as_pointer;
        int as_bool;
    };
} Word;

typedef struct {
    Word reg_value; 
    size_t reg_size;
} XRegister;

typedef enum {
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_RA,
    REG_FR,
    REG_COUNT,
} XRegisters;

typedef enum {
    REGSIZE_8BIT = 1,
    REGSIZE_16BIT,
    REGSIZE_32BIT = 4,
    REGSIZE_64BIT = 8,
} XRegisterSize;

typedef struct {
    char *name;
    XRegisters id;
    XRegisterSize size;
} XRegisterInfo;

XRegisterInfo register_table[REG_COUNT] = {
    {"RAX", REG_RAX, REGSIZE_64BIT}, {"RBX", REG_RBX, REGSIZE_64BIT},
    {"RCX", REG_RCX, REGSIZE_64BIT}, {"RDX", REG_RDX, REGSIZE_64BIT},
    {"RSI", REG_RSI, REGSIZE_64BIT}, {"RDI", REG_RDI, REGSIZE_64BIT},
    {"R8",  REG_R8,  REGSIZE_64BIT}, {"R9",  REG_R9,  REGSIZE_64BIT},
    {"R10", REG_R10, REGSIZE_64BIT}, {"R11", REG_R11, REGSIZE_64BIT},
    {"R12", REG_R12, REGSIZE_64BIT}, {"R13", REG_R13, REGSIZE_64BIT},
    {"R14", REG_R14, REGSIZE_64BIT}, {"R15", REG_R15, REGSIZE_64BIT},
    {"RA",  REG_RA,  REGSIZE_64BIT}, {"FR",  REG_FR,  REGSIZE_64BIT},
};

int is_register(const char *str) {
    for (int i = 0; i < REG_COUNT; i++) {
        if (strcasecmp(str, register_table[i].name) == 0) return 1;
    }
    return 0;
}

XRegisters register_name_to_enum(const char *reg_name) {
    for (int i = 0; i < REG_COUNT; i++) {
        if (strcasecmp(reg_name, register_table[i].name) == 0)
            return register_table[i].id;
    }
    return (XRegisters)-1;
}

typedef struct {
    Word *stack;
    size_t count;
    size_t capacity;
} XStack;

XStack xstack_create(size_t capacity) {
    XStack x = {0};
    x.stack = malloc(sizeof(Word) * capacity);
    x.capacity = capacity;
    x.count = 0;
    return x;
}

int xstack_push(XStack *stack, Word w) {
    if (stack->count >= stack->capacity) return 0;
    stack->stack[stack->count++] = w;
    return 1;
}

Word xstack_pop(XStack *stack) {
    if (stack->count == 0) {
        Word w = {.type = WPOINTER, .as_pointer = NULL};
        return w;
    }
    return stack->stack[--stack->count];
}

Word xstack_peek(XStack *stack, size_t offset) {
    if (offset >= stack->count) {
        Word w = {.type = WPOINTER, .as_pointer = NULL};
        return w;
    }
    return stack->stack[stack->count - 1 - offset];
}

void xstack_free(XStack *stack) {
    for (size_t i = 0; i < stack->count; i++) {
        if (stack->stack[i].type == WCHARP) free(stack->stack[i].as_string);
        else if (stack->stack[i].type == WPOINTER) free(stack->stack[i].as_pointer);
    }
    free(stack->stack);
}

typedef struct {
    XRegister *registers;
    XStack stack;
    size_t ip;
} XPU;

XPU xpu_init() {
    XPU xpu = {0};
    xpu.stack = xstack_create(OSTACK_SIZE);
    xpu.registers = malloc(sizeof(XRegister) * REG_COUNT);
    xpu.ip = 0;
    
    for (int i = 0; i < REG_COUNT; i++) {
        xpu.registers[i].reg_size = register_table[i].size;
        xpu.registers[i].reg_value.type = WPOINTER;
        xpu.registers[i].reg_value.as_pointer = NULL;
    }
    return xpu;
}

void xpu_free(XPU *xpu) {
    for (int i = 0; i < REG_COUNT; i++) {
        if (xpu->registers[i].reg_value.type == WCHARP)
            free(xpu->registers[i].reg_value.as_string);
        else if (xpu->registers[i].reg_value.type == WPOINTER)
            free(xpu->registers[i].reg_value.as_pointer);
    }
    xstack_free(&xpu->stack);
    free(xpu->registers);
}

typedef enum {
    IPUSH, IMOV, IPOP, IADD, ISUB, IMUL, IDIV, IMOD, IAND, IOR,
    IXOR, INOT, IEQ, INE, ILT, IGT, ILE, IGE, IJMP, IJMPIF, ICALL,
    IRET, ILOAD, ISTORE, IPRINT, IDUP, ISWAP, IDROP, IROTL, IROTR,
    IALLOC, IHALT, IMERGE, IXCALL, IPRINTMEM, ISIZEOF, IMEMCMP,
    IDEC, IINC, IEVAL, ICMP, IREADMEM, ICPYMEM, IWRITEMEM, ISYSCALL,
} Instruction;

typedef enum {
    ARG_EXACT, ARG_MIN, ARG_MAX, ARG_RANGE
} ArgRequirementType;

typedef struct {
    ArgRequirementType type;
    int value;
    int value2;
} ArgRequirement;

typedef struct {
    const char *name;
    Instruction instruction;
    ArgRequirement args;
} InstructionInfo;

static const InstructionInfo instructions[] = {
    {"push", IPUSH, {ARG_EXACT, 1, 1}}, {"mov", IMOV, {ARG_EXACT, 2, 2}},
    {"pop", IPOP, {ARG_EXACT, 1, 1}}, {"add", IADD, {ARG_MIN, 0, 0}},
    {"sub", ISUB, {ARG_MIN, 0, 0}}, {"mul", IMUL, {ARG_MIN, 0, 0}},
    {"div", IDIV, {ARG_MIN, 0, 0}}, {"mod", IMOD, {ARG_EXACT, 0, 0}},
    {"and", IAND, {ARG_EXACT, 0, 0}}, {"or", IOR, {ARG_EXACT, 0, 0}},
    {"xor", IXOR, {ARG_EXACT, 2, 2}}, {"not", INOT, {ARG_EXACT, 0, 0}},
    {"eq", IEQ, {ARG_EXACT, 0, 0}}, {"ne", INE, {ARG_EXACT, 0, 0}},
    {"lt", ILT, {ARG_EXACT, 0, 0}}, {"gt", IGT, {ARG_EXACT, 0, 0}},
    {"le", ILE, {ARG_EXACT, 0, 0}}, {"ge", IGE, {ARG_EXACT, 0, 0}},
    {"jmp", IJMP, {ARG_EXACT, 1, 1}}, {"jmpif", IJMPIF, {ARG_EXACT, 1, 1}},
    {"call", ICALL, {ARG_EXACT, 1, 1}}, {"ret", IRET, {ARG_EXACT, 0, 0}},
    {"load", ILOAD, {ARG_EXACT, 1, 1}}, {"store", ISTORE, {ARG_EXACT, 1, 1}},
    {"print", IPRINT, {ARG_MIN, 0, 0}}, {"dup", IDUP, {ARG_EXACT, 0, 0}},
    {"swap", ISWAP, {ARG_EXACT, 0, 0}}, {"drop", IDROP, {ARG_EXACT, 0, 0}},
    {"rotl", IROTL, {ARG_EXACT, 1, 1}}, {"rotr", IROTR, {ARG_EXACT, 1, 1}},
    {"alloc", IALLOC, {ARG_EXACT, 1, 1}}, {"halt", IHALT, {ARG_EXACT, 0, 0}},
    {"merge", IMERGE, {ARG_MIN, 0, 0}}, {"xcall", IXCALL, {ARG_EXACT, 0, 0}},
    {"printmem", IPRINTMEM, {ARG_EXACT, 0, 0}},
    {"sizeof", ISIZEOF, {ARG_EXACT, 1, 1}}, {"dec", IDEC, {ARG_EXACT, 1, 1}},
    {"inc", IINC, {ARG_EXACT, 1, 1}}, {"eval", IEVAL, {ARG_MIN, 0, 0}},
    {"cmp", ICMP, {ARG_MIN, 1, 1}}, {"readmem", IREADMEM, {ARG_MIN, 1, 1}},
    {"cpymem", ICPYMEM, {ARG_EXACT, 0, 0}}, {"syscall", ISYSCALL, {ARG_MIN, 1, 1}},
    {"writemem", IWRITEMEM, {ARG_MIN, 1, 1}}, {"memcmp", IMEMCMP, {ARG_MIN, 1, 1}},
};

#define INSTRUCTION_COUNT (sizeof(instructions) / sizeof(instructions[0]))

typedef struct {
    Instruction opcode;
    Vector operands;
} InstructionData;

typedef struct {
    char *name;
    size_t address;
} Label;

typedef struct {
    char *filename;
    InstructionData *instructions;
    size_t instructions_count;
    size_t instructions_capacity;
    Label *labels;
    size_t labels_count;
    size_t labels_capacity;
    void *memory;
    size_t memory_used;
    size_t memory_capacity;
    void *dead_memory;
} Program;

typedef enum {
    FLAG_NOTHING = 0, FLAG_STACK = 1, FLAG_MEMORY = 2, FLAG_CMD = 3,
} Flags;

typedef struct {
    char magic[4];
    short flags_count;
    short flags[4];
} OrtaMeta;

typedef struct {
    XPU xpu;
    Program program;
    OrtaMeta meta;
} OrtaVM;

void program_init(Program *program, const char *filename) {
    program->filename = strdup(filename);
    program->instructions_capacity = 128;
    program->instructions_count = 0;
    program->instructions = malloc(sizeof(InstructionData) * program->instructions_capacity);
    program->labels_capacity = 32;
    program->labels_count = 0;
    program->labels = malloc(sizeof(Label) * program->labels_capacity);
    program->memory = malloc(MEMORY_CAPACITY);
    program->memory_capacity = MEMORY_CAPACITY;
    program->memory_used = 0;
    program->dead_memory = malloc(1);
}

void add_label(Program *program, const char *name, size_t address) {
    if (program->labels_count >= program->labels_capacity) {
        program->labels_capacity *= 2;
        program->labels = realloc(program->labels, sizeof(Label) * program->labels_capacity);
    }
    program->labels[program->labels_count].name = strdup(name);
    program->labels[program->labels_count].address = address;
    program->labels_count++;
}

int find_label(Program *program, const char *name, size_t *address) {
    for (size_t i = 0; i < program->labels_count; i++) {
        if (strcmp(program->labels[i].name, name) == 0) {
            *address = program->labels[i].address;
            return 1;
        }
    }
    return 0;
}

void program_free(Program *program) {
    free(program->filename);
    for (size_t i = 0; i < program->instructions_count; i++) {
        VECTOR_FOR_EACH(char *, elem, &program->instructions[i].operands) {
            free(*elem);
        }
        vector_free(&program->instructions[i].operands);
    }
    free(program->instructions);
    for (size_t i = 0; i < program->labels_count; i++) {
        free(program->labels[i].name);
    }
    free(program->labels);
    free(program->memory);
    free(program->dead_memory);
}

OrtaVM ortavm_create(const char *filename) {
    OrtaVM vm;
    vm.xpu = xpu_init();
    program_init(&vm.program, filename);
    vm.meta.flags_count = 0;
    strncpy(vm.meta.magic, "XBIN", 4);
    for (size_t i = 0; i < 4; i++) {
        vm.meta.flags[i] = FLAG_NOTHING;
    }
    return vm;
}

void ortavm_free(OrtaVM *vm) {
    xpu_free(&vm->xpu);
    program_free(&vm->program);
}

void add_flag(OrtaVM *vm, Flags flag) {
    for (int i = 0; i < vm->meta.flags_count; i++) {
        if (vm->meta.flags[i] == (short)flag) return;
    }
    vm->meta.flags[vm->meta.flags_count++] = (short)flag;
}

ArgRequirement instruction_expected_args(Instruction instruction) {
    for (size_t i = 0; i < (sizeof(instructions)/sizeof(instructions[0])); i++) {
        if (instructions[i].instruction == instruction)
            return instructions[i].args;
    }
    return (ArgRequirement){ARG_EXACT, -1, 0};
}

int is_pointer(char *str) {
    return str && strlen(str) >= 2 && str[0] == '(' && str[strlen(str)-1] == ')';
}

void *get_pointer(char *str) {
    if (!is_pointer(str)) return NULL;
    str++;
    str[strlen(str)-1] = '\0';
    return (void*)(uintptr_t)strtoull(str, NULL, 0);
}

int is_number(const char *str) {
    char *endptr;
    strtol(str, &endptr, 10);
    return *endptr == '\0';
}

int is_float(const char *str) {
    char *endptr;
    strtod(str, &endptr);
    return *endptr == '\0' && strchr(str, '.') != NULL;
}

int is_string(const char *str) {
    size_t len = strlen(str);
    return len >= 2 && str[0] == '"' && str[len-1] == '"';
}

int is_label_declaration(const char *str) {
    size_t len = strlen(str);
    return len > 0 && str[len-1] == ':';
}

int is_label_reference(const char *str) {
    return !is_number(str) && !is_float(str) && !is_register(str) && !is_string(str);
}

const char *instruction_to_string(Instruction instruction) {
    for (size_t i = 0; i < (sizeof(instructions)/sizeof(instructions[0])); i++) {
        if (instructions[i].instruction == instruction)
            return instructions[i].name;
    }
    return "unknown";
}

char* trim_left(const char *str) {
    while (isspace((unsigned char)*str)) str++;
    return strdup(str);
}

int validateArgCount(ArgRequirement req, int actualArgCount) {
    switch(req.type) {
        case ARG_EXACT: return actualArgCount == req.value;
        case ARG_MIN: return actualArgCount >= req.value;
        case ARG_MAX: return actualArgCount <= req.value;
        case ARG_RANGE: return actualArgCount >= req.value && actualArgCount <= req.value2;
        default: return 0;
    }
}

void add_instruction(Program *program, InstructionData instr) {
    if (program->instructions_count >= program->instructions_capacity) {
        program->instructions_capacity *= 2;
        program->instructions = realloc(program->instructions, 
            sizeof(InstructionData) * program->instructions_capacity);
    }
    program->instructions[program->instructions_count++] = instr;
}

void microsleep(unsigned long usecs) {
    #ifdef _WIN32
        HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
        LARGE_INTEGER ft = { .QuadPart = -(10 * (LONGLONG)usecs) };
        SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);
    #else
        struct timespec ts = {
            .tv_sec = usecs / 1000000,
            .tv_nsec = (usecs % 1000000) * 1000
        };
        nanosleep(&ts, NULL);
    #endif
}

char *hmerge(InstructionData *instr) {
    char *string = calloc(1, 256);
    VECTOR_FOR_EACH(char *, elem, &instr->operands) {
        char *rcharp = *(char **)elem;
        size_t len = strlen(rcharp);
        if (len >= 1) {
            char *temp = strdup(rcharp);
            temp[len-1] = '\0';
            strcat(string, temp + 1);
            strcat(string, " ");
            free(temp);
        }
    }
    return string;
}

void xsleep(unsigned long milliseconds) {
    microsleep(milliseconds * 1000);
}

int eval(char *s) {
    if (!s || *s == '\0') return 0;
    char *p = s;
    int values[100] = {0};
    char ops[100] = {0};
    int v_idx = 0, o_idx = 0;
    values[v_idx++] = 0;
    ops[o_idx++] = '+';

    while (*p) {
        while (isspace(*p)) p++;
        if (isdigit(*p)) {
            int val = 0;
            while (isdigit(*p)) val = val * 10 + (*p++ - '0');
            char op = ops[--o_idx];
            switch(op) {
                case '+': values[v_idx-1] += val; break;
                case '-': values[v_idx-1] -= val; break;
                case '*': values[v_idx-1] *= val; break;
                case '/': 
                    if (val == 0) return 0;
                    values[v_idx-1] /= val; 
                    break;
            }
        }
        else if (*p == '+' || *p == '-') ops[o_idx++] = *p++;
        else if (*p == '*' || *p == '/') {
            ops[o_idx++] = *p++;
            while (isspace(*p)) p++;
            int val = 0;
            if (*p == '(') {
                p++;
                char *start = p;
                int paren_count = 1;
                while (*p && paren_count) {
                    if (*p == '(') paren_count++;
                    if (*p == ')') paren_count--;
                    p++;
                }
                if (paren_count) return 0;
                char temp = *--p;
                *p = '\0';
                val = eval(start);
                *p = temp;
            }
            else if (isdigit(*p)) {
                while (isdigit(*p)) val = val * 10 + (*p++ - '0');
            }
            char op = ops[--o_idx];
            switch(op) {
                case '*': values[v_idx-1] *= val; break;
                case '/': 
                    if (val == 0) return 0;
                    values[v_idx-1] /= val; 
                    break;
            }
        }
        else if (*p == '(') {
            p++;
            char *start = p;
            int paren_count = 1;
            while (*p && paren_count) {
                if (*p == '(') paren_count++;
                if (*p == ')') paren_count--;
                p++;
            }
            if (paren_count) return 0;
            char temp = *--p;
            *p = '\0';
            int val = eval(start);
            *p = temp;
            char op = ops[--o_idx];
            switch(op) {
                case '+': values[v_idx-1] += val; break;
                case '-': values[v_idx-1] -= val; break;
                case '*': values[v_idx-1] *= val; break;
                case '/': 
                    if (val == 0) return 0;
                    values[v_idx-1] /= val; 
                    break;
            }
        }
        else p++;
    }
    return values[0];
}

WordType get_type(const char *s) {
    if (strcmp(s, "int") == 0) return WINT;
    if (strcmp(s, "float") == 0) return WFLOAT;
    if (strcmp(s, "charp") == 0) return WCHARP;
    if (strcmp(s, "char") == 0) return W_CHAR;
    if (strcmp(s, "bool") == 0) return WBOOL;
    if (strcmp(s, "pointer") == 0) return WPOINTER;
    return (WordType)-1;
}

bool is_type_name(const char *s) {
    return get_type(s) != -1;
}

void execute_instruction(OrtaVM *vm, InstructionData *instr) {
    XPU *xpu = &vm->xpu;
    Word w1, w2, result;
    XRegister *regs = xpu->registers;
    XRegister *rax = &regs[REG_RAX];
    XRegister *rbx = &regs[REG_RBX];
    XRegister *rcx = &regs[REG_RCX];
    XRegister *rsi = &regs[REG_RSI];
    XRegister *rdx = &regs[REG_RDX];

    switch (instr->opcode) {
        case IPUSH: {
            char *operand = vector_get_str(&instr->operands, 0);
            Word w;
            if (is_number(operand)) {
                w.type = WINT;
                w.as_int = atoi(operand);
            }
            else if (is_float(operand)) {
                w.type = WFLOAT;
                w.as_float = atof(operand);
            }
            else if (is_string(operand)) {
                w.type = WCHARP;
                size_t len = strlen(operand) - 2;
                w.as_string = malloc(len + 1);
                strncpy(w.as_string, operand + 1, len);
                w.as_string[len] = '\0';
            }
            else if (is_register(operand)) {
                XRegisters reg = register_name_to_enum(operand);
                if (reg != -1) w = regs[reg].reg_value;
            }
            xstack_push(&xpu->stack, w);
            break;
        }

        case IMOV: {
            char *src = vector_get_str(&instr->operands, 0);
            char *dst = vector_get_str(&instr->operands, 1);
            XRegisters dst_reg = register_name_to_enum(dst);
            if (dst_reg == -1) break;

            if (is_register(src)) {
                XRegisters src_reg = register_name_to_enum(src);
                if (src_reg != -1) {
                    if (regs[dst_reg].reg_value.type == WCHARP)
                        free(regs[dst_reg].reg_value.as_string);
                    regs[dst_reg].reg_value = regs[src_reg].reg_value;
                }
            }
            else if (is_number(src)) {
                regs[dst_reg].reg_value.type = WINT;
                regs[dst_reg].reg_value.as_int = atoi(src);
            }
            else if (is_float(src)) {
                regs[dst_reg].reg_value.type = WFLOAT;
                regs[dst_reg].reg_value.as_float = atof(src);
            }
            else if (is_string(src)) {
                size_t len = strlen(src) - 2;
                regs[dst_reg].reg_value.type = WCHARP;
                regs[dst_reg].reg_value.as_string = malloc(len + 1);
                strncpy(regs[dst_reg].reg_value.as_string, src + 1, len);
                regs[dst_reg].reg_value.as_string[len] = '\0';
            }
            break;
        }

        case IPOP: {
            char *operand = vector_get_str(&instr->operands, 0);
            XRegisters reg = register_name_to_enum(operand);
            if (reg != -1) {
                if (regs[reg].reg_value.type == WCHARP)
                    //free(regs[reg].reg_value.as_string);
                regs[reg].reg_value = xstack_pop(&xpu->stack);
            }
            break;
        }

        case IADD: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && w2.type == WINT) {
                result.type = WINT;
                result.as_int = w2.as_int + w1.as_int;
            }
            else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                result.type = WFLOAT;
                result.as_float = w2.as_float + w1.as_float;
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case ISUB: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && w2.type == WINT) {
                result.type = WINT;
                result.as_int = w2.as_int - w1.as_int;
            }
            else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                result.type = WFLOAT;
                result.as_float = w2.as_float - w1.as_float;
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case IMUL: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && w2.type == WINT) {
                result.type = WINT;
                result.as_int = w2.as_int * w1.as_int;
            }
            else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                result.type = WFLOAT;
                result.as_float = w2.as_float * w1.as_float;
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case IDIV: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            if (w1.as_int == 0 || w1.as_float == 0.0f) break;
            if (w1.type == WINT && w2.type == WINT) {
                result.type = WINT;
                result.as_int = w2.as_int / w1.as_int;
            }
            else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                result.type = WFLOAT;
                result.as_float = w2.as_float / w1.as_float;
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case IMOD: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && w2.type == WINT && w1.as_int != 0) {
                result.type = WINT;
                result.as_int = w2.as_int % w1.as_int;
                xstack_push(&xpu->stack, result);
            }
            break;
        }

        case IAND: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && w2.type == WINT) {
                result.type = WINT;
                result.as_int = w2.as_int & w1.as_int;
                xstack_push(&xpu->stack, result);
            }
            break;
        }

        case IOR: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && w2.type == WINT) {
                result.type = WINT;
                result.as_int = w2.as_int | w1.as_int;
                xstack_push(&xpu->stack, result);
            }
            break;
        }

        case IXOR: {
            if (instr->operands.size >= 2) {
                char *sreg1 = vector_get_str(&instr->operands, 0);
                char *sreg2 = vector_get_str(&instr->operands, 1);
                if (is_register(sreg1) && is_register(sreg2)) {
                    XRegisters preg1 = register_name_to_enum(sreg1);
                    XRegisters preg2 = register_name_to_enum(sreg2);
                    if (preg1 != -1 && preg2 != -1 &&
                        regs[preg1].reg_value.type == WINT &&
                        regs[preg2].reg_value.type == WINT) {
                        regs[preg1].reg_value.as_int ^= regs[preg2].reg_value.as_int;
                    }
                }
            }
            break;
        }

        case INOT: {
            w1 = xstack_pop(&xpu->stack);
            if (w1.type == WINT) {
                result.type = WINT;
                result.as_int = ~w1.as_int;
                xstack_push(&xpu->stack, result);
            }
            break;
        }

        case IEQ: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            result.type = WINT;
            result.as_int = 0;
            if (w1.type == w2.type) {
                switch(w1.type) {
                    case WINT: result.as_int = (w1.as_int == w2.as_int); break;
                    case WFLOAT: result.as_int = (w1.as_float == w2.as_float); break;
                    case WCHARP: result.as_int = (strcmp(w1.as_string, w2.as_string) == 0); break;
                }
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case INE: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            result.type = WINT;
            result.as_int = 1;
            if (w1.type == w2.type) {
                switch(w1.type) {
                    case WINT: result.as_int = (w1.as_int != w2.as_int); break;
                    case WFLOAT: result.as_int = (w1.as_float != w2.as_float); break;
                    case WCHARP: result.as_int = (strcmp(w1.as_string, w2.as_string) != 0); break;
                }
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case ILT: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            result.type = WINT;
            result.as_int = 0;
            if (w1.type == w2.type) {
                switch(w1.type) {
                    case WINT: result.as_int = (w2.as_int < w1.as_int); break;
                    case WFLOAT: result.as_int = (w2.as_float < w1.as_float); break;
                }
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case IGT: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            result.type = WINT;
            result.as_int = 0;
            if (w1.type == w2.type) {
                switch(w1.type) {
                    case WINT: result.as_int = (w2.as_int > w1.as_int); break;
                    case WFLOAT: result.as_int = (w2.as_float > w1.as_float); break;
                }
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case ILE: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            result.type = WINT;
            result.as_int = 0;
            if (w1.type == w2.type) {
                switch(w1.type) {
                    case WINT: result.as_int = (w2.as_int <= w1.as_int); break;
                    case WFLOAT: result.as_int = (w2.as_float <= w1.as_float); break;
                }
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case IGE: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            result.type = WINT;
            result.as_int = 0;
            if (w1.type == w2.type) {
                switch(w1.type) {
                    case WINT: result.as_int = (w2.as_int >= w1.as_int); break;
                    case WFLOAT: result.as_int = (w2.as_float >= w1.as_float); break;
                }
            }
            xstack_push(&xpu->stack, result);
            break;
        }

        case IJMP: {
            char *operand = vector_get_str(&instr->operands, 0);
            size_t target = 0;
            if (is_number(operand)) target = atoi(operand);
            else if (is_label_reference(operand)) find_label(&vm->program, operand, &target);
            if (target < vm->program.instructions_count) xpu->ip = target - 1;
            break;
        }

        case IJMPIF: {
            w1 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && w1.as_int != 0) {
                char *operand = vector_get_str(&instr->operands, 0);
                size_t target = 0;
                if (is_number(operand)) target = atoi(operand);
                else if (is_label_reference(operand)) find_label(&vm->program, operand, &target);
                if (target < vm->program.instructions_count) xpu->ip = target - 1;
            }
            break;
        }

        case ICALL: {
            char *operand = vector_get_str(&instr->operands, 0);
            size_t target = 0;
            if (is_number(operand)) target = atoi(operand);
            else if (is_label_reference(operand)) find_label(&vm->program, operand, &target);
            if (target < vm->program.instructions_count) {
                rax->reg_value.type = WINT;
                rax->reg_value.as_int = xpu->ip + 1;
                xpu->ip = target - 1;
            }
            break;
        }

        case IRET: {
            xpu->ip = rax->reg_value.as_int;
            break;
        }

        case ILOAD: {
            char *operand = vector_get_str(&instr->operands, 0);
            if (is_register(operand)) {
                XRegisters reg = register_name_to_enum(operand);
                if (reg != -1) xstack_push(&xpu->stack, regs[reg].reg_value);
            }
            break;
        }

        case ISTORE: {
            w1 = xstack_pop(&xpu->stack);
            char *operand = vector_get_str(&instr->operands, 0);
            if (is_register(operand)) {
                XRegisters reg = register_name_to_enum(operand);
                if (reg != -1) {
                    if (regs[reg].reg_value.type == WCHARP)
                        free(regs[reg].reg_value.as_string);
                    regs[reg].reg_value = w1;
                }
            }
            break;
        }

        case IPRINT: {
            if (instr->operands.size == 0) {
                w1 = xstack_peek(&xpu->stack, 0);
                switch(w1.type) {
                    case WINT: printf("%d\n", w1.as_int); break;
                    case WFLOAT: printf("%f\n", w1.as_float); break;
                    case WCHARP: printf("%s\n", w1.as_string); break;
                    case W_CHAR: printf("%c\n", w1.as_char); break;
                }
            }
            else {
                char *str = hmerge(instr);
                printf("%s\n", str);
                free(str);
            }
            break;
        }

        case IDUP: {
            w1 = xstack_peek(&xpu->stack, 0);
            switch(w1.type) {
                case WINT: xstack_push(&xpu->stack, (Word){.type = WINT, .as_int = w1.as_int}); break;
                case WFLOAT: xstack_push(&xpu->stack, (Word){.type = WFLOAT, .as_float = w1.as_float}); break;
                case WCHARP: xstack_push(&xpu->stack, (Word){.type = WCHARP, .as_string = strdup(w1.as_string)}); break;
            }
            break;
        }

        case ISWAP: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            xstack_push(&xpu->stack, w1);
            xstack_push(&xpu->stack, w2);
            break;
        }

        case IDROP: {
            xstack_pop(&xpu->stack);
            break;
        }

        case IROTL: {
            char *operand = vector_get_str(&instr->operands, 0);
            int n = atoi(operand);
            if (n <= 0 || n >= xpu->stack.count) break;
            Word *temp = malloc(n * sizeof(Word));
            for (int i = 0; i < n; i++) temp[i] = xstack_pop(&xpu->stack);
            Word bottom = temp[0];
            for (int i = 0; i < n-1; i++) temp[i] = temp[i+1];
            temp[n-1] = bottom;
            for (int i = n-1; i >= 0; i--) xstack_push(&xpu->stack, temp[i]);
            free(temp);
            break;
        }

        case IROTR: {
            char *operand = vector_get_str(&instr->operands, 0);
            int n = atoi(operand);
            if (n <= 0 || n >= xpu->stack.count) break;
            Word *temp = malloc(n * sizeof(Word));
            for (int i = 0; i < n; i++) temp[i] = xstack_pop(&xpu->stack);
            Word top = temp[n-1];
            for (int i = n-1; i > 0; i--) temp[i] = temp[i-1];
            temp[0] = top;
            for (int i = n-1; i >= 0; i--) xstack_push(&xpu->stack, temp[i]);
            free(temp);
            break;
        }

		case IALLOC: {
		    if (instr->operands.size >= 1) {
		        char *size_str = vector_get_str(&instr->operands, 0);
		        size_t size = 0;
		        
		        if (is_type_name(size_str)) {
		            WordType type = get_type(size_str);
		            switch(type) {
		                case WINT: size = sizeof(int); break;
		                case WFLOAT: size = sizeof(float); break;
		                case WCHARP: size = sizeof(char*); break;
		                case W_CHAR: size = sizeof(char); break;
		                case WPOINTER: size = sizeof(void*); break;
		                default: size = 1; break;
		            }
		            
		            if (instr->operands.size >= 2) {
		                char *count_str = vector_get_str(&instr->operands, 1);
		                if (is_number(count_str)) {
		                    size_t count = atoi(count_str);
		                    size *= count;
		                }
		                else if (is_register(count_str)) {
		                    XRegisters reg = register_name_to_enum(count_str);
		                    if (reg != -1 && regs[reg].reg_value.type == WINT) {
		                        size *= regs[reg].reg_value.as_int;
		                    }
		                }
		            }
		        }
		        else if (is_number(size_str)) {
		            size = atoi(size_str);
		        }
		        else if (is_register(size_str)) {
		            XRegisters reg = register_name_to_enum(size_str);
		            if (reg != -1 && regs[reg].reg_value.type == WINT) {
		                size = regs[reg].reg_value.as_int;
		            }
		        }
		        
		        if (size > 0) {
		            void *mem = (char*)vm->program.memory + vm->program.memory_used;
		            vm->program.memory_used += size;
		            
		            memset(mem, 0, size);
		            
		            if (instr->operands.size >= 3) {
		                char *dest_reg = vector_get_str(&instr->operands, 2);
		                if (is_register(dest_reg)) {
		                    XRegisters reg = register_name_to_enum(dest_reg);
		                    if (reg != -1) {
		                        if (regs[reg].reg_value.type == WCHARP)
		                            free(regs[reg].reg_value.as_string);
		                        regs[reg].reg_value.type = WPOINTER;
		                        regs[reg].reg_value.as_pointer = mem;
		                    }
		                }
		            } else {
		                xstack_push(&xpu->stack, (Word){.type = WPOINTER, .as_pointer = mem});
		            }
		        }
		    }
		    break;
		}


        case IMERGE: {
            if (instr->operands.size == 0) {
                w1 = xstack_pop(&xpu->stack);
                w2 = xstack_pop(&xpu->stack);
                if (w1.type == WCHARP && w2.type == WCHARP) {
                    char *merged = malloc(strlen(w1.as_string) + strlen(w2.as_string) + 2);
                    strcpy(merged, w2.as_string);
                    strcat(merged, " ");
                    strcat(merged, w1.as_string);
                    xstack_push(&xpu->stack, (Word){.type = WCHARP, .as_string = merged});
                }
            }
            else {
                char *merged = hmerge(instr);
                xstack_push(&xpu->stack, (Word){.type = WCHARP, .as_string = merged});
            }
            break;
        }

        case IXCALL: {
            if (rax->reg_value.type == WINT) {
                switch(rax->reg_value.as_int) {
                    case 1: 
                        if (rbx->reg_value.type == WINT) xsleep(rbx->reg_value.as_int);
                        break;
                    case 2: 
                        if (rbx->reg_value.type == WCHARP) system(rbx->reg_value.as_string);
                        break;
                    case 3: 
                        if (rbx->reg_value.type == WCHARP) {
                            Word w = xstack_pop(&xpu->stack);
                            w.type = get_type(rbx->reg_value.as_string);
                            xstack_push(&xpu->stack, w);
                        }
                        break;
                    case 4: // open a library 
                        if (rbx->reg_value.type == WCHARP) {
                            XLib_Handle *h = XLib_open(rbx->reg_value.as_string);
                            if (h) {
                                rbx->reg_value.as_pointer = h;
                            } else {
                                fprintf(stderr, "ERROR: %s", XLib_error());
                            }
                        }
                        break;
                    case 5: // find a function
                        if (rbx->reg_value.type == WPOINTER && rcx->reg_value.type == WCHARP) {
                            void *function_pointer = XLib_sym((XLib_Handle *)rbx->reg_value.as_pointer, rcx->reg_value.as_string);
                            if (function_pointer) {
                                rbx->reg_value.as_pointer = function_pointer;
                                rbx->reg_value.type = WPOINTER;
                            } else {
                                fprintf(stderr, "ERROR: %s", XLib_error());
                                rbx->reg_value.as_pointer = NULL;
                                rbx->reg_value.type = WPOINTER;
                            }
                        }
                        break;
                    case 6: // call a function 
                        if (rbx->reg_value.type == WPOINTER) {
                            typedef int (*func_t)(...);
                            func_t func = (func_t)rbx->reg_value.as_pointer;
                            int num_args = rcx->reg_value.type == WINT ? rcx->reg_value.as_int : 0;
                            
                            int *args = (int *)malloc(num_args * sizeof(int));
                            for (int i = 0; i < num_args; i++) {
                                Word arg = xstack_pop(&xpu->stack);
                                if (arg.type == WINT) {
                                    args[num_args - 1 - i] = arg.as_int;
                                }
                                free(arg.as_pointer);
                            }

                            int result = 0;
                            switch (num_args) {
                                case 0: result = func(); break;
                                case 1: result = func(args[0]); break;
                                case 2: result = func(args[0], args[1]); break;
                                case 3: result = func(args[0], args[1], args[2]); break;
                                default: fprintf(stderr, "Unsupported number of arguments: %d\n", num_args);
                            }
                            free(args);

                            xstack_push(&xpu->stack, (Word){.as_int = result, .type = WINT});
                        }
                        break;

                    case 7: 
                        xstack_push(&xpu->stack, (Word){.type = WPOINTER, .as_pointer = NULL});
                        break;
                }
            }
            break;
        }




        case IWRITEMEM: {
		    void *dest_addr = NULL;
		    size_t offset = 0;
		    Word value;
		    WordType write_type = WINT;
		    
		    if (instr->operands.size >= 1) {
		        char *dest = vector_get_str(&instr->operands, 0);
		        if (is_register(dest)) {
		            XRegisters reg = register_name_to_enum(dest);
		            if (reg != -1 && regs[reg].reg_value.type == WPOINTER) {
		                dest_addr = regs[reg].reg_value.as_pointer;
		            }
		        }
		    }
		    
		    if (instr->operands.size >= 2) {
		        char *off = vector_get_str(&instr->operands, 1);
		        if (is_number(off)) {
		            offset = atoi(off);
		        }
		        else if (is_register(off)) {
		            XRegisters reg = register_name_to_enum(off);
		            if (reg != -1 && regs[reg].reg_value.type == WINT) {
		                offset = regs[reg].reg_value.as_int;
		            }
		        }
		    }
		    
		    if (instr->operands.size >= 3) {
		        char *type_str = vector_get_str(&instr->operands, 2);
		        write_type = get_type(type_str);
		    }
		    
		    if (instr->operands.size >= 4) {
		        char *val_str = vector_get_str(&instr->operands, 3);
		        if (is_register(val_str)) {
		            XRegisters reg = register_name_to_enum(val_str);
		            if (reg != -1) {
		                value = regs[reg].reg_value;
		            }
		        }
		        else if (is_number(val_str)) {
		            value.type = WINT;
		            value.as_int = atoi(val_str);
		        }
		        else if (is_float(val_str)) {
		            value.type = WFLOAT;
		            value.as_float = atof(val_str);
		        }
		        else if (is_string(val_str)) {
		            value.type = WCHARP;
		            size_t len = strlen(val_str) - 2;
		            value.as_string = malloc(len + 1);
		            strncpy(value.as_string, val_str + 1, len);
		            value.as_string[len] = '\0';
		        }
		    } else {
		        value = xstack_pop(&xpu->stack);
		    }
		    
		    if (dest_addr) {
		        void *write_addr = (char*)dest_addr + offset;
		        
		        switch (write_type) {
		            case WINT:
		                *(int*)write_addr = (value.type == WINT) ? value.as_int : 
		                                    (value.type == WFLOAT) ? (int)value.as_float : 0;
		                break;
		            case WFLOAT:
		                *(float*)write_addr = (value.type == WFLOAT) ? value.as_float : 
		                                     (value.type == WINT) ? (float)value.as_int : 0.0f;
		                break;
		            case W_CHAR:
		                *(char*)write_addr = (value.type == W_CHAR) ? value.as_char : 
		                                     (value.type == WINT) ? (char)value.as_int : 0;
		                break;
		            case WCHARP:
		                if (value.type == WCHARP) {
		                    *(char**)write_addr = strdup(value.as_string);
		                }
		                break;
		            case WPOINTER:
		                if (value.type == WPOINTER) {
		                    *(void**)write_addr = value.as_pointer;
		                }
		                break;
		        }
		    }
		    
		    if (instr->operands.size < 4 && value.type == WCHARP) {
		        free(value.as_string);
		    }
		    break;
		}

		case IPRINTMEM: {
			void *addr = NULL;
			int format = 0;
			size_t size = 0;
			size_t display_width = 16;
			
			if (instr->operands.size >= 1) {
				char *ptr = vector_get_str(&instr->operands, 0);
				if (is_register(ptr)) {
					XRegisters reg = register_name_to_enum(ptr);
					if (reg != -1 && regs[reg].reg_value.type == WPOINTER) {
						addr = regs[reg].reg_value.as_pointer;
					}
				}
			}
			
			if (instr->operands.size >= 2) {
				char *sz = vector_get_str(&instr->operands, 1);
				if (is_number(sz)) {
					size = atoi(sz);
				} else if (is_register(sz)) {
					XRegisters reg = register_name_to_enum(sz);
					if (reg != -1 && regs[reg].reg_value.type == WINT) {
						size = regs[reg].reg_value.as_int;
					}
				}
			}
			
			if (instr->operands.size >= 3) {
				char *fmt = vector_get_str(&instr->operands, 2);
				if (is_number(fmt)) {
					format = atoi(fmt);
				} else if (is_register(fmt)) {
					XRegisters reg = register_name_to_enum(fmt);
					if (reg != -1 && regs[reg].reg_value.type == WINT) {
						format = regs[reg].reg_value.as_int;
					}
				} else if (fmt[0] == '"') {
					if (strcmp(fmt, "\"hex\"") == 0) format = 0;
					else if (strcmp(fmt, "\"ascii\"") == 0) format = 1;
					else if (strcmp(fmt, "\"int\"") == 0) format = 2;
					else if (strcmp(fmt, "\"float\"") == 0) format = 3;
				}
			}
			
			if (instr->operands.size >= 4) {
				char *wd = vector_get_str(&instr->operands, 3);
				if (is_number(wd)) {
					display_width = atoi(wd);
				} else if (is_register(wd)) {
					XRegisters reg = register_name_to_enum(wd);
					if (reg != -1 && regs[reg].reg_value.type == WINT) {
						display_width = regs[reg].reg_value.as_int;
					}
				}
			}
			
			if (addr && size > 0) {
				printf("Memory at %p (size %zu):\n", addr, size);
				
				switch(format) {
					case 0:
						for (size_t i = 0; i < size; i++) {
							if (i % display_width == 0) {
								if (i > 0) {
									printf("  ");
									for (size_t j = i - display_width; j < i; j++) {
										unsigned char c = ((unsigned char*)addr)[j];
										printf("%c", (c >= 32 && c <= 126) ? c : '.');
									}
									printf("\n");
								}
								printf("%08zx: ", i);
							}
							printf("%02x ", ((unsigned char*)addr)[i]);
						}
						if (size > 0) {
							size_t padding = (display_width - (size % display_width)) % display_width;
							for (size_t i = 0; i < padding; i++) {
								printf("   ");
							}
							printf("  ");
							size_t start = size - (size % display_width);
							if (start == size) start = size - display_width;
							for (size_t i = start; i < size; i++) {
								unsigned char c = ((unsigned char*)addr)[i];
								printf("%c", (c >= 32 && c <= 126) ? c : '.');
							}
						}
						printf("\n");
						break;
						
					case 1:
						for (size_t i = 0; i < size; i++) {
							printf("%c", ((char*)addr)[i]);
						}
						printf("\n");
						break;
						
					case 2:
						for (size_t i = 0; i < size / sizeof(int); i++) {
							if (i % display_width == 0 && i > 0) printf("\n");
							printf("%d ", ((int*)addr)[i]);
						}
						printf("\n");
						break;
						
					case 3:
						for (size_t i = 0; i < size / sizeof(float); i++) {
							if (i % display_width == 0 && i > 0) printf("\n");
							printf("%f ", ((float*)addr)[i]);
						}
						printf("\n");
						break;
				}
			}
			break;
		}


        case ISIZEOF: {
            char *operand = vector_get_str(&instr->operands, 0);
            WordType type = get_type(operand);
            int size = 0;
            switch(type) {
                case WINT: size = sizeof(int); break;
                case WFLOAT: size = sizeof(float); break;
                case WCHARP: size = sizeof(char*); break;
                case W_CHAR: size = sizeof(char); break;
                case WPOINTER: size = sizeof(void*); break;
            }
            xstack_push(&xpu->stack, (Word){.type = WINT, .as_int = size});
            break;
        }

        case IDEC: {
            char *operand = vector_get_str(&instr->operands, 0);
            XRegisters reg = register_name_to_enum(operand);
            if (reg != -1 && regs[reg].reg_value.type == WINT) 
                regs[reg].reg_value.as_int--;
            break;
        }

        case IINC: {
            char *operand = vector_get_str(&instr->operands, 0);
            XRegisters reg = register_name_to_enum(operand);
            if (reg != -1 && regs[reg].reg_value.type == WINT) 
                regs[reg].reg_value.as_int++;
            break;
        }

        case IEVAL: {
            char *expr = hmerge(instr);
            int val = eval(expr);
            xstack_push(&xpu->stack, (Word){.type = WINT, .as_int = val});
            free(expr);
            break;
        }

        case ICMP: {
            if (instr->operands.size >= 2) {
                char *op1 = vector_get_str(&instr->operands, 0);
                char *op2 = vector_get_str(&instr->operands, 1);
                Word w1, w2;
                
                if (is_register(op1)) {
                    XRegisters reg = register_name_to_enum(op1);
                    w1 = regs[reg].reg_value;
                }
                else if (is_number(op1)) {
                    w1.type = WINT;
                    w1.as_int = atoi(op1);
                }
                
                if (is_register(op2)) {
                    XRegisters reg = register_name_to_enum(op2);
                    w2 = regs[reg].reg_value;
                }
                else if (is_number(op2)) {
                    w2.type = WINT;
                    w2.as_int = atoi(op2);
                }
                
                int cmp = 0;
                if (w1.type == w2.type) {
                    switch(w1.type) {
                        case WINT: cmp = w1.as_int - w2.as_int; break;
                        case WFLOAT: cmp = (w1.as_float > w2.as_float) ? 1 : -1; break;
                        case WCHARP: cmp = strcmp(w1.as_string, w2.as_string); break;
                    }
                }
                rdx->reg_value.type = WINT;
                rdx->reg_value.as_int = cmp;
            }
            break;
        }

        case IREADMEM: {
		    void *src_addr = NULL;
		    size_t offset = 0;
		    WordType read_type = WINT;
		    
		    if (instr->operands.size >= 1) {
		        char *src = vector_get_str(&instr->operands, 0);
		        if (is_register(src)) {
		            XRegisters reg = register_name_to_enum(src);
		            if (reg != -1 && regs[reg].reg_value.type == WPOINTER) {
		                src_addr = regs[reg].reg_value.as_pointer;
		            }
		        }
		    }
		    
		    if (instr->operands.size >= 2) {
		        char *off = vector_get_str(&instr->operands, 1);
		        if (is_number(off)) {
		            offset = atoi(off);
		        }
		        else if (is_register(off)) {
		            XRegisters reg = register_name_to_enum(off);
		            if (reg != -1 && regs[reg].reg_value.type == WINT) {
		                offset = regs[reg].reg_value.as_int;
		            }
		        }
		    }
		    
		    if (instr->operands.size >= 3) {
		        char *type_str = vector_get_str(&instr->operands, 2);
		        read_type = get_type(type_str);
		    }
		    
		    if (src_addr) {
		        void *read_addr = (char*)src_addr + offset;
		        Word result;
		        
		        switch (read_type) {
		            case WINT:
		                result.type = WINT;
		                result.as_int = *(int*)read_addr;
		                break;
		            case WFLOAT:
		                result.type = WFLOAT;
		                result.as_float = *(float*)read_addr;
		                break;
		            case W_CHAR:
		                result.type = W_CHAR;
		                result.as_char = *(char*)read_addr;
		                break;
		            case WCHARP:
		                result.type = WCHARP;
		                result.as_string = strdup(*(char**)read_addr);
		                break;
		            case WPOINTER:
		                result.type = WPOINTER;
		                result.as_pointer = *(void**)read_addr;
		                break;
		            default:
		                result.type = WINT;
		                result.as_int = 0;
		                break;
		        }
		        
		        if (instr->operands.size >= 4) {
		            char *dest = vector_get_str(&instr->operands, 3);
		            if (is_register(dest)) {
		                XRegisters reg = register_name_to_enum(dest);
		                if (reg != -1) {
		                    if (regs[reg].reg_value.type == WCHARP)
		                        free(regs[reg].reg_value.as_string);
		                    regs[reg].reg_value = result;
		                }
		            }
		        } else {
		            xstack_push(&xpu->stack, result);
		        }
		    }
		    break;
		}


        case ICPYMEM: {
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            Word n = xstack_pop(&xpu->stack);
            if (w1.type == WPOINTER && w2.type == WPOINTER && n.type == WINT)
                memcpy(w1.as_pointer, w2.as_pointer, n.as_int);
            break;
        }

        case ISYSCALL: {
            int syscall_num = atoi(vector_get_str(&instr->operands, 0));
            switch(syscall_num) {
                case 0: {
                    if (rbx->reg_value.type == WCHARP && rsi->reg_value.type == WINT) {
                        FILE *f = fopen(rbx->reg_value.as_string, "r");
                        rax->reg_value.as_int = fileno(f);
                    }
                    break;
                }
                case 1: {
                    if (rbx->reg_value.type == WINT && rsi->reg_value.type == WPOINTER && 
                        rdx->reg_value.type == WINT) {
                        read(rbx->reg_value.as_int, rsi->reg_value.as_pointer, rdx->reg_value.as_int);
                    }
                    break;
                }
                case 2: {
                    if (rbx->reg_value.type == WINT && rsi->reg_value.type == WPOINTER && 
                        rdx->reg_value.type == WINT) {
                        write(rbx->reg_value.as_int, rsi->reg_value.as_pointer, rdx->reg_value.as_int);
                    }
                    break;
                }
                case 3: {
                    if (rbx->reg_value.type == WINT) {
                        close(rbx->reg_value.as_int);
                    }
                    break;
                }
            }
            break;
        }
        
        case IMEMCMP: {
			void *addr1 = NULL;
			void *addr2 = NULL;
			size_t size = 0;
			
			if (instr->operands.size >= 1) {
				char *ptr1 = vector_get_str(&instr->operands, 0);
				if (is_register(ptr1)) {
					XRegisters reg = register_name_to_enum(ptr1);
					if (reg != -1 && regs[reg].reg_value.type == WPOINTER) {
						addr1 = regs[reg].reg_value.as_pointer;
					}
				}
			}
			
			if (instr->operands.size >= 2) {
				char *ptr2 = vector_get_str(&instr->operands, 1);
				if (is_register(ptr2)) {
					XRegisters reg = register_name_to_enum(ptr2);
					if (reg != -1 && regs[reg].reg_value.type == WPOINTER) {
						addr2 = regs[reg].reg_value.as_pointer;
					}
				}
			}
			
			if (instr->operands.size >= 3) {
				char *sz = vector_get_str(&instr->operands, 2);
				if (is_number(sz)) {
					size = atoi(sz);
				} else if (is_register(sz)) {
					XRegisters reg = register_name_to_enum(sz);
					if (reg != -1 && regs[reg].reg_value.type == WINT) {
						size = regs[reg].reg_value.as_int;
					}
				}
			}
			
			if (addr1 && addr2 && size > 0) {
				int result = memcmp(addr1, addr2, size);
				
				if (instr->operands.size >= 4) {
					char *dest = vector_get_str(&instr->operands, 3);
					if (is_register(dest)) {
						XRegisters reg = register_name_to_enum(dest);
						if (reg != -1) {
							if (regs[reg].reg_value.type == WCHARP)
								free(regs[reg].reg_value.as_string);
							regs[reg].reg_value.type = WINT;
							regs[reg].reg_value.as_int = result;
						}
					}
				} else {
					xstack_push(&xpu->stack, (Word){.type = WINT, .as_int = result});
				}
			}
			break;
		}


        case IHALT:
            xpu->ip = vm->program.instructions_count;
            return;

        default:
            break;
    }
    xpu->ip++;
}

int create_bytecode(OrtaVM *vm, const char *output_filename) {
    Program *program = &vm->program;
    FILE *fp = fopen(output_filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not create bytecode file '%s'\n", output_filename);
        return 0;
    }
    
    if (fwrite(&vm->meta, sizeof(OrtaMeta), 1, fp) != 1)
        goto error;

    size_t filename_len = strlen(program->filename);
    if (fwrite(&filename_len, sizeof(size_t), 1, fp) != 1)
        goto error;
    
    if (fwrite(program->filename, 1, filename_len, fp) != filename_len)
        goto error;

    size_t instructions_count = program->instructions_count;
    if (fwrite(&instructions_count, sizeof(size_t), 1, fp) != 1)
        goto error;

    for (size_t i = 0; i < instructions_count; i++) {
        InstructionData *instr = &program->instructions[i];
        
        if (fwrite(&instr->opcode, sizeof(Instruction), 1, fp) != 1)
            goto error;

        size_t operands_count = instr->operands.size;
        if (fwrite(&operands_count, sizeof(size_t), 1, fp) != 1)
            goto error;

        for (size_t j = 0; j < operands_count; j++) {
            char *operand = *(char **)vector_get(&instr->operands, j);
            size_t operand_len = strlen(operand);
            
            if (fwrite(&operand_len, sizeof(size_t), 1, fp) != 1)
                goto error;
            
            if (fwrite(operand, 1, operand_len, fp) != operand_len)
                goto error;
        }
    }

    size_t labels_count = program->labels_count;
    if (fwrite(&labels_count, sizeof(size_t), 1, fp) != 1)
        goto error;

    for (size_t i = 0; i < labels_count; i++) {
        Label *label = &program->labels[i];
        
        size_t name_len = strlen(label->name);
        if (fwrite(&name_len, sizeof(size_t), 1, fp) != 1)
            goto error;
        
        if (fwrite(label->name, 1, name_len, fp) != name_len)
            goto error;
        
        if (fwrite(&label->address, sizeof(size_t), 1, fp) != 1)
            goto error;
    }

    fclose(fp);
    return 1;

error:
    fclose(fp);
    return 0;
}

void free_program_instructions(Program *program, size_t count) {
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < program->instructions[i].operands.size; j++) {
            char **operand_ptr = *(char ***)vector_get(&program->instructions[i].operands, j);
            free(*operand_ptr);
        }
        vector_free(&program->instructions[i].operands);
    }
    free(program->instructions);
}

void free_program_labels(Program *program, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(program->labels[i].name);
    }
    free(program->labels);
}

int load_bytecode(OrtaVM *vm, const char *input_filename) {
    Program *program = &vm->program;
    FILE *fp = fopen(input_filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open bytecode file '%s'\n", input_filename);
        return 0;
    }

    program_free(program);
    
    if (fread(&vm->meta, sizeof(OrtaMeta), 1, fp) != 1)
        goto error_close;

    size_t filename_len;
    if (fread(&filename_len, sizeof(size_t), 1, fp) != 1)
        goto error_close;
    
    char *filename = (char *)malloc(filename_len + 1);
    if (!filename)
        goto error_close;
    
    if (fread(filename, 1, filename_len, fp) != filename_len) {
        free(filename);
        goto error_close;
    }
    filename[filename_len] = '\0';
    program->filename = strdup(filename);
    free(filename);

    size_t instructions_count;
    if (fread(&instructions_count, sizeof(size_t), 1, fp) != 1)
        goto error_close;

    program->instructions = (InstructionData *)malloc(sizeof(InstructionData) * instructions_count);
    if (!program->instructions)
        goto error_close;
    
    program->instructions_count = instructions_count;
    program->instructions_capacity = instructions_count;

    size_t i;
    for (i = 0; i < instructions_count; i++) {
        Instruction opcode;
        if (fread(&opcode, sizeof(Instruction), 1, fp) != 1)
            goto error_instructions;

        size_t operands_count;
        if (fread(&operands_count, sizeof(size_t), 1, fp) != 1)
            goto error_instructions;

        Vector operands;
        vector_init(&operands, 10, sizeof(char*));
        
        size_t j;
        for (j = 0; j < operands_count; j++) {
            size_t operand_len;
            if (fread(&operand_len, sizeof(size_t), 1, fp) != 1) {
                vector_free(&operands);
                goto error_instructions;
            }
            
            char *temp_operand = (char *)malloc(operand_len + 1);
            if (!temp_operand) {
                vector_free(&operands);
                goto error_instructions;
            }
            
            if (fread(temp_operand, 1, operand_len, fp) != operand_len) {
                free(temp_operand);
                vector_free(&operands);
                goto error_instructions;
            }
            temp_operand[operand_len] = '\0';
            
            char *persistent_operand = strdup(temp_operand);
            if (!persistent_operand) {
                free(temp_operand);
                vector_free(&operands);
                goto error_instructions;
            }
            
            vector_push(&operands, &persistent_operand);
            free(temp_operand);
        }

        program->instructions[i].opcode = opcode;
        program->instructions[i].operands = operands;
    }

    size_t labels_count;
    if (fread(&labels_count, sizeof(size_t), 1, fp) != 1)
        goto error_instructions;

    program->labels = (Label *)malloc(sizeof(Label) * labels_count);
    if (!program->labels)
        goto error_instructions;
    
    program->labels_count = labels_count;
    program->labels_capacity = labels_count;

    for (i = 0; i < labels_count; i++) {
        size_t name_len;
        if (fread(&name_len, sizeof(size_t), 1, fp) != 1)
            goto error_labels;
        
        char *name = (char *)malloc(name_len + 1);
        if (!name)
            goto error_labels;
        
        if (fread(name, 1, name_len, fp) != name_len) {
            free(name);
            goto error_labels;
        }
        name[name_len] = '\0';

        size_t address;
        if (fread(&address, sizeof(size_t), 1, fp) != 1) {
            free(name);
            goto error_labels;
        }

        program->labels[i].name = strdup(name);
        program->labels[i].address = address;
        free(name);
    }

    fclose(fp);
    return 1;

error_labels:
    free_program_labels(program, i);
    
error_instructions:
    free_program_instructions(program, i);
    
error_close:
    fclose(fp);
    return 0;
}

int load_bytecode_from_memory(OrtaVM *vm, const unsigned char *data, size_t data_size) {
    Program *program = &vm->program;
    size_t offset = 0;
    
    if (offset + sizeof(OrtaMeta) > data_size)
        return 0;
    memcpy(&vm->meta, data + offset, sizeof(OrtaMeta));
    offset += sizeof(OrtaMeta);

    size_t filename_len;
    if (offset + sizeof(size_t) > data_size)
        return 0;
    memcpy(&filename_len, data + offset, sizeof(size_t));
    offset += sizeof(size_t);
    
    if (offset + filename_len > data_size)
        return 0;
    char *filename = (char *)malloc(filename_len + 1);
    if (!filename)
        return 0;
    memcpy(filename, data + offset, filename_len);
    filename[filename_len] = '\0';
    program->filename = strdup(filename);
    free(filename);
    offset += filename_len;

    size_t instructions_count;
    if (offset + sizeof(size_t) > data_size)
        return 0;
    memcpy(&instructions_count, data + offset, sizeof(size_t));
    offset += sizeof(size_t);

    program->instructions = (InstructionData *)malloc(sizeof(InstructionData) * instructions_count);
    if (!program->instructions)
        return 0;
    
    program->instructions_count = instructions_count;
    program->instructions_capacity = instructions_count;

    size_t i;
    for (i = 0; i < instructions_count; i++) {
        Instruction opcode;
        if (offset + sizeof(Instruction) > data_size)
            goto error_instructions;
        memcpy(&opcode, data + offset, sizeof(Instruction));
        offset += sizeof(Instruction);

        size_t operands_count;
        if (offset + sizeof(size_t) > data_size)
            goto error_instructions;
        memcpy(&operands_count, data + offset, sizeof(size_t));
        offset += sizeof(size_t);

        Vector operands;
        vector_init(&operands, 10, sizeof(char*));
        
        size_t j;
        for (j = 0; j < operands_count; j++) {
            size_t operand_len;
            if (offset + sizeof(size_t) > data_size) {
                vector_free(&operands);
                goto error_instructions;
            }
            memcpy(&operand_len, data + offset, sizeof(size_t));
            offset += sizeof(size_t);
            
            if (offset + operand_len > data_size) {
                vector_free(&operands);
                goto error_instructions;
            }
            char *temp_operand = (char *)malloc(operand_len + 1);
            if (!temp_operand) {
                vector_free(&operands);
                goto error_instructions;
            }
            
            memcpy(temp_operand, data + offset, operand_len);
            temp_operand[operand_len] = '\0';
            offset += operand_len;
            
            char *persistent_operand = strdup(temp_operand);
            if (!persistent_operand) {
                free(temp_operand);
                vector_free(&operands);
                goto error_instructions;
            }
            
            vector_push(&operands, &persistent_operand);
            free(temp_operand);
        }

        program->instructions[i].opcode = opcode;
        program->instructions[i].operands = operands;
    }

    size_t labels_count;
    if (offset + sizeof(size_t) > data_size)
        goto error_instructions;
    memcpy(&labels_count, data + offset, sizeof(size_t));
    offset += sizeof(size_t);

    program->labels = (Label *)malloc(sizeof(Label) * labels_count);
    if (!program->labels)
        goto error_instructions;
    
    program->labels_count = labels_count;
    program->labels_capacity = labels_count;

    for (i = 0; i < labels_count; i++) {
        size_t name_len;
        if (offset + sizeof(size_t) > data_size)
            goto error_labels;
        memcpy(&name_len, data + offset, sizeof(size_t));
        offset += sizeof(size_t);
        
        if (offset + name_len > data_size)
            goto error_labels;
        char *name = (char *)malloc(name_len + 1);
        if (!name)
            goto error_labels;
        
        memcpy(name, data + offset, name_len);
        name[name_len] = '\0';
        offset += name_len;

        size_t address;
        if (offset + sizeof(size_t) > data_size) {
            free(name);
            goto error_labels;
        }
        memcpy(&address, data + offset, sizeof(size_t));
        offset += sizeof(size_t);

        program->labels[i].name = strdup(name);
        program->labels[i].address = address;
        free(name);
    }

    return 1;

error_labels:
    free_program_labels(program, i);
    
error_instructions:
    free_program_instructions(program, i);
    
    return 0;
}


void execute_program(OrtaVM *vm) {
    XPU *xpu = &vm->xpu;
    size_t entry = 0;
    if (!find_label(&vm->program, OENTRY, &entry)) xpu->ip = 0;
    else xpu->ip = entry;
    
    while (xpu->ip < vm->program.instructions_count) {
        execute_instruction(vm, &vm->program.instructions[xpu->ip]);
        xpu->ip++;
    }
}

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define CYAN    "\033[36m"

static void print_word(size_t index, Word w) {
    printf("[%zu] ", index);
    switch (w.type) {
        case WINT: printf("INT: %d", w.as_int); break;
        case WFLOAT: printf("FLOAT: %f", w.as_float); break;
        case WCHARP: printf("STRING: %s", w.as_string); break;
        case W_CHAR: printf("CHAR: %c", w.as_char); break;
        case WPOINTER: printf("POINTER: %p", w.as_pointer); break;
        case WBOOL: printf("BOOL: %s", w.as_bool ? "true" : "false"); break;
        default: printf("UNKNOWN");
    }
    printf("\n");
}

void print_stack(XPU *xpu) {
    printf(BOLD CYAN "Stack (%zu items):\n" RESET, xpu->stack.count);
    for (size_t i = 0; i < xpu->stack.count; i++) {
        print_word(i, xpu->stack.stack[i]);
    }
}

void print_registers(XPU *xpu) {
    printf(BOLD CYAN "Registers:\n" RESET);
    for (int i = 0; i < REG_COUNT; i++) {
        Word w = xpu->registers[i].reg_value;
        printf("[%3s] ", register_table[i].name);
        switch (w.type) {
            case WINT: printf("INT: %d", w.as_int); break;
            case WFLOAT: printf("FLOAT: %f", w.as_float); break;
            case WCHARP: printf("STRING: %s", w.as_string); break;
            case W_CHAR: printf("CHAR: %c", w.as_char); break;
            case WPOINTER: printf("POINTER: %p", w.as_pointer); break;
            case WBOOL: printf("BOOL: %s", w.as_bool ? "true" : "false"); break;
            default: printf("EMPTY");
        }
        printf("\n");
    }
}

#endif
