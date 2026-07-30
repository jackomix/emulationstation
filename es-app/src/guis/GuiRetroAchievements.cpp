#include "guis/GuiRetroAchievements.h"
#include "guis/GuiLoading.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiGameAchievements.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiTextEditPopup.h"
#include "components/MultiLineMenuEntry.h"
#include "components/ButtonComponent.h"
#include "SystemData.h"
#include "FileData.h"
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

#define PROGRESSHEIGHT (Renderer::getScreenHeight() * 0.008f)
#define WINDOW_WIDTH (float)Math::min(Renderer::getScreenHeight() * 1.125f, Renderer::getScreenWidth() * 0.90f)
#define BADGESIZE (Renderer::getScreenHeight() * (32.0f / 720.0f))

class PlayedGameEntry : public ComponentGrid
{
public:
    PlayedGameEntry(Window* window, GuiRetroAchievements::GameEntry* game, GuiRetroAchievements::SortMode sortMode) : 
        ComponentGrid(window, Vector2i(4, 2))
    {
        auto theme = ThemeData::getMenuTheme();
        
        mImage = std::make_shared<WebImageComponent>(mWindow);
        mImage->setMaxSize(BADGESIZE, BADGESIZE);
        
        if (game->hasRaGame && !game->raGame.badge.empty()) {
            std::string url = "http://media.retroachievements.org/Badge/" + game->raGame.badge + ".png";
            mImage->setImage(url);
        } else if (game->fileData && !game->fileData->getImagePath().empty()) {
            mImage->setImage(game->fileData->getImagePath());
        } else {
            mImage->setImage(":/cartridge.svg");
            mImage->setColorShift(theme->Text.color);
        }
        
        setEntry(mImage, Vector2i(0, 0), false, false, Vector2i(1, 2));

        mTitle = std::make_shared<TextComponent>(mWindow, game->name, theme->Text.font, theme->Text.color);
        
        std::string rightText = "";
        std::string subText = "";
        
        if (game->fileData) {
            subText = game->fileData->getSourceFileData()->getSystem()->getFullName();
        } else if (game->hasRaGame) {
            subText = game->raGame.consoleName;
        }

        if (sortMode == GuiRetroAchievements::SortMode::Recent) rightText = game->lastPlayed;
        else if (sortMode == GuiRetroAchievements::SortMode::Playtime) rightText = Utils::Time::secondsToString(game->gameTimeSeconds);
        else if (sortMode == GuiRetroAchievements::SortMode::Achievements) rightText = game->hasRaGame ? std::to_string(game->raGame.wonAchievementsSoftcore) + " earned" : "0 earned";
        else if (sortMode == GuiRetroAchievements::SortMode::Completion) {
            int percent = game->hasRaGame && game->raGame.totalAchievements > 0 ? Math::round((float)game->raGame.wonAchievementsSoftcore * 100.0f / game->raGame.totalAchievements) : 0;
            rightText = std::to_string(percent) + "%";
        }
        
        mSubtitle = std::make_shared<TextComponent>(mWindow, subText, theme->TextSmall.font, theme->Text.color);
        mSubtitle->setOpacity(160);

        mRightStat = std::make_shared<TextComponent>(mWindow, rightText, theme->TextSmall.font, theme->Text.color);
        mRightStat->setHorizontalAlignment(ALIGN_RIGHT);
        
        if (!game->fileData) {
            mTitle->setOpacity(120);
            mRightStat->setOpacity(120);
        }

        setEntry(mTitle, Vector2i(2, 0), false, true);
        setEntry(mSubtitle, Vector2i(2, 1), false, true);
        setEntry(mRightStat, Vector2i(3, 0), false, true, Vector2i(1, 2));

        float height = Math::max(BADGESIZE + 4.0f, mTitle->getSize().y() + mSubtitle->getSize().y());
        
        float hTxt = mTitle->getSize().y() / height;
        float hSub = mSubtitle->getSize().y() / height;
        float topPadding = Math::max(0.0f, (height - mTitle->getSize().y() - mSubtitle->getSize().y()) / height / 2.0f);

        setRowHeightPerc(0, topPadding + hTxt);
        setRowHeightPerc(1, hSub + Math::max(0.0f, 1.0f - topPadding - hTxt - hSub));

        float badgeW = BADGESIZE + Renderer::getScreenHeight() * 0.015f;
        setColWidth(0, badgeW, false);
        setColWidth(1, 0, false); // No spacer needed if we just use padding
        setColWidth(3, mRightStat->getSize().x() > 0 ? mRightStat->getSize().x() : 100, false);

        setSize(0, height);
    }
    
    virtual void setColor(unsigned int color)
    {
        mTitle->setColor(color);
        mSubtitle->setColor(Utils::HtmlColor::applyColorOpacity(color, 160));
        mRightStat->setColor(color);
    }

private:
    std::shared_ptr<WebImageComponent> mImage;
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mSubtitle;
    std::shared_ptr<TextComponent> mRightStat;
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

    mGrid.setEntry(mTabs, Vector2i(1, 0), false, true);
    mGrid.setEntry(mTabs, Vector2i(0, 1), false, true); // Corrected row

    mList = std::make_shared<ComponentList>(mWindow);
    mList->setUpdateType(ComponentListFlags::UPDATE_ALWAYS);
    mGrid.setEntry(mList, Vector2i(0, 2), true, true);

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
}

void GuiRetroAchievements::applyFilterAndSort()
{
    mFilteredGames.clear();
    for (auto& game : mAllGames)
    {
        if (mFilterMode == FilterMode::Over10Mins && game.gameTimeSeconds < 600) continue;
        mFilteredGames.push_back(&game);
    }
    
    std::sort(mFilteredGames.begin(), mFilteredGames.end(), [this](GameEntry* a, GameEntry* b) {
        if (mSortMode == SortMode::Recent) return a->lastPlayed > b->lastPlayed;
        if (mSortMode == SortMode::Playtime) return a->gameTimeSeconds > b->gameTimeSeconds;
        if (mSortMode == SortMode::Achievements) {
            int aAch = a->hasRaGame ? a->raGame.wonAchievementsSoftcore : 0;
            int bAch = b->hasRaGame ? b->raGame.wonAchievementsSoftcore : 0;
            return aAch > bAch;
        }
        if (mSortMode == SortMode::Completion) {
            float aComp = a->hasRaGame && a->raGame.totalAchievements > 0 ? (float)a->raGame.wonAchievementsSoftcore / a->raGame.totalAchievements : 0;
            float bComp = b->hasRaGame && b->raGame.totalAchievements > 0 ? (float)b->raGame.wonAchievementsSoftcore / b->raGame.totalAchievements : 0;
            return aComp > bComp;
        }
        return a->lastPlayed > b->lastPlayed;
    });

    if (mActiveTab == 0) populateGamesTab();
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
            if (game->hasRaGame) {
                GuiGameAchievements::show(mWindow, Utils::String::toInteger(game->raGame.id));
            } else if (game->fileData) {
                GuiGameAchievements::show(mWindow, game->fileData);
            }
        });
        
        mList->addRow(row, true, true);
    }
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
    std::vector<PlaySession> allSessions;

    for (auto& game : mAllGames) {
        if (game.fileData) {
            auto sessions = PlayHistoryManager::getInstance()->getSessions(game.fileData);
            allSessions.insert(allSessions.end(), sessions.begin(), sessions.end());
        }
    }

    std::sort(allSessions.begin(), allSessions.end(), [](const PlaySession& a, const PlaySession& b) {
        return parseDateTimeHistory(a.startTime) > parseDateTimeHistory(b.startTime);
    });

    long totalPlayTime = 0;
    for (const auto& s : allSessions) {
        totalPlayTime += s.durationSeconds;
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
    auto valCount = std::make_shared<TextComponent>(mWindow, std::to_string(allSessions.size()), theme->Text.font, theme->Text.color);
    valCount->setHorizontalAlignment(ALIGN_RIGHT);
    rowCount.addElement(lblCount, true);
    rowCount.addElement(valCount, false);
    mList->addRow(rowCount);
    
    ComponentListRow rowLast;
    auto lblLast = std::make_shared<TextComponent>(mWindow, _("LAST PLAYED"), theme->Text.font, theme->Text.color);
    
    std::string lastPlayedFormatted = _("never");
    if (!allSessions.empty()) {
        std::string isoDate = allSessions.front().startTime;
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

    for (auto& s : allSessions) {
        std::string isoDate = s.startTime;
        isoDate = Utils::String::replace(isoDate, "-", "");
        isoDate = Utils::String::replace(isoDate, ":", "");
        isoDate = Utils::String::replace(isoDate, "Z", "");
        std::string formattedDate = Utils::Time::DateTime(isoDate).toFullString();
        
        std::string durationStr = Utils::Time::secondsToString(s.durationSeconds, false, true);
        if (s.durationSeconds < 60) durationStr = std::to_string(s.durationSeconds) + " sec";
        
        ComponentListRow sessionRow;
        auto lblSession = std::make_shared<TextComponent>(mWindow, formattedDate, theme->TextSmall.font, theme->Text.color);
        lblSession->setOpacity(160);
            
        auto valSession = std::make_shared<TextComponent>(mWindow, durationStr, theme->TextSmall.font, theme->Text.color);
        valSession->setHorizontalAlignment(ALIGN_RIGHT);
        valSession->setOpacity(160);

        sessionRow.addElement(lblSession, true);
        sessionRow.addElement(valSession, false);
        mList->addRow(sessionRow);
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

void GuiRetroAchievements::openSortFilterMenu()
{
    auto s = new GuiSettings(mWindow, _("SORT & FILTER"));

    auto sortList = std::make_shared<OptionListComponent<SortMode>>(mWindow, _("SORT BY"), false);
    sortList->add(_("RECENTLY PLAYED"), SortMode::Recent, mSortMode == SortMode::Recent);
    sortList->add(_("MOST PLAYTIME"), SortMode::Playtime, mSortMode == SortMode::Playtime);
    sortList->add(_("MOST ACHIEVEMENTS"), SortMode::Achievements, mSortMode == SortMode::Achievements);
    sortList->add(_("COMPLETION %"), SortMode::Completion, mSortMode == SortMode::Completion);
    
    s->addWithLabel(_("SORT BY"), sortList);

    auto filterList = std::make_shared<OptionListComponent<FilterMode>>(mWindow, _("FILTER"), false);
    filterList->add(_("> 10 MINS PLAYTIME"), FilterMode::Over10Mins, mFilterMode == FilterMode::Over10Mins);
    filterList->add(_("ALL GAMES"), FilterMode::All, mFilterMode == FilterMode::All);

    s->addWithLabel(_("FILTER"), filterList);

    s->addSaveFunc([this, sortList, filterList] {
        mSortMode = sortList->getSelected();
        mFilterMode = filterList->getSelected();
        applyFilterAndSort();
    });

    mWindow->pushGui(s);
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
