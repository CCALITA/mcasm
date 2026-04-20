; platform.inc — cross-platform symbol naming for NASM
; Include this at the top of every .asm file

%ifdef MACHO
  ; macOS Mach-O requires underscore prefix for C-visible symbols
  %define DECL(name) _ %+ name
  default rel
%else
  ; Linux ELF uses symbols as-is
  %define DECL(name) name
%endif

; Declare a global function with correct platform symbol name
; Usage: GLOBAL_FUNC mc_math_vec3_add
%macro GLOBAL_FUNC 1
  global DECL(%1)
  DECL(%1):
%endmacro

; Declare an external function reference
; Usage: EXTERN_FUNC mc_memory_arena_alloc
%macro EXTERN_FUNC 1
  extern DECL(%1)
%endmacro

; Declare a global data symbol
%macro GLOBAL_DATA 1
  global DECL(%1)
  DECL(%1):
%endmacro

; System V AMD64 ABI reference:
; Integer/pointer args: rdi, rsi, rdx, rcx, r8, r9
; Float/double args:    xmm0-xmm7
; Return value:         rax (int/ptr), xmm0 (float/double)
; Caller-saved:         rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
; Callee-saved:         rbx, rbp, r12, r13, r14, r15
; Stack:                16-byte aligned before CALL
