//
// Mani Admin Plugin
//
// Copyright © 2009-2011 Giles Millward (Mani). All rights reserved.
//
// This file is part of ManiAdminPlugin.
//
// Mani Admin Plugin is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Mani Admin Plugin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Mani Admin Plugin.  If not, see <http://www.gnu.org/licenses/>.
//


//



#ifndef MANI_MENU_H
#define MANI_MENU_H

#define MAX_AMX_MENU (8)
#define MAX_ESCAPE_MENU (7)

#include <vector>
#include <algorithm>
#include "mani_util.h"

extern	void	DrawMenu(int player_index, int time, int range, bool back, bool more, bool cancel, char *menu_string, bool final);
extern	int		RePopOption(int return_option);

enum
{
	REDRAW_MENU = 0,
	CLOSE_MENU,
	REPOP_MENU,
	REPOP_MENU_WAIT1,
	REPOP_MENU_WAIT2,
	REPOP_MENU_WAIT3,
	NEW_MENU,
	NOTHING,
	PREVIOUS_MENU
};

class MenuTemporal;
class MenuPage;

class MenuItem
{
public:
	MenuItem ();
	~MenuItem();

	void SetDisplayText(const char *text, ...);
	void SetHiddenText(const char *text, ...);
	void AddPreText(const char *text, ...);
	void AddPostText(const char *text, ...);
	bool operator<(const MenuItem &right) const;
	virtual	int		MenuItemFired(player_t *player_ptr, MenuPage *m_page_ptr) = 0;

	ParamManager	params;
	BasicStr		display_text;
	BasicStr		hidden_text;
	std::vector<BasicStr> pre_text; // Free text that can be placed before the menu item (AMX only)
	std::vector<BasicStr> post_text; // Free text that can be placed after the menu item (AMX only)
};

class MenuPage
{
	friend class MenuTemporal;
	friend class MenuManager;
public:
	// Acsending Display Text sort


	MenuPage();
	~MenuPage();
	void SetTitle(const char *menu_title, ...);
	void SetEscLink(const char *esc_link_text, ...);
	void SetInputObject(bool enabled);
	bool IsChatHooked(void);
	virtual bool PopulateMenuPage(player_t *player_ptr) = 0;
	void Kill();
	int	 Size();
	void SetTimeout(int time);
	void AddItem(MenuItem *ptr);
	void SortDisplay();
	void SortHidden();
	void RenderPage(player_t *player_ptr, int history_level);
	void RenderInputObject(player_t *player_ptr);
	ParamManager	params;

private:
	int			timeout;
	bool		need_more;
	bool		need_back;
	int			current_index;
	BasicStr	title;
	BasicStr	esc_link;
	bool		input_object;
	bool		hook_chat;
	std::vector<MenuItem *> menu_items;
};

class FreePage
{
	friend class MenuTemporal;
public:
	FreePage() {}
	~FreePage() {}
	virtual bool	OptionSelected(player_t *player_ptr, const int option) = 0;
	void	SetTimeout(int time) {timeout = time;}
	int		timeout;
};

class MenuTemporal
{
	friend class MenuManager;
public:
	MenuTemporal();
	~MenuTemporal();
	void Kill();
	void AddMenu(MenuPage *ptr);
	void OptionSelected(player_t *player_ptr, const int option);
	
private:
	bool		allow_repop;
	int			priority;
	time_t		timeout_timestamp;
	BasicStr	title;
	std::vector<MenuPage *> menu_pages; // Most commonly used for menus
	FreePage *free_page; // More of a raw interface (used in mani_stats.cpp)
};

class MenuManager
{
	friend class MenuTemporal;
public:
	MenuManager(){};
	~MenuManager(){};
	void Kill();
	void Kill(player_t *player_ptr);
	void OptionSelected(player_t *player_ptr, const int option);
	void RepopulatePage(player_t *player_ptr);
	void AddMenu(player_t *player_ptr, MenuPage *ptr, int priority, int timeout);
	bool ChatHooked(player_t *player_ptr);
	void KillLast(player_t *player_ptr);
	void AddFreePage(player_t *player_ptr, FreePage *ptr, int priority, int timeout);
	bool CanAddMenu(player_t *player_ptr, int priority);
	const int GetHistorySize(player_t *player_ptr);
	void ClientActive(player_t *player_ptr);
	void ClientDisconnect(player_t *player_ptr);
	void Load();
	void Unload();
	void LevelInit();
	void GameFrame();

private:
	MenuTemporal player_list[64];
	int			 game_frame_repop[64];
#if defined ( ORANGE )
public:
	void ResetMenuShowing ( int player_index, bool off = true );
	
	bool GetMenuShowing ( int player_index );
	float GetExpirationTime ( int player_index );
private:
	bool	menu_showing[64];
	float	menu_expiration_time[64];
#endif

#if defined ( ORANGE )
	float		 next_time_check;
#endif
};


// Short macro to define lots of derived classes from MenuPage
#define MENUPAGE_DEC(_name) \
class _name : public MenuPage {void PopulateMenuPage(player_t *player_ptr);}

// Short macro to define lots of derived classes from MenuItem
#define MENUITEM_DEC(_name) \
class _name : public MenuItem {int MenuItemFired(player_t *player_ptr, MenuPage *m_page);}

#define MENUALL_DEC(_name) \
class _name##Page : public MenuPage {bool PopulateMenuPage(player_t *player_ptr);}; \
class _name##Item : public MenuItem {int MenuItemFired(player_t *player_ptr, MenuPage *m_page);} \

#define MENUFRIEND_DEC(_name) \
friend class _name##Page; \
friend class _name##Item

#define MENUPAGE_CREATE_FIRST(_name, _pl_ptr, _priority, _timeout) \
{ \
	g_menu_mgr.Kill(_pl_ptr); \
	MenuPage *ptr = new _name(); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.Kill(_pl_ptr); \
	} \
	else \
	{ \
		ptr->RenderPage(_pl_ptr, g_menu_mgr.GetHistorySize(_pl_ptr)); \
	} \
}

#define MENUPAGE_CREATE(_name, _pl_ptr, _priority, _timeout) \
{ \
	MenuPage *ptr = new _name(); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.KillLast(_pl_ptr); \
		return REPOP_MENU; \
	} \
	ptr->RenderPage(_pl_ptr, g_menu_mgr.GetHistorySize(_pl_ptr)); \
}

#define MENUPAGE_CREATE_PARAM(_name, _pl_ptr, _param1, _priority, _timeout) \
{ \
	MenuPage *ptr = new _name(); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	ptr->params._param1; \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.KillLast(_pl_ptr); \
		return REPOP_MENU; \
	} \
	ptr->RenderPage(_pl_ptr, g_menu_mgr.GetHistorySize(_pl_ptr)); \
}

#define MENUPAGE_CREATE_PARAM2(_name, _pl_ptr, _param1, _param2, _priority, _timeout) \
{ \
	MenuPage *ptr = new _name(); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	ptr->params._param1; \
	ptr->params._param2; \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.KillLast(_pl_ptr); \
		return REPOP_MENU; \
	} \
	ptr->RenderPage(_pl_ptr, g_menu_mgr.GetHistorySize(_pl_ptr)); \
}

#define MENUPAGE_CREATE_PARAM3(_name, _pl_ptr, _param1, _param2, _param3, _priority, _timeout) \
{ \
	MenuPage *ptr = new _name(); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	ptr->params._param1; \
	ptr->params._param2; \
	ptr->params._param3; \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.KillLast(_pl_ptr); \
		return REPOP_MENU; \
	} \
	ptr->RenderPage(_pl_ptr, g_menu_mgr.GetHistorySize(_pl_ptr)); \
}

#define MENUPAGE_CREATE_PARAM4(_name, _pl_ptr, _param1, _param2, _param3, _param4, _priority, _timeout) \
{ \
	MenuPage *ptr = new _name(); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	ptr->params._param1; \
	ptr->params._param2; \
	ptr->params._param3; \
	ptr->params._param4; \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.KillLast(_pl_ptr); \
		return REPOP_MENU; \
	} \
	ptr->RenderPage(_pl_ptr, g_menu_mgr.GetHistorySize(_pl_ptr)); \
}

#define MENUOPTION_CREATE(_class_type, _translate_id, _param) \
{ \
	MenuItem *_param = new _class_type; \
	_param->SetDisplayText("%s", Translate(player_ptr, _translate_id)); \
	_param->params.AddParam("sub_option", #_param); \
	this->AddItem(_param); \
}

#define MENUOPTION_CREATE_PARAM(_class_type, _translate_id, _param1) \
{ \
	MenuItem *_param = new _class_type; \
	_param->SetDisplayText("%s", Translate(player_ptr, _translate_id)); \
	_param->params._param1; \
	this->AddItem(_param); \
}

#define INPUTPAGE_CREATE(_name, _pl_ptr, _priority, _timeout) \
{ \
	MenuPage *ptr = new _name(); \
	ptr->SetInputObject(true); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.KillLast(_pl_ptr); \
		return REPOP_MENU; \
	} \
	ptr->RenderInputObject(_pl_ptr); \
}

#define INPUTPAGE_CREATE_PARAM(_name, _pl_ptr, _param1, _priority, _timeout) \
{ \
	MenuPage *ptr = new _name(); \
	ptr->SetInputObject(true); \
	g_menu_mgr.AddMenu(_pl_ptr, ptr, _priority, _timeout); \
	ptr->params._param1; \
	if (!ptr->PopulateMenuPage(_pl_ptr) || ptr->Size() == 0) \
	{ \
		g_menu_mgr.KillLast(_pl_ptr); \
		return REPOP_MENU; \
	} \
	ptr->RenderInputObject(_pl_ptr); \
}

extern MenuManager g_menu_mgr;
extern MenuPage *g_page_ptr;
#endif
