#pragma once

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentList.h"
#include "components/ComponentTab.h"
#include "RetroAchievements.h"
#include "GuiRetroAchievements.h"
#include <future>
#include <map>

class FileData;
class TextComponent;
class ImageComponent;
class ButtonComponent;

class GuiGameOptions;

class GuiGameAchievements : public GuiComponent
{
public:
	static void show(Window* window, int gameId);
	static void show(Window* window, FileData* game);

	void	render(const Transform4x4f& parentTrans) override;
	void	update(int deltaTime) override;
	bool	input(InputConfig* config, Input input) override;
	void	onSizeChanged() override;

	std::vector<HelpPrompt> getHelpPrompts() override;

protected:
	GuiGameAchievements(Window *window, GameInfoAndUserProgress ra, FileData* game = nullptr);

	void	centerWindow();

	void populateTabContent();
	void populateAchievementsTab();
	void populatePlayHistoryTab();
	void populateInfoTab();
	void updateAchievementsHeader();

	FileData* mFile;
	GameInfoAndUserProgress mRaInfo;
	std::future<GameInfoAndUserProgress> mRaFuture;
	bool mIsLoadingAchievements;
	int mActiveTab;
	std::map<int, int> mTabCursors;

	NinePatchComponent mBackground;
	ComponentGrid mGrid;
	std::shared_ptr<ComponentGrid> mHeaderGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextComponent> mSubtitle;
	std::shared_ptr<ImageComponent> mTitleImage;
	std::shared_ptr<RetroAchievementProgress> mProgress;

	std::shared_ptr<ComponentTab> mTabs;
	std::shared_ptr<ComponentList> mList;
	std::shared_ptr<ComponentGrid> mButtonGrid;
	std::shared_ptr<GuiGameOptions> mOptionsUI;
};
