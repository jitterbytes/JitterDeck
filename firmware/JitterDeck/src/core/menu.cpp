#include <Arduino.h>
#include <U8g2lib.h>
 
#include "menu.h"
#include "menu_defs.h"
#include "nav_stack.h"
#include "button.h"
#include "fsm.h"
#include "screens.h"
#include "config.h"
 
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

// ===================================================
//  Layout constants  (128 × 64 display)
//
//  Three-item carousel:
//
//  y= 0  ┌================================================┐
//        │  Header: parent menu title     │  10px tall
//  y=10  │                                │
//  y=14  ├================================================┤  separator line
//        │                                │
//  y=24  │  prev item  (small font)       │  5x7 font, baseline at y=24
//        │                                │
//  y=38  │  > FOCUSED ITEM  (bold font)   │  7x13B font, baseline at y=38
//        │                                │
//  y=52  │  next item  (small font)       │  5x7 font, baseline at y=52
//        │                                │
//  y=54  ├================================================┤  separator line
//        │                                │
//  y=60  │  ● ○ ○  depth dots             │  centred
//  y=64  └================================================┘
// ===================================================

static const uint8_t HEADER_H      = 14;
static const uint8_t FOCUSED_Y     = 38;   // baseline of focused label
static const uint8_t PREV_Y        = 24;   // baseline of item above
static const uint8_t NEXT_Y        = 52;   // baseline of item below
static const uint8_t INDICATOR_X   = 2;    // "▶" x position
static const uint8_t LABEL_X       = 10;   // item text x position

// =================================
//  Internal: draw depth indicator dots
//  One dot per stack level, filled = current depth
// =================================

static void drawDepthDots(uint8_t depth)
{
    const uint8_t DOT_R    = 2;
    const uint8_t DOT_GAP  = 7;
    const uint8_t DOT_Y    = 60;
    const uint8_t startX   = (DISPLAY_W - (depth * DOT_GAP - DOT_GAP / 2)) / 2;
 
    for (uint8_t i = 0; i < depth; i++)
    {
        uint8_t cx = startX + i * DOT_GAP;
        if (i == depth - 1)
            u8g2.drawDisc(cx, DOT_Y, DOT_R);    // filled = current level
        else
            u8g2.drawCircle(cx, DOT_Y, DOT_R);  // hollow = parent levels
    }
}

// =================================
//  Internal: truncate label to fit pixel width
//  Returns number of chars that fit.
//  u8g2 has no built-in clip, so we measure manually.
// =================================

static void drawClipped(const char* label, uint8_t x, uint8_t y, uint8_t maxW)
{
    char buf[24];
    uint8_t len = 0;
    uint16_t w  = 0;
 
    while (label[len] && len < sizeof(buf) - 1)
    {
        uint8_t cw = u8g2.getUTF8Width("A"); // monospace approx; refine per font
        // Use glyph width for accuracy
        char tmp[2] = { label[len], '\0' };
        cw = u8g2.getStrWidth(tmp);
 
        if (w + cw > maxW) break;
        buf[len] = label[len];
        w += cw;
        len++;
    }
 
    // Append ellipsis if truncated
    if (label[len] != '\0' && len >= 2)
    {
        buf[len - 1] = '.';
        buf[len - 2] = '.';
    }
 
    buf[len] = '\0';
    u8g2.drawStr(x, y, buf);
}

// =================================
//  drawMenu()
//  Renders the carousel from current nav frame.
// =================================

void drawMenu()
{
    const NavFrame* frame = navCurrent();
    if (!frame) return;
 
    const uint8_t  count    = frame->count;
    const uint8_t  cur      = frame->cursor;
    const uint8_t  prevIdx  = (cur == 0) ? (count - 1) : (cur - 1);
    const uint8_t  nextIdx  = (cur + 1) % count;
 
    u8g2.clearBuffer();
 
    // == Header ======================================================
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(4, 10, frame->title);
    u8g2.drawHLine(0, HEADER_H, DISPLAY_W);
 
    // == Separator above depth dots ===================================
    u8g2.drawHLine(0, 54, DISPLAY_W);
 
    // == Previous item (small, dimmed via lighter font) ===============
    u8g2.setFont(u8g2_font_5x7_tr);
    drawClipped(frame->items[prevIdx].label, LABEL_X, PREV_Y, DISPLAY_W - LABEL_X - 2);
 
    // == Focused item (bold, large, with selection arrow) =============
    u8g2.setFont(u8g2_font_7x13B_tr);
    u8g2.drawStr(INDICATOR_X, FOCUSED_Y, "\x10");    // right-arrow glyph (▶ approx)
    drawClipped(frame->items[cur].label, LABEL_X, FOCUSED_Y, DISPLAY_W - LABEL_X - 2);
 
    // Branch indicator: show ">" suffix if item has children
    if (frame->items[cur].toolID == TOOL_NONE && frame->items[cur].children != nullptr)
    {
        uint16_t labelW = u8g2.getStrWidth(frame->items[cur].label);
        uint8_t  arrowX = LABEL_X + labelW + 3;
        if (arrowX < DISPLAY_W - 6)
            u8g2.drawStr(arrowX, FOCUSED_Y, ">");
    }
 
    // == Next item (small) ============================================
    u8g2.setFont(u8g2_font_5x7_tr);
    drawClipped(frame->items[nextIdx].label, LABEL_X, NEXT_Y, DISPLAY_W - LABEL_X - 2);
 
    // == Depth dots ===================================================
    drawDepthDots(navDepth());
 
    u8g2.sendBuffer();
}

// =================================
//  handleMenu()
//  Reads buttons and drives navigation + FSM.
// =================================

void handleMenu()
{
    const ButtonEvent evUp     = btnUp();
    const ButtonEvent evDown   = btnDown();
    const ButtonEvent evSelect = btnSelect();
 
    // == UP ===========================================================
    if (evUp == BTN_EVT_SHORT_PRESS || evUp == BTN_EVT_LONG_PRESS)
        navCursorUp();
 
    // == DOWN =========================================================
    if (evDown == BTN_EVT_SHORT_PRESS || evDown == BTN_EVT_LONG_PRESS)
        navCursorDown();
 
    // == SELECT: short = back, long = enter ===========================
    if (evSelect == BTN_EVT_SHORT_PRESS)
    {
        if (!navPop())
        {
            // Already at root — short press at root does nothing
            // (could show a "Press & hold to enter" hint in future)
        }
    }
    else if (evSelect == BTN_EVT_LONG_PRESS)
    {
        const MenuItem* item = navSelectedItem();
        if (!item) return;
 
        if (item->toolID != TOOL_NONE)
        {
            // Leaf node → reset tool state then launch
            setActiveTool(item->toolID);
            onToolEnter(item->toolID);
            setScreen(SCREEN_TOOL);
        }
        else if (item->children != nullptr && item->childCount > 0)
        {
            // Branch node → push child menu onto stack
            navPush(item->children, item->childCount, item->label);
        }
    }
}