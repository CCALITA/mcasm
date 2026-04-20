; arena.asm — Arena (bump) allocator for mc_memory module
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9; return in rax
; Callee-saved: rbx, rbp, r12-r15

%include "platform.inc"
%include "mc_memory_internal.inc"

section .text

EXTERN_FUNC malloc
EXTERN_FUNC free
EXTERN_FUNC calloc

; ─── mc_memory_arena_create ───────────────────────────────────────────
; mc_arena_t* mc_memory_arena_create(size_t capacity)
; rdi = capacity
; Returns pointer to new mc_arena_t in rax, or NULL on failure.
GLOBAL_FUNC mc_memory_arena_create
    push    rbx
    push    r12
    sub     rsp, 8                  ; align stack to 16 bytes

    mov     r12, rdi                ; r12 = capacity (callee-saved)

    ; Allocate the arena struct: calloc(1, ARENA_SIZE)
    mov     rdi, 1
    mov     rsi, ARENA_SIZE
    call    DECL(calloc)
    test    rax, rax
    jz      .create_fail
    mov     rbx, rax                ; rbx = arena pointer

    ; Allocate the backing buffer: malloc(capacity)
    mov     rdi, r12
    call    DECL(malloc)
    test    rax, rax
    jz      .create_free_arena

    ; Initialize the arena struct
    mov     [rbx + ARENA_OFF_BUF],  rax
    mov     qword [rbx + ARENA_OFF_CAP],  r12
    mov     qword [rbx + ARENA_OFF_USED], 0

    mov     rax, rbx                ; return arena

    add     rsp, 8
    pop     r12
    pop     rbx
    ret

.create_free_arena:
    ; malloc for buffer failed — free the arena struct
    mov     rdi, rbx
    call    DECL(free)

.create_fail:
    xor     eax, eax                ; return NULL
    add     rsp, 8
    pop     r12
    pop     rbx
    ret


; ─── mc_memory_arena_alloc ────────────────────────────────────────────
; void* mc_memory_arena_alloc(mc_arena_t* arena, size_t size, size_t alignment)
; rdi = arena, rsi = size, rdx = alignment
; Returns pointer or NULL if out of space.
GLOBAL_FUNC mc_memory_arena_alloc
    mov     rax, [rdi + ARENA_OFF_USED]  ; rax = used
    mov     rcx, rdx                     ; rcx = alignment

    ; aligned = (used + alignment - 1) & ~(alignment - 1)
    lea     rax, [rax + rcx - 1]
    neg     rcx
    and     rax, rcx                     ; rax = aligned offset

    ; Check: aligned + size <= cap
    lea     rcx, [rax + rsi]             ; rcx = aligned + size
    cmp     rcx, [rdi + ARENA_OFF_CAP]
    ja      .alloc_fail

    ; Update used
    mov     [rdi + ARENA_OFF_USED], rcx

    ; Return buf + aligned
    add     rax, [rdi + ARENA_OFF_BUF]
    ret

.alloc_fail:
    xor     eax, eax                     ; return NULL
    ret


; ─── mc_memory_arena_reset ────────────────────────────────────────────
; void mc_memory_arena_reset(mc_arena_t* arena)
; rdi = arena
GLOBAL_FUNC mc_memory_arena_reset
    mov     qword [rdi + ARENA_OFF_USED], 0
    ret


; ─── mc_memory_arena_destroy ──────────────────────────────────────────
; void mc_memory_arena_destroy(mc_arena_t* arena)
; rdi = arena
GLOBAL_FUNC mc_memory_arena_destroy
    push    rbx                     ; ret addr + 1 push = 16 bytes, rsp aligned

    mov     rbx, rdi                ; save arena pointer

    ; Free the backing buffer
    mov     rdi, [rbx + ARENA_OFF_BUF]
    call    DECL(free)

    ; Free the arena struct
    mov     rdi, rbx
    call    DECL(free)

    pop     rbx
    ret


; ─── mc_memory_arena_used ─────────────────────────────────────────────
; size_t mc_memory_arena_used(const mc_arena_t* arena)
; rdi = arena
GLOBAL_FUNC mc_memory_arena_used
    mov     rax, [rdi + ARENA_OFF_USED]
    ret
