#pragma once
#ifndef ES_APP_GUIS_GUI_PROFILE_SELECT_H
#define ES_APP_GUIS_GUI_PROFILE_SELECT_H

#include "GuiComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"
#include "components/ImageComponent.h"
#include "ProfileManager.h"
#include <vector>

class GuiProfileSelect : public GuiComponent
{
public:
	GuiProfileSelect(Window* window, const std::function<void()>& doneCallback);
	~GuiProfileSelect();

	bool input(InputConfig* config, Input input) override;
	void render(const Transform4x4f& parentTrans) override;
	void onSizeChanged() override;

private:
	std::function<void()> mDoneCallback;
	std::vector<Profile> mProfiles;
	int mSelectedIndex;

	NinePatchComponent mBackground;
	std::shared_ptr<TextComponent> mTitleText;
	std::shared_ptr<TextComponent> mInstructionsText;
	
	void populateProfiles();
	void updateSelection();
	void createNewProfilePrompt();
};

#endif // ES_APP_GUIS_GUI_PROFILE_SELECT_H
