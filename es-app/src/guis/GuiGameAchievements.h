#pragma once

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"
#include "components/ComponentTab.h"
#include "GuiSettings.h"
#include "RetroAchievements.h"
#include "GuiRetroAchievements.h"

class FileData;

class GuiGameAchievements : public GuiSettings
{
public:
	static void show(Window* window, int gameId);

	void	render(const Transform4x4f& parentTrans) override;
	bool	input(InputConfig* config, Input input) override;

	std::vector<HelpPrompt> getHelpPrompts() override;

protected:
	GuiGameAchievements(Window *window, GameInfoAndUserProgress ra);

	void	centerWindow();

	void	populateTabContent();
	void	populateAchievementsTab();
	void	populatePlayHistoryTab();

	FileData* mFile;
	std::shared_ptr<RetroAchievementProgress> mProgress;
	std::shared_ptr<ComponentTab> mTabs;
	int mActiveTab;
	bool mTabsHasFocus;
	GameInfoAndUserProgress mRaInfo;
};
