; power_decay.asm -- Hot-path helper for redstone power decay computation.
;
; Provides: mc_redstone_decay_power(current_power, hops) -> decayed power
;           mc_redstone_max_neighbor_power(powers_array, count) -> max value
;
; System V AMD64 ABI:
;   Args: rdi, rsi, rdx, rcx, r8, r9
;   Return: rax
;   Caller-saved: rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11

%include "platform.inc"

section .text

;----------------------------------------------------------------------
; uint8_t mc_redstone_decay_power(uint8_t current_power, uint8_t hops)
;
; Returns: max(0, current_power - hops)
; rdi = current_power (uint8_t, zero-extended in edi)
; rsi = hops          (uint8_t, zero-extended in esi)
;----------------------------------------------------------------------
GLOBAL_FUNC mc_redstone_decay_power
    mov     eax, edi        ; eax = current_power
    sub     eax, esi        ; eax = current_power - hops
    ; Clamp to 0 if negative
    xor     ecx, ecx        ; ecx = 0
    cmovl   eax, ecx        ; if eax < 0, eax = 0
    ret

;----------------------------------------------------------------------
; uint8_t mc_redstone_max_neighbor_power(const uint8_t *powers, uint32_t count)
;
; Scans an array of power levels and returns the maximum.
; rdi = pointer to uint8_t array
; rsi = count (uint32_t)
; Returns: max power in rax
;----------------------------------------------------------------------
GLOBAL_FUNC mc_redstone_max_neighbor_power
    xor     eax, eax        ; max = 0
    test    esi, esi        ; count == 0?
    jz      .done
    xor     ecx, ecx        ; index = 0
.loop:
    movzx   edx, byte [rdi + rcx]
    cmp     edx, eax
    cmova   eax, edx        ; if powers[i] > max, max = powers[i]
    inc     ecx
    cmp     ecx, esi
    jb      .loop
.done:
    ret
