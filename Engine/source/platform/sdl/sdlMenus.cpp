// Native menu bars and popup menus. SDL3 has no menu API, so these are inert:
// the engine's own GUI draws menus in-canvas. Kept as stubs because the console
// bindings in platform/menus reference them unconditionally.

#include "platform/platform.h"
#include "platform/menus/popupMenu.h"
#include "platform/menus/menuBar.h"

void PopupMenu::createPlatformPopupMenuData() {}
void PopupMenu::deletePlatformPopupMenuData() {}
void PopupMenu::createPlatformMenu()         {}

S32 PopupMenu::insertItem(S32 pos, const char* title, const char* accelerator)
{
   TORQUE_UNUSED(pos); TORQUE_UNUSED(title); TORQUE_UNUSED(accelerator);
   return -1;
}

S32 PopupMenu::insertSubMenu(S32 pos, const char* title, PopupMenu* submenu)
{
   TORQUE_UNUSED(pos); TORQUE_UNUSED(title); TORQUE_UNUSED(submenu);
   return -1;
}

bool PopupMenu::setItem(S32 pos, const char* title, const char* accelerator)
{
   TORQUE_UNUSED(pos); TORQUE_UNUSED(title); TORQUE_UNUSED(accelerator);
   return false;
}

void PopupMenu::removeItem(S32 itemPos)          { TORQUE_UNUSED(itemPos); }
U32  PopupMenu::getItemCount()                   { return 0; }
void PopupMenu::enableItem(S32 pos, bool enable)  { TORQUE_UNUSED(pos); TORQUE_UNUSED(enable); }
void PopupMenu::checkItem(S32 pos, bool checked)  { TORQUE_UNUSED(pos); TORQUE_UNUSED(checked); }
bool PopupMenu::isItemChecked(S32 pos)            { TORQUE_UNUSED(pos); return false; }

void PopupMenu::checkRadioItem(S32 firstPos, S32 lastPos, S32 checkPos)
{
   TORQUE_UNUSED(firstPos); TORQUE_UNUSED(lastPos); TORQUE_UNUSED(checkPos);
}

bool PopupMenu::canHandleID(U32 id)                        { TORQUE_UNUSED(id); return false; }
bool PopupMenu::handleSelect(U32 command, const char* text)
{
   TORQUE_UNUSED(command); TORQUE_UNUSED(text);
   return false;
}

void PopupMenu::showPopup(GuiCanvas* owner, S32 x, S32 y)
{
   TORQUE_UNUSED(owner); TORQUE_UNUSED(x); TORQUE_UNUSED(y);
}

void PopupMenu::attachToMenuBar(GuiCanvas* owner, S32 pos, const char* title)
{
   TORQUE_UNUSED(owner); TORQUE_UNUSED(pos); TORQUE_UNUSED(title);
}

void PopupMenu::removeFromMenuBar() {}

void MenuBar::createPlatformPopupMenuData() {}
void MenuBar::deletePlatformPopupMenuData() {}
void MenuBar::updateMenuBar(PopupMenu* menu)          { TORQUE_UNUSED(menu); }
void MenuBar::removeFromCanvas()                      {}
void MenuBar::attachToCanvas(GuiCanvas* owner, S32 pos)
{
   TORQUE_UNUSED(owner); TORQUE_UNUSED(pos);
}
