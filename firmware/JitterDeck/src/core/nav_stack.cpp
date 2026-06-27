#include "nav_stack.h"
#include "menu_defs.h"
#include "config.h"

// ========================================================
//  Nav Stack variables 
//  static memory allocation of the stack for 8 levels
//  Zero heap allocation and no lag 
// ========================================================

static NavFrame _stack[NAV_STACK_MAX_DEPTH];
static uint8_t  _depth = 0;

// ====================================================
//  navInit() - seeds the stack with root main menu
// ====================================================

void navInit()
{
    _depth = 0;
    navPush(MENU_MAIN, MENU_MAIN_COUNT, "JITTERDECK");
}

// ============================================================
//  navPush() - going deeper into stack and with depth guard
// ============================================================

bool navPush(const MenuItem* items, uint8_t count, const char* title)
{
    if (_depth >= NAV_STACK_MAX_DEPTH)
        return false;

    _stack[_depth].items  = items;
    _stack[_depth].count  = count;
    _stack[_depth].cursor = 0;
    _stack[_depth].title  = title;
    _depth++;

    return true;
}

// ======================
//  navPop()
//  Returns false if already at root (nothing to pop).
// ======================

// ============================
//  navPop() - going back
// ============================

bool navPop()
{
    if (_depth <= 1)
        return false;

    _depth--;
    return true;
}

// ===================================
//  navCurrent() - reading the top
// ===================================

const NavFrame* navCurrent()
{
    if (_depth == 0)
        return nullptr;

    return &_stack[_depth - 1];
}

// ==========================================
//  navCursorUp() / Down() - wrap around
// ==========================================

void navCursorUp()
{
    if (_depth == 0) return;

    NavFrame& f = _stack[_depth - 1];
    f.cursor = (f.cursor == 0) ? (f.count - 1) : (f.cursor - 1);
}

void navCursorDown()
{
    if (_depth == 0) return;

    NavFrame& f = _stack[_depth - 1];
    f.cursor = (f.cursor + 1) % f.count;
}

// ==========================
//  navSelectedItem()
// ==========================

const MenuItem* navSelectedItem()
{
    const NavFrame* f = navCurrent();
    if (!f) return nullptr;

    return &f->items[f->cursor];
}

// ======================
//  navDepth()
// ======================

uint8_t navDepth()
{
    return _depth;
}