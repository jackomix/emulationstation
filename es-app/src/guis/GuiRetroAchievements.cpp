#include "guis/GuiRetroAchievements.h"
#include "guis/GuiLoading.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiGameAchievements.h"
#include "components/MultiLineMenuEntry.h"
#include "SystemData.h"
#include "FileData.h"
#include "views/ViewController.h"
#include "LocaleES.h"
#include "ThemeData.h"
#include "Log.h"
#include "Settings.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include <algorithm>

#define PROGRESSHEIGHT (Renderer::getScreenHeight() * 0.008f)

void GuiRetroAchievements::show(Window* window)
{
	window->pushGui(new GuiLoading<RetroAchievementInfo>(window, _("PLEASE WAIT"), 
		[window](auto gui)
		{
			auto summary = RetroAchievements::getUserSummary();
			return RetroAchievements::toRetroAchivementInfo(summary);
		}, 
		[window](RetroAchievementInfo ra)
		{
			if (ra.username.empty() && !ra.error.empty())
				window->pushGui(new GuiMsgBox(window, _("AN ERROR OCCURRED") + "\r\n" + ra.error, _("OK")));
			else if (ra.username.empty())
				window->pushGui(new GuiMsgBox(window, _("AN ERROR OCCURRED"), _("OK")));
			else
				window->pushGui(new GuiRetroAchievements(window, ra));
		}));
}

RetroAchievementProgress::RetroAchievementProgress(Window* window, int valueSoftcore, int valueHardcore, int max, const std::string& label) : GuiComponent(window), 
	mValueSoftCore(valueSoftcore), mValueHardCore(valueHardcore), mMax(max)
{ 
	auto theme = ThemeData::getMenuTheme();

	mText = std::make_shared<TextComponent>(mWindow, label, theme->TextSmall.font, theme->Text.color);		
	mText->setVerticalAlignment(Alignment::ALIGN_CENTER);
	mText->setHorizontalAlignment(Alignment::ALIGN_CENTER);
}

void RetroAchievementProgress::onSizeChanged()
{
	GuiComponent::onSizeChanged();
	float padding = mSize.x() * 0.1f;
	float y = (mSize.y() + PROGRESSHEIGHT) / 2.0f;
	mText->setPosition(padding, y);
	mText->setSize(mSize.x() - 2.0f * padding, mText->getFont()->getLetterHeight());
}

void RetroAchievementProgress::setColor(unsigned int color)
{
	mText->setColor(color);
}

void RetroAchievementProgress::render(const Transform4x4f& parentTrans)
{
	if (!isVisible()) return;
	Transform4x4f trans = parentTrans * getTransform();
	auto rect = Renderer::getScreenRect(trans, mSize);
	if (!Renderer::isVisibleOnScreen(rect)) return;
		
	int padding = mSize.x() * 0.1f;
	int w = mSize.x() - 2.0 * padding;
	float height = PROGRESSHEIGHT;
	float y = mSize.y() / 2.0f - 1.5f * height;

	Renderer::setMatrix(trans);
	Renderer::drawRect(padding, y, w, height, 0x00000032, 0x00000032);

	if (mMax > 0)
	{
		if (mValueSoftCore > 0 && mValueSoftCore > mValueHardCore)
		{
			int cur = (w * mValueSoftCore) / mMax;
			Renderer::drawRect(padding, y, cur, height, 0x0B71C1FF);
		}
		if (mValueHardCore > 0)
		{
			int cur = (w * mValueHardCore) / mMax;
			Renderer::drawRect(padding, y, cur, height, 0xCC9900FF);
		}
	}
	mText->render(trans);
}

GuiRetroAchievements::GuiRetroAchievements(Window* window, RetroAchievementInfo ra) 
    : GuiComponent(window), mBackground(window, ":/frame.png"), mGrid(window, Vector2i(2, 1)), mRaInfo(ra)
{
    auto theme = ThemeData::getMenuTheme();
    mBackground.setImagePath(theme->Background.path);
    mBackground.setEdgeColor(theme->Background.color);
    mBackground.setCenterColor(theme->Background.color);

    mList = std::make_shared<ComponentList>(mWindow);
    mList->setUpdateType(ComponentListFlags::UPDATE_ALWAYS);
    mList->setCursorChangedCallback([this](const CursorState& state) { updateDetailPanel(); });

    mBoxArt = std::make_shared<ImageComponent>(mWindow);
    mBadge = std::make_shared<WebImageComponent>(mWindow);
    mGameTitle = std::make_shared<TextComponent>(mWindow, "", theme->Text.font, theme->Text.color, Alignment::ALIGN_CENTER);
    mPlayTime = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->Text.color, Alignment::ALIGN_CENTER);
    mPlayCount = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->Text.color, Alignment::ALIGN_CENTER);
    mLastPlayed = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->Text.color, Alignment::ALIGN_CENTER);
    mSortFilterLabel = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->Text.color, Alignment::ALIGN_CENTER);
    mProgress = std::make_shared<RetroAchievementProgress>(mWindow, 0, 0, 0, ""); // Dummy

    mRightPanel = std::make_shared<ComponentGrid>(mWindow, Vector2i(1, 8));
    mRightPanel->setEntry(mBoxArt, Vector2i(0, 0), false, true);
    mRightPanel->setEntry(mBadge, Vector2i(0, 1), false, true);
    mRightPanel->setEntry(mGameTitle, Vector2i(0, 2), false, true);
    mRightPanel->setEntry(mPlayTime, Vector2i(0, 3), false, true);
    mRightPanel->setEntry(mPlayCount, Vector2i(0, 4), false, true);
    mRightPanel->setEntry(mLastPlayed, Vector2i(0, 5), false, true);
    mRightPanel->setEntry(mProgress, Vector2i(0, 6), false, true);
    mRightPanel->setEntry(mSortFilterLabel, Vector2i(0, 7), false, true);

    mGrid.setEntry(mList, Vector2i(0, 0), true, true);
    mGrid.setEntry(mRightPanel, Vector2i(1, 0), false, true);

    addChild(&mBackground);
    addChild(&mGrid);

    populateGameList();
    centerWindow();
}

void GuiRetroAchievements::populateGameList()
{
    mAllGames.clear();

    for (auto sys : SystemData::sSystemVector)
    {
        if (!sys->isCheevosSupported()) continue;
        for (auto file : sys->getRootFolder()->getFilesRecursive(GAME))
        {
            int playCount = Utils::String::toInteger(file->getMetadata(MetaDataId::PlayCount));
            std::string cheevosId = file->getMetadata(MetaDataId::CheevosId);
            
            if (playCount > 0 || !cheevosId.empty())
            {
                GameEntry entry;
                entry.fileData = file;
                entry.hasRaGame = false;
                entry.name = file->getName();
                entry.gameTimeSeconds = Utils::String::toInteger(file->getMetadata(MetaDataId::GameTime));
                entry.playCount = playCount;
                entry.lastPlayed = file->getMetadata(MetaDataId::LastPlayed);
                mAllGames.push_back(entry);
            }
        }
    }

    for (const auto& raGame : mRaInfo.games)
    {
        bool found = false;
        for (auto& entry : mAllGames)
        {
            if (entry.fileData && entry.fileData->getMetadata(MetaDataId::CheevosId) == raGame.id)
            {
                entry.raGame = raGame;
                entry.hasRaGame = true;
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            GameEntry entry;
            entry.fileData = nullptr;
            entry.raGame = raGame;
            entry.hasRaGame = true;
            entry.name = raGame.name;
            entry.gameTimeSeconds = 0;
            entry.playCount = 0;
            entry.lastPlayed = raGame.lastplayed;
            mAllGames.push_back(entry);
        }
    }

    cycleFilter(); 
}

void GuiRetroAchievements::cycleFilter()
{
    if (mFilterMode == FilterMode::All) mFilterMode = FilterMode::WithAchievements;
    else if (mFilterMode == FilterMode::WithAchievements) mFilterMode = FilterMode::Completed;
    else mFilterMode = FilterMode::All;
    
    cycleSort();
}

void GuiRetroAchievements::cycleSort()
{
    mFilteredGames.clear();
    for (auto& game : mAllGames)
    {
        if (mFilterMode == FilterMode::WithAchievements && !game.hasRaGame) continue;
        if (mFilterMode == FilterMode::Completed)
        {
            if (!game.hasRaGame || game.raGame.wonAchievementsSoftcore < game.raGame.totalAchievements || game.raGame.totalAchievements == 0) continue;
        }
        mFilteredGames.push_back(&game);
    }
    
    std::sort(mFilteredGames.begin(), mFilteredGames.end(), [this](GameEntry* a, GameEntry* b) {
        if (mSortMode == SortMode::MostPlayed) return a->gameTimeSeconds > b->gameTimeSeconds;
        if (mSortMode == SortMode::LastPlayed) return a->lastPlayed > b->lastPlayed;
        return Utils::String::toUpper(a->name) < Utils::String::toUpper(b->name);
    });

    mList->clear();
    auto theme = ThemeData::getMenuTheme();

    for (auto* game : mFilteredGames)
    {
        ComponentListRow row;
        auto text = std::make_shared<TextComponent>(mWindow, game->name, theme->Text.font, theme->Text.color);
        if (!game->fileData) text->setOpacity(120);
        
        row.addElement(text, true);
        
        auto spacer = std::make_shared<GuiComponent>(mWindow);
        spacer->setSize(Renderer::getScreenWidth() * 0.015f, 0);
        row.addElement(spacer, false);
        
        mList->addRow(row, false, true);
    }

    updateDetailPanel();
}

void GuiRetroAchievements::updateDetailPanel()
{
    if (mFilteredGames.empty()) return;
    int idx = mList->getCursorId();
    if (idx < 0 || idx >= mFilteredGames.size()) return;
    
    auto* game = mFilteredGames[idx];
    
    if (game->fileData) mBoxArt->setImage(game->fileData->getImagePath());
    else mBoxArt->setImage("");
    
    if (!game->fileData && game->hasRaGame && !game->raGame.badge.empty())
        mBadge->setImage(game->raGame.badge);
    else
        mBadge->setImage("");
    
    mGameTitle->setText(game->name);
    mPlayTime->setText(_("Play Time") + ": " + Utils::Time::secondsToString(game->gameTimeSeconds));
    mPlayCount->setText(_("Play Count") + ": " + std::to_string(game->playCount));
    mLastPlayed->setText(_("Last Played") + ": " + game->lastPlayed);
    
    std::string filterStr = (mFilterMode == FilterMode::All) ? _("All") : (mFilterMode == FilterMode::WithAchievements ? _("With Achievements") : _("Completed"));
    std::string sortStr = (mSortMode == SortMode::MostPlayed) ? _("Most Played") : (mSortMode == SortMode::LastPlayed ? _("Last Played") : _("Title"));
    mSortFilterLabel->setText(_("Filter") + ": " + filterStr + " | " + _("Sort") + ": " + sortStr);
    
    mRightPanel->removeEntry(mProgress);
    if (game->hasRaGame)
    {
        int percent = game->raGame.totalAchievements == 0 ? 0 : Math::round(game->raGame.wonAchievementsSoftcore * 100.0f / game->raGame.totalAchievements);
        std::string progStr = std::to_string(percent) + "% (" + std::to_string(game->raGame.wonAchievementsSoftcore) + " of " + std::to_string(game->raGame.totalAchievements) + ")";
        mProgress = std::make_shared<RetroAchievementProgress>(mWindow, game->raGame.wonAchievementsSoftcore, game->raGame.wonAchievementsHardcore, game->raGame.totalAchievements, progStr);
        mRightPanel->setEntry(mProgress, Vector2i(0, 6), false, true);
    }
}

void GuiRetroAchievements::centerWindow()
{
    float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));
    
    if (Renderer::ScreenSettings::fullScreenMenus())
        setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    else
        setSize(width, Renderer::getScreenHeight() * 0.901f);
        
    mBackground.setSize(mSize);
    mGrid.setSize(mSize);
    mGrid.setColWidthPerc(0, 0.45f);
    mGrid.setColWidthPerc(1, 0.55f);
    
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, (Renderer::getScreenHeight() - mSize.y()) / 2);
}

void GuiRetroAchievements::update(int deltaTime)
{
    GuiComponent::update(deltaTime);
}

void GuiRetroAchievements::render(const Transform4x4f& parentTrans)
{
    Transform4x4f trans = parentTrans * getTransform();
    Renderer::setMatrix(trans);
    mBackground.render(trans);
    mGrid.render(trans);
}

bool GuiRetroAchievements::input(InputConfig* config, Input input)
{
    if (input.value != 0)
    {
        if (config->isMappedTo(BUTTON_BACK, input))
        {
            mWindow->postToUiThread([this]() { delete this; });
            return true;
        }
        else if (config->isMappedTo(BUTTON_OK, input))
        {
            if (!mFilteredGames.empty() && mList->getCursorId() >= 0 && mList->getCursorId() < mFilteredGames.size())
            {
                auto game = mFilteredGames[mList->getCursorId()];
                if (game->hasRaGame)
                {
                    GuiGameAchievements::show(mWindow, Utils::String::toInteger(game->raGame.id));
                }
            }
            return true;
        }
        else if (config->isMappedTo("x", input))
        {
            if (!mFilteredGames.empty() && mList->getCursorId() >= 0 && mList->getCursorId() < mFilteredGames.size())
            {
                auto game = mFilteredGames[mList->getCursorId()];
                if (game->fileData)
                {
                    Window* window = mWindow;
                    while (window->peekGui() && window->peekGui() != ViewController::get())
                        delete window->peekGui();

                    ViewController::get()->launch(game->fileData);
                }
            }
            return true;
        }
        else if (config->isMappedTo("pageup", input) || config->isMappedTo("pagedown", input))
        {
            cycleFilter();
            return true;
        }
        else if (config->isMappedTo("l2", input) || config->isMappedTo("r2", input))
        {
            if (mSortMode == SortMode::MostPlayed) mSortMode = SortMode::LastPlayed;
            else if (mSortMode == SortMode::LastPlayed) mSortMode = SortMode::Title;
            else mSortMode = SortMode::MostPlayed;
            
            cycleSort();
            return true;
        }
    }
    
    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiRetroAchievements::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
    prompts.push_back(HelpPrompt(BUTTON_OK, _("VIEW DETAILS")));
    
    if (!mFilteredGames.empty() && mList->getCursorId() >= 0 && mList->getCursorId() < mFilteredGames.size())
    {
        auto game = mFilteredGames[mList->getCursorId()];
        if (game->fileData)
            prompts.push_back(HelpPrompt("x", _("LAUNCH")));
    }
    
    std::string filterStr = (mFilterMode == FilterMode::All) ? _("All") : (mFilterMode == FilterMode::WithAchievements ? _("With Achievements") : _("Completed"));
    prompts.push_back(HelpPrompt("pageup", filterStr));
    
    std::string sortStr = (mSortMode == SortMode::MostPlayed) ? _("Most Played") : (mSortMode == SortMode::LastPlayed ? _("Last Played") : _("Title"));
    prompts.push_back(HelpPrompt("l2", sortStr));

    return prompts;
}

FileData* GuiRetroAchievements::getFileData(const std::string& cheevosGameId)
{
    for (auto sys : SystemData::sSystemVector)
    {
        if (!sys->isCheevosSupported()) continue;
        for (auto file : sys->getRootFolder()->getFilesRecursive(GAME))
            if (file->getMetadata(MetaDataId::CheevosId) == cheevosGameId)
                return file;
    }
    return nullptr;
}
