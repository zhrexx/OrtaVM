// TODO: fix 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "orta.h"

char *reg_to_asm(const char *reg_name) {
    if (strcasecmp(reg_name, "RAX") == 0) return "rax";
    if (strcasecmp(reg_name, "RBX") == 0) return "rbx";
    if (strcasecmp(reg_name, "RCX") == 0) return "rcx";
    if (strcasecmp(reg_name, "RDX") == 0) return "rdx";
    if (strcasecmp(reg_name, "RSI") == 0) return "rsi";
    if (strcasecmp(reg_name, "RDI") == 0) return "rdi";
    if (strcasecmp(reg_name, "R8") == 0) return "r8";
    if (strcasecmp(reg_name, "R9") == 0) return "r9";
    return NULL;
}

char *instruction_to_asm(InstructionData *instr) {
    char *asm_line = malloc(1024);
    const char *instr_name = instruction_to_string(instr->opcode);
    
    switch (instr->opcode) {
        case IPUSH: {
            char *operand = vector_get_str(&instr->operands, 0);
            if (is_number(operand)) {
                sprintf(asm_line, "    push %s", operand);
            } else if (is_register(operand)) {
                char *reg = reg_to_asm(operand);
                if (reg) sprintf(asm_line, "    push %s", reg);
                else sprintf(asm_line, "    push %s", operand);
            } else if (is_string(operand)) {
                sprintf(asm_line, "    push offset str_%zu", (size_t)operand);
            } else {
                sprintf(asm_line, "    push %s", operand);
            }
            break;
        }
        case IMOV: {
            char *src = vector_get_str(&instr->operands, 0);
            char *dst = vector_get_str(&instr->operands, 1);
            
            if (is_register(dst)) {
                char *dst_reg = reg_to_asm(dst);
                
                if (is_register(src)) {
                    char *src_reg = reg_to_asm(src);
                    sprintf(asm_line, "    mov %s, %s", dst_reg, src_reg);
                } else if (is_number(src)) {
                    sprintf(asm_line, "    mov %s, %s", dst_reg, src);
                } else if (is_string(src)) {
                    sprintf(asm_line, "    mov %s, offset str_%zu", dst_reg, (size_t)src);
                } else {
                    sprintf(asm_line, "    mov %s, %s", dst_reg, src);
                }
            } else {
                sprintf(asm_line, "    mov %s, %s", dst, src);
            }
            break;
        }
        case IPOP: {
            char *operand = vector_get_str(&instr->operands, 0);
            if (is_register(operand)) {
                char *reg = reg_to_asm(operand);
                sprintf(asm_line, "    pop %s", reg);
            } else {
                sprintf(asm_line, "    pop %s", operand);
            }
            break;
        }
        case IADD:
            sprintf(asm_line, "    pop rbx\n    pop rax\n    add rax, rbx\n    push rax");
            break;
        case ISUB:
            sprintf(asm_line, "    pop rbx\n    pop rax\n    sub rax, rbx\n    push rax");
            break;
        case IMUL:
            sprintf(asm_line, "    pop rbx\n    pop rax\n    imul rax, rbx\n    push rax");
            break;
        case IDIV:
            sprintf(asm_line, "    pop rbx\n    pop rax\n    xor rdx, rdx\n    div rbx\n    push rax");
            break;
        case IMOD:
            sprintf(asm_line, "    pop rbx\n    pop rax\n    xor rdx, rdx\n    div rbx\n    push rdx");
            break;
        case IJMP: {
            char *operand = vector_get_str(&instr->operands, 0);
            if (is_number(operand)) {
                sprintf(asm_line, "    jmp addr_%s", operand);
            } else if (is_label_reference(operand)) {
                sprintf(asm_line, "    jmp %s", operand);
            } else {
                sprintf(asm_line, "    jmp %s", operand);
            }
            break;
        }
        case IJMPIF: {
            char *operand = vector_get_str(&instr->operands, 0);
            if (is_number(operand)) {
                sprintf(asm_line, "    pop rax\n    test rax, rax\n    jnz addr_%s", operand);
            } else if (is_label_reference(operand)) {
                sprintf(asm_line, "    pop rax\n    test rax, rax\n    jnz %s", operand);
            } else {
                sprintf(asm_line, "    pop rax\n    test rax, rax\n    jnz %s", operand);
            }
            break;
        }
        case ICALL: {
            char *operand = vector_get_str(&instr->operands, 0);
            if (is_number(operand)) {
                sprintf(asm_line, "    call addr_%s", operand);
            } else if (is_label_reference(operand)) {
                sprintf(asm_line, "    call %s", operand);
            } else {
                sprintf(asm_line, "    call %s", operand);
            }
            break;
        }
        case IRET:
            sprintf(asm_line, "    ret");
            break;
        case IPRINT:
            sprintf(asm_line, "    pop rdi\n    call print_value");
            break;
        case IDUP:
            sprintf(asm_line, "    pop rax\n    push rax\n    push rax");
            break;
        case ISWAP:
            sprintf(asm_line, "    pop rbx\n    pop rax\n    push rbx\n    push rax");
            break;
        case IDROP:
            sprintf(asm_line, "    pop rax");
            break;
        case IALLOC: {
            char *operand = vector_get_str(&instr->operands, 0);
            sprintf(asm_line, "    mov rdi, %s\n    call malloc\n    push rax", operand);
            break;
        }
        case IHALT:
            sprintf(asm_line, "    mov rax, 60\n    xor rdi, rdi\n    syscall");
            break;
        default:
            free(asm_line);
            return NULL;
    }
    
    return asm_line;
}

size_t add_string_to_data(Vector *data_section, const char *str) {
    size_t len = strlen(str);
    char *clean_str = malloc(len - 1);
    strncpy(clean_str, str + 1, len - 2);
    clean_str[len - 2] = '\0';
    
    for (size_t i = 0; i < vector_len(data_section); i++) {
        char *existing = vector_get_str(data_section, i);
        if (strcmp(existing, clean_str) == 0) {
            free(clean_str);
            return i;
        }
    }
    
    vector_push(data_section, &clean_str);
    return vector_len(data_section) - 1;
}

void process_strings(InstructionData *instr, Vector *data_section) {
    for (size_t i = 0; i < instr->operands.size; i++) {
        char *operand = vector_get_str(&instr->operands, i);
        if (is_string(operand)) {
            size_t string_index = add_string_to_data(data_section, operand);
            vector_set(&instr->operands, i, (void*)(uintptr_t)string_index);
        }
    }
}

void convert_xbin_to_asm(const char *input_file, const char *output_file) {
    Vector data_section;
    Vector text_section;
    
    vector_init(&data_section, 16, sizeof(char*));
    vector_init(&text_section, 128, sizeof(char*));
    
    OrtaVM vm = ortavm_create(input_file);
    if (!load_bytecode(&vm, input_file)) {
        fprintf(stderr, "Failed to parse XBIN file: %s\n", input_file);
        ortavm_free(&vm);
        vector_free(&data_section);
        vector_free(&text_section);
        return;
    }
    
    for (size_t i = 0; i < vm.program.instructions_count; i++) {
        process_strings(&vm.program.instructions[i], &data_section);
    }
    
    for (size_t i = 0; i < vm.program.instructions_count; i++) {
        for (size_t j = 0; j < vm.program.labels_count; j++) {
            if (vm.program.labels[j].address == i) {
                char *label_line = malloc(strlen(vm.program.labels[j].name) + 10);
                sprintf(label_line, "%s:", vm.program.labels[j].name);
                vector_push(&text_section, &label_line);
            }
        }
        
        char *addr_line = malloc(20);
        sprintf(addr_line, "addr_%zu:", i);
        vector_push(&text_section, &addr_line);
    
        char *asm_line = instruction_to_asm(&vm.program.instructions[i]);
        if (asm_line) vector_push(&text_section, &asm_line);
    }
    
    FILE *fp = fopen(output_file, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output file: %s\n", output_file);
        ortavm_free(&vm);
        vector_free(&data_section);
        vector_free(&text_section);
        return;
    }
    fprintf(fp, ";; GENERATED BY ORTAVM\nBITS 64\n"); 
    fprintf(fp, "section .data\n");
    for (size_t i = 0; i < vector_len(&data_section); i++) {
        char *str = vector_get_str(&data_section, i);
        fprintf(fp, "str_%zu db \"%s\", 0\n", i, str);
    }
    fprintf(fp, "\n");
    
    fprintf(fp, "section .bss\n");
    fprintf(fp, "memory_buffer resb %d\n\n", MEMORY_CAPACITY);
    
    fprintf(fp, "extern malloc\n");
    fprintf(fp, "extern free\n");
    fprintf(fp, "extern printf\n\n");
    
    fprintf(fp, "section .text\n");
    fprintf(fp, "global main\n\n");
    
    fprintf(fp, "print_value:\n");
    fprintf(fp, "    mov rsi, rdi\n");
    fprintf(fp, "    mov rdi, format_int\n"); 
    fprintf(fp, "    xor rax, rax\n");
    fprintf(fp, "    call printf\n");
    fprintf(fp, "    ret\n\n");
    
    fprintf(fp, "format_int db \"%%d\", 10, 0\n");
    fprintf(fp, "format_str db \"%%s\", 10, 0\n\n");
    
    fprintf(fp, "main:\n");
    fprintf(fp, "    push rbp\n");
    fprintf(fp, "    mov rbp, rsp\n");
    fprintf(fp, "    sub rsp, 128\n\n");
    
    for (size_t i = 0; i < vector_len(&text_section); i++) {
        fprintf(fp, "%s\n", vector_get_str(&text_section, i));
    }
    
    fprintf(fp, "\n    mov rsp, rbp\n");
    fprintf(fp, "    pop rbp\n");
    fprintf(fp, "    xor rax, rax\n");
    fprintf(fp, "    ret\n");
    
    fclose(fp);
    
    ortavm_free(&vm);
    vector_free(&data_section);
    vector_free(&text_section);
    
    printf("Assembly file generated: %s\n", output_file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <input.xbin> <output.asm>\n", argv[0]);
        return 1;
    }

    convert_xbin_to_asm(argv[1], argv[2]);
    return 0;
}
