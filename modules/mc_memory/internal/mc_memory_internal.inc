; mc_memory_internal.inc — struct layouts for arena and pool allocators
; Must match the C struct definitions exactly in size and field offsets.

; struct mc_arena {
;     size_t cap;     ; offset 0  (8 bytes)
;     size_t used;    ; offset 8  (8 bytes)
;     char*  buf;     ; offset 16 (8 bytes)
; };
; sizeof(mc_arena) = 24

%define ARENA_OFF_CAP   0
%define ARENA_OFF_USED  8
%define ARENA_OFF_BUF   16
%define ARENA_SIZE      24

; struct mc_pool {
;     size_t   elem_size;     ; offset 0  (8 bytes)
;     uint32_t max_elements;  ; offset 8  (4 bytes)
;     uint32_t count;         ; offset 12 (4 bytes)
;     void*    backing;       ; offset 16 (8 bytes) — the single mmap/malloc block
;     void*    freelist;      ; offset 24 (8 bytes) — head of intrusive free list
; };
; sizeof(mc_pool) = 32

%define POOL_OFF_ELEM      0
%define POOL_OFF_MAX       8
%define POOL_OFF_COUNT     12
%define POOL_OFF_BACKING   16
%define POOL_OFF_FREELIST  24
%define POOL_SIZE          32
