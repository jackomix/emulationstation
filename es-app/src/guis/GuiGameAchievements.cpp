#include "guis/GuiGameAchievements.h"
#include "components/WebImageComponent.h"
#include "components/TextComponent.h"
#include "components/ButtonComponent.h"
#include "components/MultiLineMenuEntry.h"
#include "components/ScrollableContainer.h"
#include "components/RectangleComponent.h"
#include "components/ScrollbarComponent.h"
#include "utils/HtmlColor.h"
#include "views/ViewController.h"
#include "Window.h"
#include <string>
#include "Log.h"
#include "Settings.h"
#include "ApiSystem.h"
#include "LocaleES.h"
#include "GuiLoading.h"
#include "FileData.h"
#include "SystemData.h"
#include "GuiGameOptions.h"
#include "PlayHistoryManager.h"
#include "utils/TimeUtil.h"
#include <ctime>

#define WINDOW_WIDTH (float)Math::min(Renderer::getScreenHeight() * 1.125f, Renderer::getScreenWidth() * 0.90f)
#define IMAGESIZE (Renderer::getScreenHeight() * (48.0 / 720.0))
#define IMAGESPACER (Renderer::getScreenHeight() * (10.0 / 720.0))

static std::string formatTimeIn(int seconds)
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

static time_t parseDateTime(const std::string& dt)
{
	if (dt.empty()) return 0;
	
	// Check if it's the EmulationStation local time format (e.g., "20260730T123456")
	if (dt.find("T") != std::string::npos && dt.find("-") == std::string::npos)
	{
		// stringToTime parses it as Local Time and returns the correct UTC epoch
		return Utils::Time::stringToTime(dt);
	}
	
	struct tm t = {};
	// Try RetroAchievements API UTC format "YYYY-MM-DD HH:MM:SS"
	if (sscanf(dt.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) == 6)
	{
		t.tm_year -= 1900;
		t.tm_mon -= 1;
		// timegm parses it directly as UTC and returns the correct UTC epoch
		return timegm(&t);
	}
	// Try ISO 8601 "YYYY-MM-DDTHH:MM:SSZ" (just in case)
	if (sscanf(dt.c_str(), "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) == 6)
	{
		t.tm_year -= 1900;
		t.tm_mon -= 1;
		return timegm(&t);
	}
	return 0;
}

void GuiGameAchievements::show(Window* window, FileData* game)
{
	window->pushGui(new GuiGameAchievements(window, GameInfoAndUserProgress(), game));
}

void GuiGameAchievements::show(Window* window, int gameId)
{
	window->pushGui(new GuiLoading<GameInfoAndUserProgress>(window, _("PLEASE WAIT"),
		[window, gameId](auto gui)
	{
		return RetroAchievements::getGameInfoAndUserProgress(gameId);
	},
		[window](GameInfoAndUserProgress ra)
	{
		if (ra.ID == 0 && !ra.Title.empty())
			window->pushGui(new GuiMsgBox(window, _("AN ERROR OCCURRED") + "\r\n" + ra.Title, _("OK")));
		else if (ra.ID == 0)
			window->pushGui(new GuiMsgBox(window, _("AN ERROR OCCURRED"), _("OK")));
		else
			window->pushGui(new GuiGameAchievements(window, ra));
	}));
}

class GameAchievementEntry : public ComponentGrid
{
public:
	GameAchievementEntry(Window* window, GameInfoAndUserProgress& raInfo, Achievement& ra) :
		ComponentGrid(window, Vector2i(4, 4))
	{
		mGameInfo = ra;

		auto theme = ThemeData::getMenuTheme();

		mImage = std::make_shared<WebImageComponent>(mWindow);

		setEntry(mImage, Vector2i(0, 0), false, false, Vector2i(1, 4));
				
		std::string desc = mGameInfo.Description;

		if (!mGameInfo.DateEarnedHardcore.empty()) {
			desc += _U(" \u00b7 \uf091 ") + mGameInfo.DateEarnedHardcore + _U(" \u00b7 ") + _("HARDCORE MODE");
		}
		else if (!mGameInfo.DateEarned.empty()) {
			desc += _U(" \u00b7 \uf091 ") + mGameInfo.DateEarned;
		}

		mText = std::make_shared<TextComponent>(mWindow, mGameInfo.Title, theme->Text.font, theme->Text.color);
		mText->setVerticalAlignment(ALIGN_TOP);

		mSubstring = std::make_shared<TextComponent>(mWindow, desc, theme->TextSmall.font, theme->Text.color);
		mSubstring->setOpacity(192);
		mSubstring->setAutoScrollDelay(500);

		float percentage = 0.0f;
		float distinctPlayers = Utils::String::toFloat(raInfo.NumDistinctPlayersCasual);
		if (distinctPlayers > 0.0f)
			percentage = (Utils::String::toFloat(mGameInfo.NumAwarded) / distinctPlayers) * 100.0f;

		char pctStr[32];
		snprintf(pctStr, sizeof(pctStr), "%.1f%%", percentage);

		mPoints = std::make_shared<TextComponent>(mWindow, mGameInfo.Points + _U(" \uf091"), theme->Text.font, theme->Text.color, ALIGN_RIGHT);
		mPercentage = std::make_shared<TextComponent>(mWindow, pctStr, theme->TextSmall.font, theme->Text.color, ALIGN_RIGHT);
		mPercentage->setOpacity(192);

		setEntry(mText, Vector2i(2, 1), false, true);
		setEntry(mSubstring, Vector2i(2, 2), false, true);
		setEntry(mPoints, Vector2i(3, 1), false, true);
		setEntry(mPercentage, Vector2i(3, 2), false, true);

		int height = Math::max(IMAGESIZE + IMAGESPACER, mText->getSize().y() + mSubstring->getSize().y());

		float hTxt = mText->getSize().y() / height;
		float hSub = mSubstring->getSize().y() / height;
		float topPadding = Math::max(0.0f, (height - mText->getSize().y() - mSubstring->getSize().y()) / height / 2.0f);

		setRowHeightPerc(0, topPadding);
		setRowHeightPerc(1, hTxt);
		setRowHeightPerc(2, hSub);
		setRowHeightPerc(3, Math::max(0.0f, 1.0f - topPadding - hTxt - hSub));

		float pointsColWidth = Renderer::getScreenHeight() * 0.12f / WINDOW_WIDTH;
		setColWidthPerc(0, (height - IMAGESPACER) / WINDOW_WIDTH);
		setColWidthPerc(1, IMAGESPACER / WINDOW_WIDTH);
		setColWidthPerc(3, pointsColWidth);
	
		mImage->setMaxSize(height - IMAGESPACER, height - IMAGESPACER);
		mImage->setImage(mGameInfo.getBadgeUrl());

		if (mGameInfo.DateEarnedHardcore.empty() && mGameInfo.DateEarned.empty())
			mImage->setOpacity(120);

		setSize(0, height);
	}

	virtual void setColor(unsigned int color)
	{
		mText->setColor(color);
		mSubstring->setColor(color);
		if (mPoints) mPoints->setColor(color);
		if (mPercentage) mPercentage->setColor(color);
	}

	void onFocusLost() override
	{
		mSubstring->setAutoScroll(TextComponent::NONE);
		ComponentGrid::onFocusLost();
	}

	void onFocusGained() override
	{
		mSubstring->setAutoScroll(TextComponent::HORIZONTAL);
		mSubstring->onShow();
		ComponentGrid::onFocusGained();
	}

private:
	std::shared_ptr<TextComponent> mText;
	std::shared_ptr<TextComponent> mSubstring;
	std::shared_ptr<TextComponent> mPoints;
	std::shared_ptr<TextComponent> mPercentage;
	std::shared_ptr<WebImageComponent> mImage;
	Achievement mGameInfo;
};

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

		// Build "X min Y sec in" offset string from session start + achievement earn time
		std::string timeInStr;
		if (!sessionStartTime.empty() && !mGameInfo.DateEarned.empty())
		{
			time_t sessionT = parseDateTime(sessionStartTime);
			time_t earnedT  = parseDateTime(mGameInfo.DateEarned);
			if (sessionT > 0 && earnedT > 0 && earnedT >= sessionT)
				timeInStr = formatTimeIn((int)(earnedT - sessionT));
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
		// column 3 (Description) is left unset so it auto-sizes to remaining width
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

class GuiGameAchievements;

class ScrollableDescription : public GuiComponent
{
public:
	ScrollableDescription(Window* window, const std::string& text, GuiGameAchievements* parent) : GuiComponent(window), mParent(parent)
	{
		auto theme = ThemeData::getMenuTheme();
		mBgColor = (theme->Background.centerColor & 0xFFFFFF00) | 0xFF;
		mLabel = std::make_shared<TextComponent>(window, _("DESCRIPTION"), theme->Text.font, theme->Text.color);
		mLabel->setVerticalAlignment(ALIGN_TOP);

		mText = std::make_shared<TextComponent>(window, text, theme->TextSmall.font, theme->Text.color);
		mText->setVerticalAlignment(ALIGN_TOP);
		mText->setOpacity(192);
		mText->setMultiLine(TextComponent::MultiLineType::MULTILINE);

		mContainer = std::make_shared<ScrollableContainer>(window);
		mContainer->addChild(mText.get());

		mScrollbar = std::make_shared<ScrollbarComponent>(window);
		mScrollbar->loadFromMenuTheme();
		if (theme->Background.scrollbarColor == 0) {
			mScrollbar->setColor((theme->Text.color & 0xFFFFFF00) | 0x50);
			mScrollbar->setEnabled(true);
		}

		addChild(mLabel.get());
		addChild(mContainer.get());
		addChild(mScrollbar.get());
	}

	void onSizeChanged() override
	{
		GuiComponent::onSizeChanged();
		float labelHeight = mLabel->getFont()->getLetterHeight() * 2.0f;
		mLabel->setSize(mSize.x(), labelHeight);
		mLabel->setPosition(0, 0);

		if (mSize.x() > 0)
			mText->setSize(mSize.x(), 0);

		float textHeight = mText->getSize().y();
		
		float containerHeight = Math::max(0.0f, mSize.y() - labelHeight);
		float containerY = labelHeight;
		
		float lineHeight = mText->getFont()->getHeight();
		int numLines = (int)(containerHeight / lineHeight);
		containerHeight = numLines * lineHeight;

		mContainer->setPosition(0, containerY);
		mContainer->setSize(mSize.x(), containerHeight);

		mScrollbar->setContainerBounds(Vector3f(0, labelHeight, 0), Vector2f(mSize.x(), containerHeight), true);
		mScrollbar->setRange(0, mText->getSize().y(), containerHeight);
	}

	void update(int deltaTime) override
	{
		GuiComponent::update(deltaTime);
		
		if (mScrollDir != 0) {
			mScrollAccumulator += deltaTime;
			while (mScrollAccumulator >= mScrollDelay) {
				mScrollAccumulator -= mScrollDelay;
				mScrollDelay = 114;

				float scrollAmount = mText->getFont()->getLetterHeight() * 2.0f;
				bool scrolled = false;
				if (mScrollDir == -1 && mContainer->getScrollPos().y() > 0) {
					float newY = Math::max(0.0f, mContainer->getScrollPos().y() - scrollAmount);
					mContainer->setScrollPos(Vector2f(mContainer->getScrollPos().x(), newY));
					scrolled = true;
				}
				else if (mScrollDir == 1 && mContainer->getScrollPos().y() + mContainer->getSize().y() < mText->getSize().y()) {
					float newY = Math::min(mText->getSize().y() - mContainer->getSize().y(), mContainer->getScrollPos().y() + scrollAmount);
					mContainer->setScrollPos(Vector2f(mContainer->getScrollPos().x(), newY));
					scrolled = true;
				}

				if (!scrolled) {
					int oldScrollDir = mScrollDir;
					mScrollDir = 0;
					if (mBoundaryCallback)
						mBoundaryCallback(oldScrollDir);
					break;
				}
			}
		}

		mScrollbar->update(deltaTime);
		mScrollbar->setScrollPosition(mContainer->getScrollPos().y());
		mScrollbar->onCursorChanged();
	}

	void onFocusLost() override
	{
		mIsFocused = false;
		mScrollDir = 0;
		mScrollbar->loadFromMenuTheme();
		auto theme = ThemeData::getMenuTheme();
		if (theme->Background.scrollbarColor == 0) {
			mScrollbar->setColor((theme->Text.color & 0xFFFFFF00) | 0x50);
			mScrollbar->setEnabled(true);
		}
		GuiComponent::onFocusLost();
	}

	void onFocusGained() override
	{
		mIsFocused = true;
		auto theme = ThemeData::getMenuTheme();
		mScrollbar->setColor(theme->Text.selectorColor);

		if (mParent->mDownHeld) {
			mScrollDir = 1;
			mScrollAccumulator = 0;
			mScrollDelay = (mParent->mDownTime > 400) ? 114 : 500;
		} else if (mParent->mUpHeld) {
			mScrollDir = -1;
			mScrollAccumulator = 0;
			mScrollDelay = (mParent->mUpTime > 400) ? 114 : 500;
			
			float maxScroll = Math::max(0.0f, mText->getSize().y() - mContainer->getSize().y());
			mContainer->setScrollPos(Vector2f(mContainer->getScrollPos().x(), maxScroll));
		}

		GuiComponent::onFocusGained();
	}

	bool input(InputConfig* config, Input input) override
	{
		if (config->isMappedLike("up", input)) {
			if (input.value != 0) {
				if (mContainer->getScrollPos().y() > 0) {
					mScrollDir = -1;
					mScrollAccumulator = 0;
					mScrollDelay = 500;
					float scrollAmount = mText->getFont()->getLetterHeight() * 2.0f;
					float newY = Math::max(0.0f, mContainer->getScrollPos().y() - scrollAmount);
					mContainer->setScrollPos(Vector2f(mContainer->getScrollPos().x(), newY));
					return true;
				}
			} else {
				if (mScrollDir == -1) {
					mScrollDir = 0;
					return true;
				}
			}
		}
		if (config->isMappedLike("down", input)) {
			if (input.value != 0) {
				if (mContainer->getScrollPos().y() + mContainer->getSize().y() < mText->getSize().y()) {
					mScrollDir = 1;
					mScrollAccumulator = 0;
					mScrollDelay = 500;
					float scrollAmount = mText->getFont()->getLetterHeight() * 2.0f;
					float newY = Math::min(mText->getSize().y() - mContainer->getSize().y(), mContainer->getScrollPos().y() + scrollAmount);
					mContainer->setScrollPos(Vector2f(mContainer->getScrollPos().x(), newY));
					return true;
				}
			} else {
				if (mScrollDir == 1) {
					mScrollDir = 0;
					return true;
				}
			}
		}
		return false;
	}

	void setColor(unsigned int color) override { 
		mLabel->setColor(color);
		mText->setColor(Utils::HtmlColor::applyColorOpacity(color, 192));
	}

	unsigned int mBgColor;
	std::shared_ptr<TextComponent> mLabel;
	std::shared_ptr<TextComponent> mText;
	std::shared_ptr<ScrollableContainer> mContainer;
	std::shared_ptr<ScrollbarComponent> mScrollbar;
	bool mIsFocused = false;
	int mScrollDir = 0;
	int mScrollAccumulator = 0;
	int mScrollDelay = 500;
	GuiGameAchievements* mParent;
	std::function<void(int)> mBoundaryCallback;
};

GuiGameAchievements::GuiGameAchievements(Window* window, GameInfoAndUserProgress ra, FileData* game) : 
	GuiComponent(window), mGrid(window, Vector2i(1, 4)), mBackground(window, ":/frame.png")
{
	mRaInfo = ra;
	mActiveTab = 0;
	mFile = game != nullptr ? game : GuiRetroAchievements::getFileData(std::to_string(ra.ID));

	int gameId = 0;
	if (mFile != nullptr) gameId = Utils::String::toInteger(mFile->getMetadata(MetaDataId::CheevosId));
	if (gameId == 0) gameId = mRaInfo.ID;

	if (gameId != 0 && mRaInfo.ID == 0) {
		mIsLoadingAchievements = true;
		mRaFuture = std::async(std::launch::async, [gameId]() {
			return RetroAchievements::getGameInfoAndUserProgress(gameId);
		});
	} else {
		mIsLoadingAchievements = false;
	}

	addChild(&mBackground);
	addChild(&mGrid);

	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path);
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);
	mBackground.setPostProcessShader(theme->Background.menuShader);

	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(2, 3));

	mTitle = std::make_shared<TextComponent>(mWindow, "", theme->Title.font, theme->Title.color, ALIGN_LEFT);
	mSubtitle = std::make_shared<TextComponent>(mWindow, "", theme->TextSmall.font, theme->Text.color, ALIGN_LEFT);
	mTitleImage = std::make_shared<WebImageComponent>(mWindow);

	mHeaderGrid->setEntry(mTitle, Vector2i(0, 0), false, true, Vector2i(1, 1));
	mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 1), false, true, Vector2i(1, 1));
	mHeaderGrid->setEntry(mTitleImage, Vector2i(1, 0), false, false, Vector2i(1, 3));

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

	mTabs = std::make_shared<ComponentTab>(mWindow);
	mTabs->addTab(_("ACHIEVEMENTS"));
	mTabs->addTab(_("PLAY HISTORY"));
	mTabs->addTab(_("INFO"));
	mTabs->addTab(_("OPTIONS"));

	if (mFile != nullptr)
	{
		mOptionsUI = std::make_shared<GuiGameOptions>(mWindow, mFile, true);
		mOptionsUI->setCloseCallback([this]() {
			delete this;
		});
	}

	mTabs->setCursorChangedCallback([this](const CursorState& state) {
		if (mActiveTab != mTabs->getCursorIndex()) {
			if (mActiveTab != 3) mTabCursors[mActiveTab] = mList->getCursorIndex();
			mActiveTab = mTabs->getCursorIndex();
			populateTabContent();
		}
	});

	mGrid.setEntry(mTabs, Vector2i(0, 1), false, true);

	mList = std::make_shared<ComponentList>(mWindow);
	mList->setUpdateType(ComponentListFlags::UPDATE_ALWAYS);
	mGrid.setEntry(mList, Vector2i(0, 2), true, true);

	std::vector<std::shared_ptr<ButtonComponent>> buttons;
	if (mFile != nullptr)
	{
		buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("LAUNCH"), _("LAUNCH"), [this]
		{ 			
			Window* window = mWindow;
			while (window->peekGui() && window->peekGui() != ViewController::get())
				delete window->peekGui();
			ViewController::get()->launch(mFile);
		}));
	}
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("BACK"), _("go back"), [this] { delete this; }));

	mButtonGrid = makeButtonGrid(mWindow, buttons);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 3), true, false);

	mGrid.setUnhandledInputCallback([this](InputConfig* config, Input input) -> bool
		{
			if (config->isMappedLike("down", input)) { 
                if (mActiveTab == 3 && mOptionsUI) { mGrid.setCursorTo(mOptionsUI->getMenu()->getList()); mOptionsUI->getMenu()->getList()->setCursorIndex(0); }
                else { mGrid.setCursorTo(mList); mList->setCursorIndex(0); }
                return true; 
            }
			if (config->isMappedLike("up", input)) { 
                if (mActiveTab == 3 && mOptionsUI) { mOptionsUI->getMenu()->getList()->setCursorIndex(mOptionsUI->getMenu()->getList()->size() - 1); mGrid.moveCursor(Vector2i(0, 1)); }
                else { mList->setCursorIndex(mList->size() - 1); mGrid.moveCursor(Vector2i(0, 1)); }
                return true; 
            }
			return false;
		});

	updateAchievementsHeader();
	centerWindow();
	populateTabContent();
}

void GuiGameAchievements::onSizeChanged()
{
	GuiComponent::onSizeChanged();

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	
	mGrid.setSize(mSize);

	const float titleHeight = mTitle ? mTitle->getFont()->getLetterHeight() * 1.5f : 0;
	const float subtitleHeight = mSubtitle ? mSubtitle->getFont()->getLetterHeight() * 1.5f : 0;
	const float progressHeight = Renderer::getScreenHeight() * 0.05f;

	if (mTabs)
		mTabs->setSize(mGrid.getSize().x(), Renderer::getScreenHeight() * 0.06f);

	if (mHeaderGrid)
	{
		mHeaderGrid->setRowHeight(0, titleHeight);
		mHeaderGrid->setRowHeight(1, subtitleHeight);
		mHeaderGrid->setRowHeight(2, progressHeight);

		float imageWidth = Renderer::getScreenHeight() * 0.15f;
		float headerWidth = mGrid.getSize().x();
		float textWidth = headerWidth - imageWidth;

		mHeaderGrid->setColWidth(0, textWidth);
		mHeaderGrid->setColWidth(1, imageWidth);

		if (mTitleImage) mTitleImage->setMaxSize(imageWidth, titleHeight + subtitleHeight + progressHeight);
		if (mProgress) mProgress->setSize(textWidth * 0.6f, progressHeight);
	}

	float headerTotalHeight = titleHeight + subtitleHeight + progressHeight + (Renderer::getScreenHeight() * 0.02f);
	mGrid.setRowHeight(0, headerTotalHeight);
	mGrid.setRowHeight(1, Renderer::getScreenHeight() * 0.06f);
	if (mButtonGrid)
		mGrid.setRowHeight(3, mButtonGrid->getSize().y());
}

void GuiGameAchievements::centerWindow()
{
	if (Renderer::ScreenSettings::fullScreenMenus())
		setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		setSize(WINDOW_WIDTH, Renderer::getScreenHeight() * 0.901f);

	setPosition((Renderer::getScreenWidth() - getSize().x()) / 2, (Renderer::getScreenHeight() - getSize().y()) / 2);
}

void GuiGameAchievements::populateTabContent()
{
	mList->clear();
	mGrid.removeEntry(mList);
	if (mOptionsUI) mGrid.removeEntry(mOptionsUI->getMenu()->getList());

	if (mActiveTab == 3 && mOptionsUI) {
		mGrid.setEntry(mOptionsUI->getMenu()->getList(), Vector2i(0, 2), true, true);
	} else {
		mGrid.setEntry(mList, Vector2i(0, 2), true, true);
		if (mActiveTab == 0) populateAchievementsTab();
		else if (mActiveTab == 1) populatePlayHistoryTab();
		else if (mActiveTab == 2) populateInfoTab();

		if (mTabCursors.find(mActiveTab) != mTabCursors.end() && mList->size() > 0) {
			int cursorIndex = mTabCursors[mActiveTab];
			if (cursorIndex >= mList->size()) cursorIndex = mList->size() - 1;
			if (cursorIndex < 0) cursorIndex = 0;
			mList->setCursorIndex(cursorIndex);
		}
	}

	centerWindow();
}

void GuiGameAchievements::populateAchievementsTab()
{
	for (auto game : mRaInfo.Achievements)
	{
		ComponentListRow row;
		auto itstring = std::make_shared<GameAchievementEntry>(mWindow, mRaInfo, game);
		row.addElement(itstring, true);
		mList->addRow(row);
	}
}

void GuiGameAchievements::populatePlayHistoryTab()
{
	if (mFile == nullptr) return;
	auto theme = ThemeData::getMenuTheme();
	
	auto sessions = PlayHistoryManager::getInstance()->getSessions(mFile);
	std::sort(sessions.begin(), sessions.end(), [](const PlaySession& a, const PlaySession& b) {
		return a.startTime > b.startTime;
	});

	long totalPlayTime = 0;
	for (const auto& s : sessions) {
		totalPlayTime += s.durationSeconds;
	}

	ComponentListRow rowTime;
	auto lblTime = std::make_shared<TextComponent>(mWindow, _("PLAY TIME"), theme->Text.font, theme->Text.color);
	auto valTime = std::make_shared<TextComponent>(mWindow, Utils::Time::secondsToString(totalPlayTime, false, true), theme->Text.font, theme->Text.color);
	valTime->setHorizontalAlignment(ALIGN_RIGHT);
	rowTime.addElement(lblTime, true);
	rowTime.addElement(valTime, false);
	mList->addRow(rowTime);
	
	ComponentListRow rowCount;
	auto lblCount = std::make_shared<TextComponent>(mWindow, _("PLAY COUNT"), theme->Text.font, theme->Text.color);
	auto valCount = std::make_shared<TextComponent>(mWindow, std::to_string(sessions.size()), theme->Text.font, theme->Text.color);
	valCount->setHorizontalAlignment(ALIGN_RIGHT);
	rowCount.addElement(lblCount, true);
	rowCount.addElement(valCount, false);
	mList->addRow(rowCount);
	
	ComponentListRow rowLast;
	auto lblLast = std::make_shared<TextComponent>(mWindow, _("LAST PLAYED"), theme->Text.font, theme->Text.color);
	
	std::string lastPlayedFormatted = _("never");
	if (!sessions.empty()) {
		std::string isoDate = sessions.front().startTime;
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

	std::map<std::string, std::vector<Achievement>> sessionAchievements;

	for (auto& s : sessions) {
		for (auto& id : s.achievementIds) {
			for (auto& ach : mRaInfo.Achievements) {
				if (ach.ID == id) {
					sessionAchievements[s.id].push_back(ach);
					break;
				}
			}
		}
		std::sort(sessionAchievements[s.id].begin(), sessionAchievements[s.id].end(), [](const Achievement& a, const Achievement& b) {
			return parseDateTime(a.DateEarned) > parseDateTime(b.DateEarned);
		});
	}

	for (auto& s : sessions) {
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
		
		int sessionPoints = 0;
		for (const auto& a : sessionAchievements[s.id]) {
			sessionPoints += Utils::String::toInteger(a.Points);
		}
		
		std::string rightStr = durationStr;
			
		auto valSession = std::make_shared<TextComponent>(mWindow, rightStr, theme->TextSmall.font, theme->Text.color);
		valSession->setHorizontalAlignment(ALIGN_RIGHT);
		valSession->setOpacity(160);

		sessionRow.addElement(lblSession, true);
		sessionRow.addElement(valSession, false);
		mList->addRow(sessionRow);

		for (auto ach : sessionAchievements[s.id]) {
			ComponentListRow achRow;
			achRow.no_separator = true;
			auto entry = std::make_shared<SessionAchievementEntry>(mWindow, ach, s.startTime);
			
			auto spacer = std::make_shared<GuiComponent>(mWindow);
			spacer->setSize(Renderer::getScreenHeight() * 0.02f, 0);
			achRow.addElement(spacer, false);
			achRow.addElement(entry, true);
			achRow.makeAcceptInputHandler([this, ach] {
				int index = 0;
				for (auto& a : mRaInfo.Achievements) {
					if (a.ID == ach.ID) break;
					index++;
				}
				mWindow->postToUiThread([this, index]() {
					mTabs->setCursorIndex(0);
					mList->setCursorIndex(index);
				});
			});
			mList->addRow(achRow);
		}
	}
}

void GuiGameAchievements::populateInfoTab()
{
	if (mFile == nullptr) return;
	auto theme = ThemeData::getMenuTheme();
	
	std::string consoleName = mFile->getSourceFileData()->getSystem()->getFullName();
	std::string developer = mFile->getMetadata(MetaDataId::Developer);
	std::string genre = mFile->getMetadata(MetaDataId::Genre);
	std::string releaseDate = mFile->getMetadata(MetaDataId::ReleaseDate);
	std::string desc = mFile->getMetadata(MetaDataId::Desc);
	
	if (!consoleName.empty()) {
		ComponentListRow rowSys;
		auto lblSys = std::make_shared<TextComponent>(mWindow, _("SYSTEM"), theme->Text.font, theme->Text.color);
		auto valSys = std::make_shared<TextComponent>(mWindow, consoleName, theme->Text.font, theme->Text.color);
		valSys->setHorizontalAlignment(ALIGN_RIGHT);
		rowSys.addElement(lblSys, true);
		rowSys.addElement(valSys, false);
		mList->addRow(rowSys);
	}

	if (!developer.empty() && developer != "Unknown") {
		ComponentListRow rowDev;
		auto lblDev = std::make_shared<TextComponent>(mWindow, _("DEVELOPER"), theme->Text.font, theme->Text.color);
		auto valDev = std::make_shared<TextComponent>(mWindow, developer, theme->Text.font, theme->Text.color);
		valDev->setHorizontalAlignment(ALIGN_RIGHT);
		rowDev.addElement(lblDev, true);
		rowDev.addElement(valDev, false);
		mList->addRow(rowDev);
	}

	if (!genre.empty() && genre != "Unknown") {
		ComponentListRow rowGen;
		auto lblGen = std::make_shared<TextComponent>(mWindow, _("GENRE"), theme->Text.font, theme->Text.color);
		auto valGen = std::make_shared<TextComponent>(mWindow, genre, theme->Text.font, theme->Text.color);
		valGen->setHorizontalAlignment(ALIGN_RIGHT);
		rowGen.addElement(lblGen, true);
		rowGen.addElement(valGen, false);
		mList->addRow(rowGen);
	}

	if (releaseDate.length() >= 4) {
		std::string year = releaseDate.substr(0, 4);
		ComponentListRow rowYear;
		auto lblYear = std::make_shared<TextComponent>(mWindow, _("RELEASE YEAR"), theme->Text.font, theme->Text.color);
		auto valYear = std::make_shared<TextComponent>(mWindow, year, theme->Text.font, theme->Text.color);
		valYear->setHorizontalAlignment(ALIGN_RIGHT);
		rowYear.addElement(lblYear, true);
		rowYear.addElement(valYear, false);
		mList->addRow(rowYear);
	}

	if (!desc.empty()) {
		ComponentListRow rowDesc;
		auto valDesc = std::make_shared<ScrollableDescription>(mWindow, desc, this);
		valDesc->mBoundaryCallback = [this](int dir) {
			if (dir == 1) {
				mGrid.moveCursor(Vector2i(0, 1));
			} else if (dir == -1) {
				int target = mList->getCursorIndex() - 1;
				if (target >= 0) {
					mList->setCursorIndex(target);
				}
			}
		};
		
		float exactWidth = mList->getSize().x() - 20.0f;
		valDesc->mLabel->setSize(exactWidth, 0);
		valDesc->mText->setSize(exactWidth, 0);
		
		float textH = valDesc->mText->getSize().y();
		float labelH = valDesc->mLabel->getFont()->getLetterHeight() * 2.0f;
		float maxLinesH = theme->Text.font->getLetterHeight() * 8.5f;
		float lineHeight = valDesc->mText->getFont()->getHeight();

		float targetContainerH = Math::min(textH, maxLinesH);
		int numLines = (int)(targetContainerH / lineHeight);
		float snappedContainerH = numLines * lineHeight;

		float finalHeight = labelH + snappedContainerH;
		
		valDesc->setSize(mList->getSize().x(), finalHeight);
		rowDesc.addElement(valDesc, true);
		rowDesc.hide_cursor = true;
		mList->addRow(rowDesc);
	}
}

void GuiGameAchievements::updateAchievementsHeader()
{
	auto theme = ThemeData::getMenuTheme();

	std::string titleText = mRaInfo.Title.empty() && mFile ? mFile->getName() : mRaInfo.Title;
	mTitle->setText(titleText);

	int totalPoints = 0;
	int userPoints = 0;
	for (auto game : mRaInfo.Achievements)
	{
		if (!game.DateEarned.empty() || !game.DateEarnedHardcore.empty())
			userPoints += Utils::String::toInteger(game.Points);
		totalPoints += Utils::String::toInteger(game.Points);
	}

	std::string subtitleText = "";
	if (mIsLoadingAchievements)
		subtitleText = _("LOADING ACHIEVEMENTS...");
	else if (mRaInfo.Achievements.size() == 0)
		subtitleText = _("THIS GAME HAS NO ACHIEVEMENTS YET");
	else
	{
		subtitleText = std::to_string(mRaInfo.NumAwardedToUser) + "/" + std::to_string(mRaInfo.NumAchievements) + " " + _("achievements");
		subtitleText += _U(" \u00b7 ") + std::to_string(userPoints) + "/" + std::to_string(totalPoints) + " " + _("points");
	}

	mSubtitle->setText(subtitleText);
	
	if (!mRaInfo.getImageUrl().empty())
		mTitleImage->setImage(mRaInfo.getImageUrl());

	if (mRaInfo.Achievements.size() > 0)
	{
		int percent = Math::round(mRaInfo.NumAwardedToUser * 100.0f / mRaInfo.Achievements.size());
		char trstring[256];
		snprintf(trstring, 256, _("%d%% complete").c_str(), percent);
		
		if (mProgress)
			mHeaderGrid->removeEntry(mProgress);

		mProgress = std::make_shared<RetroAchievementProgress>(mWindow, mRaInfo.NumAwardedToUser, mRaInfo.NumAwardedToUserHardcore, mRaInfo.Achievements.size(), Utils::String::trim(trstring));
		mHeaderGrid->setEntry(mProgress, Vector2i(0, 2), false, true, Vector2i(1, 1));
	}

	onSizeChanged();
}

void GuiGameAchievements::render(const Transform4x4f& parentTrans)
{
	GuiComponent::render(parentTrans);
}

void GuiGameAchievements::update(int deltaTime)
{
	if (mDownHeld) mDownTime += deltaTime; else mDownTime = 0;
	if (mUpHeld) mUpTime += deltaTime; else mUpTime = 0;

	if (mGrid.isCursorTo(mList) && !mList->isScrolling()) {
		if (mUpHeld && mUpTime > 400 && mList->getCursorIndex() < mList->size() - 1) {
			mManualScrollAccum += deltaTime;
			if (mManualScrollAccum >= 114) {
				mManualScrollAccum -= 114;
				int target = mList->getCursorIndex() - 1;
				if (target >= 0) {
					mList->setCursorIndex(target);
				} else {
					mGrid.moveCursor(Vector2i(0, 1));
				}
			}
		} else if (mDownHeld && mDownTime > 400 && mList->getCursorIndex() < mList->size() - 1) {
			mManualScrollAccum += deltaTime;
			if (mManualScrollAccum >= 114) {
				mManualScrollAccum -= 114;
				int target = mList->getCursorIndex() + 1;
				if (target <= mList->size() - 1) {
					mList->setCursorIndex(target);
				}
			}
		} else {
			mManualScrollAccum = 0;
		}
	} else {
		mManualScrollAccum = 0;
	}

	GuiComponent::update(deltaTime);

	if (mIsLoadingAchievements && mRaFuture.valid() && mRaFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		mRaInfo = mRaFuture.get();
		mIsLoadingAchievements = false;
		
		updateAchievementsHeader();
		if (mActiveTab == 0) populateTabContent();
	}
}

bool GuiGameAchievements::input(InputConfig* config, Input input)
{
	if (config->isMappedLike("down", input)) mDownHeld = (input.value != 0);
	if (config->isMappedLike("up", input)) mUpHeld = (input.value != 0);

	if (GuiComponent::input(config, input))
		return true;

	if (input.value != 0 && config->isMappedTo(BUTTON_BACK, input))
	{
		delete this;
		return true;
	}

	if (mActiveTab == 3 && mOptionsUI)
	{
		if (mOptionsUI->input(config, input))
			return true;
	}

	if (mGrid.isCursorTo(mList) || (mOptionsUI && mGrid.isCursorTo(mOptionsUI->getMenu()->getList())))
	{
		if (mTabs->input(config, input))
			return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiGameAchievements::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	if (mFile != nullptr)
		prompts.push_back(HelpPrompt("x", _("LAUNCH")));

	return prompts;
}
