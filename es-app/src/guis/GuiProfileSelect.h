#pragma once
#ifndef ES_APP_GUIS_GUI_PROFILE_SELECT_H
#define ES_APP_GUIS_GUI_PROFILE_SELECT_H

#include "guis/GuiSettings.h"
#include "ProfileManager.h"
#include <vector>

class GuiProfileSelect : public GuiSettings
{
public:
	GuiProfileSelect(Window* window, const std::function<void()>& doneCallback);
	~GuiProfileSelect();

private:
	std::function<void()> mDoneCallback;
	std::vector<Profile> mProfiles;

	void populateProfiles();
	void createNewProfilePrompt();
};

#endif // ES_APP_GUIS_GUI_PROFILE_SELECT_H
