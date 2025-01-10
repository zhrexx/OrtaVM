#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define STACK_CAPACITY 20000 // 1mb * sizeof(int64_t)

typedef enum {
    RTS_OK,
    RTS_STACK_OVERFLOW,
    RTS_STACK_UNDERFLOW,
    RTS_ERROR,
    RTS_UNDEFINED_INSTRUCTION,
} RTStatus;

typedef enum {
    I_PUSH,
    I_POP,
    I_ADD,
    I_SUB,
    I_PRINT,
    I_MOV,
    I_MUL,
    I_DIV,
} Instruction;

typedef struct {
    Instruction inst;
    union {
        int64_t int_value;
        char register_name[4];
    };
} Token;

typedef struct {
    Token tokens[1024];
    size_t size;
} Program;

typedef struct {
    int64_t stack[STACK_CAPACITY];
    int64_t registers[4];
    size_t stack_size;
} OrtaVM;

RTStatus push(OrtaVM *vm, int64_t value) {
    if (vm->stack_size >= STACK_CAPACITY) {
        return RTS_STACK_OVERFLOW;
    }
    vm->stack[vm->stack_size++] = value;
    return RTS_OK;
}

int parse_register_str(const char *register_str) {
    if (strcmp(register_str, "rax") == 0) return 0;
    if (strcmp(register_str, "rbx") == 0) return 1;
    if (strcmp(register_str, "rcx") == 0) return 2;
    if (strcmp(register_str, "rdx") == 0) return 3;
    return -1;
}

RTStatus pop(OrtaVM *vm, const char *register_name) {
    if (vm->stack_size == 0) {
        return RTS_STACK_UNDERFLOW;
    }
    int register_num = parse_register_str(register_name);
    if (register_num < 0) {
        return RTS_ERROR;
    }
    vm->registers[register_num] = vm->stack[--vm->stack_size];
    return RTS_OK;
}

RTStatus add(OrtaVM *vm) {
    if (vm->stack_size < 2) {
        return RTS_STACK_UNDERFLOW;
    }
    int64_t a = vm->stack[--vm->stack_size];
    int64_t b = vm->stack[--vm->stack_size];
    return push(vm, a + b);
}

RTStatus sub(OrtaVM *vm) {
    if (vm->stack_size < 2) {
        return RTS_STACK_UNDERFLOW;
    }
    int64_t a = vm->stack[--vm->stack_size];
    int64_t b = vm->stack[--vm->stack_size];
    return push(vm, b - a);
}

RTStatus print(OrtaVM *vm) {
    if (vm->stack_size == 0) {
        return RTS_STACK_UNDERFLOW;
    }
    printf("%ld\n", vm->stack[vm->stack_size - 1]);
    return RTS_OK;
}

RTStatus OrtaVM_execute(OrtaVM *vm, Program program) {
    for (size_t i = 0; i < program.size; i++) {
        Token token = program.tokens[i];
        switch (token.inst) {
            case I_PUSH:
                if (push(vm, token.int_value) != RTS_OK) {
                    return RTS_STACK_OVERFLOW;
                }
                break;
            case I_POP:
                if (pop(vm, token.register_name) != RTS_OK) {
                    return RTS_STACK_UNDERFLOW;
                }
                break;
            case I_ADD:
                if (add(vm) != RTS_OK) {
                    return RTS_STACK_UNDERFLOW;
                }
                break;
            case I_SUB:
                if (sub(vm) != RTS_OK) {
                    return RTS_STACK_UNDERFLOW;
                }
                break;
            case I_PRINT:
                if (print(vm) != RTS_OK) {
                    return RTS_STACK_UNDERFLOW;
                }
                break;
            case I_MUL:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t a = vm->stack[--vm->stack_size];
                int64_t b = vm->stack[--vm->stack_size];
                return push(vm, a * b);
            case I_DIV:
                if (vm->stack_size < 2) {
                    return RTS_STACK_UNDERFLOW;
                }
                int64_t c = vm->stack[--vm->stack_size];
                int64_t d = vm->stack[--vm->stack_size];
                if (d == 0) {
                    return RTS_ERROR;
                }
                return push(vm, c / d);
            case I_MOV:
                int register_num = parse_register_str(token.register_name);
                if (register_num < 0) {
                    return RTS_ERROR;
                }
                vm->registers[register_num] = vm->stack[vm->stack_size - 1];

            default:
                return RTS_UNDEFINED_INSTRUCTION;
        }
    }
    return RTS_OK;
}

void OrtaVM_dump(OrtaVM *vm) {
    printf("Registers:\n");
    for (int i = 0; i < 4; i++) {
        printf("r%c%c: %ld\n", 'a' + i, 'x', vm->registers[i]);
    }

    printf("Stack (top to bottom):\n");
    for (size_t i = vm->stack_size; i > 0; i--) {
        if (vm->stack[i - 1] == 0) continue;
        printf("%ld\n", vm->stack[i - 1]);
    }
}

#define PUSH(val) {I_PUSH, .int_value = (val)}
#define ADD {I_ADD}
#define POP(reg) {I_POP, .register_name = (reg)}
#define SUB {I_SUB}
#define PRINT {I_PRINT}
#define MOV(reg_name) {I_MOV, .register_name = (reg_name)}
#define MUL {I_MUL}
#define DIV {I_DIV}

int main() {
    Program program = {
        .tokens = {
            PUSH(42),
            PUSH(10),
            ADD,
            PUSH(2),
            MOV("rax"),
        },
        .size = 9,
    };

    OrtaVM vm = {0};

    RTStatus status = OrtaVM_execute(&vm, program);
    if (status != RTS_OK) {
        printf("Error: %d\n", status);
        return 1;
    }
    OrtaVM_dump(&vm);

    return 0;
}
