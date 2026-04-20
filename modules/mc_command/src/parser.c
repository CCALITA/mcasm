#include "command_internal.h"
#include <string.h>
#include <ctype.h>

static const char* skip_spaces(const char* s)
{
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

uint32_t mc_command_parse(const char* input,
                          char* out_name, uint32_t name_max,
                          const char** out_args, uint32_t max_args,
                          char* scratch, uint32_t scratch_max)
{
    if (input == NULL || out_name == NULL || scratch == NULL || scratch_max == 0) {
        if (out_name != NULL && name_max > 0) {
            out_name[0] = '\0';
        }
        return 0;
    }

    /* Copy input to scratch buffer for in-place tokenization. */
    uint32_t input_len = (uint32_t)strlen(input);
    if (input_len >= scratch_max) {
        input_len = scratch_max - 1;
    }
    memcpy(scratch, input, input_len);
    scratch[input_len] = '\0';

    const char* pos = skip_spaces(scratch);

    if (*pos != '/') {
        out_name[0] = '\0';
        return 0;
    }
    pos++; /* skip '/' */

    uint32_t name_len = 0;
    while (*pos != '\0' && !isspace((unsigned char)*pos) && name_len < name_max - 1) {
        out_name[name_len++] = *pos++;
    }
    out_name[name_len] = '\0';

    if (name_len == 0) {
        return 0;
    }

    uint32_t argc = 0;
    while (argc < max_args) {
        pos = skip_spaces(pos);
        if (*pos == '\0') {
            break;
        }

        if (*pos == '"') {
            pos++; /* skip opening quote */
            out_args[argc++] = (char*)pos;
            while (*pos != '\0' && *pos != '"') {
                pos++;
            }
            if (*pos == '"') {
                *(char*)pos = '\0';
                pos++;
            }
        } else {
            out_args[argc++] = (char*)pos;
            while (*pos != '\0' && !isspace((unsigned char)*pos)) {
                pos++;
            }
            if (*pos != '\0') {
                *(char*)pos = '\0';
                pos++;
            }
        }
    }

    return argc;
}
