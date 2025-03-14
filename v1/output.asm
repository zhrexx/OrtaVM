;; OVM RUNTIME
%include "helpers/runtime.asm"
;; OVM START
global main
global _entry
_start: ;; Does nothing only jumps to entry point | Because of elf shit convention with _start
	jmp _entry
main:
	jmp _entry

label_0:
	;; inst::jmp 
	jmp label_2
label_1:
	;; stack::clear
	mov rsp, 0
label_2:
_entry: ;; Entry point
label_3:
	;; stack::push | Integer
	push 10
label_4:
	;; io::print
	pop rdi
	call ifc_orta_print_int
label_5:
	;; stack::push | Integer
	push 20
label_6:
	;; io::print
	pop rdi
	call ifc_orta_print_int
label_7:
	;; TODO: stack::push | String


;; OVM END
ovm_exit 0
