; particle_update.asm — SIMD hot-path for particle position / velocity update
;
; void mc_particle_update_asm(particle_pool_t *pool, uint32_t count, float dt)
;
; System V AMD64 ABI:
;   rdi  = pool pointer
;   esi  = count (uint32_t)
;   xmm0 = dt (float)
;
; SoA layout offsets within particle_pool_t (MAX_PARTICLES = 4096):
;   Each float array = 4096 * 4 = 16384 bytes, 16-byte aligned.
;   pos_x:  offset 0
;   pos_y:  offset 16384
;   pos_z:  offset 32768
;   vel_x:  offset 49152
;   vel_y:  offset 65536
;   vel_z:  offset 81920

%include "platform.inc"

section .data
    align 16
gravity_const: dd 9.81, 9.81, 9.81, 9.81

section .text

%define POOL_POS_X   0
%define POOL_POS_Y   16384
%define POOL_POS_Z   32768
%define POOL_VEL_X   49152
%define POOL_VEL_Y   65536
%define POOL_VEL_Z   81920
%define FLOAT_ARRAY_SIZE 16384

GLOBAL_FUNC mc_particle_update_asm
    ; rdi = pool, esi = count, xmm0 = dt
    push    rbx
    push    r12
    push    r13
    push    r14
    push    r15

    mov     r12, rdi            ; r12 = pool base
    mov     r13d, esi           ; r13d = count

    ; Broadcast dt into all 4 lanes of xmm1
    shufps  xmm0, xmm0, 0x00
    movaps  xmm1, xmm0         ; xmm1 = {dt, dt, dt, dt}

    ; Load gravity vector: {g, g, g, g} where g = 9.81
    lea     rax, [rel gravity_const]
    movaps  xmm2, [rax]

    ; gravity_dt = gravity * dt
    mulps   xmm2, xmm1         ; xmm2 = {g*dt, g*dt, g*dt, g*dt}

    ; Process particles in batches of 4 (SSE)
    mov     r14d, r13d
    shr     r14d, 2             ; r14d = count / 4  (full SIMD batches)
    and     r13d, 3             ; r13d = count % 4  (remainder)

    xor     r15d, r15d          ; r15 = byte offset into float arrays (i * 4)

    test    r14d, r14d
    jz      .remainder

.loop4:
    ; --- velocity.y -= gravity * dt ---
    lea     rax, [r12 + POOL_VEL_Y]
    movaps  xmm3, [rax + r15]
    subps   xmm3, xmm2
    movaps  [rax + r15], xmm3

    ; --- pos.x += vel.x * dt ---
    lea     rax, [r12 + POOL_VEL_X]
    movaps  xmm3, [rax + r15]          ; vel_x[i..i+3]
    mulps   xmm3, xmm1                 ; vel_x * dt
    lea     rbx, [r12 + POOL_POS_X]
    movaps  xmm4, [rbx + r15]          ; pos_x[i..i+3]
    addps   xmm4, xmm3
    movaps  [rbx + r15], xmm4

    ; --- pos.y += vel.y * dt ---
    lea     rax, [r12 + POOL_VEL_Y]
    movaps  xmm3, [rax + r15]          ; vel_y (already updated with gravity)
    mulps   xmm3, xmm1
    lea     rbx, [r12 + POOL_POS_Y]
    movaps  xmm4, [rbx + r15]
    addps   xmm4, xmm3
    movaps  [rbx + r15], xmm4

    ; --- pos.z += vel.z * dt ---
    lea     rax, [r12 + POOL_VEL_Z]
    movaps  xmm3, [rax + r15]
    mulps   xmm3, xmm1
    lea     rbx, [r12 + POOL_POS_Z]
    movaps  xmm4, [rbx + r15]
    addps   xmm4, xmm3
    movaps  [rbx + r15], xmm4

    add     r15, 16             ; advance 4 floats = 16 bytes
    dec     r14d
    jnz     .loop4

.remainder:
    ; Handle remaining 0-3 particles one at a time (scalar SSE)
    test    r13d, r13d
    jz      .done

.loop1:
    ; vel_y[i] -= gravity * dt  (scalar)
    lea     rax, [r12 + POOL_VEL_Y]
    movss   xmm3, [rax + r15]
    lea     rbx, [rel gravity_const]
    movss   xmm4, [rbx]                ; scalar gravity
    mulss   xmm4, xmm1                 ; g * dt (xmm1 still has dt in low lane)
    subss   xmm3, xmm4
    movss   [rax + r15], xmm3

    ; pos.x += vel.x * dt
    lea     rax, [r12 + POOL_VEL_X]
    movss   xmm3, [rax + r15]
    mulss   xmm3, xmm1
    lea     rbx, [r12 + POOL_POS_X]
    movss   xmm4, [rbx + r15]
    addss   xmm4, xmm3
    movss   [rbx + r15], xmm4

    ; pos.y += vel.y * dt
    lea     rax, [r12 + POOL_VEL_Y]
    movss   xmm3, [rax + r15]
    mulss   xmm3, xmm1
    lea     rbx, [r12 + POOL_POS_Y]
    movss   xmm4, [rbx + r15]
    addss   xmm4, xmm3
    movss   [rbx + r15], xmm4

    ; pos.z += vel.z * dt
    lea     rax, [r12 + POOL_VEL_Z]
    movss   xmm3, [rax + r15]
    mulss   xmm3, xmm1
    lea     rbx, [r12 + POOL_POS_Z]
    movss   xmm4, [rbx + r15]
    addss   xmm4, xmm3
    movss   [rbx + r15], xmm4

    add     r15, 4              ; advance 1 float = 4 bytes
    dec     r13d
    jnz     .loop1

.done:
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbx
    ret
