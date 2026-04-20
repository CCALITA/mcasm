/* chat.c -- Chat message display */

#include "mc_ui.h"
#include "ui_internal.h"

void mc_ui_draw_chat(const char **messages, uint32_t count)
{
    if (!messages || count == 0) return;

    uint32_t visible = count;
    if (visible > CHAT_MAX_LINES) visible = CHAT_MAX_LINES;

    /* Draw from bottom up so newest messages are at the bottom */
    for (uint32_t i = 0; i < visible; ++i) {
        uint32_t idx = count - visible + i;
        if (!messages[idx]) continue;

        float y = CHAT_BASE_Y + (float)i * CHAT_LINE_HEIGHT;
        mc_ui_draw_text(messages[idx], CHAT_X, y, 1.0f);
    }
}
