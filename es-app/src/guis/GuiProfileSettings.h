#pragma once
#ifndef ES_APP_GUIS_GUI_PROFILE_SETTINGS_H
#define ES_APP_GUIS_GUI_PROFILE_SETTINGS_H

#include "guis/GuiSettings.h"
#include "ProfileManager.h"

class GuiProfileSettings : public GuiSettings
{
public:
	GuiProfileSettings(Window* window);
	~GuiProfileSettings();

private:
	void refreshMenu();
	void createProfile();
	void deleteProfilePrompt();
	void renameProfilePrompt();
	void switchProfile();
};

#endif // ES_APP_GUIS_GUI_PROFILE_SETTINGS_H
