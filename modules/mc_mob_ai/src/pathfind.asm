; pathfind.asm — A* pathfinding on block grid
; Implements mc_mob_ai_pathfind(start, end, out_path, max_steps, out_length)
;
; System V AMD64 ABI for this signature:
;   uint8_t mc_mob_ai_pathfind(vec3_t start, vec3_t end,
;                              vec3_t* out_path, uint32_t max_steps,
;                              uint32_t* out_length);
;
; vec3_t is 16 bytes {float x,y,z,_pad} aligned(16).
; ABI classification: two SSE eightbytes per vec3_t.
;   start:      xmm0 (x,y), xmm1 (z,_pad)
;   end:        xmm2 (x,y), xmm3 (z,_pad)
;   out_path:   rdi
;   max_steps:  esi
;   out_length: rdx
;   return:     al (uint8_t)

%include "types.inc"
%include "error.inc"
%include "block_defs.inc"

; ── External references ───────────────────────────────────────────────
EXTERN_FUNC g_block_query

; ── Constants ─────────────────────────────────────────────────────────
%define MAX_OPEN      4096
%define MAX_CLOSED    4096
%define NODE_SIZE     32          ; see node layout below

; Node layout (32 bytes):
;   [0]  int32 x
;   [4]  int32 y
;   [8]  int32 z
;   [12] int32 parent_index      ; index into closed set, -1 = none
;   [16] float g_cost
;   [20] float h_cost
;   [24] float f_cost
;   [28] uint32 flags            ; 0=unused, 1=open, 2=closed

section .bss
align 16
open_set:    resb MAX_OPEN * NODE_SIZE
closed_set:  resb MAX_CLOSED * NODE_SIZE
open_count:  resd 1
closed_count: resd 1

section .data
align 16
; 6 neighbors: +x, -x, +y, -y, +z, -z
neighbor_dx: dd  1, -1,  0,  0,  0,  0
neighbor_dy: dd  0,  0,  1, -1,  0,  0
neighbor_dz: dd  0,  0,  0,  0,  1, -1

float_one: dd 1.0

section .text

; ── Helper: calc_manhattan ────────────────────────────────────────────
; Input:  r12d=x1, r13d=y1, r14d=z1, ebx=x2, ecx=y2, r15d=z2
; Output: xmm0 = float(|x1-x2| + |y1-y2| + |z1-z2|)
; Clobbers: eax, edx, r8d
calc_manhattan:
    mov     eax, r12d
    sub     eax, ebx
    cdq
    xor     eax, edx
    sub     eax, edx
    mov     r8d, eax

    mov     eax, r13d
    sub     eax, ecx
    cdq
    xor     eax, edx
    sub     eax, edx
    add     r8d, eax

    mov     eax, r14d
    sub     eax, r15d
    cdq
    xor     eax, edx
    sub     eax, edx
    add     r8d, eax

    cvtsi2ss xmm0, r8d
    ret

; ── Helper: is_walkable ───────────────────────────────────────────────
; Input:  edi=x, esi=y, edx=z
; Output: al=1 if walkable, 0 otherwise
; Clobbers: rax, rcx, rdi, rsi, rdx, r8-r11, xmm0-xmm7
is_walkable:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 32
    mov     [rbp-4], edi
    mov     [rbp-8], esi
    mov     [rbp-12], edx

    ; Query block at (x, y, z) — should be AIR
    ; block_pos_t = {int32 x, y, z, _pad} = 16 bytes
    ; SysV ABI: rdi = (y<<32)|x, rsi = (_pad<<32)|z
    mov     edi, [rbp-4]
    movsxd  rax, dword [rbp-8]
    shl     rax, 32
    or      rdi, rax

    mov     esi, [rbp-12]

    lea     rax, [rel DECL(g_block_query)]
    mov     rax, [rax]
    call    rax

    cmp     ax, BLOCK_AIR
    jne     .iw_not_walkable

    ; Check block below (x, y-1, z)
    mov     eax, [rbp-8]
    sub     eax, 1
    cmp     eax, 0
    jl      .iw_not_walkable

    movsxd  rcx, eax
    shl     rcx, 32
    movsxd  rdi, dword [rbp-4]
    or      rdi, rcx

    mov     esi, [rbp-12]

    lea     rax, [rel DECL(g_block_query)]
    mov     rax, [rax]
    call    rax

    cmp     ax, BLOCK_AIR
    je      .iw_not_walkable
    cmp     ax, BLOCK_WATER
    je      .iw_not_walkable
    cmp     ax, BLOCK_LAVA
    je      .iw_not_walkable

    mov     al, 1
    leave
    ret

.iw_not_walkable:
    xor     eax, eax
    leave
    ret

; ── find_lowest_f ─────────────────────────────────────────────────────
; Output: eax = index of best node, -1 if empty
; Clobbers: ecx, edx, r8, r9, xmm0, xmm1
find_lowest_f:
    lea     r9, [rel open_count]
    mov     ecx, [r9]
    test    ecx, ecx
    jz      .flf_empty
    lea     r9, [rel open_set]
    xor     eax, eax
    movss   xmm0, [r9 + 24]     ; best_f = open_set[0].f_cost
    mov     edx, 1
.flf_loop:
    cmp     edx, ecx
    jge     .flf_done
    mov     r8d, edx
    shl     r8d, 5
    movss   xmm1, [r9 + r8 + 24]
    comiss  xmm1, xmm0
    jae     .flf_skip
    movaps  xmm0, xmm1
    mov     eax, edx
.flf_skip:
    inc     edx
    jmp     .flf_loop
.flf_done:
    ret
.flf_empty:
    mov     eax, -1
    ret

; ── find_in_open ──────────────────────────────────────────────────────
; Input: edi=x, esi=y, edx=z
; Output: eax = index, or -1
find_in_open:
    push    r9
    lea     r9, [rel open_set]
    push    rcx
    lea     rcx, [rel open_count]
    mov     ecx, [rcx]
    xor     eax, eax
.fio_loop:
    cmp     eax, ecx
    jge     .fio_nf
    mov     r8d, eax
    shl     r8d, 5
    cmp     [r9 + r8 + 0], edi
    jne     .fio_nx
    cmp     [r9 + r8 + 4], esi
    jne     .fio_nx
    cmp     [r9 + r8 + 8], edx
    jne     .fio_nx
    pop     rcx
    pop     r9
    ret
.fio_nx:
    inc     eax
    jmp     .fio_loop
.fio_nf:
    mov     eax, -1
    pop     rcx
    pop     r9
    ret

; ── find_in_closed ────────────────────────────────────────────────────
; Input: edi=x, esi=y, edx=z
; Output: eax = index, or -1
find_in_closed:
    push    r9
    lea     r9, [rel closed_set]
    push    rcx
    lea     rcx, [rel closed_count]
    mov     ecx, [rcx]
    xor     eax, eax
.fic_loop:
    cmp     eax, ecx
    jge     .fic_nf
    mov     r8d, eax
    shl     r8d, 5
    cmp     [r9 + r8 + 0], edi
    jne     .fic_nx
    cmp     [r9 + r8 + 4], esi
    jne     .fic_nx
    cmp     [r9 + r8 + 8], edx
    jne     .fic_nx
    pop     rcx
    pop     r9
    ret
.fic_nx:
    inc     eax
    jmp     .fic_loop
.fic_nf:
    mov     eax, -1
    pop     rcx
    pop     r9
    ret

; ── copy_node: copy 32 bytes from [rsi] to [rdi] ─────────────────────
; Clobbers: rax
copy_node:
    mov     rax, [rsi]
    mov     [rdi], rax
    mov     rax, [rsi + 8]
    mov     [rdi + 8], rax
    mov     rax, [rsi + 16]
    mov     [rdi + 16], rax
    mov     rax, [rsi + 24]
    mov     [rdi + 24], rax
    ret

; ── Stack frame layout for mc_mob_ai_pathfind ─────────────────────────
; We use a large local frame. Offsets from rbp (negative):
;   -8   : out_path (ptr)
;   -16  : out_length (ptr)
;   -20  : max_steps (int32)
;   -24  : end.x (int32)
;   -28  : end.y (int32)
;   -32  : end.z (int32)
;   -36  : best_open_index (int32)
;   -40  : current_closed_index (int32)
;   -44  : neighbor.x (int32)
;   -48  : neighbor.y (int32)
;   -52  : neighbor.z (int32)
;   -56  : saved cur.x (int32)
;   -60  : saved cur.y (int32)
;   -64  : saved cur.z (int32)
;   -68  : saved xmm6 (float, 4 bytes)
;   -72  : tentative_g (float, 4 bytes)
;   -76  : open_idx_result (int32)
;   -80  : neighbor_dir (int32)
;   -84  : path_len (int32)
;   -88  : iteration counter (int32)

%define LOCAL_SIZE     128

GLOBAL_FUNC mc_mob_ai_pathfind
    push    rbp
    mov     rbp, rsp
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15
    sub     rsp, LOCAL_SIZE

    ; Save parameters
    mov     [rbp-8], rdi         ; out_path
    mov     [rbp-16], rdx        ; out_length
    mov     [rbp-20], esi        ; max_steps

    ; Extract start position as integers
    cvttss2si r12d, xmm0
    shufps  xmm0, xmm0, 0x55
    cvttss2si r13d, xmm0
    cvttss2si r14d, xmm1

    ; Extract end position as integers
    cvttss2si ebx, xmm2
    shufps  xmm2, xmm2, 0x55
    cvttss2si ecx, xmm2
    cvttss2si r15d, xmm3

    mov     [rbp-24], ebx        ; end.x
    mov     [rbp-28], ecx        ; end.y
    mov     [rbp-32], r15d       ; end.z

    ; Initialize counts
    lea     rax, [rel open_count]
    mov     dword [rax], 0
    lea     rax, [rel closed_count]
    mov     dword [rax], 0

    ; Add start node to open set: open_set[0]
    lea     rax, [rel open_set]
    mov     [rax + 0], r12d
    mov     [rax + 4], r13d
    mov     [rax + 8], r14d
    mov     dword [rax + 12], -1
    xorps   xmm0, xmm0
    movss   [rax + 16], xmm0    ; g = 0

    ; calc h for start
    mov     ecx, [rbp-28]
    call    calc_manhattan       ; xmm0 = h
    lea     rax, [rel open_set]
    movss   [rax + 20], xmm0    ; h
    movss   [rax + 24], xmm0    ; f = g(0) + h
    mov     dword [rax + 28], 1  ; flags = open

    lea     rax, [rel open_count]
    mov     dword [rax], 1

    ; Main A* loop
    mov     dword [rbp-88], 0    ; iteration = 0

.astar_loop:
    mov     eax, [rbp-88]
    cmp     eax, [rbp-20]
    jge     .no_path

    lea     rax, [rel open_count]
    mov     ecx, [rax]
    test    ecx, ecx
    jz      .no_path

    call    find_lowest_f
    cmp     eax, -1
    je      .no_path
    mov     [rbp-36], eax        ; best_open_index

    ; Load best node coords
    mov     r9d, eax
    shl     r9d, 5
    lea     rax, [rel open_set]
    mov     r12d, [rax + r9 + 0]
    mov     r13d, [rax + r9 + 4]
    mov     r14d, [rax + r9 + 8]

    ; Check if reached goal
    cmp     r12d, [rbp-24]
    jne     .not_goal
    cmp     r13d, [rbp-28]
    jne     .not_goal
    cmp     r14d, [rbp-32]
    jne     .not_goal
    jmp     .found_path

.not_goal:
    ; Move current from open to closed
    mov     eax, [rbp-36]
    mov     r9d, eax
    shl     r9d, 5

    lea     rcx, [rel closed_count]
    mov     ecx, [rcx]
    cmp     ecx, MAX_CLOSED
    jge     .no_path

    mov     r10d, ecx
    shl     r10d, 5

    ; Copy node: open_set[best] → closed_set[closed_count]
    lea     rsi, [rel open_set]
    add     rsi, r9
    lea     rdi, [rel closed_set]
    add     rdi, r10
    call    copy_node

    mov     [rbp-40], ecx        ; current_closed_index

    inc     ecx
    lea     rax, [rel closed_count]
    mov     [rax], ecx

    ; Remove from open set: swap with last
    mov     eax, [rbp-36]
    lea     rcx, [rel open_count]
    mov     ecx, [rcx]
    dec     ecx
    lea     r8, [rel open_count]
    mov     [r8], ecx
    cmp     eax, ecx
    je      .removed

    mov     r9d, eax
    shl     r9d, 5
    mov     r10d, ecx
    shl     r10d, 5
    lea     rsi, [rel open_set]
    lea     rdi, [rel open_set]
    add     rsi, r10
    add     rdi, r9
    call    copy_node

.removed:
    ; Get current g_cost from closed set
    mov     eax, [rbp-40]
    shl     eax, 5
    lea     rcx, [rel closed_set]
    movss   xmm6, [rcx + rax + 16]

    ; Explore 6 neighbors
    mov     dword [rbp-80], 0

.neighbor_loop:
    cmp     dword [rbp-80], 6
    jge     .neighbors_done

    mov     eax, [rbp-80]
    lea     rcx, [rel neighbor_dx]
    mov     edi, r12d
    add     edi, [rcx + rax*4]
    lea     rcx, [rel neighbor_dy]
    mov     esi, r13d
    add     esi, [rcx + rax*4]
    lea     rcx, [rel neighbor_dz]
    mov     edx, r14d
    add     edx, [rcx + rax*4]

    mov     [rbp-44], edi
    mov     [rbp-48], esi
    mov     [rbp-52], edx

    ; Save state
    mov     [rbp-56], r12d
    mov     [rbp-60], r13d
    mov     [rbp-64], r14d
    movss   [rbp-68], xmm6

    ; Check if in closed set
    call    find_in_closed
    cmp     eax, -1
    jne     .skip_neighbor

    ; Check walkability
    mov     edi, [rbp-44]
    mov     esi, [rbp-48]
    mov     edx, [rbp-52]
    call    is_walkable
    test    al, al
    jz      .skip_neighbor

    ; tentative_g = current.g + 1.0
    movss   xmm6, [rbp-68]
    lea     rax, [rel float_one]
    movss   xmm1, [rax]
    addss   xmm1, xmm6
    movss   [rbp-72], xmm1      ; save tentative_g

    ; Check if in open set
    mov     edi, [rbp-44]
    mov     esi, [rbp-48]
    mov     edx, [rbp-52]
    call    find_in_open
    mov     [rbp-76], eax

    cmp     eax, -1
    je      .add_to_open

    ; Already in open — check if better
    mov     r9d, eax
    shl     r9d, 5
    movss   xmm1, [rbp-72]
    lea     rax, [rel open_set]
    movss   xmm2, [rax + r9 + 16]
    comiss  xmm1, xmm2
    jae     .skip_neighbor

    ; Update with better path
    lea     rax, [rel open_set]
    movss   [rax + r9 + 16], xmm1

    mov     r12d, [rbp-44]
    mov     r13d, [rbp-48]
    mov     r14d, [rbp-52]
    mov     ebx, [rbp-24]
    mov     ecx, [rbp-28]
    mov     r15d, [rbp-32]
    call    calc_manhattan
    lea     rax, [rel open_set]
    movss   [rax + r9 + 20], xmm0
    movss   xmm1, [rbp-72]
    addss   xmm0, xmm1
    movss   [rax + r9 + 24], xmm0
    mov     ecx, [rbp-40]
    mov     [rax + r9 + 12], ecx
    jmp     .skip_neighbor

.add_to_open:
    lea     rax, [rel open_count]
    mov     ecx, [rax]
    cmp     ecx, MAX_OPEN
    jge     .skip_neighbor

    mov     r9d, ecx
    shl     r9d, 5
    lea     rax, [rel open_set]
    mov     r8d, [rbp-44]
    mov     [rax + r9 + 0], r8d
    mov     r8d, [rbp-48]
    mov     [rax + r9 + 4], r8d
    mov     r8d, [rbp-52]
    mov     [rax + r9 + 8], r8d
    mov     r8d, [rbp-40]
    mov     [rax + r9 + 12], r8d

    movss   xmm1, [rbp-72]
    movss   [rax + r9 + 16], xmm1

    ; Save r9 before calc_manhattan (it clobbers r8)
    push    r9
    mov     r12d, [rbp-44]
    mov     r13d, [rbp-48]
    mov     r14d, [rbp-52]
    mov     ebx, [rbp-24]
    mov     ecx, [rbp-28]
    mov     r15d, [rbp-32]
    call    calc_manhattan
    pop     r9

    lea     rax, [rel open_set]
    movss   [rax + r9 + 20], xmm0
    movss   xmm1, [rbp-72]
    addss   xmm0, xmm1
    movss   [rax + r9 + 24], xmm0
    mov     dword [rax + r9 + 28], 1

    lea     rax, [rel open_count]
    mov     ecx, [rax]
    inc     ecx
    mov     [rax], ecx

.skip_neighbor:
    mov     r12d, [rbp-56]
    mov     r13d, [rbp-60]
    mov     r14d, [rbp-64]
    movss   xmm6, [rbp-68]
    mov     ebx, [rbp-24]
    mov     r15d, [rbp-32]

    inc     dword [rbp-80]
    jmp     .neighbor_loop

.neighbors_done:
    inc     dword [rbp-88]
    jmp     .astar_loop

; ── Path found: reconstruct ───────────────────────────────────────────
.found_path:
    ; Add goal node from open to closed
    mov     r9d, [rbp-36]
    shl     r9d, 5

    lea     rax, [rel closed_count]
    mov     ecx, [rax]
    mov     r10d, ecx
    shl     r10d, 5

    lea     rsi, [rel open_set]
    add     rsi, r9
    lea     rdi, [rel closed_set]
    add     rdi, r10
    call    copy_node

    ; ebx = goal_closed_index
    lea     rax, [rel closed_count]
    mov     ebx, [rax]
    mov     ecx, ebx
    inc     ecx
    mov     [rax], ecx

    ; Trace path backward: store coords in open_set scratch area
    mov     eax, ebx
    xor     ecx, ecx             ; path_len = 0

.trace_loop:
    cmp     eax, -1
    je      .trace_done
    cmp     ecx, 256
    jge     .trace_done

    mov     r9d, eax
    shl     r9d, 5
    mov     r10d, ecx
    imul    r10d, 12

    lea     r11, [rel closed_set]
    lea     r8, [rel open_set]

    mov     edi, [r11 + r9 + 0]
    mov     [r8 + r10 + 0], edi
    mov     edi, [r11 + r9 + 4]
    mov     [r8 + r10 + 4], edi
    mov     edi, [r11 + r9 + 8]
    mov     [r8 + r10 + 8], edi

    mov     eax, [r11 + r9 + 12]
    inc     ecx
    jmp     .trace_loop

.trace_done:
    mov     [rbp-84], ecx
    mov     rdi, [rbp-8]        ; out_path

    lea     r11, [rel open_set]
    xor     r8d, r8d

.copy_loop:
    cmp     r8d, ecx
    jge     .copy_done

    ; src = scratch[path_len - 1 - i]
    mov     eax, ecx
    sub     eax, 1
    sub     eax, r8d
    imul    eax, 12

    ; dst = i * 16
    mov     r9d, r8d
    shl     r9d, 4

    cvtsi2ss xmm0, dword [r11 + rax + 0]
    movss   [rdi + r9 + 0], xmm0
    cvtsi2ss xmm0, dword [r11 + rax + 4]
    movss   [rdi + r9 + 4], xmm0
    cvtsi2ss xmm0, dword [r11 + rax + 8]
    movss   [rdi + r9 + 8], xmm0
    xorps   xmm0, xmm0
    movss   [rdi + r9 + 12], xmm0

    inc     r8d
    jmp     .copy_loop

.copy_done:
    mov     rdi, [rbp-16]
    mov     eax, [rbp-84]
    mov     [rdi], eax
    mov     al, 1
    jmp     .cleanup

.no_path:
    mov     rdi, [rbp-16]
    mov     dword [rdi], 0
    xor     eax, eax

.cleanup:
    add     rsp, LOCAL_SIZE
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    pop     rbp
    ret
