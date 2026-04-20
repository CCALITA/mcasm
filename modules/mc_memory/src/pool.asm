; pool.asm — Pool (freelist) allocator for mc_memory module
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9; return in rax
; Callee-saved: rbx, rbp, r12-r15
;
; Design: A contiguous backing buffer divided into fixed-size elements.
; Each free element stores a pointer to the next free element (intrusive
; freelist). The minimum effective element size is 8 bytes (pointer width).

%include "platform.inc"
%include "mc_memory_internal.inc"

section .text

EXTERN_FUNC malloc
EXTERN_FUNC free
EXTERN_FUNC calloc

; ─── mc_memory_pool_create ────────────────────────────────────────────
; mc_pool_t* mc_memory_pool_create(size_t element_size, uint32_t max_elements)
; rdi = element_size, esi = max_elements
; Returns pointer to new mc_pool_t, or NULL on failure.
GLOBAL_FUNC mc_memory_pool_create
    push    rbx
    push    r12
    push    r13                     ; 3 pushes + ret addr = 32 bytes, rsp 16-aligned

    ; Ensure element_size >= 8 (must hold a pointer for the freelist)
    mov     r12, rdi                ; r12 = element_size
    cmp     r12, 8
    jae     .size_ok
    mov     r12, 8
.size_ok:
    mov     r13d, esi               ; r13d = max_elements

    ; Allocate the pool struct: calloc(1, POOL_SIZE)
    mov     rdi, 1
    mov     rsi, POOL_SIZE
    call    DECL(calloc)
    test    rax, rax
    jz      .create_fail
    mov     rbx, rax                ; rbx = pool pointer

    ; Allocate backing buffer: malloc(element_size * max_elements)
    mov     rax, r12
    mov     ecx, r13d
    imul    rax, rcx                ; rax = total bytes
    mov     rdi, rax
    call    DECL(malloc)
    test    rax, rax
    jz      .create_free_pool

    ; Fill pool struct fields
    mov     [rbx + POOL_OFF_BACKING],  rax
    mov     [rbx + POOL_OFF_ELEM],     r12
    mov     [rbx + POOL_OFF_MAX],      r13d
    mov     dword [rbx + POOL_OFF_COUNT], 0

    ; Build the intrusive freelist.
    ; Each element[i] stores a pointer to element[i+1].
    ; The last element stores NULL.
    mov     rcx, rax                ; rcx = current element pointer
    mov     edx, r13d               ; edx = remaining count
    dec     edx                     ; loop count = max - 1
    jz      .single_element

.build_freelist:
    lea     rdi, [rcx + r12]        ; rdi = next element
    mov     [rcx], rdi              ; current->next = next
    mov     rcx, rdi                ; advance
    dec     edx
    jnz     .build_freelist

.single_element:
    mov     qword [rcx], 0          ; last element->next = NULL

    ; Set freelist head
    mov     rax, [rbx + POOL_OFF_BACKING]
    mov     [rbx + POOL_OFF_FREELIST], rax

    mov     rax, rbx                ; return pool
    pop     r13
    pop     r12
    pop     rbx
    ret

.create_free_pool:
    mov     rdi, rbx
    call    DECL(free)

.create_fail:
    xor     eax, eax                ; return NULL
    pop     r13
    pop     r12
    pop     rbx
    ret


; ─── mc_memory_pool_alloc ─────────────────────────────────────────────
; void* mc_memory_pool_alloc(mc_pool_t* pool)
; rdi = pool
; Returns pointer to an element, or NULL if pool is exhausted.
GLOBAL_FUNC mc_memory_pool_alloc
    mov     rax, [rdi + POOL_OFF_FREELIST]
    test    rax, rax
    jz      .pool_exhausted

    ; Pop head of freelist
    mov     rcx, [rax]                          ; rcx = head->next
    mov     [rdi + POOL_OFF_FREELIST], rcx      ; freelist = head->next

    ; Increment count
    inc     dword [rdi + POOL_OFF_COUNT]

    ; rax already points to the allocated element
    ret

.pool_exhausted:
    xor     eax, eax                ; return NULL
    ret


; ─── mc_memory_pool_free ──────────────────────────────────────────────
; void mc_memory_pool_free(mc_pool_t* pool, void* ptr)
; rdi = pool, rsi = ptr
GLOBAL_FUNC mc_memory_pool_free
    ; Push ptr onto the freelist head
    mov     rax, [rdi + POOL_OFF_FREELIST]
    mov     [rsi], rax                          ; ptr->next = old head
    mov     [rdi + POOL_OFF_FREELIST], rsi      ; freelist = ptr

    ; Decrement count
    dec     dword [rdi + POOL_OFF_COUNT]
    ret


; ─── mc_memory_pool_destroy ───────────────────────────────────────────
; void mc_memory_pool_destroy(mc_pool_t* pool)
; rdi = pool
GLOBAL_FUNC mc_memory_pool_destroy
    push    rbx                     ; ret addr + 1 push = 16 bytes, rsp aligned

    mov     rbx, rdi                ; save pool pointer

    ; Free the backing buffer
    mov     rdi, [rbx + POOL_OFF_BACKING]
    call    DECL(free)

    ; Free the pool struct
    mov     rdi, rbx
    call    DECL(free)

    pop     rbx
    ret


; ─── mc_memory_pool_count ─────────────────────────────────────────────
; uint32_t mc_memory_pool_count(const mc_pool_t* pool)
; rdi = pool
GLOBAL_FUNC mc_memory_pool_count
    mov     eax, [rdi + POOL_OFF_COUNT]
    ret
