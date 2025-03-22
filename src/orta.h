#ifndef ORTA_H 
#define ORTA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
    #include <dirent.h> 
    #include <sys/stat.h>
    #define mkdir(path, skip) mkdir(path);
    #define stat _stat
#else
    #include <unistd.h>
    #include <time.h>
    #include <sys/stat.h> 
    #include <sys/types.h>
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
    void *value;
    WordType type;
} Word;

Word word_create(void *value, WordType type) {
    Word result = {0};
    result.value = value; 
    result.type = type;
    return result;
}

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
    REG_RA, // Return Address | Dont use!
    REG_FR, // Function return pass all returns here
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
    {"RAX", REG_RAX, REGSIZE_64BIT},
    {"RBX", REG_RBX, REGSIZE_64BIT},
    {"RCX", REG_RCX, REGSIZE_64BIT},
    {"RDX", REG_RDX, REGSIZE_64BIT},
    {"RSI", REG_RSI, REGSIZE_64BIT},
    {"RDI", REG_RDI, REGSIZE_64BIT},
    {"R8",  REG_R8,  REGSIZE_64BIT},
    {"R9",  REG_R9,  REGSIZE_64BIT},
    {"R10", REG_R10, REGSIZE_64BIT},
    {"R11", REG_R11, REGSIZE_64BIT},
    {"R12", REG_R12, REGSIZE_64BIT},
    {"R13", REG_R13, REGSIZE_64BIT},
    {"R14", REG_R14, REGSIZE_64BIT},
    {"R15", REG_R15, REGSIZE_64BIT},
    {"RA",  REG_RA,  REGSIZE_64BIT}, // Return Address | Don't use!
    {"FR",  REG_FR,  REGSIZE_64BIT}, // Function return
};

int is_register(const char *str) {
    for (int i = 0; i < REG_COUNT; i++) {
        if (strcasecmp(str, register_table[i].name) == 0) return 1;
    }
    
    return 0;
}

XRegisters register_name_to_enum(const char *reg_name) {
    for (int i = 0; i < REG_COUNT; i++) {
        if (strcasecmp(reg_name, register_table[i].name) == 0) {
            return register_table[i].id;
        }
    }
    return -1;
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
    if (stack->count >= stack->capacity) {
        return 0;
    }
    stack->stack[stack->count++] = w;
    return 1;
}

Word xstack_pop(XStack *stack) {
    Word w;
    if (stack->count == 0) {
        return word_create(NULL, WPOINTER);
    } else {
        w = stack->stack[--stack->count];
    }
    return w;
}

Word xstack_peek(XStack *stack, size_t offset) {
    if (offset >= stack->count) {
        return word_create(NULL, WPOINTER);
    }
    return stack->stack[stack->count - 1 - offset];
}

void xstack_free(XStack *stack) {
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
        xpu.registers[i].reg_value = word_create(NULL, WPOINTER);
    }
    
    return xpu;
}


void xpu_free(XPU *xpu) {
    xstack_free(&xpu->stack);
    free(xpu->registers);
}

typedef enum {
    IPUSH, IMOV, IPOP, IADD,
    ISUB, IMUL, IDIV, IMOD, IAND, IOR,
    IXOR, INOT, IEQ, INE, ILT, IGT,
    ILE, IGE, IJMP, IJMPIF, ICALL, IRET,
    ILOAD, ISTORE, IPRINT, IDUP,
    ISWAP, IDROP, IROTL, IROTR,
    IALLOC,IHALT, IMERGE, IXCALL, 
    IWRITE, IPRINTMEM, ISIZEOF,
    IDEC, IINC, IEVAL, ICMP, ILOADB 
} Instruction;

typedef enum {
    ARG_EXACT,
    ARG_MIN,
    ARG_MAX,
    ARG_RANGE
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

#define ExactArgs(n) {ARG_EXACT, (n), (n)}
#define MinArgs(n) {ARG_MIN, (n), 0}
#define MaxArgs(n) {ARG_MAX, (n), 0}
#define RangeArgs(min, max) {ARG_EXACT, (min), (max)}

static const InstructionInfo instructions[] = {
    {"push", IPUSH, ExactArgs(1)}, {"mov", IMOV, ExactArgs(2)}, {"pop", IPOP, ExactArgs(1)},
    {"add", IADD, MinArgs(0)}, {"sub", ISUB, MinArgs(0)}, {"mul", IMUL, MinArgs(0)},
    {"div", IDIV, MinArgs(0)}, {"mod", IMOD, ExactArgs(0)}, {"and", IAND, ExactArgs(0)},
    {"or", IOR, ExactArgs(0)}, {"xor", IXOR, ExactArgs(2)}, {"not", INOT, ExactArgs(0)},
    {"eq", IEQ, ExactArgs(0)}, {"ne", INE, ExactArgs(0)}, {"lt", ILT, ExactArgs(0)},
    {"gt", IGT, ExactArgs(0)}, {"le", ILE, ExactArgs(0)}, {"ge", IGE, ExactArgs(0)},
    {"jmp", IJMP, ExactArgs(1)}, {"jmpif", IJMPIF, ExactArgs(1)}, {"call", ICALL, ExactArgs(1)},
    {"ret", IRET, ExactArgs(0)}, {"load", ILOAD, ExactArgs(1)}, {"store", ISTORE, ExactArgs(1)},
    {"print", IPRINT, MinArgs(0)}, {"dup", IDUP, ExactArgs(0)}, {"swap", ISWAP, ExactArgs(0)},
    {"drop", IDROP, ExactArgs(0)}, {"rotl", IROTL, ExactArgs(1)}, {"rotr", IROTR, ExactArgs(1)},
    {"alloc", IALLOC, ExactArgs(1)}, {"halt", IHALT, ExactArgs(0)}, {"merge", IMERGE, MinArgs(0)},
    {"xcall", IXCALL, ExactArgs(0)}, {"write", IWRITE, ExactArgs(0)}, {"printmem", IPRINTMEM, ExactArgs(0)}, 
    {"sizeof", ISIZEOF, ExactArgs(1)}, {"dec", IDEC, ExactArgs(1)}, {"inc", IINC, ExactArgs(1)},
    {"eval", IEVAL, MinArgs(0)}, {"cmp", ICMP, MinArgs(1)}, {"loadb", ILOADB, MinArgs(1)},
};


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

    // Memory 
    void *memory; // UserSpaceMemory
    size_t memory_used;
    size_t memory_capacity;
    void *dead_memory;
} Program;

typedef enum {
    FLAG_NOTHING     = 0,
    FLAG_STACK       = 1,
    FLAG_MEMORY      = 2,
    FLAG_CMD         = 3,
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
        vector_free(&program->instructions[i].operands);
    }
    free(program->instructions);
    for (size_t i = 0; i < program->labels_count; i++) {
        free(program->labels[i].name);
    }
    free(program->labels);
    program->labels = NULL;
    // auto freed free(program->memory);
    program->memory = NULL;
}

OrtaVM ortavm_create(const char *filename) {
    OrtaVM vm;
    vm.xpu = xpu_init();
    program_init(&vm.program, filename);
    vm.meta.flags_count = 0;
    sprintf(vm.meta.magic, "XBIN");
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
        if (vm->meta.flags[i] == (short)flag) {
            return;
        }
    }
    vm->meta.flags[vm->meta.flags_count++] = (short)flag;
}

#define INSTRUCTION_COUNT (sizeof(instructions) / sizeof(instructions[0]))

Instruction parse_instruction(const char *instruction) {
    for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
        if (strcmp(instructions[i].name, instruction) == 0) {
            return instructions[i].instruction;
        }
    }
    return -1;
}

const char *instruction_to_string(Instruction instruction) {
    for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
        if (instructions[i].instruction == instruction) {
            return instructions[i].name;
        }
    }
    return "unknown";
}

ArgRequirement instruction_expected_args(Instruction instruction) {
    for (size_t i = 0; i < INSTRUCTION_COUNT; i++) {
        if (instructions[i].instruction == instruction) {
            return instructions[i].args;
        }
    }
    return (ArgRequirement){ARG_EXACT, -1, 0};
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
    return len >= 2 && str[0] == '"' && str[len - 1] == '"';
}

bool is_pointer(char *str) {
    if (str == NULL) return false;
    if (strlen(str) < 2) return false;
    if (str[0]!= '(' || str[strlen(str) - 1]!= ')') return false;
    return true;
}

void *get_pointer(char *str) {
    if (is_pointer(str)) {
        str++;
        str[strlen(str) - 1] = '\0';
        return (void*)(uintptr_t)strtoull(str, NULL, 0);
    } else {
        return NULL;
    }
}

int is_label_declaration(const char *str) {
    size_t len = strlen(str);
    return (len > 0) && (str[len - 1] == ':');
}

int is_label_reference(const char *str) {
    return !is_number(str) && !is_float(str) && !is_register(str) && !is_string(str);
}

void add_instruction(Program *program, InstructionData instr) {
    if (program->instructions_count >= program->instructions_capacity) {
        program->instructions_capacity *= 2;
        program->instructions = realloc(program->instructions, sizeof(InstructionData) * program->instructions_capacity);
    }
    
    program->instructions[program->instructions_count++] = instr;
}

void microsleep(unsigned long usecs) {
    #ifdef _WIN32
        HANDLE timer;
        LARGE_INTEGER ft;
        
        ft.QuadPart = -(10 * usecs);
        
        timer = CreateWaitableTimer(NULL, TRUE, NULL);
        SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);
    #else
        struct timespec ts;
        ts.tv_sec = usecs / 1000000;
        ts.tv_nsec = (usecs % 1000000) * 1000;
        nanosleep(&ts, NULL);
    #endif
}

void xsleep(unsigned long miliseconds) {
    microsleep(miliseconds * 1000);
}

int eval(char *s) {
    if (!s || *s == '\0') return 0;
    
    char *p = s;
    int values[100];
    char ops[100];
    int v_idx = 0;
    int o_idx = 0;
    
    values[v_idx++] = 0;
    ops[o_idx++] = '+';
    
    while (*p != '\0') {
        while (isspace(*p)) p++;
        
        if (isdigit(*p)) {
            int val = 0;
            while (isdigit(*p)) {
                val = val * 10 + (*p - '0');
                p++;
            }
            
            char op = ops[--o_idx];
            if (op == '+') values[v_idx - 1] += val;
            else if (op == '-') values[v_idx - 1] -= val;
            else if (op == '*') values[v_idx - 1] *= val;
            else if (op == '/') {
                if (val == 0) {
                    fprintf(stderr, "Error: Division by zero\n");
                    return 0;
                }
                values[v_idx - 1] /= val;
            }
        } 
        else if (*p == '+' || *p == '-') {
            ops[o_idx++] = *p;
            p++;
        }
        else if (*p == '*' || *p == '/') {
            ops[o_idx++] = *p;
            p++;
            
            while (isspace(*p)) p++;
            
            int val = 0;
            if (*p == '(') {
                p++;
                int paren_count = 1;
                char *start = p;
                
                while (paren_count > 0 && *p != '\0') {
                    if (*p == '(') paren_count++;
                    else if (*p == ')') paren_count--;
                    if (paren_count > 0) p++;
                }
                
                if (*p == ')') {
                    *p = '\0';
                    val = eval(start);
                    *p = ')';
                    p++;
                }
            } else if (isdigit(*p)) {
                while (isdigit(*p)) {
                    val = val * 10 + (*p - '0');
                    p++;
                }
            }
            
            char op = ops[--o_idx];
            if (op == '*') values[v_idx - 1] *= val;
            else if (op == '/') {
                if (val == 0) {
                    fprintf(stderr, "Error: Division by zero\n");
                    return 0;
                }
                values[v_idx - 1] /= val;
            }
        }
        else if (*p == '(') {
            p++;
            int paren_count = 1;
            char *start = p;
            
            while (paren_count > 0 && *p != '\0') {
                if (*p == '(') paren_count++;
                else if (*p == ')') paren_count--;
                if (paren_count > 0) p++;
            }
            
            if (*p == ')') {
                *p = '\0';
                int val = eval(start);
                *p = ')';
                p++;
                
                char op = ops[--o_idx];
                if (op == '+') values[v_idx - 1] += val;
                else if (op == '-') values[v_idx - 1] -= val;
                else if (op == '*') values[v_idx - 1] *= val;
                else if (op == '/') {
                    if (val == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        return 0;
                    }
                    values[v_idx - 1] /= val;
                }
            }
        }
        else {
            p++;
        }
    }
    
    return values[0];
}

char* trim_left(const char *str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }
    return strdup(str);
}
bool validateArgCount(ArgRequirement req, int actualArgCount) {
    switch(req.type) {
        case ARG_EXACT: 
            return actualArgCount == req.value;
        case ARG_MIN:
            return actualArgCount >= req.value;
        case ARG_MAX:
            return actualArgCount <= req.value;
        case ARG_RANGE:
            return actualArgCount >= req.value && actualArgCount <= req.value2;
        default:
            return false;
    }
}

int parse_program(OrtaVM *vm, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        return 0;
    }
    
    char buffer[8192];
    size_t bytes_read = fread(buffer, sizeof(char), 8191, fp);
    buffer[bytes_read] = '\0';
    fclose(fp);
    
    Vector lines = split_to_vector(buffer, "\n");
    
    VECTOR_FOR_EACH(char *, line, &lines) {
        if (*line == NULL) continue;
        
        char *trimmed = trim_left(*line);
        if (trimmed == NULL || strlen(trimmed) == 0 || starts_with(";", trimmed)) {
            free(trimmed);
            continue;
        }
        
        Vector tokens = split_to_vector(trimmed, " ");
        free(trimmed);
        if (tokens.size < 1) {
            vector_free(&tokens);
            continue;
        }
        
        for (size_t i = 0; i < tokens.size; i++) {
            char *token = vector_get_str(&tokens, i);
            if (token != NULL && starts_with(";", token)) {
                while (tokens.size > i) {
                    vector_remove(&tokens, i);
                }
                break;
            }
        }
        
        if (tokens.size == 0) {
            vector_free(&tokens);
            continue;
        }
        
        char *first_token = vector_get_str(&tokens, 0);
        if (first_token != NULL && is_label_declaration(first_token)) {
            size_t label_len = strlen(first_token);
            char *label_name = strdup(first_token);
            label_name[label_len - 1] = '\0';

            add_label(&vm->program, label_name, vm->program.instructions_count);
            //printf("Adding label: '%s' at address %zu\n", label_name, vm->program.instructions_count);
            free(label_name);
            
            vector_remove(&tokens, 0);
            
            if (tokens.size == 0) {
                vector_free(&tokens);
                continue;
            }
            
            first_token = vector_get_str(&tokens, 0);
        }
        
        if (first_token == NULL) {
            vector_free(&tokens);
            continue;
        }
        
        char *instruction_name = strdup(first_token);
        vector_remove(&tokens, 0);
        
        Instruction parsed_instruction = parse_instruction(instruction_name);
        free(instruction_name);
        
        if (parsed_instruction == -1) {
            fprintf(stderr, "Error: Unknown instruction '%s'\n", first_token);
            vector_free(&tokens);
            vector_free(&lines);
            return 0;
        }
        
        ArgRequirement expected_args = instruction_expected_args(parsed_instruction);
        if (!validateArgCount(expected_args, tokens.size)) {
            fprintf(stderr, "Error: Expected %d arguments for instruction '%s', but got %zu\n",
                    expected_args.value, instruction_to_string(parsed_instruction), tokens.size);
            vector_free(&tokens);
            vector_free(&lines);
            return 0;
        }
        
        InstructionData instr;
        instr.opcode = parsed_instruction;
        instr.operands = tokens;
        add_instruction(&vm->program, instr);
    }
    
    vector_free(&lines);
    return 1;
}

char *hmerge(InstructionData *instr) {
     char *string = malloc(256);
     VECTOR_FOR_EACH(char *, elem, &instr->operands) {
         char *rcharp = *(char **)elem;
         rcharp[strlen(rcharp)-1] = '\0'; 
         strcat(string, rcharp+1);
         strcat(string, " ");
     }
     return string;
}


WordType get_type(char *s) {
    if (strcmp(s, "int") == 0) {
        return WINT;
    } else if (strcmp(s, "float") == 0) {
        return WFLOAT;
    } else if (strcmp(s, "charp") == 0) {
        return WCHARP;
    } else if (strcmp(s, "char") == 0) {
        return W_CHAR;
    } else if (strcmp(s, "bool") == 0) {
        return WBOOL;
    } else if (strcmp(s, "pointer") == 0) {
        return WPOINTER;
    }
    return -1;
}

// TODO: add instructions with operands else use stack
void execute_instruction(OrtaVM *vm, InstructionData *instr) {
    XPU *xpu = &vm->xpu;
    Word w1, w2, result;
    int *int_result;
    float *float_result;
    char *str_result;
    XRegister *regs = vm->xpu.registers;
    XRegister *rax = &regs[REG_RAX];
    XRegister *rbx = &regs[REG_RBX];
    XRegister *rcx = &regs[REG_RCX];
    XRegister *rsi = &regs[REG_RSI];
    XRegister *rdx = &regs[REG_RDX];

    switch (instr->opcode) {
        case IPUSH:
            if (instr->operands.size > 0) {
                add_flag(vm, FLAG_STACK);
                char *operand = vector_get_str(&instr->operands, 0);
                if (is_number(operand)) {
                    int *value = malloc(sizeof(int));
                    *value = atoi(operand);
                    xstack_push(&xpu->stack, word_create(value, WINT));
                } else if (is_float(operand)) {
                    float *value = malloc(sizeof(float));
                    *value = atof(operand);
                    xstack_push(&xpu->stack, word_create(value, WFLOAT));
                } else if (is_string(operand)) {
                    size_t len = strlen(operand);
                    char *value = malloc(len - 1);
                    strncpy(value, operand + 1, len-2);
                    value[len-2] = '\0';
                    xstack_push(&xpu->stack, word_create(value, WCHARP));
                } else if (is_register(operand)) {
                    XRegisters reg = register_name_to_enum(operand);
                    if (reg != -1) {
                        xstack_push(&xpu->stack, xpu->registers[reg].reg_value);
                    }
                }
            }
            break;
        case IMOV:
            if (instr->operands.size >= 2) {
                char *src_operand = vector_get_str(&instr->operands, 0);
                char *dst_operand = vector_get_str(&instr->operands, 1);
        
                if (is_register(dst_operand)) {
                    XRegisters dst_reg = register_name_to_enum(dst_operand);
                    if (dst_reg != -1) {
                        if (is_register(src_operand)) {
                            XRegisters src_reg = register_name_to_enum(src_operand);
                            if (src_reg != -1) {
                                Word src_word = xpu->registers[src_reg].reg_value;
                                if (dst_reg == src_reg) break;
                                if (xpu->registers[dst_reg].reg_value.value && xpu->registers[dst_reg].reg_value.value != NULL) free(xpu->registers[dst_reg].reg_value.value);
                                if (src_word.type == WINT) {
                                    int *value = malloc(sizeof(int));
                                    *value = *(int*)src_word.value;
                                    xpu->registers[dst_reg].reg_value = word_create(value, WINT);
                                } else if (src_word.type == WFLOAT) {
                                    float *value = malloc(sizeof(float));
                                    *value = *(float*)src_word.value;
                                    xpu->registers[dst_reg].reg_value = word_create(value, WFLOAT);
                                } else if (src_word.type == WCHARP) {
                                    char *value = strdup((char*)src_word.value);
                                    xpu->registers[dst_reg].reg_value = word_create(value, WCHARP);
                                } else if (src_word.type == WPOINTER) {
                                    void **value = malloc(sizeof(void*));
                                    *value = src_word.value;
                                    xpu->registers[dst_reg].reg_value = word_create(value, WPOINTER);
                                } else if (src_word.type == W_CHAR) {
                                    char *value = malloc(sizeof(char));
                                    *value = *(char*)src_word.value;
                                    xpu->registers[dst_reg].reg_value = word_create(value, W_CHAR);
                                } else if (src_word.type == WBOOL) {
                                    int *value = malloc(sizeof(int));
                                    *value = *(int*)src_word.value;
                                    xpu->registers[dst_reg].reg_value = word_create(value, WBOOL);
                                } else {
                                    xpu->registers[dst_reg].reg_value = word_create(NULL, WPOINTER);
                                }
                            }
                        } else if (is_number(src_operand)) {
                            int *value = malloc(sizeof(int));
                            *value = atoi(src_operand);
                            xpu->registers[dst_reg].reg_value = word_create(value, WINT);
                        } else if (is_float(src_operand)) {
                            float *value = malloc(sizeof(float));
                            *value = atof(src_operand);
                            xpu->registers[dst_reg].reg_value = word_create(value, WFLOAT);
                        } else if (is_string(src_operand)) {
                            size_t len = strlen(src_operand) - 2;
                            char *value = malloc(len + 1);
                            strncpy(value, src_operand + 1, len);
                            value[len] = '\0';
                            xpu->registers[dst_reg].reg_value = word_create(value, WCHARP);
                        }
            }
        }
    }
    break;

        case IPOP:
            add_flag(vm, FLAG_STACK);
            if (instr->operands.size >= 1) {
				char *operand = vector_get_str(&instr->operands, 0);
                if (is_register(operand)) {
                    XRegisters target_reg = register_name_to_enum(operand);
                    if (target_reg != -1) {
                        xpu->registers[target_reg].reg_value = xstack_pop(&xpu->stack);
                    }
                }
            }
            break;
            
        case IADD:
            if (instr->operands.size == 0) {
                w1 = xstack_pop(&xpu->stack);
                w2 = xstack_pop(&xpu->stack);
                
                if (w1.type == WINT && w2.type == WINT) {
                    int_result = malloc(sizeof(int));
                    *int_result = *(int*)w2.value + *(int*)w1.value;
                    xstack_push(&xpu->stack, word_create(int_result, WINT));
                } else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                    float_result = malloc(sizeof(float));
                    *float_result = *(float*)w2.value + *(float*)w1.value;
                    xstack_push(&xpu->stack, word_create(float_result, WFLOAT));
                }
                free(w1.value);
                free(w2.value);
            } else {
                if (instr->operands.size < 2) {
                    fprintf(stderr, "Error: IADD requires two operands\n");
                    break;
                }
                char *dest_op = vector_get_str(&instr->operands, 0);
                char *src_op = vector_get_str(&instr->operands, 1);

                if (!is_register(dest_op)) {
                    fprintf(stderr, "Error: Destination must be a register\n");
                    break;
                }

                XRegisters dest_reg = register_name_to_enum(dest_op);
                if (dest_reg == -1) {
                    fprintf(stderr, "Error: Invalid register %s\n", dest_op);
                    break;
                }

                XRegister *dest_reg_ptr = &xpu->registers[dest_reg];
                Word *dest_word = &dest_reg_ptr->reg_value;

                Word src_word;
                bool src_is_temp = false;

                if (is_register(src_op)) {
                    XRegisters src_reg = register_name_to_enum(src_op);
                    if (src_reg == -1) {
                        fprintf(stderr, "Error: Invalid register %s\n", src_op);
                        break;
                    }
                    src_word = xpu->registers[src_reg].reg_value;
                } else if (is_number(src_op)) {
                    int *val = malloc(sizeof(int));
                    *val = atoi(src_op);
                    src_word = word_create(val, WINT);
                    src_is_temp = true;
                } else if (is_float(src_op)) {
                    float *val = malloc(sizeof(float));
                    *val = atof(src_op);
                    src_word = word_create(val, WFLOAT);
                    src_is_temp = true;
                } else {
                    fprintf(stderr, "Error: Invalid source operand %s\n", src_op);
                    break;
                }

                if (dest_word->type == WINT && src_word.type == WINT) {
                    int result = *(int*)dest_word->value + *(int*)src_word.value;
                    free(dest_word->value);
                    int *new_val = malloc(sizeof(int));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WFLOAT && src_word.type == WFLOAT) {
                    float result = *(float*)dest_word->value + *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WINT && src_word.type == WFLOAT) {
                    float result = (float)*(int*)dest_word->value + *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                    dest_word->type = WFLOAT;
                } else if (dest_word->type == WFLOAT && src_word.type == WINT) {
                    float result = *(float*)dest_word->value + (float)*(int*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else {
                    fprintf(stderr, "Error: Type mismatch in IADD\n");
                    if (src_is_temp) free(src_word.value);
                    break;
                }

                if (src_is_temp) free(src_word.value);
            }
            break;
            
        case ISUB:
            if (instr->operands.size == 0) {
                w1 = xstack_pop(&xpu->stack);
                w2 = xstack_pop(&xpu->stack);
                
                if (w1.type == WINT && w2.type == WINT) {
                    int_result = malloc(sizeof(int));
                    *int_result = *(int*)w2.value - *(int*)w1.value;
                    xstack_push(&xpu->stack, word_create(int_result, WINT));
                } else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                    float_result = malloc(sizeof(float));
                    *float_result = *(float*)w2.value - *(float*)w1.value;
                    xstack_push(&xpu->stack, word_create(float_result, WFLOAT));
                }
                free(w1.value);
                free(w2.value);
            } else {
                if (instr->operands.size < 2) {
                    fprintf(stderr, "Error: ISUB requires two operands\n");
                    break;
                }
                char *dest_op = vector_get_str(&instr->operands, 0);
                char *src_op = vector_get_str(&instr->operands, 1);

                if (!is_register(dest_op)) {
                    fprintf(stderr, "Error: Destination must be a register\n");
                    break;
                }

                XRegisters dest_reg = register_name_to_enum(dest_op);
                if (dest_reg == -1) {
                    fprintf(stderr, "Error: Invalid register %s\n", dest_op);
                    break;
                }

                XRegister *dest_reg_ptr = &xpu->registers[dest_reg];
                Word *dest_word = &dest_reg_ptr->reg_value;

                Word src_word;
                bool src_is_temp = false;

                if (is_register(src_op)) {
                    XRegisters src_reg = register_name_to_enum(src_op);
                    if (src_reg == -1) {
                        fprintf(stderr, "Error: Invalid register %s\n", src_op);
                        break;
                    }
                    src_word = xpu->registers[src_reg].reg_value;
                } else if (is_number(src_op)) {
                    int *val = malloc(sizeof(int));
                    *val = atoi(src_op);
                    src_word = word_create(val, WINT);
                    src_is_temp = true;
                } else if (is_float(src_op)) {
                    float *val = malloc(sizeof(float));
                    *val = atof(src_op);
                    src_word = word_create(val, WFLOAT);
                    src_is_temp = true;
                } else {
                    fprintf(stderr, "Error: Invalid source operand %s\n", src_op);
                    break;
                }

                if (dest_word->type == WINT && src_word.type == WINT) {
                    int result = *(int*)dest_word->value - *(int*)src_word.value;
                    free(dest_word->value);
                    int *new_val = malloc(sizeof(int));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WFLOAT && src_word.type == WFLOAT) {
                    float result = *(float*)dest_word->value - *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WINT && src_word.type == WFLOAT) {
                    float result = (float)*(int*)dest_word->value - *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                    dest_word->type = WFLOAT;
                } else if (dest_word->type == WFLOAT && src_word.type == WINT) {
                    float result = *(float*)dest_word->value - (float)*(int*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else {
                    fprintf(stderr, "Error: Type mismatch in ISUB\n");
                    if (src_is_temp) free(src_word.value);
                    break;
                }

                if (src_is_temp) free(src_word.value);
            }
            break;
            
        case IMUL:
            if (instr->operands.size == 0) {
                w1 = xstack_pop(&xpu->stack);
                w2 = xstack_pop(&xpu->stack);
                
                if (w1.type == WINT && w2.type == WINT) {
                    int_result = malloc(sizeof(int));
                    *int_result = *(int*)w2.value * *(int*)w1.value;
                    xstack_push(&xpu->stack, word_create(int_result, WINT));
                } else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                    float_result = malloc(sizeof(float));
                    *float_result = *(float*)w2.value * *(float*)w1.value;
                    xstack_push(&xpu->stack, word_create(float_result, WFLOAT));
                }
                free(w1.value);
                free(w2.value);
            } else {
                if (instr->operands.size < 2) {
                    fprintf(stderr, "Error: IMUL requires two operands\n");
                    break;
                }
                char *dest_op = vector_get_str(&instr->operands, 0);
                char *src_op = vector_get_str(&instr->operands, 1);

                if (!is_register(dest_op)) {
                    fprintf(stderr, "Error: Destination must be a register\n");
                    break;
                }

                XRegisters dest_reg = register_name_to_enum(dest_op);
                if (dest_reg == -1) {
                    fprintf(stderr, "Error: Invalid register %s\n", dest_op);
                    break;
                }

                XRegister *dest_reg_ptr = &xpu->registers[dest_reg];
                Word *dest_word = &dest_reg_ptr->reg_value;

                Word src_word;
                bool src_is_temp = false;

                if (is_register(src_op)) {
                    XRegisters src_reg = register_name_to_enum(src_op);
                    if (src_reg == -1) {
                        fprintf(stderr, "Error: Invalid register %s\n", src_op);
                        break;
                    }
                    src_word = xpu->registers[src_reg].reg_value;
                } else if (is_number(src_op)) {
                    int *val = malloc(sizeof(int));
                    *val = atoi(src_op);
                    src_word = word_create(val, WINT);
                    src_is_temp = true;
                } else if (is_float(src_op)) {
                    float *val = malloc(sizeof(float));
                    *val = atof(src_op);
                    src_word = word_create(val, WFLOAT);
                    src_is_temp = true;
                } else {
                    fprintf(stderr, "Error: Invalid source operand %s\n", src_op);
                    break;
                }

                if (dest_word->type == WINT && src_word.type == WINT) {
                    int result = *(int*)dest_word->value * *(int*)src_word.value;
                    free(dest_word->value);
                    int *new_val = malloc(sizeof(int));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WFLOAT && src_word.type == WFLOAT) {
                    float result = *(float*)dest_word->value * *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WINT && src_word.type == WFLOAT) {
                    float result = (float)*(int*)dest_word->value * *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                    dest_word->type = WFLOAT;
                } else if (dest_word->type == WFLOAT && src_word.type == WINT) {
                    float result = *(float*)dest_word->value * (float)*(int*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else {
                    fprintf(stderr, "Error: Type mismatch in IMUL\n");
                    if (src_is_temp) free(src_word.value);
                    break;
                }

                if (src_is_temp) free(src_word.value);
            }
            break;
            
        case IDIV:
            if (instr->operands.size == 0) {
                w1 = xstack_pop(&xpu->stack);
                w2 = xstack_pop(&xpu->stack);
                
                if (w1.type == WINT && w2.type == WINT) {
                    if (*(int*)w1.value == 0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        free(w1.value);
                        free(w2.value);
                    } else {
                        int_result = malloc(sizeof(int));
                        *int_result = *(int*)w2.value / *(int*)w1.value;
                        xstack_push(&xpu->stack, word_create(int_result, WINT));
                    }
                } else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                    if (*(float*)w1.value == 0.0f) {
                        fprintf(stderr, "Error: Division by zero\n");
                        free(w1.value);
                        free(w2.value);
                    } else {
                        float_result = malloc(sizeof(float));
                        *float_result = *(float*)w2.value / *(float*)w1.value;
                        xstack_push(&xpu->stack, word_create(float_result, WFLOAT));
                    }
                }
                free(w1.value);
                free(w2.value);
            } else {
                if (instr->operands.size < 2) {
                    fprintf(stderr, "Error: IDIV requires two operands\n");
                    break;
                }
                char *dest_op = vector_get_str(&instr->operands, 0);
                char *src_op = vector_get_str(&instr->operands, 1);

                if (!is_register(dest_op)) {
                    fprintf(stderr, "Error: Destination must be a register\n");
                    break;
                }

                XRegisters dest_reg = register_name_to_enum(dest_op);
                if (dest_reg == -1) {
                    fprintf(stderr, "Error: Invalid register %s\n", dest_op);
                    break;
                }

                XRegister *dest_reg_ptr = &xpu->registers[dest_reg];
                Word *dest_word = &dest_reg_ptr->reg_value;

                Word src_word;
                bool src_is_temp = false;

                if (is_register(src_op)) {
                    XRegisters src_reg = register_name_to_enum(src_op);
                    if (src_reg == -1) {
                        fprintf(stderr, "Error: Invalid register %s\n", src_op);
                        break;
                    }
                    src_word = xpu->registers[src_reg].reg_value;
                } else if (is_number(src_op)) {
                    int *val = malloc(sizeof(int));
                    *val = atoi(src_op);
                    src_word = word_create(val, WINT);
                    src_is_temp = true;
                } else if (is_float(src_op)) {
                    float *val = malloc(sizeof(float));
                    *val = atof(src_op);
                    src_word = word_create(val, WFLOAT);
                    src_is_temp = true;
                } else {
                    fprintf(stderr, "Error: Invalid source operand %s\n", src_op);
                    break;
                }

                if (src_word.type == WINT && *(int*)src_word.value == 0) {
                    fprintf(stderr, "Error: Division by zero\n");
                    if (src_is_temp) free(src_word.value);
                    break;
                } else if (src_word.type == WFLOAT && *(float*)src_word.value == 0.0f) {
                    fprintf(stderr, "Error: Division by zero\n");
                    if (src_is_temp) free(src_word.value);
                    break;
                }

                if (dest_word->type == WINT && src_word.type == WINT) {
                    int result = *(int*)dest_word->value / *(int*)src_word.value;
                    free(dest_word->value);
                    int *new_val = malloc(sizeof(int));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WFLOAT && src_word.type == WFLOAT) {
                    float result = *(float*)dest_word->value / *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else if (dest_word->type == WINT && src_word.type == WFLOAT) {
                    float result = (float)*(int*)dest_word->value / *(float*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                    dest_word->type = WFLOAT;
                } else if (dest_word->type == WFLOAT && src_word.type == WINT) {
                    float result = *(float*)dest_word->value / (float)*(int*)src_word.value;
                    free(dest_word->value);
                    float *new_val = malloc(sizeof(float));
                    *new_val = result;
                    dest_word->value = new_val;
                } else {
                    fprintf(stderr, "Error: Type mismatch in IDIV\n");
                    if (src_is_temp) free(src_word.value);
                    break;
                }

                if (src_is_temp) free(src_word.value);
            }
            break;

        case IMOD:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT && w2.type == WINT) {
                if (*(int*)w1.value == 0) {
                    fprintf(stderr, "Error: Modulo by zero\n");
                    free(w1.value);
                    free(w2.value);
                } else {
                    int_result = malloc(sizeof(int));
                    *int_result = *(int*)w2.value % *(int*)w1.value;
                    xstack_push(&xpu->stack, word_create(int_result, WINT));
                }
            }
            free(w1.value);
            free(w2.value);
            break;
            
        case IAND:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT && w2.type == WINT) {
                int_result = malloc(sizeof(int));
                *int_result = *(int*)w2.value & *(int*)w1.value;
                xstack_push(&xpu->stack, word_create(int_result, WINT));
            }
            free(w1.value);
            free(w2.value);
            break;
            
        case IOR:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT && w2.type == WINT) {
                int_result = malloc(sizeof(int));
                *int_result = *(int*)w2.value | *(int*)w1.value;
                xstack_push(&xpu->stack, word_create(int_result, WINT));
            }
            free(w1.value);
            break;
            
        case IXOR:
            if (instr->operands.size >= 2) {
				char *sreg1 = vector_get_str(&instr->operands, 0);
                char *sreg2 = vector_get_str(&instr->operands, 1);
                if (is_register(sreg1) && is_register(sreg2)) {
                    XRegisters preg1 = register_name_to_enum(sreg1);
                    XRegisters preg2 = register_name_to_enum(sreg2);
                    XRegister reg1 = xpu->registers[preg1];
                    XRegister reg2 = xpu->registers[preg2];
                    if (preg1 != -1 && preg2 != -1 && reg1.reg_value.type == WINT && reg1.reg_value.type == WINT) {
                        *(int*)reg1.reg_value.value ^= *(int*)reg2.reg_value.value;
                    }
                }
            } 
            break;
            
        case INOT:
            w1 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT) {
                int_result = malloc(sizeof(int));
                *int_result = ~(*(int*)w1.value);
                xstack_push(&xpu->stack, word_create(int_result, WINT));
            }
            
            break;
            
        case IEQ:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            int_result = malloc(sizeof(int));
            *int_result = 0;
            
            if (w1.type == w2.type) {
                if (w1.type == WINT) {
                    *int_result = (*(int*)w1.value == *(int*)w2.value) ? 1 : 0;
                } else if (w1.type == WFLOAT) {
                    *int_result = (*(float*)w1.value == *(float*)w2.value) ? 1 : 0;
                } else if (w1.type == WCHARP) {
                    *int_result = (strcmp((char*)w1.value, (char*)w2.value) == 0) ? 1 : 0;
                }
            }
            
            xstack_push(&xpu->stack, word_create(int_result, WINT));
            break;
            
        case INE:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            int_result = malloc(sizeof(int));
            *int_result = 1;
            
            if (w1.type == w2.type) {
                if (w1.type == WINT) {
                    *int_result = (*(int*)w1.value != *(int*)w2.value) ? 1 : 0;
                } else if (w1.type == WFLOAT) {
                    *int_result = (*(float*)w1.value != *(float*)w2.value) ? 1 : 0;
                } else if (w1.type == WCHARP) {
                    *int_result = (strcmp((char*)w1.value, (char*)w2.value) != 0) ? 1 : 0;
                }
            }
            
            xstack_push(&xpu->stack, word_create(int_result, WINT));
            break;
            
        case ILT:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            int_result = malloc(sizeof(int));
            *int_result = 0;
            
            if (w1.type == w2.type) {
                if (w1.type == WINT) {
                    *int_result = (*(int*)w2.value < *(int*)w1.value) ? 1 : 0;
                } else if (w1.type == WFLOAT) {
                    *int_result = (*(float*)w2.value < *(float*)w1.value) ? 1 : 0;
                }
            }
            
            xstack_push(&xpu->stack, word_create(int_result, WINT));
            break;
            
        case IGT:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            int_result = malloc(sizeof(int));
            *int_result = 0;
            
            if (w1.type == w2.type) {
                if (w1.type == WINT) {
                    *int_result = (*(int*)w2.value > *(int*)w1.value) ? 1 : 0;
                } else if (w1.type == WFLOAT) {
                    *int_result = (*(float*)w2.value > *(float*)w1.value) ? 1 : 0;
                }
            }
            
            xstack_push(&xpu->stack, word_create(int_result, WINT));
            free(w1.value);
            free(w2.value);
            break;
            
        case ILE:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            int_result = malloc(sizeof(int));
            *int_result = 0;
            
            if (w1.type == w2.type) {
                if (w1.type == WINT) {
                    *int_result = (*(int*)w2.value <= *(int*)w1.value) ? 1 : 0;
                } else if (w1.type == WFLOAT) {
                    *int_result = (*(float*)w2.value <= *(float*)w1.value) ? 1 : 0;
                }
            }
           
            xstack_push(&xpu->stack, word_create(int_result, WINT));
            free(w1.value);
            free(w2.value);
            break;
            
        case IGE:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            int_result = malloc(sizeof(int));
            *int_result = 0;
            
            if (w1.type == w2.type) {
                if (w1.type == WINT) {
                    *int_result = (*(int*)w2.value >= *(int*)w1.value) ? 1 : 0;
                } else if (w1.type == WFLOAT) {
                    *int_result = (*(float*)w2.value >= *(float*)w1.value) ? 1 : 0;
                }
            }
            
            xstack_push(&xpu->stack, word_create(int_result, WINT));
            free(w1.value);
            free(w2.value);
            break;
            
        case IJMP:
           if (instr->operands.size > 0) {
                char *operand = vector_get_str(&instr->operands, 0);
                size_t target = 0;
                
                if (is_number(operand)) {
                    target = atoi(operand);
                } else if (is_label_reference(operand)) {
                    if (!find_label(&vm->program, operand, &target)) {
                        fprintf(stderr, "Error: Unknown label '%s'\n", operand);
                        break;
                    }
                }
                
                if (target < vm->program.instructions_count) {
                    xpu->ip = target;
                    return;
                }
            }
            break; 
        case IJMPIF:
            w1 = xstack_pop(&xpu->stack);
            if (w1.type == WINT && *(int*)w1.value != 0) {
                if (instr->operands.size > 0) {
                    char *operand = vector_get_str(&instr->operands, 0);
                    size_t target = 0;
                    
                    if (is_number(operand)) {
                        target = atoi(operand);
                    } else if (is_label_reference(operand)) {
                        if (!find_label(&vm->program, operand, &target)) {
                            fprintf(stderr, "Error: Unknown label '%s'\n", operand);
                            free(w1.value);
                            break;
                        }
                    }
                    
                    if (target < vm->program.instructions_count) {
                        xpu->ip = target;
                        free(w1.value);
                        return;
                    }
                }
            }
            free(w1.value);
            break; 
        case ICALL:
                if (instr->operands.size > 0) {
                char *operand = vector_get_str(&instr->operands, 0);
                size_t target = 0;
                
                if (is_number(operand)) {
                    target = atoi(operand);
                } else if (is_label_reference(operand)) {
                    if (!find_label(&vm->program, operand, &target)) {
                        fprintf(stderr, "Error: Unknown label '%s'\n", operand);
                        break;
                    }
                }
                
                if (target < vm->program.instructions_count) {
                    int *return_address = malloc(sizeof(int));
                    *return_address = xpu->ip + 1;
                    xpu->registers[REG_RA].reg_value = word_create(return_address, WINT);
                    xpu->ip = target;
                    return;
                }
            }
            break; 
        case IRET:
            w1 = xpu->registers[REG_RA].reg_value;
            if (w1.type == WINT) {
                xpu->ip = *(int*)w1.value;
                free(w1.value);
                xpu->registers[REG_RA].reg_value.value = NULL;
                xpu->registers[REG_RA].reg_value.type = WPOINTER;
                return;
            }
            free(w1.value);
            xpu->registers[REG_RA].reg_value.value = NULL;
            xpu->registers[REG_RA].reg_value.type = WPOINTER;
            break;

        case ILOAD:
            if (instr->operands.size > 0) {
                char *operand = vector_get_str(&instr->operands, 0);
                if (is_register(operand)) {
                    XRegisters reg = register_name_to_enum(operand);
                    if (reg != -1) {
                        xstack_push(&xpu->stack, xpu->registers[reg].reg_value);
                    }
                }
            }
            break;
            
        case ISTORE:
            w1 = xstack_pop(&xpu->stack);
            if (instr->operands.size > 0) {
                char *operand = vector_get_str(&instr->operands, 0);
                if (is_register(operand)) {
                    XRegisters reg = register_name_to_enum(operand);
                    if (reg != -1) {
                        if (xpu->registers[reg].reg_value.value) {
                            free(xpu->registers[reg].reg_value.value);
                        }
                        xpu->registers[reg].reg_value = w1;
                    } else {
                        free(w1.value);
                    }
                } else {
                    free(w1.value);
                }
            } else {
                free(w1.value);
            }
            break;
            
        case IPRINT: {
            if (instr->operands.size == 0) {
                w1 = xstack_peek(&xpu->stack, 0);
                if (w1.type == WINT) {
                    printf("%d\n", *(int*)w1.value);
                } else if (w1.type == WFLOAT) {
                    printf("%f\n", *(float*)w1.value);
                } else if (w1.type == WCHARP) {
                    printf("%s\n", (char*)w1.value);
                } else if (w1.type == W_CHAR) {
                    printf("%c\n", *(char*)w1.value);
                }
            } else if (instr->operands.size == 1 && is_register(vector_get_str(&instr->operands, 0))) {
                XRegister reg = regs[register_name_to_enum(vector_get_str(&instr->operands, 0))];
                if (reg.reg_value.type == WINT) {
                    printf("%d\n", *(int*)reg.reg_value.value);
                } else if (reg.reg_value.type == WCHARP) {
                    printf("%s", (char*)reg.reg_value.value);
                } // TODO: add more
            } else {
                char *string = hmerge(instr);
                printf("%s\n", string); 
            }
                     } break;
            
        case IDUP:
            w1 = xstack_peek(&xpu->stack, 0);
            if (w1.type == WINT) {
                int *value = malloc(sizeof(int));
                *value = *(int*)w1.value;
                xstack_push(&xpu->stack, word_create(value, WINT));
            } else if (w1.type == WFLOAT) {
                float *value = malloc(sizeof(float));
                *value = *(float*)w1.value;
                xstack_push(&xpu->stack, word_create(value, WFLOAT));
            } else if (w1.type == WCHARP) {
                char *value = strdup((char*)w1.value);
                xstack_push(&xpu->stack, word_create(value, WCHARP));
            }
            break;
            
        case ISWAP:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            xstack_push(&xpu->stack, w1);
            xstack_push(&xpu->stack, w2);
            break;
            
        case IDROP:
            w1 = xstack_pop(&xpu->stack);
            break;
            
        case IROTL:
            if (instr->operands.size > 0) {
                char *operand = vector_get_str(&instr->operands, 0);
                if (is_number(operand)) {
                    int n = atoi(operand);
                    if (n <= 0 || n >= xpu->stack.count) break;
                    
                    Word *temp = malloc(sizeof(Word) * n);
                    for (int i = 0; i < n; i++) {
                        temp[i] = xstack_pop(&xpu->stack);
                    }
                    
                    Word bottom = temp[0];
                    for (int i = 0; i < n - 1; i++) {
                        temp[i] = temp[i + 1];
                    }
                    temp[n - 1] = bottom;
                    
                    for (int i = n - 1; i >= 0; i--) {
                        xstack_push(&xpu->stack, temp[i]);
                    }
                    
                    free(temp);
                }
            }
            break;
            
        case IROTR:
            if (instr->operands.size > 0) {
                char *operand = vector_get_str(&instr->operands, 0);
                if (is_number(operand)) {
                    int n = atoi(operand);
                    if (n <= 0 || n >= xpu->stack.count) break;
                    
                    Word *temp = malloc(sizeof(Word) * n);
                    for (int i = 0; i < n; i++) {
                        temp[i] = xstack_pop(&xpu->stack);
                    }
                    
                    Word top = temp[n - 1];
                    for (int i = n - 1; i > 0; i--) {
                        temp[i] = temp[i - 1];
                    }
                    temp[0] = top;
                    
                    for (int i = n - 1; i >= 0; i--) {
                        xstack_push(&xpu->stack, temp[i]);
                    }
                    
                    free(temp);
                }
            }
            break;
            
        case IALLOC:
            if (instr->operands.size > 0) {
                add_flag(vm, FLAG_MEMORY);
                char *operand = vector_get_str(&instr->operands, 0);
                if (is_number(operand)) {
                    size_t size = atoi(operand);
                    void *memory = (char *)vm->program.memory + vm->program.memory_used;
                    vm->program.memory_used += size;
                    xstack_push(&xpu->stack, word_create(memory, WPOINTER));
                }
            }
            break;
            
        case IMERGE: {
            if (instr->operands.size == 0) {
                Word w1 = xstack_pop(&xpu->stack);
                Word w2 = xstack_pop(&xpu->stack);
    
                if (w1.type == WCHARP && w2.type == WCHARP) {
                    char *str1 = (char *)w1.value;
                    char *str2 = (char *)w2.value;
                    size_t len1 = strlen(str1);
                    size_t len2 = strlen(str2);
                    char *merged = malloc(len1 + len2 + 2);
                    if (!merged) {
                        fprintf(stderr, "Error: Memory allocation failed for IMERGE\n");
                        free(str1);
                        free(str2);
                        break;
                    }
                    strcpy(merged, str2);
                    strcat(merged, " ");
                    strcat(merged, str1);
                    xstack_push(&xpu->stack, word_create(merged, WCHARP));
                    free(str1);
                    free(str2);
                } else {
                    fprintf(stderr, "Error: IMERGE requires two string operands\n");
                    free(w1.value);
                    free(w2.value);
                }
            } else {
                char *tp = hmerge(instr);
                xstack_push(&xpu->stack, word_create(tp, WCHARP));
            }
            break;
        }
        case IXCALL: {
            if (rax->reg_value.type == WINT) {
                switch (*(int *)rax->reg_value.value) {
                    case 1: { // sleep 
                                 if (rbx->reg_value.type == WINT) {
                                    xsleep(*(int *)rbx->reg_value.value);
                                 }
                            } break;
                    case 2: { // CMD call 
                                if (rbx->reg_value.type == WCHARP) {
                                    add_flag(vm, FLAG_CMD);
                                    int r = system((char *)rbx->reg_value.value);
                                    printf("[XCALL] %s returned %d\n", (char*)rbx->reg_value.value);
                                }                    
        
                            } break;
                    case 3: { // cast
                                if (rbx->reg_value.type == WCHARP) {
                                    Word w = xstack_pop(&xpu->stack);
                                    w.type = get_type((char *)rbx->reg_value.value);
                                    xstack_push(&xpu->stack, w);
                                }
                            } break;
                    case 4: { // get nil onto the stack 
                                xstack_push(&xpu->stack, word_create(NULL, WPOINTER));
                            } break; 
                    case 5: { // strlen
                                if (rbx->reg_value.type == WCHARP) {
                                    int r = strlen((char *)rbx->reg_value.value);
                                    int *result = malloc(sizeof(int));
                                    *result = r;
                                    xstack_push(&xpu->stack, word_create(result, WINT));
                                }
                            } break;
                    default: {  
                                printf("WARNING: Unknown opcode\n");
                             } break;
                }
            } else {
                fprintf(stderr, "ERROR: invalid code (Invalid type)\n");
            } 


                     } break;
        case IWRITE: {
                if (rbx->reg_value.type == WPOINTER) {
                    void *address = rbx->reg_value.value;
        
                    if (rcx->reg_value.type == WINT) {
                        int offset = *(int*)rcx->reg_value.value;
                        address = (char*)address + offset;
                    }
        
                    if (rax->reg_value.type == WINT) {
                        *(int*)address = *(int*)rax->reg_value.value;
                    } else if (rax->reg_value.type == WFLOAT) {
                        *(float*)address = *(float*)rax->reg_value.value;
                    } else if (rax->reg_value.type == WCHARP) {
                        size_t len = strlen((char*)rax->reg_value.value) + 1;
                        memcpy(address, rax->reg_value.value, len);
                    } else if (rax->reg_value.type == W_CHAR) {
                        *(char*)address = *(char*)rax->reg_value.value;
                    } else {
                        fprintf(stderr, "Error: Unsupported type in RAX for IWRITE\n");
                    }
                } else {
                    fprintf(stderr, "Error: IWRITE requires a pointer in RBX\n");
                }
                     } break;
        case IPRINTMEM: {
            if (rbx->reg_value.type == WPOINTER) {
                void *address = rbx->reg_value.value;
        
                int format = 0; // 0=bytes, 1=chars, 2=ints, 3=floats, 4=string
                if (rax->reg_value.type == WINT) {
                    format = *(int*)rax->reg_value.value;
                }
        
                if (rcx->reg_value.type == WINT) {
                    int size = *(int*)rcx->reg_value.value;
            
                    printf("Memory at %p (size %d):\n", address, size);
            
                    switch (format) {
                        case 0:
                            for (int i = 0; i < size; i++) {
                                if (i > 0 && i % 16 == 0) printf("\n");
                                printf("%02x ", ((unsigned char*)address)[i]);
                            }
                            printf("\n");
                            break;
                    
                        case 1:
                            for (int i = 0; i < size; i++) {
                                if (i > 0 && i % 64 == 0) printf("\n");
                                char c = ((char*)address)[i];
                                printf("%c", (c >= 32 && c <= 126) ? c : '.');
                            }
                            printf("\n");
                            break;
                    
                        case 2:
                            for (int i = 0; i < size / sizeof(int); i++) {
                                if (i > 0 && i % 8 == 0) printf("\n");
                                printf("%d ", ((int*)address)[i]);
                            }
                            printf("\n");
                            break;
                    
                        case 3:
                            for (int i = 0; i < size / sizeof(float); i++) {
                                if (i > 0 && i % 8 == 0) printf("\n");
                                printf("%f ", ((float*)address)[i]);
                            }
                            printf("\n");
                            break;
                    
                        case 4:
                            if (size > 0) {
                                printf("%s\n", (char*)address);
                            }
                            break;
                
                        default:
                            fprintf(stderr, "Error: Unknown format %d for IPRINTMEM\n", format);
                    }
                } else {
                    fprintf(stderr, "Error: IPRINTMEM requires an integer size in RCX\n");
                }
            } else {
                fprintf(stderr, "Error: IPRINTMEM requires a pointer in RBX\n");
            }
                        } break;
        case ISIZEOF: {
                char *operand = vector_get_str(&instr->operands, 0);   
                printf("o = %s\n", operand);
                WordType type = get_type(operand);
                int size = 0;

                switch (type) {
                    case WINT:     size = sizeof(int);      break;
                    case W_CHAR:    size = sizeof(char);     break;
                    case WCHARP:   size = sizeof(char *);   break;
                    case WPOINTER: size = sizeof(void *);   break;
                    case WFLOAT:   size = sizeof(float);    break;
                    case WBOOL:    size = sizeof(bool);     break;
                }
                xstack_push(&xpu->stack, word_create(&size, WINT));

                      } break;
        case IDEC: {
                char *operand = vector_get_str(&instr->operands, 0); // reg
                if (is_register(operand)) {
                    XRegister *reg = &xpu->registers[register_name_to_enum(operand)];
                     if (reg->reg_value.type == WINT) {
                        *(int *)reg->reg_value.value -= 1;
                     }
                }

                   } break;
        case IINC: {
                char *operand = vector_get_str(&instr->operands, 0); // reg
                if (is_register(operand)) {
                    XRegister *reg = &xpu->registers[register_name_to_enum(operand)];
                     if (reg->reg_value.type == WINT) {
                        *(int *)reg->reg_value.value += 1;
                     }
                }
                   } break;
        case IEVAL: {
                char *str = hmerge(instr);
                int e = eval(str);
                xstack_push(&xpu->stack, word_create(&e, WINT));
                    } break;
        case ICMP: {
				if (instr->operands.size >= 2) {
						char *operand1 = vector_get_str(&instr->operands, 0);
						char *operand2 = vector_get_str(&instr->operands, 1);
						
						Word val1, val2;
						bool free_val1 = false, free_val2 = false;
						
						if (is_register(operand1)) {
								XRegisters reg = register_name_to_enum(operand1);
								if (reg != -1) {
										val1 = xpu->registers[reg].reg_value;
								} else {
										fprintf(stderr, "Error: Invalid register '%s'\n", operand1);
										break;
								}
						} else if (is_number(operand1)) {
								int *value = malloc(sizeof(int));
								*value = atoi(operand1);
								val1 = word_create(value, WINT);
								free_val1 = true;
						} else if (is_float(operand1)) {
								float *value = malloc(sizeof(float));
								*value = atof(operand1);
								val1 = word_create(value, WFLOAT);
								free_val1 = true;
						} else if (is_string(operand1)) {
								size_t len = strlen(operand1) - 2;
								char *value = malloc(len + 1);
								strncpy(value, operand1 + 1, len);
								value[len] = '\0';
								val1 = word_create(value, WCHARP);
								free_val1 = true;
						} else if (operand1[0] == '\'' && strlen(operand1) >= 2) {
								char *value = malloc(sizeof(char));
								*value = operand1[1];
								val1 = word_create(value, W_CHAR);
								free_val1 = true;
						} else {
								fprintf(stderr, "Error: Invalid operand '%s'\n", operand1);
								break;
						}
						
						if (is_register(operand2)) {
								XRegisters reg = register_name_to_enum(operand2);
								if (reg != -1) {
										val2 = xpu->registers[reg].reg_value;
								} else {
										if (free_val1) free(val1.value);
										fprintf(stderr, "Error: Invalid register '%s'\n", operand2);
										break;
								}
						} else if (is_number(operand2)) {
								int *value = malloc(sizeof(int));
								*value = atoi(operand2);
								val2 = word_create(value, WINT);
								free_val2 = true;
						} else if (is_float(operand2)) {
								float *value = malloc(sizeof(float));
								*value = atof(operand2);
								val2 = word_create(value, WFLOAT);
								free_val2 = true;
						} else if (is_string(operand2)) {
								size_t len = strlen(operand2) - 2;
								char *value = malloc(len + 1);
								strncpy(value, operand2 + 1, len);
								value[len] = '\0';
								val2 = word_create(value, WCHARP);
								free_val2 = true;
						} else if (operand2[0] == '\'' && strlen(operand2) >= 2) {
								char *value = malloc(sizeof(char));
								*value = operand2[1];
								val2 = word_create(value, W_CHAR);
								free_val2 = true;
						} else {
								if (free_val1) free(val1.value);
								fprintf(stderr, "Error: Invalid operand '%s'\n", operand2);
								break;
						}
						
						int *result = malloc(sizeof(int));
						*result = 0;
						
						if (val1.type == val2.type) {
								if (val1.type == WINT) {
										int v1 = *(int*)val1.value;
										int v2 = *(int*)val2.value;
										if (v1 == v2) *result = 0;
										else if (v1 < v2) *result = -1;
										else *result = 1;
								} else if (val1.type == WFLOAT) {
										float v1 = *(float*)val1.value;
										float v2 = *(float*)val2.value;
										if (v1 == v2) *result = 0;
										else if (v1 < v2) *result = -1;
										else *result = 1;
								} else if (val1.type == WCHARP) {
										int cmp = strcmp((char*)val1.value, (char*)val2.value);
										*result = cmp;
								} else if (val1.type == W_CHAR) {
										char v1 = *(char*)val1.value;
										char v2 = *(char*)val2.value;
										if (v1 == v2) *result = 0;
										else if (v1 < v2) *result = -1;
										else *result = 1;
								}
						} else {
							printf("Type val1: %d & type val2: %d\n", val1.type, val2.type);	
                            *result = -999;
						}
						
						if (rdx->reg_value.value) {
								free(rdx->reg_value.value);
						}
						rdx->reg_value = word_create(result, WINT);
						
						if (free_val1) free(val1.value);
						if (free_val2) free(val2.value);
				}
                   } break;
            case ILOADB: {
                if (instr->operands.size > 0) {
                    char *operand = vector_get_str(&instr->operands, 0);
                    if (is_register(operand)) {
                        XRegisters reg = register_name_to_enum(operand);
                        if (reg != -1) {
                            Word w = xpu->registers[reg].reg_value;
                            if (w.type == WPOINTER) {
                                void *address = w.value;
                                if (rcx->reg_value.type == WINT) {
                                    int offset = *(int*)rcx->reg_value.value;
                                    address = (char*)address + offset;
                                }
                                if (address) {
                                    char *value = malloc(sizeof(char));
                                    *value = *(char*)address;
                                    xstack_push(&xpu->stack, word_create(value, W_CHAR));
                                } else {
                                    fprintf(stderr, "Error: Invalid pointer\n");
                                    exit(1);
                                }
                            } else if (w.type == WCHARP) {
                                char *str = (char*)w.value;
                                char *value = malloc(sizeof(char));
                                *value = *str;
                                xstack_push(&xpu->stack, word_create(value, W_CHAR));
                            } else {
                                fprintf(stderr, "Error: ILOADB requires a pointer or string in register '%s'\n", operand);
                            }
                        } else {
                            fprintf(stderr, "Error: Invalid register '%s'\n", operand);
                        }
                    } else if (is_pointer(operand)) {
                        void *address = get_pointer(operand);
                        if (address) {
                            char *value = malloc(sizeof(char));
                            *value = *(char*)address;
                            xstack_push(&xpu->stack, word_create(value, W_CHAR));
                        } else {
                            fprintf(stderr, "Error: Invalid pointer format '%s'\n", operand);
                            exit(1);
                        }
                        if (rcx->reg_value.type == WINT) {
                            int offset = *(int*)rcx->reg_value.value;
                            address = (char*)address + offset;
                        }

                    } else if (is_string(operand)) {
                        operand = operand + 1;
                        operand[strlen(operand) - 1] = '\0';
                        if (xpu->registers[REG_RCX].reg_value.type == WINT) {
                            int offset = *(int*)xpu->registers[REG_RCX].reg_value.value;
                            operand = operand + offset;
                        }
                        if (operand != NULL && *operand != '\0') {
                            char *value = malloc(sizeof(char));
                            *value = *operand;
                            xstack_push(&xpu->stack, word_create(value, W_CHAR));
                        } else {
                            fprintf(stderr, "Error: ILOADB out of bounds string access\n");
                        }
                    } else {
                        fprintf(stderr, "Error: ILOADB operand must be a register or pointer\n");
                    }
                } else {
                    Word w = xstack_pop(&xpu->stack);
                    if (w.type == WPOINTER) {
                        void *address = w.value;
                        if (rcx->reg_value.type == WINT) {
                            int offset = *(int*)rcx->reg_value.value;
                            address = (char*)address + offset;
                        }
                        char *value = malloc(sizeof(char));
                        *value = *(char*)address;
                        xstack_push(&xpu->stack, word_create(value, W_CHAR));
                    } else if (w.type == WCHARP) {
                        char *str = (char*)w.value;
                        if (xpu->registers[REG_RCX].reg_value.type == WINT) {
                           int offset = *(int*)xpu->registers[REG_RCX].reg_value.value;
                            str = str + offset;
                        }
                        if (str!= NULL && *str!= '\0') {
                            char *value = malloc(sizeof(char));
                            *value = *str;
                            xstack_push(&xpu->stack, word_create(value, W_CHAR));
                        } else {
                            fprintf(stderr, "Error: ILOADB out of bounds string access\n");
                        }
                    } else {
                        fprintf(stderr, "Error: ILOADB requires pointer or string on stack\n");
                    }
                }
            } break;
        case IHALT:
            return;
    }
    
    xpu->ip++;
}

void execute_program(OrtaVM *vm) {
    XPU *xpu = &vm->xpu;
    size_t f = 0;
    if (!find_label(&vm->program, OENTRY, &f)) {
        xpu->ip = 0;
        printf("WARNING: No entry found running at 0\n");
    } else {
        xpu->ip = f;
    }
    
    while (xpu->ip < vm->program.instructions_count) {
        execute_instruction(vm, &vm->program.instructions[xpu->ip]);
    }
}

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define CYAN    "\033[36m"

static void print_word(size_t index, Word w) {
    printf("[%zu] ", index);
    switch (w.type) {
        case WINT:
            printf("INT     : %d", *(int*)w.value);
            break;
        case WFLOAT:
            printf("FLOAT   : %f", *(float*)w.value);
            break;
        case WCHARP:
            printf("STRING  : %s", (char*)w.value);
            break;
        case W_CHAR:
            printf("CHAR    : %c", *(char*)w.value);
            break;
        case WPOINTER:
            printf("POINTER : %p", w.value);
            break;
        case WBOOL:
            printf("BOOL    : %s", (*(int*)w.value) ? "true" : "false");
            break;
        default:
            printf("UNKNOWN");
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
            case WINT:
                printf("INT     : %d", *(int*)w.value);
                break;
            case WFLOAT:
                printf("FLOAT   : %f", *(float*)w.value);
                break;
            case WCHARP:
                printf("STRING  : %s", (char*)w.value);
                break;
            case W_CHAR:
                printf("CHAR    : %c", *(char*)w.value);
                break;
            case WPOINTER:
                printf("POINTER : %p", w.value);
                break;
            case WBOOL:
                printf("BOOL    : %s", (*(int*)w.value) ? "true" : "false");
                break;
            default:
                printf("EMPTY");
        }
        printf("\n");
    }
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
            char **operand_ptr = vector_get(&program->instructions[i].operands, j);
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
    
    char *filename = malloc(filename_len + 1);
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

    program->instructions = malloc(sizeof(InstructionData) * instructions_count);
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
            
            char *temp_operand = malloc(operand_len + 1);
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

    program->labels = malloc(sizeof(Label) * labels_count);
    if (!program->labels)
        goto error_instructions;
    
    program->labels_count = labels_count;
    program->labels_capacity = labels_count;

    for (i = 0; i < labels_count; i++) {
        size_t name_len;
        if (fread(&name_len, sizeof(size_t), 1, fp) != 1)
            goto error_labels;
        
        char *name = malloc(name_len + 1);
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

typedef struct {
    char name[MAX_DEFINE_NAME];
    char value[MAX_DEFINE_VALUE];
    int is_function_macro;
    int argc;
    char args[MAX_ARGS][MAX_ARG_LEN];
} Define;

Define defines[MAX_DEFINES];
int define_count = 0;

char include_paths[MAX_INCLUDE_PATHS][MAX_LINE_LENGTH];
int include_path_count = 0;


void add_include_path(const char *path) {
    if (include_path_count >= MAX_INCLUDE_PATHS) {
        fprintf(stderr, "Warning: Too many include paths\n");
        return;
    }

    char resolved_path[MAX_LINE_LENGTH];
    if (path[0] == '~') {
        const char *home = NULL;

        #if defined(_WIN32) || defined(_WIN64)
        home = getenv("USERPROFILE");
        #else
        home = getenv("HOME");
        #endif

        if (home) {
            snprintf(resolved_path, sizeof(resolved_path), "%s%s", home, path + 1);
        } else {
            strncpy(resolved_path, path, sizeof(resolved_path) - 1);
            resolved_path[sizeof(resolved_path) - 1] = '\0';
        }
    } else {
        strncpy(resolved_path, path, sizeof(resolved_path) - 1);
        resolved_path[sizeof(resolved_path) - 1] = '\0';
    }

    strncpy(include_paths[include_path_count], resolved_path, MAX_LINE_LENGTH - 1);
    include_paths[include_path_count][MAX_LINE_LENGTH - 1] = '\0';
    include_path_count++;
}

void add_simple_define(const char *name, const char *value) {
    if (define_count >= MAX_DEFINES) {
        fprintf(stderr, "Warning: Too many #define directives\n");
        return;
    }
    
    strncpy(defines[define_count].name, name, MAX_DEFINE_NAME - 1);
    defines[define_count].name[MAX_DEFINE_NAME - 1] = '\0';
    
    strncpy(defines[define_count].value, value, MAX_DEFINE_VALUE - 1);
    defines[define_count].value[MAX_DEFINE_VALUE - 1] = '\0';
    
    defines[define_count].is_function_macro = 0;
    defines[define_count].argc = 0;
    
    define_count++;
}

void add_function_macro(const char *name, const char *args_str, const char *value) {
    if (define_count >= MAX_DEFINES) {
        fprintf(stderr, "Warning: Too many #define directives\n");
        return;
    }
    
    Define *d = &defines[define_count];
    
    strncpy(d->name, name, MAX_DEFINE_NAME - 1);
    d->name[MAX_DEFINE_NAME - 1] = '\0';
    
    strncpy(d->value, value, MAX_DEFINE_VALUE - 1);
    d->value[MAX_DEFINE_VALUE - 1] = '\0';
    
    d->is_function_macro = 1;
    d->argc = 0;
    
    char arg_buffer[MAX_LINE_LENGTH];
    strncpy(arg_buffer, args_str, MAX_LINE_LENGTH - 1);
    arg_buffer[MAX_LINE_LENGTH - 1] = '\0';
    
    char *token = strtok(arg_buffer, ",");
    while (token && d->argc < MAX_ARGS) {
        while (isspace(*token)) token++;
        
        char *end = token + strlen(token) - 1;
        while (end > token && isspace(*end)) end--;
        *(end + 1) = '\0';
        
        strncpy(d->args[d->argc], token, MAX_ARG_LEN - 1);
        d->args[d->argc][MAX_ARG_LEN - 1] = '\0';
        d->argc++;
        
        token = strtok(NULL, ",");
    }
    
    define_count++;
}

char *replace_args_in_macro(Define *d, char args_values[MAX_ARGS][MAX_ARG_LEN]) {
    static char result[MAX_DEFINE_VALUE];
    char *p = d->value;
    int i = 0;
    
    while (*p && i < MAX_DEFINE_VALUE - 1) {
        int replaced = 0;
        
        for (int j = 0; j < d->argc; j++) {
            if (strncmp(p, d->args[j], strlen(d->args[j])) == 0 &&
                (!isalnum(p[strlen(d->args[j])]) && p[strlen(d->args[j])] != '_')) {
                
                strcpy(result + i, args_values[j]);
                i += strlen(args_values[j]);
                p += strlen(d->args[j]);
                replaced = 1;
                break;
            }
        }
        
        if (!replaced) {
            result[i++] = *p++;
        }
    }
    
    result[i] = '\0';
    return result;
}

char *parse_macro_args(char *p, Define *d, char args_values[MAX_ARGS][MAX_ARG_LEN]) {
    int arg_count = 0;
    int paren_depth = 1;
    char arg_buffer[MAX_ARG_LEN];
    int buffer_idx = 0;
    
    p++;
    
    while (*p && paren_depth > 0 && arg_count < MAX_ARGS) {
        if (*p == '(' && *(p-1) != '\\') {
            paren_depth++;
            arg_buffer[buffer_idx++] = *p++;
        } else if (*p == ')' && *(p-1) != '\\') {
            paren_depth--;
            if (paren_depth == 0) {
                if (buffer_idx > 0) {
                    arg_buffer[buffer_idx] = '\0';
                    strncpy(args_values[arg_count], arg_buffer, MAX_ARG_LEN - 1);
                    args_values[arg_count][MAX_ARG_LEN - 1] = '\0';
                    arg_count++;
                }
                p++;
                break;
            } else {
                arg_buffer[buffer_idx++] = *p++;
            }
        } else if (*p == ',' && paren_depth == 1 && *(p-1) != '\\') {
            arg_buffer[buffer_idx] = '\0';
            strncpy(args_values[arg_count], arg_buffer, MAX_ARG_LEN - 1);
            args_values[arg_count][MAX_ARG_LEN - 1] = '\0';
            arg_count++;
            buffer_idx = 0;
            p++;
        } else {
            if (buffer_idx < MAX_ARG_LEN - 1) {
                arg_buffer[buffer_idx++] = *p;
            }
            p++;
        }
    }
    
    if (arg_count != d->argc) {
        fprintf(stderr, "Warning: Macro %s expects %d arguments, got %d\n", 
                d->name, d->argc, arg_count);
    }
    
    return p;
}

char *process_defines(char *line) {
    static char result[MAX_LINE_LENGTH];
    char *p = line;
    int i = 0;
    
    while (*p && i < MAX_LINE_LENGTH - 1) {
        int replaced = 0;
        
        for (int j = 0; j < define_count; j++) {
            Define *d = &defines[j];
            
            if (strncmp(p, d->name, strlen(d->name)) == 0) {
                char next_char = p[strlen(d->name)];
                
                if (d->is_function_macro && next_char == '(') {
                    char args_values[MAX_ARGS][MAX_ARG_LEN];
                    char *new_p = parse_macro_args(p + strlen(d->name), d, args_values);
                    
                    char *expanded = replace_args_in_macro(d, args_values);
                    strcpy(result + i, expanded);
                    i += strlen(expanded);
                    p = new_p;
                    replaced = 1;
                    break;
                } else if (!d->is_function_macro && (!isalnum(next_char) && next_char != '_')) {
                    strcpy(result + i, d->value);
                    i += strlen(d->value);
                    p += strlen(d->name);
                    replaced = 1;
                    break;
                }
            }
        }
        
        if (!replaced) {
            result[i++] = *p++;
        }
    }
    
    result[i] = '\0';
    return result;
}

int parse_define_directive(char *line) {
    char *p = line + 7;
    while (isspace(*p)) p++;
    
    char define_name[MAX_DEFINE_NAME];
    int name_idx = 0;
    
    while (*p && !isspace(*p) && *p != '(' && name_idx < MAX_DEFINE_NAME - 1) {
        define_name[name_idx++] = *p++;
    }
    define_name[name_idx] = '\0';
    
    if (*p == '(') {
        p++;
        char args_str[MAX_LINE_LENGTH];
        int args_idx = 0;
        int paren_depth = 1;
        
        while (*p && paren_depth > 0 && args_idx < MAX_LINE_LENGTH - 1) {
            if (*p == '(') paren_depth++;
            else if (*p == ')') paren_depth--;
            
            if (paren_depth > 0) {
                args_str[args_idx++] = *p;
            }
            p++;
        }
        args_str[args_idx] = '\0';
        
        while (isspace(*p)) p++;
        
        add_function_macro(define_name, args_str, p);
    } else {
        while (isspace(*p)) p++;
        add_simple_define(define_name, p);
    }
    
    return 0;
}

FILE *find_include_file(const char *filename) {
    FILE *file = NULL;
    
    file = fopen(filename, "r");
    if (file) return file;
    
    char full_path[MAX_LINE_LENGTH];
    for (int i = 0; i < include_path_count; i++) {
        snprintf(full_path, MAX_LINE_LENGTH, "%s/%s", include_paths[i], filename);
        file = fopen(full_path, "r");
        if (file) {
            return file;
        }
    }
    
    return NULL;
}

int process_include(const char *filename, FILE *output, int depth) {
    if (depth >= MAX_INCLUDE_DEPTH) {
        fprintf(stderr, "Error: Maximum include depth exceeded for %s\n", filename);
        return 0;
    }
    
    FILE *include_fp = find_include_file(filename);
    if (!include_fp) {
        fprintf(stderr, "Warning: Cannot include file %s\n", filename);
        return 0;
    }
    
    char include_line[MAX_LINE_LENGTH];
    while (fgets(include_line, MAX_LINE_LENGTH, include_fp)) {
        char trimmed_line[MAX_LINE_LENGTH] = {0};
        int i = 0, j = 0;
        int in_string = 0;

        while (include_line[i] != '\0' && include_line[i] != '\n') {
            if (include_line[i] == '"') in_string = !in_string;
            if (!in_string && include_line[i] == ';') {
                trimmed_line[j] = '\0';
                break;
            }
            trimmed_line[j++] = include_line[i++];
        }

        while (j > 0 && (trimmed_line[j - 1] == ' ' || trimmed_line[j - 1] == '\t')) j--;
        trimmed_line[j] = '\0';

        if (j > 0) {
            if (strncmp(trimmed_line, "#include", 8) == 0) {
                char nested_include_file[MAX_LINE_LENGTH];
                if (sscanf(trimmed_line + 8, " \"%[^\"]\"", nested_include_file) == 1) {
                    process_include(nested_include_file, output, depth + 1);
                } else if (sscanf(trimmed_line + 8, " <%[^>]>", nested_include_file) == 1) {
                    process_include(nested_include_file, output, depth + 1);
                }
            } else if (strncmp(trimmed_line, "#define", 7) == 0) {
                parse_define_directive(trimmed_line);
            } else {
                char *processed = process_defines(trimmed_line);
                if (processed) {
                    fputs(processed, output);
                    fputs("\n", output);
                }
            }
        }
    }
    
    fclose(include_fp);
    return 1;
}

int orta_preprocess(char *filename, char *output_file) {
    char *dpaths[] = {
        ".",
        "~/.orta/",
    };
    int path_count = 2;
    for (int i = 0; i < path_count && i < MAX_INCLUDE_PATHS; i++) {
        add_include_path(dpaths[i]);
    }
    
    FILE *input = fopen(filename, "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file %s\n", filename);
        return 0;
    }

    FILE *output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file %s\n", output_file);
        fclose(input);
        return 0;
    }

    char line[MAX_LINE_LENGTH];
    char *SNULL = malloc(sizeof(void*));
    sprintf(SNULL, "%p", NULL);
    add_simple_define("NULL", SNULL);

    while (fgets(line, MAX_LINE_LENGTH, input)) {
        char trimmed_line[MAX_LINE_LENGTH] = {0};
        int i = 0, j = 0;
        int in_string = 0;

        while (line[i] != '\0' && line[i] != '\n') {
            if (line[i] == '"') in_string = !in_string;
            if (!in_string && line[i] == ';') {
                trimmed_line[j] = '\0';
                break;
            }
            trimmed_line[j++] = line[i++];
        }

        while (j > 0 && (trimmed_line[j - 1] == ' ' || trimmed_line[j - 1] == '\t')) j--;
        trimmed_line[j] = '\0';

        if (j > 0) {
            if (strncmp(trimmed_line, "#include", 8) == 0) {
                char include_file[MAX_LINE_LENGTH];
                if (sscanf(trimmed_line + 8, " \"%[^\"]\"", include_file) == 1) {
                    process_include(include_file, output, 0);
                } else if (sscanf(trimmed_line + 8, " <%[^>]>", include_file) == 1) {
                    process_include(include_file, output, 0);
                }
            } else if (strncmp(trimmed_line, "#define", 7) == 0) {
                parse_define_directive(trimmed_line);
            } else {
                char *processed = process_defines(trimmed_line);
                if (processed) {
                    fputs(processed, output);
                    fputs("\n", output);
                }
            }
        }
    }
    
    free(SNULL);
    fclose(input);
    fclose(output);
    return 1;
}

#endif
