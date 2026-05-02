#pragma once
#ifndef ES_APP_GUIS_GUI_PROFILE_MANAGER_H
#define ES_APP_GUIS_GUI_PROFILE_MANAGER_H

#include "guis/GuiSettings.h"

class GuiProfileManager : public GuiSettings
{
public:
	GuiProfileManager(Window* window);
	virtual ~GuiProfileManager();

	bool input(InputConfig* config, Input input) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	void populateList();
	void createNewProfile();
	void openOptions(const std::string& profile);
	void deleteProfile(const std::string& profile);
	void renameProfile(const std::string& profile);
};

#endif // ES_APP_GUIS_GUI_PROFILE_MANAGER_H
