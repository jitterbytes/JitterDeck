#include "fsm.h"

// ================================================================
// _screen : the currently active top level state. 
// The ino file reads this every loop tick
// _activeTool : only meaningful when _screen == SCREEN_TOOL. 
// Stores which tool was selected 
// ================================================================

static Screen _screen    = SCREEN_BOOT;
static ToolID _activeTool = TOOL_NONE;

void fsmInit()
{
    _screen     = SCREEN_BOOT;
    _activeTool = TOOL_NONE;
}

void setScreen(Screen screen)  { _screen = screen; }
Screen getScreen()             { return _screen;   }

void   setActiveTool(ToolID t) { _activeTool = t; }
ToolID getActiveTool()         { return _activeTool; }