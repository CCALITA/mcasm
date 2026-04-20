; entity_defs.inc — Component struct sizes for mc_entity module
; Must match C headers — CI validates via abi-check
;
; Shared between ecs.asm and components.asm to avoid duplication.

%define SIZEOF_PHYSICS  48
%define SIZEOF_HEALTH   12
