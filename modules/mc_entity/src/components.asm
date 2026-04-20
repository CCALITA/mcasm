; components.asm — Component accessors for the ECS SoA arrays
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9
;                     float args in xmm0-xmm7
;                     return in rax (ptr), xmm0 (float)

%include "platform.inc"
%include "types.inc"
%include "entity_defs.inc"

; ---------------------------------------------------------------------------
; Import SoA array symbols from ecs.asm (these are the arrays themselves)
; ---------------------------------------------------------------------------
EXTERN_FUNC ecs_transforms
EXTERN_FUNC ecs_physics
EXTERN_FUNC ecs_healths

section .text

; ===================================================================
; transform_component_t* mc_entity_get_transform(entity_id_t id)
;   rdi = id. Returns pointer into transforms[], or NULL.
; ===================================================================
GLOBAL_FUNC mc_entity_get_transform
    xor     eax, eax
    cmp     edi, MAX_ENTITIES
    jge     .ret

    ; pointer = ecs_transforms + id * SIZEOF_XFORM
    lea     rdx, [rel DECL(ecs_transforms)]
    imul    rdi, rdi, SIZEOF_XFORM
    lea     rax, [rdx + rdi]
.ret:
    ret

; ===================================================================
; physics_component_t* mc_entity_get_physics(entity_id_t id)
;   rdi = id. Returns pointer into physics[], or NULL.
; ===================================================================
GLOBAL_FUNC mc_entity_get_physics
    xor     eax, eax
    cmp     edi, MAX_ENTITIES
    jge     .ret

    lea     rdx, [rel DECL(ecs_physics)]
    imul    rdi, rdi, SIZEOF_PHYSICS
    lea     rax, [rdx + rdi]
.ret:
    ret

; ===================================================================
; health_component_t* mc_entity_get_health(entity_id_t id)
;   rdi = id. Returns pointer into healths[], or NULL.
; ===================================================================
GLOBAL_FUNC mc_entity_get_health
    xor     eax, eax
    cmp     edi, MAX_ENTITIES
    jge     .ret

    lea     rdx, [rel DECL(ecs_healths)]
    imul    rdi, rdi, SIZEOF_HEALTH
    lea     rax, [rdx + rdi]
.ret:
    ret

; ===================================================================
; void mc_entity_set_position(entity_id_t id, vec3_t pos)
;   System V AMD64 ABI: 16-byte struct vec3_t is split across two
;   SSE registers — xmm0 = bytes [0..7] (x, y), xmm1 = bytes [8..15]
;   (z, _pad). We store both halves to reconstruct the full vec3_t.
;   rdi = id, xmm0 = low qword, xmm1 = high qword
; ===================================================================
GLOBAL_FUNC mc_entity_set_position
    cmp     edi, MAX_ENTITIES
    jge     .done

    lea     rdx, [rel DECL(ecs_transforms)]
    imul    rdi, rdi, SIZEOF_XFORM
    ; Store low 8 bytes (x, y) then high 8 bytes (z, _pad)
    movlps  [rdx + rdi + XFORM_POS], xmm0
    movlps  [rdx + rdi + XFORM_POS + 8], xmm1
.done:
    ret

; ===================================================================
; void mc_entity_set_velocity(entity_id_t id, vec3_t vel)
;   rdi = id, xmm0 = low qword (x, y), xmm1 = high qword (z, _pad)
; ===================================================================
GLOBAL_FUNC mc_entity_set_velocity
    cmp     edi, MAX_ENTITIES
    jge     .done

    lea     rdx, [rel DECL(ecs_transforms)]
    imul    rdi, rdi, SIZEOF_XFORM
    movlps  [rdx + rdi + XFORM_VEL], xmm0
    movlps  [rdx + rdi + XFORM_VEL + 8], xmm1
.done:
    ret
