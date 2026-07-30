#pragma once

#include "GuiComponent.h"
#include "RetroAchievements.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentGrid.h"
#include "components/ComponentList.h"
#include "components/ImageComponent.h"
#include "components/WebImageComponent.h"
#include "components/TextComponent.h"
#include "components/ComponentTab.h"
#include <memory>
#include <vector>
#include <map>

class FileData;
class Window;

class RetroAchievementProgress : public GuiComponent
{
public:
	RetroAchievementProgress(Window* window, int valueSoftcore, int valueHardcore, int max, const std::string& label);

	void onSizeChanged() override;
	void render(const Transform4x4f& parentTrans) override;
	void setColor(unsigned int color) override;
	void setValues(int valueSoftcore, int valueHardcore, int max, const std::string& label);

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

    struct GameEntry {
        FileData* fileData;
        RetroAchievementGame raGame;
        bool hasRaGame;
        std::string name;
        long gameTimeSeconds;
        int playCount;
        std::string lastPlayed;
    };

    enum class SortMode { Recent, Playtime, Achievements, Completion, System };

private:
    GuiRetroAchievements(Window* window, RetroAchievementInfo ra);
    void onSizeChanged() override;
    void centerWindow();
    
    void populateGameList();
    void populateTabContent();
    void populateGamesTab();
    void populatePlayHistoryTab();
    void populateOptionsTab();
    
    void applyFilterAndSort();
    void openSortFilterMenu();

private:
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<ComponentGrid> mHeaderGrid;
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mSubtitle;
    std::shared_ptr<WebImageComponent> mTitleImage;

    std::shared_ptr<ComponentTab> mTabs;
    std::shared_ptr<ComponentList> mList;
    std::shared_ptr<ComponentGrid> mButtonGrid;

    int mActiveTab = 0;
    std::map<int, int> mTabCursors;

    std::vector<GameEntry> mAllGames;
    std::vector<GameEntry*> mFilteredGames;
    
    SortMode mSortMode = SortMode::Recent;
    std::string mFilterSystem = "All";
    int mFilterMinPlaytime = 0;

    RetroAchievementInfo mRaInfo;
};
