; registry.asm — Block property registry
; Static array of 256 block_properties_t structs with fast lookup functions.

%include "platform.inc"
%include "error.inc"
%include "block_defs.inc"

; block_properties_t layout (16 bytes total):
;   offset 0:  float    hardness          (4 bytes)
;   offset 4:  uint8_t  transparent       (1 byte)
;   offset 5:  uint8_t  solid             (1 byte)
;   offset 6:  uint8_t  light_emission    (1 byte)
;   offset 7:  uint8_t  flammable         (1 byte)
;   offset 8:  uint16_t texture_top       (2 bytes)
;   offset 10: uint16_t texture_side      (2 bytes)
;   offset 12: uint16_t texture_bottom    (2 bytes)
;   offset 14: (2 bytes padding)

%define SIZEOF_BLOCK_PROPS   16
%define BPROP_TRANSPARENT     4
%define BPROP_SOLID           5
%define BPROP_LIGHT_EMISSION  6

section .bss
    align 16
block_registry: resb BLOCK_TYPE_COUNT * SIZEOF_BLOCK_PROPS

section .text

;-----------------------------------------------------------------------------
; mc_error_t mc_block_register(block_id_t id, const block_properties_t* props)
; Args: rdi = id (uint16_t), rsi = pointer to block_properties_t
; Returns: eax = mc_error_t
;-----------------------------------------------------------------------------
GLOBAL_FUNC mc_block_register
    movzx   eax, di
    cmp     eax, BLOCK_TYPE_COUNT
    jge     .bad_arg

    test    rsi, rsi
    jz      .bad_arg

    ; destination = block_registry + id * 16
    shl     rax, 4
    lea     rcx, [rel block_registry]
    add     rcx, rax

    ; Copy 16 bytes via two 8-byte moves
    mov     rax, [rsi]
    mov     [rcx], rax
    mov     rax, [rsi + 8]
    mov     [rcx + 8], rax

    xor     eax, eax                    ; MC_OK
    ret

.bad_arg:
    mov     eax, MC_ERR_INVALID_ARG
    ret

;-----------------------------------------------------------------------------
; const block_properties_t* mc_block_get_properties(block_id_t id)
; Args: rdi = id (uint16_t)
; Returns: rax = pointer to entry, or NULL
;-----------------------------------------------------------------------------
GLOBAL_FUNC mc_block_get_properties
    movzx   eax, di
    cmp     eax, BLOCK_TYPE_COUNT
    jge     .ret_null

    shl     rax, 4
    lea     rcx, [rel block_registry]
    lea     rax, [rcx + rax]
    ret

.ret_null:
    xor     eax, eax
    ret

;-----------------------------------------------------------------------------
; uint8_t mc_block_is_solid(block_id_t id)
;-----------------------------------------------------------------------------
GLOBAL_FUNC mc_block_is_solid
    movzx   eax, di
    cmp     eax, BLOCK_TYPE_COUNT
    jge     .solid_zero

    shl     rax, 4
    lea     rcx, [rel block_registry]
    movzx   eax, byte [rcx + rax + BPROP_SOLID]
    ret

.solid_zero:
    xor     eax, eax
    ret

;-----------------------------------------------------------------------------
; uint8_t mc_block_is_transparent(block_id_t id)
;-----------------------------------------------------------------------------
GLOBAL_FUNC mc_block_is_transparent
    movzx   eax, di
    cmp     eax, BLOCK_TYPE_COUNT
    jge     .transp_def

    shl     rax, 4
    lea     rcx, [rel block_registry]
    movzx   eax, byte [rcx + rax + BPROP_TRANSPARENT]
    ret

.transp_def:
    mov     eax, 1
    ret

;-----------------------------------------------------------------------------
; uint8_t mc_block_get_light(block_id_t id)
;-----------------------------------------------------------------------------
GLOBAL_FUNC mc_block_get_light
    movzx   eax, di
    cmp     eax, BLOCK_TYPE_COUNT
    jge     .light_zero

    shl     rax, 4
    lea     rcx, [rel block_registry]
    movzx   eax, byte [rcx + rax + BPROP_LIGHT_EMISSION]
    ret

.light_zero:
    xor     eax, eax
    ret
