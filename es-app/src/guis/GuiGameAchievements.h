#pragma once

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"
#include "GuiSettings.h"
#include "RetroAchievements.h"
#include "GuiRetroAchievements.h"

class FileData;

class GuiGameAchievements : public GuiSettings
{
public:
	static void show(Window* window, int gameId);
	static void show(Window* window, FileData* game);

	void	render(const Transform4x4f& parentTrans) override;
	bool	input(InputConfig* config, Input input) override;

	std::vector<HelpPrompt> getHelpPrompts() override;

protected:
	GuiGameAchievements(Window *window, GameInfoAndUserProgress ra, FileData* game = nullptr);

	void	centerWindow();
	void	updateTab();

	FileData* mFile;
	std::shared_ptr<RetroAchievementProgress> mProgress;
	int mActiveTab = 0; // 0 = Achievements, 1 = Play History
	std::vector<ComponentListRow> mAchievementRows;
	std::string mAchievementSubtitle;
};
