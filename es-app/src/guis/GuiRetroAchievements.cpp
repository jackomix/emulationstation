#include "guis/GuiRetroAchievements.h"
#include "AchievementCache.h"
#include "guis/GuiLoading.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiGameAchievements.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiTextEditPopup.h"
#include "components/MultiLineMenuEntry.h"
#include "components/ButtonComponent.h"
#include "SystemData.h"
#include "FileData.h"
#include "Paths.h"
#include "views/ViewController.h"
#include "LocaleES.h"
#include "ThemeData.h"
#include "Log.h"
#include "Settings.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "PlayHistoryManager.h"
#include "ProfileManager.h"
#include "components/OptionListComponent.h"
#include "guis/GuiSettings.h"
#include "guis/GuiSettings.h"
#include "components/WebImageComponent.h"
#include "utils/HtmlColor.h"
#include <algorithm>

static time_t parseDateTimeHistory(const std::string& dt);

#define PROGRESSHEIGHT (Renderer::getScreenHeight() * 0.008f)
#define WINDOW_WIDTH (float)Math::min(Renderer::getScreenHeight() * 1.125f, Renderer::getScreenWidth() * 0.90f)
#define BADGESIZE (Renderer::getScreenHeight() * (32.0f / 720.0f))

#define IMAGESIZE (Renderer::getScreenHeight() * (48.0 / 720.0))
#define IMAGESPACER (Renderer::getScreenHeight() * (10.0 / 720.0))

static GuiRetroAchievements::SortMode sLastSortMode = GuiRetroAchievements::SortMode::Recent;
static std::string sLastFilterSystem = "All";
static int sLastFilterMinPlaytime = 0;

class PlayedGameEntry : public ComponentGrid
{
public:
    PlayedGameEntry(Window* window, GuiRetroAchievements::GameEntry* game, GuiRetroAchievements::SortMode sortMode) : 
        ComponentGrid(window, Vector2i(4, 4))
    {
        auto theme = ThemeData::getMenuTheme();
        
        mImage = std::make_shared<WebImageComponent>(mWindow);
        if (game->hasRaGame && !game->raGame.badge.empty()) {
            mImage->setImage(game->raGame.badge);
        } else if (game->fileData && !game->fileData->getImagePath().empty()) {
            mImage->setImage(game->fileData->getImagePath());
        } else {
            mImage->setImage(":/cartridge.svg");
            mImage->setColorShift(theme->Text.color);
        }
        
        setEntry(mImage, Vector2i(0, 0), false, false, Vector2i(1, 4));

        mTitle = std::make_shared<TextComponent>(mWindow, game->hasRaGame ? game->raGame.name : game->name, theme->Text.font, theme->Text.color);
        mTitle->setVerticalAlignment(ALIGN_TOP);
        
        std::string topRightText = "";
        std::string botRightText = "";
        std::string subText = "";
        
        if (game->fileData) {
            subText = game->fileData->getSourceFileData()->getSystem()->getFullName();
        } else if (game->hasRaGame) {
            subText = game->raGame.consoleName;
        }

        if (sortMode == GuiRetroAchievements::SortMode::Recent) {
            if (game->lastPlayed.empty() || game->lastPlayed == "0") topRightText = _("Never");
            else {
                time_t time = parseDateTimeHistory(game->lastPlayed);
                if (time > 0) topRightText = Utils::Time::getElapsedSinceString(time);
                else topRightText = _("Never");
            }
        }
        else if (sortMode == GuiRetroAchievements::SortMode::Playtime) {
            topRightText = Utils::Time::secondsToString(game->gameTimeSeconds, false, true);
        }
        else if (sortMode == GuiRetroAchievements::SortMode::Achievements) {
            int won = game->hasRaGame ? game->raGame.wonAchievementsSoftcore : 0;
            topRightText = std::to_string(won) + " " + (won == 1 ? _("achievement") : _("achievements"));
        }
        else if (sortMode == GuiRetroAchievements::SortMode::Completion) {
            int percent = game->hasRaGame && game->raGame.totalAchievements > 0 ? Math::round((float)game->raGame.wonAchievementsSoftcore * 100.0f / game->raGame.totalAchievements) : 0;
            topRightText = std::to_string(percent) + "% " + _("completed");
        } else {
            topRightText = game->lastPlayed;
        }
        
        mSubtitle = std::make_shared<TextComponent>(mWindow, subText, theme->TextSmall.font, theme->Text.color);
        mSubtitle->setOpacity(192);

        mPoints = std::make_shared<TextComponent>(mWindow, topRightText, theme->Text.font, theme->Text.color, ALIGN_RIGHT);
        
        if (botRightText.empty()) {
            mPoints->setVerticalAlignment(ALIGN_CENTER);
            setEntry(mPoints, Vector2i(3, 1), false, true, Vector2i(1, 2));
        } else {
            mPercentage = std::make_shared<TextComponent>(mWindow, botRightText, theme->TextSmall.font, theme->Text.color, ALIGN_RIGHT);
            mPercentage->setOpacity(192);
            mPoints->setVerticalAlignment(ALIGN_BOTTOM);
            mPercentage->setVerticalAlignment(ALIGN_TOP);
            setEntry(mPoints, Vector2i(3, 1), false, true);
            setEntry(mPercentage, Vector2i(3, 2), false, true);
        }

        if (!game->fileData) {
            mTitle->setOpacity(120);
            mSubtitle->setOpacity(120);
            mPoints->setOpacity(120);
            if (mPercentage) mPercentage->setOpacity(120);
        }

        setEntry(mTitle, Vector2i(2, 1), false, true);
        setEntry(mSubtitle, Vector2i(2, 2), false, true);

        int height = Math::max(IMAGESIZE + IMAGESPACER, mTitle->getSize().y() + mSubtitle->getSize().y());
        
        float hTxt = mTitle->getSize().y() / height;
        float hSub = mSubtitle->getSize().y() / height;
        float topPadding = Math::max(0.0f, (height - mTitle->getSize().y() - mSubtitle->getSize().y()) / height / 2.0f);

        setRowHeightPerc(0, topPadding);
        setRowHeightPerc(1, hTxt);
        setRowHeightPerc(2, hSub);
        setRowHeightPerc(3, Math::max(0.0f, 1.0f - topPadding - hTxt - hSub));

        float textWidth = mPoints->getFont()->sizeText(topRightText).x();
        if (mPercentage) {
            float percWidth = mPercentage->getFont()->sizeText(botRightText).x();
            if (percWidth > textWidth) textWidth = percWidth;
        }
        float pointsColWidth = (textWidth + (Renderer::getScreenHeight() * 0.02f)) / WINDOW_WIDTH;
        setColWidthPerc(0, (height - IMAGESPACER) / WINDOW_WIDTH);
        setColWidthPerc(1, IMAGESPACER / WINDOW_WIDTH);
        setColWidthPerc(3, pointsColWidth);

        mImage->setMaxSize(height - IMAGESPACER, height - IMAGESPACER);

        setSize(0, height);
    }
    
    virtual void setColor(unsigned int color)
    {
        mTitle->setColor(color);
        mSubtitle->setColor(color);
        mPoints->setColor(color);
        if (mPercentage) mPercentage->setColor(color);
    }

private:
    std::shared_ptr<WebImageComponent> mImage;
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mSubtitle;
    std::shared_ptr<TextComponent> mPoints;
    std::shared_ptr<TextComponent> mPercentage;
};

static time_t parseDateTimeHistory(const std::string& dt);

static std::string formatTimeIn2(int seconds)
{
    if (seconds < 0) seconds = 0;
    if (seconds < 3600)
    {
        int m = seconds / 60;
        int s = seconds % 60;
        return std::to_string(m) + "m" + std::to_string(s) + "s in";
    }
    else
    {
        int h = seconds / 3600;
        int m = (seconds % 3600) / 60;
        return std::to_string(h) + "h" + std::to_string(m) + "m in";
    }
}

class SessionAchievementEntry : public ComponentGrid
{
public:
    SessionAchievementEntry(Window* window, Achievement& ra, const std::string& sessionStartTime) :
        ComponentGrid(window, Vector2i(8, 1))
    {
        mGameInfo = ra;
        auto theme = ThemeData::getMenuTheme();

        float badgeSize = Renderer::getScreenHeight() * (24.0f / 720.0f);
        float rowHeight = badgeSize * 1.5f;

        mImage = std::make_shared<WebImageComponent>(mWindow);
        mImage->setMaxSize(badgeSize, badgeSize);
        mImage->setImage(mGameInfo.getBadgeUrl());
        setEntry(mImage, Vector2i(0, 0), false, false);

        auto boldFont = theme->Text.font;
        mTitle = std::make_shared<TextComponent>(mWindow, mGameInfo.Title, boldFont, theme->Text.color);

        std::string timeInStr;
        if (!sessionStartTime.empty() && !mGameInfo.DateEarned.empty())
        {
            time_t sessionT = parseDateTimeHistory(sessionStartTime);
            time_t earnedT  = parseDateTimeHistory(mGameInfo.DateEarned);
            if (sessionT > 0 && earnedT > 0 && earnedT >= sessionT)
                timeInStr = formatTimeIn2((int)(earnedT - sessionT));
        }

        std::string descText = mGameInfo.Description;

        mSeparator = std::make_shared<TextComponent>(mWindow, _U(" \u00b7 "), theme->TextSmall.font, theme->Text.color);
        mSeparator->setOpacity(192);

        mDesc = std::make_shared<TextComponent>(mWindow, descText, theme->TextSmall.font, theme->Text.color);
        mDesc->setOpacity(192);
        mDesc->setAutoScrollDelay(500);
        
        auto tinyFont = Font::get((int)(theme->TextSmall.font->getSize() * 0.85f), theme->TextSmall.font->getPath());
        
        mTimeIn = std::make_shared<TextComponent>(mWindow, timeInStr, tinyFont, theme->Text.color, ALIGN_RIGHT);
        mTimeIn->setOpacity(160);

        mTimeSeparator = std::make_shared<TextComponent>(mWindow, timeInStr.empty() ? "" : _U(" \u00b7 "), tinyFont, theme->Text.color);
        mTimeSeparator->setOpacity(160);

        mPoints = std::make_shared<TextComponent>(mWindow, mGameInfo.Points + _U(" \uf091"), theme->Text.font, theme->Text.color, ALIGN_RIGHT);

        setEntry(mTitle, Vector2i(1, 0), false, true);
        setEntry(mSeparator, Vector2i(2, 0), false, true);
        setEntry(mDesc, Vector2i(3, 0), false, true);
        setEntry(mTimeIn, Vector2i(4, 0), false, true);
        setEntry(mTimeSeparator, Vector2i(5, 0), false, true);
        setEntry(mPoints, Vector2i(6, 0), false, true);

        float badgeW = badgeSize + Renderer::getScreenHeight() * 0.015f;
        float titleW = mTitle->getSize().x();
        float sepW = mSeparator->getSize().x();
        float descPadding = theme->TextSmall.font->sizeText(" ").x();
        float timeInW = mTimeIn->getSize().x() > 0 ? (mTimeIn->getSize().x() + descPadding) : 0;
        float timeSepW = mTimeSeparator->getSize().x() > 0 ? mTimeSeparator->getSize().x() : 0;
        float pointsW = mPoints->getSize().x();
        float rightPadW = Renderer::getScreenHeight() * 0.02f;

        setColWidth(0, badgeW, false);
        setColWidth(1, titleW, false);
        setColWidth(2, sepW, false);
        setColWidth(4, timeInW, false);
        setColWidth(5, timeSepW, false);
        setColWidth(6, pointsW, false);
        setColWidth(7, rightPadW, false);

        setSize(0, rowHeight);
    }

    virtual void setColor(unsigned int color)
    {
        mTitle->setColor(color);
        mSeparator->setColor(color);
        mDesc->setColor(color);
        if (mTimeIn) mTimeIn->setColor(color);
        if (mTimeSeparator) mTimeSeparator->setColor(color);
        if (mPoints) mPoints->setColor(color);
    }

    void onFocusLost() override
    {
        mDesc->setAutoScroll(TextComponent::NONE);
        ComponentGrid::onFocusLost();
    }

    void onFocusGained() override
    {
        mDesc->setAutoScroll(TextComponent::HORIZONTAL);
        mDesc->onShow();
        ComponentGrid::onFocusGained();
    }

private:
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mSeparator;
    std::shared_ptr<TextComponent> mDesc;
    std::shared_ptr<TextComponent> mTimeIn;
    std::shared_ptr<TextComponent> mTimeSeparator;
    std::shared_ptr<TextComponent> mPoints;
    std::shared_ptr<WebImageComponent> mImage;
    Achievement mGameInfo;
};

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

void RetroAchievementProgress::setValues(int valueSoftcore, int valueHardcore, int max, const std::string& label)
{
    mValueSoftCore = valueSoftcore;
    mValueHardCore = valueHardcore;
    mMax = max;
    mText->setText(label);
}

void RetroAchievementProgress::onSizeChanged()
{
	GuiComponent::onSizeChanged();
	
	float textWidth = mText->getFont()->sizeText(mText->getValue()).x();
	
	float y = (mSize.y() - mText->getFont()->getLetterHeight()) / 2.0f;
	mText->setPosition(0, y);
	mText->setSize(textWidth, mText->getFont()->getLetterHeight());
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

	float textWidth = mText->getSize().x();
	float padding = Renderer::getScreenWidth() * 0.01f;
	float barX = textWidth + padding;
	float w = mSize.x() - barX;
	float height = PROGRESSHEIGHT;
	float y = (mSize.y() - height) / 2.0f;

	Renderer::setMatrix(trans);
	Renderer::drawRect(barX, y, w, height, 0x00000032, 0x00000032);

	if (mMax > 0)
	{
		if (mValueSoftCore > 0 && mValueSoftCore > mValueHardCore)
		{
			int cur = (w * mValueSoftCore) / mMax;
			Renderer::drawRect(barX, y, cur, height, 0x0B71C1FF);
		}
		if (mValueHardCore > 0)
		{
			int cur = (w * mValueHardCore) / mMax;
			Renderer::drawRect(barX, y, cur, height, 0xCC9900FF);
		}
	}
	mText->render(trans);
}

GuiRetroAchievements::GuiRetroAchievements(Window* window, RetroAchievementInfo ra) 
    : GuiComponent(window), mBackground(window, ":/frame.png"), mGrid(window, Vector2i(1, 4)), mRaInfo(ra)
{
    mSortMode = sLastSortMode;
    mFilterSystem = sLastFilterSystem;
    mFilterMinPlaytime = sLastFilterMinPlaytime;
    auto theme = ThemeData::getMenuTheme();
    mBackground.setImagePath(theme->Background.path.empty() ? ":/frame.png" : theme->Background.path);
    mBackground.setEdgeColor(theme->Background.color);
    mBackground.setCenterColor(theme->Background.centerColor);
    mBackground.setCornerSize(theme->Background.cornerSize);
    mBackground.setPostProcessShader(theme->Background.menuShader);

    addChild(&mBackground);
    addChild(&mGrid);

    mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(2, 2));

    mTitle = std::make_shared<TextComponent>(mWindow, ra.username, theme->Title.font, theme->Title.color, ALIGN_LEFT);
    mSubtitle = std::make_shared<TextComponent>(mWindow, ra.points + " Points", theme->TextSmall.font, theme->Text.color, ALIGN_LEFT);
    mTitleImage = std::make_shared<WebImageComponent>(mWindow);
    mTitleImage->setImage(ra.userpic);

    mHeaderGrid->setEntry(mTitle, Vector2i(0, 0), false, true);
    mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 1), false, true);
    mHeaderGrid->setEntry(mTitleImage, Vector2i(1, 0), false, false, Vector2i(1, 2));

    mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

    mTabs = std::make_shared<ComponentTab>(mWindow);
    mTabs->addTab(_("GAMES"));
    mTabs->addTab(_("PLAY HISTORY"));
    mTabs->addTab(_("OPTIONS"));

    mTabs->setCursorChangedCallback([this](const CursorState& state) {
        if (mActiveTab != mTabs->getCursorIndex()) {
            mTabCursors[mActiveTab] = mList->getCursorIndex();
            mActiveTab = mTabs->getCursorIndex();
            populateTabContent();
        }
    });

    mGrid.setEntry(mTabs, Vector2i(0, 1), false, true);

    mList = std::make_shared<ComponentList>(mWindow);
    mList->setUpdateType(ComponentListFlags::UPDATE_ALWAYS);
    mGrid.setEntry(mList, Vector2i(0, 2), true, true);

    std::vector<std::shared_ptr<ButtonComponent>> buttons;
    buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("BACK"), _("go back"), [this] { delete this; }));
    mButtonGrid = makeButtonGrid(mWindow, buttons);
    mGrid.setEntry(mButtonGrid, Vector2i(0, 3), true, false);

    mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool {
        if (config->isMappedLike("down", input)) { 
            mGrid.setCursorTo(mList); mList->setCursorIndex(0); return true; 
        }
        if (config->isMappedLike("up", input)) { 
            mList->setCursorIndex(mList->size() - 1); mGrid.moveCursor(Vector2i(0, 1)); return true; 
        }
        return false;
    });

    populateGameList();
    applyFilterAndSort();
    centerWindow();
    populateTabContent();
}

void GuiRetroAchievements::onSizeChanged()
{
	GuiComponent::onSizeChanged();

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	mGrid.setSize(mSize);

	const float titleHeight = mTitle ? mTitle->getFont()->getLetterHeight() * 1.5f : 0;
	const float subtitleHeight = mSubtitle ? mSubtitle->getFont()->getLetterHeight() * 1.5f : 0;

	if (mTabs)
		mTabs->setSize(mGrid.getSize().x(), Renderer::getScreenHeight() * 0.06f);

	if (mHeaderGrid)
	{
		mHeaderGrid->setRowHeight(0, titleHeight);
		mHeaderGrid->setRowHeight(1, subtitleHeight);

		float imageWidth = Renderer::getScreenHeight() * 0.15f;
		float headerWidth = mGrid.getSize().x();
		float textWidth = headerWidth - imageWidth;

		mHeaderGrid->setColWidth(0, textWidth);
		mHeaderGrid->setColWidth(1, imageWidth);

		if (mTitleImage) mTitleImage->setMaxSize(imageWidth, titleHeight + subtitleHeight);
	}

	float headerTotalHeight = titleHeight + subtitleHeight + (Renderer::getScreenHeight() * 0.02f);
	mGrid.setRowHeight(0, headerTotalHeight);
	mGrid.setRowHeight(1, Renderer::getScreenHeight() * 0.06f);
	if (mButtonGrid)
		mGrid.setRowHeight(3, mButtonGrid->getSize().y());
}

void GuiRetroAchievements::centerWindow()
{
	if (Renderer::ScreenSettings::fullScreenMenus())
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		setSize(WINDOW_WIDTH, Renderer::getScreenHeight() * 0.901f);

	setPosition((Renderer::getScreenWidth() - getSize().x()) / 2, (Renderer::getScreenHeight() - getSize().y()) / 2);
}

void GuiRetroAchievements::populateGameList()
{
    mAllGames.clear();

    for (auto sys : SystemData::sSystemVector)
    {
        if (sys->isCollection() || sys->isGroupSystem()) continue;

        for (auto file : sys->getRootFolder()->getFilesRecursive(GAME))
        {
            auto sessions = PlayHistoryManager::getInstance()->getSessions(file);
            int profilePlayCount = sessions.size();
            int profileGameTime = 0;
            std::string profileLastPlayed = "";
            for (const auto& s : sessions) {
                profileGameTime += s.durationSeconds;
                if (profileLastPlayed.empty() || s.startTime > profileLastPlayed) {
                    profileLastPlayed = s.startTime;
                }
            }

            std::string cheevosId = file->getMetadata(MetaDataId::CheevosId);
            
            if (profilePlayCount > 0)
            {
                GameEntry entry;
                entry.fileData = file;
                entry.hasRaGame = false;
                entry.name = file->getName();
                entry.gameTimeSeconds = profileGameTime;
                entry.playCount = profilePlayCount;
                entry.lastPlayed = profileLastPlayed;
                
                if (!cheevosId.empty()) {
                    int gid = Utils::String::toInteger(cheevosId);
                    if (AchievementCache::hasGameData(gid)) {
                        GameInfoAndUserProgress prog = RetroAchievements::getGameInfoAndUserProgress(gid);
                        
                        entry.hasRaGame = true;
                        entry.raGame.id = cheevosId;
                        entry.raGame.name = prog.Title;
                        entry.raGame.consoleName = prog.ConsoleName;
                        entry.raGame.badge = prog.getImageUrl(prog.ImageIcon);
                        entry.raGame.totalAchievements = prog.NumAchievements;
                        
                        bool hasProfileProgress = Utils::FileSystem::exists(Paths::getAchievementProgressPath() + "/" + cheevosId + ".json");
                        if (hasProfileProgress) {
                            entry.raGame.wonAchievementsSoftcore = prog.NumAwardedToUser;
                        } else {
                            entry.raGame.wonAchievementsSoftcore = 0;
                        }
                    }
                }
                
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
                if (!entry.hasRaGame) {
                    entry.raGame = raGame;
                    entry.hasRaGame = true;
                }
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
}

void GuiRetroAchievements::applyFilterAndSort()
{
    mFilteredGames.clear();

    for (auto& game : mAllGames)
    {
        if (game.gameTimeSeconds < mFilterMinPlaytime) continue;

        std::string sys = game.fileData ? game.fileData->getSourceFileData()->getSystem()->getFullName() : (game.hasRaGame ? game.raGame.consoleName : "Unknown");
        if (mFilterSystem != "All" && sys != mFilterSystem) continue;

        mFilteredGames.push_back(&game);
    }

    std::sort(mFilteredGames.begin(), mFilteredGames.end(), [this](GameEntry* a, GameEntry* b) {
        if (mSortMode == SortMode::Recent) {
            return a->lastPlayed > b->lastPlayed;
        } else if (mSortMode == SortMode::Playtime) {
            return a->gameTimeSeconds > b->gameTimeSeconds;
        } else if (mSortMode == SortMode::Achievements) {
            int aEarned = a->hasRaGame ? a->raGame.wonAchievementsSoftcore : 0;
            int bEarned = b->hasRaGame ? b->raGame.wonAchievementsSoftcore : 0;
            return aEarned > bEarned;
        } else if (mSortMode == SortMode::Completion) {
            float aPercent = a->hasRaGame && a->raGame.totalAchievements > 0 ? (float)a->raGame.wonAchievementsSoftcore / a->raGame.totalAchievements : 0.0f;
            float bPercent = b->hasRaGame && b->raGame.totalAchievements > 0 ? (float)b->raGame.wonAchievementsSoftcore / b->raGame.totalAchievements : 0.0f;
            return aPercent > bPercent;
        }
        return a->name < b->name;
    });

    if (mActiveTab == 0) populateGamesTab();
}

void GuiRetroAchievements::openSortFilterMenu()
{
    auto s = new GuiSettings(mWindow, _("SORT & FILTER"));

    auto sortList = std::make_shared<OptionListComponent<SortMode>>(mWindow, _("SORT BY"), false);
    sortList->add(_("RECENTLY PLAYED"), SortMode::Recent, mSortMode == SortMode::Recent);
    sortList->add(_("PLAYTIME"), SortMode::Playtime, mSortMode == SortMode::Playtime);
    sortList->add(_("ACHIEVEMENTS EARNED"), SortMode::Achievements, mSortMode == SortMode::Achievements);
    sortList->add(_("COMPLETION PERCENTAGE"), SortMode::Completion, mSortMode == SortMode::Completion);
    s->addWithLabel(_("SORT BY"), sortList);

    auto sysList = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SYSTEM"), false);
    sysList->add(_("ALL"), "All", mFilterSystem == "All");
    
    std::set<std::string> systems;
    for (auto& game : mAllGames) {
        if (game.fileData) systems.insert(game.fileData->getSourceFileData()->getSystem()->getFullName());
        else if (game.hasRaGame) systems.insert(game.raGame.consoleName);
    }
    
    for (auto& sys : systems) {
        sysList->add(sys, sys, mFilterSystem == sys);
    }
    s->addWithLabel(_("SYSTEM"), sysList);

    auto timeList = std::make_shared<OptionListComponent<int>>(mWindow, _("MINIMUM PLAYTIME"), false);
    timeList->add(_("NONE"), 0, mFilterMinPlaytime == 0);
    timeList->add(_("1 MINUTE"), 60, mFilterMinPlaytime == 60);
    timeList->add(_("5 MINUTES"), 300, mFilterMinPlaytime == 300);
    timeList->add(_("10 MINUTES"), 600, mFilterMinPlaytime == 600);
    timeList->add(_("30 MINUTES"), 1800, mFilterMinPlaytime == 1800);
    timeList->add(_("1 HOUR"), 3600, mFilterMinPlaytime == 3600);
    s->addWithLabel(_("MINIMUM PLAYTIME"), timeList);

    s->addSaveFunc([this, sortList, sysList, timeList] {
        mSortMode = sortList->getSelected();
        mFilterSystem = sysList->getSelected();
        mFilterMinPlaytime = timeList->getSelected();
        sLastSortMode = mSortMode;
        sLastFilterSystem = mFilterSystem;
        sLastFilterMinPlaytime = mFilterMinPlaytime;
        applyFilterAndSort();
    });

    mWindow->pushGui(s);
}

void GuiRetroAchievements::populateTabContent()
{
	mList->clear();

	if (mActiveTab == 0) populateGamesTab();
	else if (mActiveTab == 1) populatePlayHistoryTab();
	else if (mActiveTab == 2) populateOptionsTab();

	if (mTabCursors.find(mActiveTab) != mTabCursors.end() && mList->size() > 0) {
		int cursorIndex = mTabCursors[mActiveTab];
		if (cursorIndex >= mList->size()) cursorIndex = mList->size() - 1;
		if (cursorIndex < 0) cursorIndex = 0;
		mList->setCursorIndex(cursorIndex);
	}
}

void GuiRetroAchievements::populateGamesTab()
{
    mList->clear();
    auto theme = ThemeData::getMenuTheme();

    ComponentListRow filterRow;
    auto filterText = std::make_shared<TextComponent>(mWindow, _("Sort & Filter..."), theme->Text.font, theme->Text.color);
    filterRow.addElement(filterText, true);
    filterRow.makeAcceptInputHandler([this] { openSortFilterMenu(); });
    mList->addRow(filterRow);

    for (auto* game : mFilteredGames)
    {
        ComponentListRow row;
        auto entry = std::make_shared<PlayedGameEntry>(mWindow, game, mSortMode);
        
        row.addElement(entry, true);

        row.makeAcceptInputHandler([this, game] {
            if (game->fileData) {
                GuiGameAchievements::show(mWindow, game->fileData);
            } else if (game->hasRaGame) {
                GuiGameAchievements::show(mWindow, Utils::String::toInteger(game->raGame.id));
            }
        });
        
        mList->addRow(row, true, true);
    }
    mList->setCursorIndex(0);
}

static time_t parseDateTimeHistory(const std::string& dt)
{
	if (dt.empty()) return 0;
	if (dt.find("T") != std::string::npos && dt.find("-") == std::string::npos)
		return Utils::Time::stringToTime(dt);
	struct tm t = {};
	if (sscanf(dt.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) == 6)
	{
		t.tm_year -= 1900;
		t.tm_mon -= 1;
		return timegm(&t);
	}
	if (sscanf(dt.c_str(), "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) == 6)
	{
		t.tm_year -= 1900;
		t.tm_mon -= 1;
		return timegm(&t);
	}
	return 0;
}

void GuiRetroAchievements::populatePlayHistoryTab()
{
    auto theme = ThemeData::getMenuTheme();
    
    // Fetch summary for achievements
    auto summary = RetroAchievements::getUserSummary(mRaInfo.username);

    std::vector<GameEntry*> gamesWithSessions;
    for (auto& game : mAllGames) {
        if (game.fileData && !PlayHistoryManager::getInstance()->getSessions(game.fileData).empty()) {
            gamesWithSessions.push_back(&game);
        }
    }

    std::sort(gamesWithSessions.begin(), gamesWithSessions.end(), [](GameEntry* a, GameEntry* b) {
        auto aSess = PlayHistoryManager::getInstance()->getSessions(a->fileData);
        auto bSess = PlayHistoryManager::getInstance()->getSessions(b->fileData);
        
        std::string latestA = "";
        std::string latestB = "";
        
        if (!aSess.empty()) latestA = aSess.front().startTime;
        if (!bSess.empty()) latestB = bSess.front().startTime;
        
        return parseDateTimeHistory(latestA) > parseDateTimeHistory(latestB);
    });

    long totalPlayTime = 0;
    int totalLaunches = 0;
    std::string latestGlobal = "";

    for (auto& game : gamesWithSessions) {
        auto sessions = PlayHistoryManager::getInstance()->getSessions(game->fileData);
        totalLaunches += sessions.size();
        for (const auto& s : sessions) {
            totalPlayTime += s.durationSeconds;
            if (latestGlobal.empty() || parseDateTimeHistory(s.startTime) > parseDateTimeHistory(latestGlobal)) {
                latestGlobal = s.startTime;
            }
        }
    }

    ComponentListRow rowTime;
    auto lblTime = std::make_shared<TextComponent>(mWindow, _("TOTAL PLAY TIME"), theme->Text.font, theme->Text.color);
    auto valTime = std::make_shared<TextComponent>(mWindow, Utils::Time::secondsToString(totalPlayTime, false, true), theme->Text.font, theme->Text.color);
    valTime->setHorizontalAlignment(ALIGN_RIGHT);
    rowTime.addElement(lblTime, true);
    rowTime.addElement(valTime, false);
    mList->addRow(rowTime);
    
    ComponentListRow rowCount;
    auto lblCount = std::make_shared<TextComponent>(mWindow, _("TOTAL LAUNCHES"), theme->Text.font, theme->Text.color);
    auto valCount = std::make_shared<TextComponent>(mWindow, std::to_string(totalLaunches), theme->Text.font, theme->Text.color);
    valCount->setHorizontalAlignment(ALIGN_RIGHT);
    rowCount.addElement(lblCount, true);
    rowCount.addElement(valCount, false);
    mList->addRow(rowCount);
    
    ComponentListRow rowLast;
    auto lblLast = std::make_shared<TextComponent>(mWindow, _("LAST PLAYED"), theme->Text.font, theme->Text.color);
    
    std::string lastPlayedFormatted = _("never");
    if (!latestGlobal.empty()) {
        std::string isoDate = latestGlobal;
        isoDate = Utils::String::replace(isoDate, "-", "");
        isoDate = Utils::String::replace(isoDate, ":", "");
        isoDate = Utils::String::replace(isoDate, "Z", "");
        lastPlayedFormatted = Utils::Time::DateTime(isoDate).toFullString();
    }
    
    auto valLast = std::make_shared<TextComponent>(mWindow, lastPlayedFormatted, theme->Text.font, theme->Text.color);
    valLast->setHorizontalAlignment(ALIGN_RIGHT);
    rowLast.addElement(lblLast, true);
    rowLast.addElement(valLast, false);
    mList->addRow(rowLast);

    for (auto game : gamesWithSessions) {
        ComponentListRow gameRow;
        float badgeSize = Renderer::getScreenHeight() * (24.0f / 720.0f);
        
        auto icon = std::make_shared<WebImageComponent>(mWindow);
        icon->setMaxSize(badgeSize, badgeSize);
        if (game->hasRaGame && !game->raGame.badge.empty()) {
            icon->setImage(game->raGame.badge);
        } else if (game->fileData && !game->fileData->getImagePath().empty()) {
            icon->setImage(game->fileData->getImagePath());
        } else {
            icon->setImage(":/cartridge.svg");
            icon->setColorShift(theme->Text.color);
        }
        gameRow.addElement(icon, false);

        auto spacer = std::make_shared<GuiComponent>(mWindow);
        spacer->setSize(Renderer::getScreenHeight() * 0.015f, 0);
        gameRow.addElement(spacer, false);

        auto gameTitle = std::make_shared<TextComponent>(mWindow, game->hasRaGame ? game->raGame.name : game->name, theme->Text.font, theme->Text.color);
        gameRow.addElement(gameTitle, true);
        
        auto launchGameAchievements = [this, game](const std::string& achId) {
            if (game->fileData) {
                GuiGameAchievements::show(mWindow, game->fileData, achId);
            } else if (game->hasRaGame) {
                GuiGameAchievements::show(mWindow, Utils::String::toInteger(game->raGame.id));
            }
        };

        gameRow.makeAcceptInputHandler([launchGameAchievements] { launchGameAchievements(""); });
        
        mList->addRow(gameRow);

        auto sessions = PlayHistoryManager::getInstance()->getSessions(game->fileData);
        std::sort(sessions.begin(), sessions.end(), [](const PlaySession& a, const PlaySession& b) {
            return parseDateTimeHistory(a.startTime) > parseDateTimeHistory(b.startTime);
        });

        std::map<std::string, std::vector<Achievement>> sessionAchievements;

        if (game->hasRaGame) {
            int gid = Utils::String::toInteger(game->raGame.id);
            if (AchievementCache::hasGameData(gid)) {
                GameInfoAndUserProgress prog = RetroAchievements::getGameInfoAndUserProgress(gid);
                for (auto& s : sessions) {
                    for (auto& id : s.achievementIds) {
                        for (auto& ra : prog.Achievements) {
                            if (ra.ID == id) {
                                sessionAchievements[s.id].push_back(ra);
                                break;
                            }
                        }
                    }
                    std::sort(sessionAchievements[s.id].begin(), sessionAchievements[s.id].end(), [](const Achievement& a, const Achievement& b) {
                        return parseDateTimeHistory(a.DateEarned) > parseDateTimeHistory(b.DateEarned);
                    });
                }
            }
        }

        for (auto& s : sessions) {
            std::string isoDate = s.startTime;
            isoDate = Utils::String::replace(isoDate, "-", "");
            isoDate = Utils::String::replace(isoDate, ":", "");
            isoDate = Utils::String::replace(isoDate, "Z", "");
            std::string formattedDate = Utils::Time::DateTime(isoDate).toFullString();
            
            std::string durationStr = Utils::Time::secondsToString(s.durationSeconds, false, false);
            
            ComponentListRow sessionRow;
            auto lblSession = std::make_shared<TextComponent>(mWindow, formattedDate, theme->TextSmall.font, theme->Text.color);
            lblSession->setOpacity(160);
                
            auto valSession = std::make_shared<TextComponent>(mWindow, durationStr, theme->TextSmall.font, theme->Text.color);
            valSession->setHorizontalAlignment(ALIGN_RIGHT);
            valSession->setOpacity(160);

            auto sessionSpacer = std::make_shared<GuiComponent>(mWindow);
            sessionSpacer->setSize(Renderer::getScreenHeight() * 0.02f, 0);
            sessionRow.addElement(sessionSpacer, false);

            sessionRow.addElement(lblSession, true);
            sessionRow.addElement(valSession, false);
            sessionRow.makeAcceptInputHandler([launchGameAchievements] { launchGameAchievements(""); });
            mList->addRow(sessionRow);

            for (auto ach : sessionAchievements[s.id]) {
                ComponentListRow achRow;
                achRow.no_separator = true;
                auto entry = std::make_shared<SessionAchievementEntry>(mWindow, ach, s.startTime);
                
                auto spacer = std::make_shared<GuiComponent>(mWindow);
                spacer->setSize(Renderer::getScreenHeight() * 0.04f, 0);
                achRow.addElement(spacer, false);
                achRow.addElement(entry, true);
                
                std::string achId = ach.ID;
                achRow.makeAcceptInputHandler([launchGameAchievements, achId] { launchGameAchievements(achId); });
                
                mList->addRow(achRow);
            }
        }
    }
}

void GuiRetroAchievements::populateOptionsTab()
{
    auto theme = ThemeData::getMenuTheme();

    ComponentListRow renameRow;
    auto renameText = std::make_shared<TextComponent>(mWindow, _("RENAME PROFILE"), theme->Text.font, theme->Text.color);
    renameRow.addElement(renameText, true);
    renameRow.makeAcceptInputHandler([this] {
        std::string active = ProfileManager::getInstance()->getActiveProfileName();
        auto updateVal = [this, active](const std::string& newVal) {
            if (newVal != active && !newVal.empty()) {
                if (ProfileManager::getInstance()->renameProfile(active, newVal)) {
                    ProfileManager::getInstance()->setActiveProfile(newVal);
                } else {
                    mWindow->pushGui(new GuiMsgBox(mWindow, _("FAILED TO RENAME PROFILE"), _("OK")));
                }
            }
        };
        mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("Enter Profile Name"), active, updateVal, false));
    });
    mList->addRow(renameRow);

    ComponentListRow pictureRow;
    auto pictureText = std::make_shared<TextComponent>(mWindow, _("CHANGE PROFILE PICTURE"), theme->Text.font, theme->Text.color);
    pictureRow.addElement(pictureText, true);
    pictureRow.makeAcceptInputHandler([this] {
        mWindow->pushGui(new GuiMsgBox(mWindow, _("NOT IMPLEMENTED YET"), _("OK")));
    });
    mList->addRow(pictureRow);

    ComponentListRow deleteRow;
    auto deleteText = std::make_shared<TextComponent>(mWindow, _("DELETE PROFILE"), theme->Text.font, theme->Text.color);
    deleteRow.addElement(deleteText, true);
    deleteRow.makeAcceptInputHandler([this] {
        std::string active = ProfileManager::getInstance()->getActiveProfileName();
        mWindow->pushGui(new GuiMsgBox(mWindow, _("DELETE PROFILE '") + active + _("'?"), _("YES"), [this, active]() {
            if (ProfileManager::getInstance()->deleteProfile(active)) {
                auto profiles = ProfileManager::getInstance()->getProfiles();
                if (!profiles.empty()) {
                    ProfileManager::getInstance()->setActiveProfile(profiles.front().name);
                }
                mWindow->postToUiThread([this]() { delete this; });
            } else {
                mWindow->pushGui(new GuiMsgBox(mWindow, _("FAILED TO DELETE PROFILE"), _("OK")));
            }
        }, _("NO"), nullptr));
    });
    mList->addRow(deleteRow);
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
        if (config->isMappedTo("pageup", input) || config->isMappedTo("l1", input))
        {
            mTabs->setCursorIndex(mTabs->getCursorIndex() - 1);
            return true;
        }
        if (config->isMappedTo("pagedown", input) || config->isMappedTo("r1", input))
        {
            mTabs->setCursorIndex(mTabs->getCursorIndex() + 1);
            return true;
        }
    }
    
    if (mGrid.isCursorTo(mList))
    {
        if (mTabs->input(config, input))
            return true;
    }

    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiRetroAchievements::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
    prompts.push_back(HelpPrompt(BUTTON_OK, _("SELECT")));
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
