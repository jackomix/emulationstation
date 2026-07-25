#include "guis/GuiGameAchievements.h"
#include "guis/GuiSettings.h"
#include "components/WebImageComponent.h"

#include "Window.h"
#include <string>
#include "Log.h"
#include "Settings.h"
#include "ApiSystem.h"
#include "LocaleES.h"
#include "GuiLoading.h"

#include "components/MultiLineMenuEntry.h"
#include "GuiGameAchievements.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "SystemData.h"
#include "components/ComponentTab.h"

#define WINDOW_WIDTH (float)Math::min(Renderer::getScreenHeight() * 1.125f, Renderer::getScreenWidth() * 0.90f)
#define IMAGESIZE (Renderer::getScreenHeight() * (48.0 / 720.0))
#define IMAGESPACER (Renderer::getScreenHeight() * (10.0 / 720.0))

void GuiGameAchievements::show(Window* window, FileData* game)
{
	int gameId = Utils::String::toInteger(game->getMetadata(MetaDataId::CheevosId));
	window->pushGui(new GuiLoading<GameInfoAndUserProgress>(window, _("PLEASE WAIT"),
		[gameId](auto gui)
	{
		if (gameId == 0) return GameInfoAndUserProgress();
		return RetroAchievements::getGameInfoAndUserProgress(gameId);
	},
		[window, game](GameInfoAndUserProgress ra)
	{
		window->pushGui(new GuiGameAchievements(window, ra, game));
	}));
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
	GameAchievementEntry(Window* window, Achievement& ra) :
		ComponentGrid(window, Vector2i(3, 4))
	{
		mGameInfo = ra;

		auto theme = ThemeData::getMenuTheme();

		mImage = std::make_shared<WebImageComponent>(mWindow);

		setEntry(mImage, Vector2i(0, 0), false, false, Vector2i(1, 4));
				
		std::string desc = mGameInfo.Description;
		desc += _U(" - ") + _("Points") + ": " + mGameInfo.Points;

		if (!mGameInfo.DateEarnedHardcore.empty()) {
			desc += _U("  \uf091  ") + _("Unlocked on") + ": " + mGameInfo.DateEarnedHardcore + _U(" - ") + _("HARDCORE MODE");
		}
		else if (!mGameInfo.DateEarned.empty()) {
			desc += _U("  \uf091  ") + _("Unlocked on") + ": " + mGameInfo.DateEarned;
		}

		mText = std::make_shared<TextComponent>(mWindow, mGameInfo.Title, theme->Text.font, theme->Text.color);
		mText->setVerticalAlignment(ALIGN_TOP);

		mSubstring = std::make_shared<TextComponent>(mWindow, desc, theme->TextSmall.font, theme->Text.color);
		mSubstring->setOpacity(192);

		setEntry(mText, Vector2i(2, 1), false, true);
		setEntry(mSubstring, Vector2i(2, 2), false, true);

		int height = Math::max(IMAGESIZE + IMAGESPACER, mText->getSize().y() + mSubstring->getSize().y());

		float hTxt = mText->getSize().y() / height;
		float hSub = mSubstring->getSize().y() / height;
		float topPadding = Math::max(0.0f, (height - mText->getSize().y() - mSubstring->getSize().y()) / height / 2.0f);

		setRowHeightPerc(0, topPadding);
		setRowHeightPerc(1, hTxt);
		setRowHeightPerc(2, hSub);
		setRowHeightPerc(3, Math::max(0.0f, 1.0f - topPadding - hTxt - hSub));

		setColWidthPerc(0, (height - IMAGESPACER) / WINDOW_WIDTH);
		setColWidthPerc(1, IMAGESPACER / WINDOW_WIDTH);
	
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
	}

private:
	std::shared_ptr<TextComponent> mText;
	std::shared_ptr<TextComponent> mSubstring;

	std::shared_ptr<WebImageComponent> mImage;

	Achievement mGameInfo;
};


GuiGameAchievements::GuiGameAchievements(Window* window, GameInfoAndUserProgress ra, FileData* game) : 
	GuiSettings(window, _("ACHIEVEMENTS"), ([&ra]() {
		std::string title = ra.Title;
		if (ra.isOfflineData) {
			title += " (\U0001F4E6 Offline Data)";
		}
		return title;
	})(), nullptr)
{
	// Required for WebImageComponent
	setUpdateType(ComponentListFlags::UPDATE_ALWAYS);

	setTitle(ra.Title);

	mMenu.clearButtons();

	mFile = game != nullptr ? game : GuiRetroAchievements::getFileData(std::to_string(ra.ID));
	if (mFile != nullptr)
	{
		auto file = mFile;
		mMenu.addButton(_("LAUNCH"), _("LAUNCH"), [this, file]
		{ 			
			Window* window = mWindow;
			while (window->peekGui() && window->peekGui() != ViewController::get())
				delete window->peekGui();

			ViewController::get()->launch(file);
		});
	}

	mMenu.addButton(_("BACK"), _("go back"), [this] { close(); });

	mActiveTab = 0;
	mTabsHasFocus = false;
	mRaInfo = ra;

	mTabs = std::make_shared<ComponentTab>(mWindow);
	mTabs->addTab(_("ACHIEVEMENTS"));
	mTabs->addTab(_("PLAY HISTORY"));

	mTabs->setCursorChangedCallback([this](const CursorState& state) {
		if (mActiveTab != mTabs->getCursorIndex()) {
			mActiveTab = mTabs->getCursorIndex();
			populateTabContent();
		}
	});

	auto image = std::make_shared<WebImageComponent>(mWindow);
	image->setImage(ra.getImageUrl());
	setTitleImage(image);

	if (ra.Achievements.size() > 0)
	{
		int percent = Math::round(ra.NumAwardedToUser * 100.0f / ra.Achievements.size());

		char trstring[256];
		snprintf(trstring, 256, _("%d%% complete").c_str(), percent);
		mProgress = std::make_shared<RetroAchievementProgress>(mWindow, ra.NumAwardedToUser, ra.NumAwardedToUserHardcore, ra.Achievements.size(), Utils::String::trim(trstring));
	}

	populateTabContent();
}

void GuiGameAchievements::centerWindow()
{
	float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));

	if (Renderer::ScreenSettings::fullScreenMenus())
		mMenu.setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		mMenu.setSize(WINDOW_WIDTH, Renderer::getScreenHeight() * 0.901f);

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);

	if (mTabs)
	{
		float tabHeight = Renderer::getScreenHeight() * 0.06f;
		mTabs->setSize(mMenu.getSize().x(), tabHeight);
		mTabs->setPosition(mMenu.getPosition().x(), mMenu.getPosition().y() + mMenu.getHeaderGridHeight());
	}
}

void GuiGameAchievements::populateTabContent()
{
	mMenu.clear();

	if (mActiveTab == 0)
		populateAchievementsTab();
	else
		populatePlayHistoryTab();

	mMenu.updateSize();
	centerWindow();
}

void GuiGameAchievements::populateAchievementsTab()
{
	int totalPoints = 0;
	int userPoints = 0;

	for (auto game : mRaInfo.Achievements)
	{
		if (!game.DateEarned.empty() || !game.DateEarnedHardcore.empty())
			userPoints += Utils::String::toInteger(game.Points);

		totalPoints += Utils::String::toInteger(game.Points);
	}

	if (mRaInfo.Achievements.size() == 0)
		setSubTitle(_("THIS GAME HAS NO ACHIEVEMENTS YET") + "\n\n\n");
	else
	{
		auto txt = _("Achievements (softcore)") + ": \t" + std::to_string(mRaInfo.NumAwardedToUser) + "/" + std::to_string(mRaInfo.NumAchievements);
		txt += "\r\n" + _("Achievements (hardcore)") + ": \t" + std::to_string(mRaInfo.NumAwardedToUserHardcore) + "/" + std::to_string(mRaInfo.NumAchievements);
		txt += "\r\n" + _("Points") + ": \t" + std::to_string(userPoints) + "/" + std::to_string(totalPoints);
		txt += "\n\n";

		setSubTitle(txt);
	}

	if (mProgress) mProgress->setVisible(true);

	for (auto game : mRaInfo.Achievements)
	{
		ComponentListRow row;
		auto itstring = std::make_shared<GameAchievementEntry>(mWindow, game);
		row.addElement(itstring, true);
		addRow(row);
	}
}

void GuiGameAchievements::populatePlayHistoryTab()
{
	if (mFile == nullptr)
	{
		setSubTitle(_("NO GAME METADATA AVAILABLE") + "\n\n\n");
		if (mProgress) mProgress->setVisible(false);
		return;
	}
	
	std::string consoleName = mFile->getSourceFileData()->getSystem()->getFullName();
	std::string developer = mFile->getMetadata(MetaDataId::Developer);
	std::string genre = mFile->getMetadata(MetaDataId::Genre);
	
	std::string txt = consoleName;
	if (!developer.empty()) txt += "\r\n" + developer;
	if (!genre.empty()) txt += "\r\n" + genre;
	txt += "\n\n";
	setSubTitle(txt);
	
	if (mProgress) mProgress->setVisible(true);
	
	auto theme = ThemeData::getMenuTheme();
	
	// Play Time
	ComponentListRow rowTime;
	auto lblTime = std::make_shared<TextComponent>(mWindow, _("PLAY TIME"), theme->Text.font, theme->Text.color);
	auto valTime = std::make_shared<TextComponent>(mWindow, Utils::Time::secondsToString(Utils::String::toInteger(mFile->getMetadata(MetaDataId::GameTime))), theme->Text.font, theme->Text.color);
	valTime->setHorizontalAlignment(ALIGN_RIGHT);
	rowTime.addElement(lblTime, true);
	rowTime.addElement(valTime, false);
	addRow(rowTime);
	
	// Play Count
	ComponentListRow rowCount;
	auto lblCount = std::make_shared<TextComponent>(mWindow, _("PLAY COUNT"), theme->Text.font, theme->Text.color);
	auto valCount = std::make_shared<TextComponent>(mWindow, mFile->getMetadata(MetaDataId::PlayCount), theme->Text.font, theme->Text.color);
	valCount->setHorizontalAlignment(ALIGN_RIGHT);
	rowCount.addElement(lblCount, true);
	rowCount.addElement(valCount, false);
	addRow(rowCount);
	
	// Last Played
	ComponentListRow rowLast;
	auto lblLast = std::make_shared<TextComponent>(mWindow, _("LAST PLAYED"), theme->Text.font, theme->Text.color);
	auto valLast = std::make_shared<TextComponent>(mWindow, mFile->getMetadata(MetaDataId::LastPlayed), theme->Text.font, theme->Text.color);
	valLast->setHorizontalAlignment(ALIGN_RIGHT);
	rowLast.addElement(lblLast, true);
	rowLast.addElement(valLast, false);
	addRow(rowLast);
	
	// Description
	ComponentListRow rowDesc;
	auto lblDesc = std::make_shared<TextComponent>(mWindow, mFile->getMetadata(MetaDataId::Desc), theme->TextSmall.font, theme->Text.color);
	rowDesc.addElement(lblDesc, true);
	addRow(rowDesc);
}

void GuiGameAchievements::render(const Transform4x4f& parentTrans)
{
	GuiSettings::render(parentTrans);

	if (mTabs)
	{
		Transform4x4f trans = parentTrans * mMenu.getTransform();
		mTabs->render(parentTrans);
	}

	if (mProgress != nullptr && mProgress->isVisible())
	{
		auto theme = ThemeData::getMenuTheme();

		float h = theme->TextSmall.font->sizeText("A8O\rA8O", 1.1).y();
		float sz = mMenu.getHeaderGridHeight() + Renderer::getScreenHeight() * 0.005;

		float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));
		float iw = mMenu.getTitleHeight() / width;

		float xx = mMenu.getSize().x() - (mMenu.getSize().x() * iw);

		mProgress->setPosition(xx * 0.55f, sz);
		mProgress->setSize(xx * 0.36f, h);

		Transform4x4f trans = parentTrans * mMenu.getTransform();
		mProgress->render(trans);
	}
}

bool GuiGameAchievements::input(InputConfig* config, Input input)
{
	if (input.value != 0)
	{
		if (config->isMappedTo("leftshoulder", input) || config->isMappedTo("pageup", input))
		{
			int currentTab = mTabs->getCursorIndex();
			if (currentTab > 0)
				mTabs->setCursorIndex(currentTab - 1);
			return true;
		}
		else if (config->isMappedTo("rightshoulder", input) || config->isMappedTo("pagedown", input))
		{
			int currentTab = mTabs->getCursorIndex();
			if (currentTab < mTabs->size() - 1)
				mTabs->setCursorIndex(currentTab + 1);
			return true;
		}
	}

	if (config->isMappedTo("up", input) && input.value != 0 && mMenu.getCursorIndex() == 0 && !mTabsHasFocus)
	{
		mTabsHasFocus = true;
		// Wait, visually, how to show focus? We might need to call mTabs->input?
		// Actually, let's just intercept left/right when focused.
		return true;
	}

	if (mTabsHasFocus)
	{
		if (config->isMappedTo("down", input) && input.value != 0)
		{
			mTabsHasFocus = false;
			return true;
		}
		if (mTabs->input(config, input))
			return true;
	}

	if (config->isMappedTo("x", input) && input.value != 0)
	{
		if (mFile != nullptr)
		{
			auto file = mFile;
			if (file != nullptr)
			{
				Window* window = mWindow;
				while (window->peekGui() && window->peekGui() != ViewController::get())
					delete window->peekGui();

				ViewController::get()->launch(file);
			}
		}

		return true;
	}

	return GuiSettings::input(config, input);
}
std::vector<HelpPrompt> GuiGameAchievements::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("l/r", _("TAB")));
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	if (mFile != nullptr)
		prompts.push_back(HelpPrompt("x", _("LAUNCH")));

	return prompts;
}
