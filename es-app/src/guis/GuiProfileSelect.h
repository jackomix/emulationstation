#pragma once
#ifndef ES_APP_GUIS_GUI_PROFILE_SELECT_H
#define ES_APP_GUIS_GUI_PROFILE_SELECT_H

#include "GuiComponent.h"
#include "ProfileManager.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include <vector>
#include <memory>
#include <functional>

class GuiProfileSelect : public GuiComponent
{
public:
	GuiProfileSelect(Window* window, const std::function<void()>& doneCallback);
	~GuiProfileSelect();

	void onSizeChanged() override;
	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	std::vector<HelpPrompt> getHelpPrompts() override;

private:
	std::function<void()> mDoneCallback;
	std::vector<Profile> mProfiles;

	std::shared_ptr<ComponentGrid> mGrid;
	NinePatchComponent mBackground;

	bool mBypass;

	void populateProfiles();
	void createNewProfilePrompt();
	void selectProfile(const Profile& profile);
	void showProfileOptions(const Profile& profile);
	void deleteProfile(const Profile& profile);
};

#endif // ES_APP_GUIS_GUI_PROFILE_SELECT_H
