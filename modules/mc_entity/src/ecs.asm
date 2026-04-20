; ecs.asm — Core ECS: SoA arrays, free list, entity lifecycle
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9
;                     return in rax / xmm0

%include "platform.inc"
%include "types.inc"
%include "entity_defs.inc"

; ---------------------------------------------------------------------------
; BSS — SoA arrays (static allocation, zero-initialised)
; Arrays are exported as global symbols so components.asm and iterate.c
; can reference them directly.
; ---------------------------------------------------------------------------
section .bss

align 16
GLOBAL_DATA ecs_masks
    resd MAX_ENTITIES                   ; uint32_t masks[4096]

align 16
GLOBAL_DATA ecs_transforms
    resb MAX_ENTITIES * SIZEOF_XFORM   ; transform_component_t[4096]

align 16
GLOBAL_DATA ecs_physics
    resb MAX_ENTITIES * SIZEOF_PHYSICS  ; physics_component_t[4096]

align 4
GLOBAL_DATA ecs_healths
    resb MAX_ENTITIES * SIZEOF_HEALTH   ; health_component_t[4096]

align 4
free_list:   resd MAX_ENTITIES          ; uint32_t free_list[4096] (private)

; ---------------------------------------------------------------------------
; DATA — mutable scalars
; ---------------------------------------------------------------------------
section .data

align 8
GLOBAL_DATA ecs_entity_count
    dd 0                                ; number of live entities

align 8
GLOBAL_DATA ecs_next_id
    dd 1                                ; next never-used ID (grows monotonically)

align 4
free_top:     dd 0                      ; number of entries on the free list (private)

section .text

; ===================================================================
; mc_error_t mc_entity_init(void)
;   Reset all ECS state. Returns MC_OK (0).
; ===================================================================
GLOBAL_FUNC mc_entity_init
    ; Zero the masks array: 4096 * 4 = 16384 bytes = 2048 qwords
    ; rep stosq clobbers rdi/rcx/rax — all caller-saved, no saves needed
    lea     rdi, [rel DECL(ecs_masks)]
    xor     eax, eax
    mov     ecx, 2048
    rep     stosq

    ; Reset scalars
    lea     rax, [rel DECL(ecs_entity_count)]
    mov     dword [rax], 0
    lea     rax, [rel DECL(ecs_next_id)]
    mov     dword [rax], 1
    lea     rax, [rel free_top]
    mov     dword [rax], 0

    xor     eax, eax                    ; MC_OK
    ret

; ===================================================================
; void mc_entity_shutdown(void)
;   No-op — static arrays need no deallocation.
; ===================================================================
GLOBAL_FUNC mc_entity_shutdown
    ret

; ===================================================================
; entity_id_t mc_entity_create(uint32_t component_mask)
;   Allocates an entity. Returns ID or ENTITY_INVALID (0) if full.
;   rdi = component_mask
; ===================================================================
GLOBAL_FUNC mc_entity_create
    ; Try free list first
    lea     rax, [rel free_top]
    mov     ecx, [rax]
    test    ecx, ecx
    jz      .try_fresh

    ; Pop from free list
    dec     ecx
    mov     [rax], ecx                  ; --free_top
    lea     rdx, [rel free_list]
    mov     eax, [rdx + rcx*4]          ; id = free_list[free_top]
    jmp     .assign

.try_fresh:
    ; Allocate next sequential ID
    lea     rdx, [rel DECL(ecs_next_id)]
    mov     eax, [rdx]
    cmp     eax, MAX_ENTITIES
    jge     .full
    lea     ecx, [eax + 1]
    mov     [rdx], ecx                  ; next_id++
    jmp     .assign

.full:
    xor     eax, eax                    ; return ENTITY_INVALID
    ret

.assign:
    ; eax = entity ID, edi = component_mask
    lea     rdx, [rel DECL(ecs_masks)]
    mov     [rdx + rax*4], edi          ; masks[id] = component_mask

    ; ++entity_count
    lea     rdx, [rel DECL(ecs_entity_count)]
    inc     dword [rdx]

    ret                                 ; eax already holds the ID

; ===================================================================
; void mc_entity_destroy(entity_id_t id)
;   rdi = id
; ===================================================================
GLOBAL_FUNC mc_entity_destroy
    ; Bounds check
    cmp     edi, MAX_ENTITIES
    jge     .done
    test    edi, edi
    jz      .done

    ; Check if alive
    lea     rax, [rel DECL(ecs_masks)]
    mov     ecx, [rax + rdi*4]
    test    ecx, ecx
    jz      .done                       ; already dead

    ; Clear mask
    mov     dword [rax + rdi*4], 0

    ; Push onto free list
    lea     rax, [rel free_top]
    mov     ecx, [rax]
    lea     rdx, [rel free_list]
    mov     [rdx + rcx*4], edi          ; free_list[free_top] = id
    inc     ecx
    mov     [rax], ecx                  ; free_top++

    ; --entity_count
    lea     rax, [rel DECL(ecs_entity_count)]
    dec     dword [rax]

.done:
    ret

; ===================================================================
; uint8_t mc_entity_alive(entity_id_t id)
;   rdi = id. Returns 1 if alive, 0 otherwise.
; ===================================================================
GLOBAL_FUNC mc_entity_alive
    xor     eax, eax
    cmp     edi, MAX_ENTITIES
    jge     .ret
    test    edi, edi
    jz      .ret

    lea     rdx, [rel DECL(ecs_masks)]
    mov     ecx, [rdx + rdi*4]
    test    ecx, ecx
    setnz   al
.ret:
    ret

; ===================================================================
; uint32_t mc_entity_get_mask(entity_id_t id)
;   rdi = id. Returns masks[id] or 0.
; ===================================================================
GLOBAL_FUNC mc_entity_get_mask
    xor     eax, eax
    cmp     edi, MAX_ENTITIES
    jge     .ret
    lea     rdx, [rel DECL(ecs_masks)]
    mov     eax, [rdx + rdi*4]
.ret:
    ret

; ===================================================================
; uint32_t mc_entity_count(void)
;   Returns number of live entities.
; ===================================================================
GLOBAL_FUNC mc_entity_count
    lea     rax, [rel DECL(ecs_entity_count)]
    mov     eax, [rax]
    ret
