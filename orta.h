// TODO: add more registers
// TODO: add flags to xbin
// TODO: fix preprocessing and rewrite it 

#ifndef ORTA_H 
#define ORTA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "vector.h"
#include "str.h"

#define ODEFAULT_STACK_SIZE 16384
#define ODEFAULT_ENTRY "__entry"
#define MEMORY_CAPACITY 1024

static size_t OSTACK_SIZE = ODEFAULT_STACK_SIZE;
static char *OENTRY = ODEFAULT_ENTRY;

typedef enum {
    WINT,
    WFLOAT,
    WCHARP, 
    WCHAR,
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
    REG_RA, // Return Address | Dont use!
    REG_COUNT,
} XRegisters;

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
        xpu.registers[i].reg_size = sizeof(void *);
        xpu.registers[i].reg_value = word_create(NULL, WPOINTER);
    }
    
    return xpu;
}

void xpu_free(XPU *xpu) {
    xstack_free(&xpu->stack);
    free(xpu->registers);
}

typedef enum {
    IPUSH,
    IMOV,
    IPOP,
    IADD,
    ISUB,
    IMUL,
    IDIV,
    IMOD,
    IAND,
    IOR,
    IXOR,
    INOT,
    IEQ,
    INE,
    ILT,
    IGT,
    ILE,
    IGE,
    IJMP,
    IJMPIF,
    ICALL,
    IRET,
    ILOAD,
    ISTORE,
    IPRINT,
    IDUP,
    ISWAP,
    IDROP,
    IROTL,
    IROTR,
    IALLOC,
    IHALT,
    IMERGE,
    IXCALL,
    IWRITE,
    IPRINTMEM,
} Instruction;

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
} Program;

typedef struct {
    XPU xpu;
    Program program;
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
    free(program->memory);
}

OrtaVM ortavm_create(const char *filename) {
    OrtaVM vm;
    vm.xpu = xpu_init();
    program_init(&vm.program, filename);
    return vm;
}

void ortavm_free(OrtaVM *vm) {
    xpu_free(&vm->xpu);
    program_free(&vm->program);
}

Instruction parse_instruction(const char *instruction) {
    if (strcmp("push", instruction) == 0) return IPUSH;
    if (strcmp("mov", instruction) == 0) return IMOV;
    if (strcmp("pop", instruction) == 0) return IPOP;
    if (strcmp("add", instruction) == 0) return IADD;
    if (strcmp("sub", instruction) == 0) return ISUB;
    if (strcmp("mul", instruction) == 0) return IMUL;
    if (strcmp("div", instruction) == 0) return IDIV;
    if (strcmp("mod", instruction) == 0) return IMOD;
    if (strcmp("and", instruction) == 0) return IAND;
    if (strcmp("or", instruction) == 0) return IOR;
    if (strcmp("xor", instruction) == 0) return IXOR;
    if (strcmp("not", instruction) == 0) return INOT;
    if (strcmp("eq", instruction) == 0) return IEQ;
    if (strcmp("ne", instruction) == 0) return INE;
    if (strcmp("lt", instruction) == 0) return ILT;
    if (strcmp("gt", instruction) == 0) return IGT;
    if (strcmp("le", instruction) == 0) return ILE;
    if (strcmp("ge", instruction) == 0) return IGE;
    if (strcmp("jmp", instruction) == 0) return IJMP;
    if (strcmp("jmpif", instruction) == 0) return IJMPIF;
    if (strcmp("call", instruction) == 0) return ICALL;
    if (strcmp("ret", instruction) == 0) return IRET;
    if (strcmp("load", instruction) == 0) return ILOAD;
    if (strcmp("store", instruction) == 0) return ISTORE;
    if (strcmp("print", instruction) == 0) return IPRINT;
    if (strcmp("dup", instruction) == 0) return IDUP;
    if (strcmp("swap", instruction) == 0) return ISWAP;
    if (strcmp("drop", instruction) == 0) return IDROP;
    if (strcmp("rotl", instruction) == 0) return IROTL;
    if (strcmp("rotr", instruction) == 0) return IROTR;
    if (strcmp("alloc", instruction) == 0) return IALLOC;
    if (strcmp("halt", instruction) == 0) return IHALT;
    if (strcmp("merge", instruction) == 0) return IMERGE;
    if (strcmp("xcall", instruction) == 0) return IXCALL;
    if (strcmp("write", instruction) == 0) return IWRITE;
    if (strcmp("printmem", instruction) == 0) return IPRINTMEM;
    return -1;
}

const char *instruction_to_string(Instruction instruction) {
    switch (instruction) {
        case IPUSH: return "push";
        case IMOV: return "mov";
        case IPOP: return "pop";
        case IADD: return "add";
        case ISUB: return "sub";
        case IMUL: return "mul";
        case IDIV: return "div";
        case IMOD: return "mod";
        case IAND: return "and";
        case IOR: return "or";
        case IXOR: return "xor";
        case INOT: return "not";
        case IEQ: return "eq";
        case INE: return "ne";
        case ILT: return "lt";
        case IGT: return "gt";
        case ILE: return "le";
        case IGE: return "ge";
        case IJMP: return "jmp";
        case IJMPIF: return "jmpif";
        case ICALL: return "call";
        case IRET: return "ret";
        case ILOAD: return "load";
        case ISTORE: return "store";
        case IPRINT: return "print";
        case IDUP: return "dup";
        case ISWAP: return "swap";
        case IDROP: return "drop";
        case IROTL: return "rotl";
        case IROTR: return "rotr";
        case IALLOC: return "alloc";
        case IHALT: return "halt";
        case IMERGE: return "merge";
        case IXCALL: return "xcall";
        case IWRITE: return "write";
        case IPRINTMEM: return "printmem";
        default: return "unknown";
    }
}

int instruction_expected_args(Instruction instruction) {
    switch (instruction) {
        case IPUSH: return 1;
        case IMOV: return 2;
        case IPOP: return 1;
        case IADD: return 0;
        case ISUB: return 0;
        case IMUL: return 0;
        case IDIV: return 0;
        case IMOD: return 0;
        case IAND: return 0;
        case IOR: return 0;
        case IXOR: return 0;
        case INOT: return 0;
        case IEQ: return 0;
        case INE: return 0;
        case ILT: return 0;
        case IGT: return 0;
        case ILE: return 0;
        case IGE: return 0;
        case IJMP: return 1;
        case IJMPIF: return 1;
        case ICALL: return 1;
        case IRET: return 0;
        case ILOAD: return 1;
        case ISTORE: return 1;
        case IPRINT: return 0;
        case IDUP: return 0;
        case ISWAP: return 0;
        case IDROP: return 0;
        case IROTL: return 1;
        case IROTR: return 1;
        case IALLOC: return 1;
        case IHALT: return 0;
        case IMERGE: return 0;
        case IXCALL: return 0;
        case IWRITE: return 0;
        case IPRINTMEM: return 0;
        default: return -1;
    }
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

int is_register(const char *str) {
    if (str[0] != 'r' && str[0] != 'R') return 0;
    
    const char *reg_names[] = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9"};
    for (int i = 0; i < REG_COUNT; i++) {
        if (strcasecmp(str + 1, reg_names[i] + 1) == 0) return 1;
    }
    
    return 0;
}

int is_label_declaration(const char *str) {
    return str[0] == ':';
}

int is_label_reference(const char *str) {
    return !is_number(str) && !is_float(str) && !is_register(str) && !is_string(str);
}

XRegisters register_name_to_enum(const char *reg_name) {
    if (strcasecmp(reg_name, "rax") == 0) return REG_RAX;
    if (strcasecmp(reg_name, "rbx") == 0) return REG_RBX;
    if (strcasecmp(reg_name, "rcx") == 0) return REG_RCX;
    if (strcasecmp(reg_name, "rdx") == 0) return REG_RDX;
    if (strcasecmp(reg_name, "rsi") == 0) return REG_RSI;
    if (strcasecmp(reg_name, "rdi") == 0) return REG_RDI;
    if (strcasecmp(reg_name, "r8") == 0) return REG_R8;
    if (strcasecmp(reg_name, "r9") == 0) return REG_R9;
    if (strcasecmp(reg_name, "ra") == 0) return REG_RA;
    return -1;
}

void add_instruction(Program *program, InstructionData instr) {
    if (program->instructions_count >= program->instructions_capacity) {
        program->instructions_capacity *= 2;
        program->instructions = realloc(program->instructions, sizeof(InstructionData) * program->instructions_capacity);
    }
    
    program->instructions[program->instructions_count++] = instr;
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
        if (strlen(*line) == 0 || starts_with("//", *line)) continue;

        Vector tokens = split_to_vector(*line, " ");
        if (tokens.size < 1) {
            vector_free(&tokens);
            continue;
        }

        char *first_token = vector_get_str(&tokens, 0);
        if (is_label_declaration(first_token)) {
            char *label_name = strdup(first_token + 1);
            add_label(&vm->program, label_name, vm->program.instructions_count);
            free(label_name);
            
            if (tokens.size == 1) {
                vector_free(&tokens);
                continue;
            }
            
            vector_remove(&tokens, 0);
        }
        
        vector_free(&tokens);
    }
    
    size_t instruction_index = 0;
    VECTOR_FOR_EACH(char *, line, &lines) {
        if (strlen(*line) == 0 || starts_with("//", *line)) continue;

        Vector tokens = split_to_vector(*line, " ");
        if (tokens.size < 1) {
            vector_free(&tokens);
            continue;
        }

        char *first_token = vector_get_str(&tokens, 0);
        if (is_label_declaration(first_token)) {
            if (tokens.size == 1) {
                vector_free(&tokens);
                continue;
            }
            
            vector_remove(&tokens, 0);
            first_token = vector_get_str(&tokens, 0);
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

        int expected_args = instruction_expected_args(parsed_instruction);
        if (expected_args != -1 && tokens.size != expected_args) {
            fprintf(stderr, "Error: Expected %d arguments for instruction '%s', but got %zu\n",
                    expected_args, instruction_to_string(parsed_instruction), tokens.size);
            vector_free(&tokens);
            vector_free(&lines);
            return 0;
        }

        InstructionData instr;
        instr.opcode = parsed_instruction;
        instr.operands = tokens;

        add_instruction(&vm->program, instr);
        instruction_index++;
    }

    vector_free(&lines);
    return 1;
}

WordType get_type(char *s) {
    if (strcmp(s, "int") == 0) {
        return WINT;
    } else if (strcmp(s, "float") == 0) {
        return WFLOAT;
    } else if (strcmp(s, "charp") == 0) {
        return WCHARP;
    } else if (strcmp(s, "char") == 0) {
        return WCHAR;
    } else if (strcmp(s, "bool") == 0) {
        return WBOOL;
    } else if (strcmp(s, "pointer") == 0) {
        return WPOINTER;
    }
    return -1;
}

void execute_instruction(OrtaVM *vm, InstructionData *instr) {
    XPU *xpu = &vm->xpu;
    Word w1, w2, result;
    int *int_result;
    float *float_result;
    char *str_result;
    
    switch (instr->opcode) {
        case IPUSH:
            if (instr->operands.size > 0) {
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
						if (xpu->registers[dst_reg].reg_value.value) {
							free(xpu->registers[dst_reg].reg_value.value);
                            xpu->registers[dst_reg].reg_value.value = NULL;
						}
						
						if (is_register(src_operand)) {
							XRegisters src_reg = register_name_to_enum(src_operand);
							if (src_reg != -1) {
								Word src_word = xpu->registers[src_reg].reg_value;
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
            break;
            
        case ISUB:
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
            break;
            
        case IMUL:
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
            break;
            
        case IDIV:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT && w2.type == WINT) {
                if (*(int*)w1.value == 0) {
                    fprintf(stderr, "Error: Division by zero\n");
                } else {
                    int_result = malloc(sizeof(int));
                    *int_result = *(int*)w2.value / *(int*)w1.value;
                    xstack_push(&xpu->stack, word_create(int_result, WINT));
                }
            } else if (w1.type == WFLOAT && w2.type == WFLOAT) {
                if (*(float*)w1.value == 0.0f) {
                    fprintf(stderr, "Error: Division by zero\n");
                } else {
                    float_result = malloc(sizeof(float));
                    *float_result = *(float*)w2.value / *(float*)w1.value;
                    xstack_push(&xpu->stack, word_create(float_result, WFLOAT));
                }
            }
            
            free(w1.value);
            free(w2.value);
            break;
            
        case IMOD:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT && w2.type == WINT) {
                if (*(int*)w1.value == 0) {
                    fprintf(stderr, "Error: Modulo by zero\n");
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
            free(w2.value);
            break;
            
        case IXOR:
            w1 = xstack_pop(&xpu->stack);
            w2 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT && w2.type == WINT) {
                int_result = malloc(sizeof(int));
                *int_result = *(int*)w2.value ^ *(int*)w1.value;
                xstack_push(&xpu->stack, word_create(int_result, WINT));
            }
            
            free(w1.value);
            free(w2.value);
            break;
            
        case INOT:
            w1 = xstack_pop(&xpu->stack);
            
            if (w1.type == WINT) {
                int_result = malloc(sizeof(int));
                *int_result = ~(*(int*)w1.value);
                xstack_push(&xpu->stack, word_create(int_result, WINT));
            }
            
            free(w1.value);
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
            free(w1.value);
            free(w2.value);
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
            free(w1.value);
            free(w2.value);
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
            free(w1.value);
            free(w2.value);
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
                return;
            }
            free(w1.value);
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
            
        case IPRINT:
            w1 = xstack_peek(&xpu->stack, 0);
            if (w1.type == WINT) {
                printf("%d\n", *(int*)w1.value);
            } else if (w1.type == WFLOAT) {
                printf("%f\n", *(float*)w1.value);
            } else if (w1.type == WCHARP) {
                printf("%s\n", (char*)w1.value);
            } else if (w1.type == WCHAR) {
                printf("%c\n", *(char*)w1.value);
            }
            break;
            
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
            free(w1.value);
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
            break;
        }
        case IXCALL: { // TODO: logic like syscalls but for my asm | xcall <rax = code>
            XRegister *regs = vm->xpu.registers;
            XRegister rax = regs[REG_RAX];
            XRegister rbx = regs[REG_RBX];
            XRegister rcx = regs[REG_RCX];
            XRegister rsi = regs[REG_RSI];
            XRegister rdx = regs[REG_RDX];
            if (rax.reg_value.type == WINT) {
                switch (*(int *)rax.reg_value.value) {
    
                }
            } else {
                fprintf(stderr, "ERROR: invalid code (Invalid type)\n");
            }


                     } break;
        case IWRITE: {
                XRegister *rax = &xpu->registers[REG_RAX];
                XRegister *rbx = &xpu->registers[REG_RBX];
                XRegister *rcx = &xpu->registers[REG_RCX];
    
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
                    } else if (rax->reg_value.type == WCHAR) {
                        *(char*)address = *(char*)rax->reg_value.value;
                    } else {
                        fprintf(stderr, "Error: Unsupported type in RAX for IWRITE\n");
                    }
                } else {
                    fprintf(stderr, "Error: IWRITE requires a pointer in RBX\n");
                }
                     } break;
        case IPRINTMEM: {
            XRegister *rbx = &xpu->registers[REG_RBX]; // pointer
            XRegister *rcx = &xpu->registers[REG_RCX]; // amount
            XRegister *rax = &xpu->registers[REG_RAX]; // type
    
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
        printf("WARNING: No entry found running at 0");
    } else {
        xpu->ip = f;
    }
    
    while (xpu->ip < vm->program.instructions_count) {
        execute_instruction(vm, &vm->program.instructions[xpu->ip]);
    }
}

void print_stack(XPU *xpu) {
    printf("Stack (%zu items):\n", xpu->stack.count);
    for (size_t i = 0; i < xpu->stack.count; i++) {
        Word w = xpu->stack.stack[i];
        if (w.type == WINT) {
            printf("[%zu] INT: %d\n", i, *(int*)w.value);
        } else if (w.type == WFLOAT) {
            printf("[%zu] FLOAT: %f\n", i, *(float*)w.value);
        } else if (w.type == WCHARP) {
            printf("[%zu] STRING: %s\n", i, (char*)w.value);
        } else if (w.type == WCHAR) {
            printf("[%zu] CHAR: %c\n", i, *(char*)w.value);
        } else if (w.type == WPOINTER) {
            printf("[%zu] POINTER: %p\n", i, w.value);
        } else if (w.type == WBOOL) {
            printf("[%zu] BOOL: %s\n", i, (*(int*)w.value) ? "true" : "false");
        }
    }
}

void print_registers(XPU *xpu) {
    printf("Registers:\n");
    const char *reg_names[] = {"RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "R8", "R9", "RA"};
    
    for (int i = 0; i < REG_COUNT; i++) {
        Word w = xpu->registers[i].reg_value;
        printf("[%s] ", reg_names[i]);
        
        if (w.type == WINT) {
            printf("INT: %d\n", *(int*)w.value);
        } else if (w.type == WFLOAT) {
            printf("FLOAT: %f\n", *(float*)w.value);
        } else if (w.type == WCHARP) {
            printf("STRING: %s\n", (char*)w.value);
        } else if (w.type == WCHAR) {
            printf("CHAR: %c\n", *(char*)w.value);
        } else if (w.type == WPOINTER) {
            printf("POINTER: %p\n", w.value);
        } else if (w.type == WBOOL) {
            printf("BOOL: %s\n", (*(int*)w.value) ? "true" : "false");
        } else {
            printf("EMPTY\n");
        }
    }
}

int create_bytecode(Program *program, const char *output_filename) {
    FILE *fp = fopen(output_filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not create bytecode file '%s'\n", output_filename);
        return 0;
    }

    size_t filename_len = strlen(program->filename);
    if (fwrite(&filename_len, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    if (fwrite(program->filename, 1, filename_len, fp) != filename_len) {
        fclose(fp);
        return 0;
    }

    size_t instructions_count = program->instructions_count;
    if (fwrite(&instructions_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    for (size_t i = 0; i < instructions_count; i++) {
        InstructionData *instr = &program->instructions[i];
        
        if (fwrite(&instr->opcode, sizeof(Instruction), 1, fp) != 1) {
            fclose(fp);
            return 0;
        }

        size_t operands_count = instr->operands.size;
        if (fwrite(&operands_count, sizeof(size_t), 1, fp) != 1) {
            fclose(fp);
            return 0;
        }

        for (size_t j = 0; j < operands_count; j++) {
            char *operand = *(char **)vector_get(&instr->operands, j);
            size_t operand_len = strlen(operand);
            
            if (fwrite(&operand_len, sizeof(size_t), 1, fp) != 1) {
                fclose(fp);
                return 0;
            }
            
            if (fwrite(operand, 1, operand_len, fp) != operand_len) {
                fclose(fp);
                return 0;
            }
        }
    }

    size_t labels_count = program->labels_count;
    if (fwrite(&labels_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    for (size_t i = 0; i < labels_count; i++) {
        Label *label = &program->labels[i];
        
        size_t name_len = strlen(label->name);
        if (fwrite(&name_len, sizeof(size_t), 1, fp) != 1) {
            fclose(fp);
            return 0;
        }
        
        if (fwrite(label->name, 1, name_len, fp) != name_len) {
            fclose(fp);
            return 0;
        }
        
        size_t address = label->address;
        if (fwrite(&address, sizeof(size_t), 1, fp) != 1) {
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return 1;
}

int load_bytecode(Program *program, const char *input_filename) {
    FILE *fp = fopen(input_filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open bytecode file '%s'\n", input_filename);
        return 0;
    }

    program_free(program);

    size_t filename_len;
    if (fread(&filename_len, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    
    char *filename = malloc(filename_len + 1);
    if (!filename) {
        fclose(fp);
        return 0;
    }
    
    if (fread(filename, 1, filename_len, fp) != filename_len) {
        free(filename);
        fclose(fp);
        return 0;
    }
    filename[filename_len] = '\0';
    program->filename = strdup(filename);
    free(filename);

    size_t instructions_count;
    if (fread(&instructions_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    program->instructions = malloc(sizeof(InstructionData) * instructions_count);
    if (!program->instructions) {
        fclose(fp);
        return 0;
    }
    program->instructions_count = instructions_count;
    program->instructions_capacity = instructions_count;

    for (size_t i = 0; i < instructions_count; i++) {
        Instruction opcode;
        if (fread(&opcode, sizeof(Instruction), 1, fp) != 1) {
            fclose(fp);
            return 0;
        }

        size_t operands_count;
        if (fread(&operands_count, sizeof(size_t), 1, fp) != 1) {
            fclose(fp);
            return 0;
        }

        Vector operands;
        vector_init(&operands, 10, sizeof(char*));
        
        for (size_t j = 0; j < operands_count; j++) {
            size_t operand_len;
            if (fread(&operand_len, sizeof(size_t), 1, fp) != 1) {
                vector_free(&operands);
                fclose(fp);
                return 0;
            }
            
            char *temp_operand = malloc(operand_len + 1);
            if (!temp_operand) {
                vector_free(&operands);
                fclose(fp);
                return 0;
            }
            
            if (fread(temp_operand, 1, operand_len, fp) != operand_len) {
                free(temp_operand);
                vector_free(&operands);
                fclose(fp);
                return 0;
            }
            temp_operand[operand_len] = '\0';
            
            char *persistent_operand = strdup(temp_operand);
            if (!persistent_operand) {
                free(temp_operand);
                vector_free(&operands);
                fclose(fp);
                return 0;
            }
            
            vector_push(&operands, &persistent_operand);
            free(temp_operand);
        }

        program->instructions[i].opcode = opcode;
        program->instructions[i].operands = operands;
    }

    size_t labels_count;
    if (fread(&labels_count, sizeof(size_t), 1, fp) != 1) {
        for (size_t i = 0; i < program->instructions_count; i++) {
            for (size_t j = 0; j < program->instructions[i].operands.size; j++) {
                char **operand_ptr = vector_get(&program->instructions[i].operands, j);
                free(*operand_ptr);
            }
            vector_free(&program->instructions[i].operands);
        }
        free(program->instructions);
        fclose(fp);
        return 0;
    }

    program->labels = malloc(sizeof(Label) * labels_count);
    if (!program->labels) {
        for (size_t i = 0; i < program->instructions_count; i++) {
            for (size_t j = 0; j < program->instructions[i].operands.size; j++) {
                char **operand_ptr = vector_get(&program->instructions[i].operands, j);
                free(*operand_ptr);
            }
            vector_free(&program->instructions[i].operands);
        }
        free(program->instructions);
        fclose(fp);
        return 0;
    }
    program->labels_count = labels_count;
    program->labels_capacity = labels_count;

    for (size_t i = 0; i < labels_count; i++) {
        size_t name_len;
        if (fread(&name_len, sizeof(size_t), 1, fp) != 1) {
            for (size_t j = 0; j < program->instructions_count; j++) {
                for (size_t k = 0; k < program->instructions[j].operands.size; k++) {
                    char **operand_ptr = vector_get(&program->instructions[j].operands, k);
                    free(*operand_ptr);
                }
                vector_free(&program->instructions[j].operands);
            }
            free(program->instructions);
            for (size_t j = 0; j < i; j++) {
                free(program->labels[j].name);
            }
            free(program->labels);
            fclose(fp);
            return 0;
        }
        
        char *name = malloc(name_len + 1);
        if (!name) {
            for (size_t j = 0; j < program->instructions_count; j++) {
                for (size_t k = 0; k < program->instructions[j].operands.size; k++) {
                    char **operand_ptr = vector_get(&program->instructions[j].operands, k);
                    free(*operand_ptr);
                }
                vector_free(&program->instructions[j].operands);
            }
            free(program->instructions);
            for (size_t j = 0; j < i; j++) {
                free(program->labels[j].name);
            }
            free(program->labels);
            fclose(fp);
            return 0;
        }
        
        if (fread(name, 1, name_len, fp) != name_len) {
            free(name);
            for (size_t j = 0; j < program->instructions_count; j++) {
                for (size_t k = 0; k < program->instructions[j].operands.size; k++) {
                    char **operand_ptr = vector_get(&program->instructions[j].operands, k);
                    free(*operand_ptr);
                }
                vector_free(&program->instructions[j].operands);
            }
            free(program->instructions);
            for (size_t j = 0; j < i; j++) {
                free(program->labels[j].name);
            }
            free(program->labels);
            fclose(fp);
            return 0;
        }
        name[name_len] = '\0';

        size_t address;
        if (fread(&address, sizeof(size_t), 1, fp) != 1) {
            free(name);
            for (size_t j = 0; j < program->instructions_count; j++) {
                for (size_t k = 0; k < program->instructions[j].operands.size; k++) {
                    char **operand_ptr = vector_get(&program->instructions[j].operands, k);
                    free(*operand_ptr);
                }
                vector_free(&program->instructions[j].operands);
            }
            free(program->instructions);
            for (size_t j = 0; j < i; j++) {
                free(program->labels[j].name);
            }
            free(program->labels);
            fclose(fp);
            return 0;
        }

        program->labels[i].name = strdup(name);
        program->labels[i].address = address;
        free(name);
    }

    fclose(fp);
    return 1;
}

static char *trim_whitespace(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static char *orta_preprocess_file(const char *filename, char *output_file) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s': %s\n", filename, strerror(errno));
        return NULL;
    }

    char *output = malloc(1);
    if (!output) {
        fclose(fp);
        return NULL;
    }
    output[0] = '\0';
    size_t output_size = 1;

    char line[8192];
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '#') {
            if (strncmp(trimmed, "#include", 8) == 0) {
                char *start = strchr(trimmed, '<');
                if (!start) {
                    fprintf(stderr, "Error: Invalid #include directive in '%s'\n", filename);
                    free(output);
                    fclose(fp);
                    return NULL;
                }
                start++;
                char *end = strchr(start, '>');
                if (!end) {
                    fprintf(stderr, "Error: Unterminated #include in '%s'\n", filename);
                    free(output);
                    fclose(fp);
                    return NULL;
                }
                *end = '\0';
                char *include_file = strdup(start);
                if (!include_file) {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    free(output);
                    fclose(fp);
                    return NULL;
                }

                char *included_content = orta_preprocess_file(include_file, NULL);
                free(include_file);
                if (!included_content) {
                    free(output);
                    fclose(fp);
                    return NULL;
                }

                size_t included_len = strlen(included_content);
                char *new_output = realloc(output, output_size + included_len);
                if (!new_output) {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    free(included_content);
                    free(output);
                    fclose(fp);
                    return NULL;
                }
                output = new_output;
                strcat(output, included_content);
                output_size += included_len;
                free(included_content);
                continue;
            } else if (strncmp(trimmed, "#stack", 6) == 0) {
                char *size_str = trimmed + 6;
                while (*size_str && isspace((unsigned char)*size_str)) size_str++;
                if (*size_str == '\0') {
                    fprintf(stderr, "Error: Missing stack size in '%s'\n", filename);
                    free(output);
                    fclose(fp);
                    return NULL;
                }

                char *endptr;
                long new_size = strtol(size_str, &endptr, 10);
                if (*endptr != '\0' || new_size <= 0) {
                    fprintf(stderr, "Error: Invalid stack size '%s' in '%s'\n", size_str, filename);
                    free(output);
                    fclose(fp);
                    return NULL;
                }
                OSTACK_SIZE = (size_t)new_size;
                continue;
            } else if (strncmp(trimmed, "#entry", 6) == 0) {
                char *entry_str = trimmed + 6;
                while (*entry_str && isspace((unsigned char)*entry_str)) entry_str++;
                if (*entry_str == '\0') {
                    fprintf(stderr, "Error: Missing entry in '%s'\n", filename);
                    free(output);
                    fclose(fp);
                    return NULL;
                }

                OENTRY = strdup(entry_str);
                continue;
            }
        }

        size_t line_len = strlen(line);
        char *new_output = realloc(output, output_size + line_len);
        if (!new_output) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            free(output);
            fclose(fp);
            return NULL;
        }
        output = new_output;
        strcat(output, line);
        output_size += line_len;
    }

    fclose(fp);
    
    if (output_file) {
        FILE *out_fp = fopen(output_file, "w");
        if (!out_fp) {
            fprintf(stderr, "rror: Cannot open output file '%s': %s\n", output_file, strerror(errno));
            free(output);
            return NULL;
        }
        fputs(output, out_fp);
        fclose(out_fp);
    }
    
    return output;
}
#endif
