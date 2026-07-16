#pragma once

#include "GuiComponent.h"
#include "RetroAchievements.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/ImageComponent.h"
#include "components/WebImageComponent.h"
#include "components/TextComponent.h"
#include <memory>
#include <vector>

class FileData;
class Window;

class RetroAchievementProgress : public GuiComponent
{
public:
	RetroAchievementProgress(Window* window, int valueSoftcore, int valueHardcore, int max, const std::string& label);

	void onSizeChanged() override;
	void render(const Transform4x4f& parentTrans) override;
	void setColor(unsigned int color) override;

private:
	int mValueSoftCore;
	int mValueHardCore;
	int mMax;

	std::shared_ptr<TextComponent> mText;
};

class GuiRetroAchievements : public GuiComponent
{
public:
    static void show(Window* window);

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const Transform4x4f& parentTrans) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

    static FileData* getFileData(const std::string& cheevosGameId);

private:
    GuiRetroAchievements(Window* window, RetroAchievementInfo ra);
    void centerWindow();
    void populateGameList();
    void updateDetailPanel();
    void cycleSort();
    void cycleFilter();

    NinePatchComponent mBackground;
    ComponentGrid mGrid;
    std::shared_ptr<ComponentGrid> mRightPanel;

    std::shared_ptr<ComponentList> mList;

    std::shared_ptr<ImageComponent> mBoxArt;
    std::shared_ptr<WebImageComponent> mBadge;
    std::shared_ptr<TextComponent> mGameTitle;
    std::shared_ptr<TextComponent> mPlayTime;
    std::shared_ptr<TextComponent> mPlayCount;
    std::shared_ptr<TextComponent> mLastPlayed;
    std::shared_ptr<RetroAchievementProgress> mProgress;
    std::shared_ptr<TextComponent> mSortFilterLabel;

    struct GameEntry {
        FileData* fileData;
        RetroAchievementGame raGame;
        bool hasRaGame;
        std::string name;
        long gameTimeSeconds;
        int playCount;
        std::string lastPlayed;
    };
    std::vector<GameEntry> mAllGames;
    std::vector<GameEntry*> mFilteredGames;

    enum class SortMode { MostPlayed, LastPlayed, Title };
    enum class FilterMode { All, WithAchievements, Completed };
    SortMode mSortMode = SortMode::MostPlayed;
    FilterMode mFilterMode = FilterMode::All;

    RetroAchievementInfo mRaInfo;
};
