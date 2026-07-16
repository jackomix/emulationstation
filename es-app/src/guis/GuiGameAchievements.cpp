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
	GuiSettings(window, _("ACHIEVEMENTS"), ([&ra, game]() {
		std::string title = game != nullptr ? game->getName() : ra.Title;
		if (ra.isOfflineData) {
			title += " (\U0001F4E6 Offline Data)";
		}
		return title;
	})(), nullptr)
{
	// Required for WebImageComponent
	setUpdateType(ComponentListFlags::UPDATE_ALWAYS);

	setTitle(game != nullptr ? game->getName() : ra.Title);

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

	int totalPoints = 0;
	int userPoints = 0;

	for (auto game : ra.Achievements)
	{
		if (!game.DateEarned.empty() || !game.DateEarnedHardcore.empty())
			userPoints += Utils::String::toInteger(game.Points);

		totalPoints += Utils::String::toInteger(game.Points);
	}

	if (ra.Achievements.size() == 0)
		mAchievementSubtitle = _("THIS GAME HAS NO ACHIEVEMENTS YET");
	else
	{
		auto txt = _("Achievements (softcore)") + ": \t" + std::to_string(ra.NumAwardedToUser) + "/" + std::to_string(ra.NumAchievements);
		txt += "\r\n" + _("Achievements (hardcore)") + ": \t" + std::to_string(ra.NumAwardedToUserHardcore) + "/" + std::to_string(ra.NumAchievements);
		txt += "\r\n" + _("Points") + ": \t" + std::to_string(userPoints) + "/" + std::to_string(totalPoints);
		txt += "\r\n ";

		mAchievementSubtitle = txt;
	}

	auto image = std::make_shared<WebImageComponent>(mWindow);
	if (ra.ID != 0)
		image->setImage(ra.getImageUrl());
	else if (game != nullptr)
		image->setImage(game->getImagePath());
	setTitleImage(image);

	if (ra.Achievements.size() > 0)
	{
		int percent = Math::round(ra.NumAwardedToUser * 100.0f / ra.Achievements.size());

		char trstring[256];
		snprintf(trstring, 256, _("%d%% complete").c_str(), percent);
		mProgress = std::make_shared<RetroAchievementProgress>(mWindow, ra.NumAwardedToUser, ra.NumAwardedToUserHardcore, ra.Achievements.size(), Utils::String::trim(trstring));
	}

	auto theme = ThemeData::getMenuTheme();

	if (ra.Achievements.size() == 0)
	{
		auto text = std::make_shared<TextComponent>(mWindow, _("No achievements"), theme->Text.font, theme->Text.color);
		text->setOpacity(128);
		text->setHorizontalAlignment(ALIGN_CENTER);
		ComponentListRow row;
		row.addElement(text, false);
		mAchievementRows.push_back(row);
	}
	else
	{
		for (auto achievement : ra.Achievements)
		{
			ComponentListRow row;
			auto itstring = std::make_shared<GameAchievementEntry>(mWindow, achievement);
			row.addElement(itstring, true);
			mAchievementRows.push_back(row);
		}
	}

	updateTab();
	centerWindow();	
}

void GuiGameAchievements::updateTab()
{
	mMenu.clear();

	if (mActiveTab == 0)
		mMenu.setSubTitle(mAchievementSubtitle);
	else
		mMenu.setSubTitle("");

	auto theme = ThemeData::getMenuTheme();

	// Tab Bar UI
	auto grid = std::make_shared<ComponentGrid>(mWindow, Vector2i(2, 1));
	
	auto leftTab = std::make_shared<TextComponent>(mWindow, _("ACHIEVEMENTS"), theme->Text.font, 
		mActiveTab == 0 ? theme->Background.color : theme->Text.color, ALIGN_CENTER);
	if (mActiveTab == 0) leftTab->setBackgroundColor(theme->Text.color);

	auto rightTab = std::make_shared<TextComponent>(mWindow, _("PLAY HISTORY"), theme->Text.font, 
		mActiveTab == 1 ? theme->Background.color : theme->Text.color, ALIGN_CENTER);
	if (mActiveTab == 1) rightTab->setBackgroundColor(theme->Text.color);

	grid->setEntry(leftTab, Vector2i(0, 0), false, true);
	grid->setEntry(rightTab, Vector2i(1, 0), false, true);
	
	float h = leftTab->getSize().y() + Renderer::getScreenHeight() * 0.02f;
	grid->setSize(WINDOW_WIDTH, h);

	ComponentListRow tabRow;
	tabRow.selectable = false;
	tabRow.addElement(grid, false);
	addRow(tabRow);

	if (mActiveTab == 0) {
		for (auto& row : mAchievementRows)
			addRow(row);
	} else {
		auto text = std::make_shared<TextComponent>(mWindow, _("No play history found"), theme->Text.font, theme->Text.color);
		text->setOpacity(128);
		text->setHorizontalAlignment(ALIGN_CENTER);
		ComponentListRow row;
		row.addElement(text, false);
		addRow(row);
	}
}

void GuiGameAchievements::centerWindow()
{
	float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));

	if (Renderer::ScreenSettings::fullScreenMenus())
		mMenu.setSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
	else
		mMenu.setSize(WINDOW_WIDTH, Renderer::getScreenHeight() * 0.901f);

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
}

void GuiGameAchievements::render(const Transform4x4f& parentTrans)
{
	GuiSettings::render(parentTrans);

	auto theme = ThemeData::getMenuTheme();
	float yBase = mMenu.getTitleHeight();
	
	Transform4x4f trans = parentTrans * mMenu.getTransform();

	float statsX = mMenu.getSize().x() * 0.04f;

	float progY = yBase + (theme->TextSmall.font->sizeText("A", 1.1f).y() * 3.3f);

	if (mProgress != nullptr && mActiveTab == 0)
	{
		float h = theme->TextSmall.font->sizeText("A8O\rA8O", 1.1).y();
		float width = (float)Math::min((int)Renderer::getScreenHeight(), (int)(Renderer::getScreenWidth() * 0.90f));
		float iw = mMenu.getTitleHeight() / width;
		float xx = mMenu.getSize().x() - (mMenu.getSize().x() * iw);

		mProgress->setPosition(statsX, progY);
		mProgress->setSize(xx * 0.45f, h);

		mProgress->render(trans);
	}
}

bool GuiGameAchievements::input(InputConfig* config, Input input)
{
	if (input.value != 0 && (config->isMappedTo("pageup", input) || config->isMappedTo("pagedown", input) || config->isMappedTo("l1", input) || config->isMappedTo("r1", input) || config->isMappedTo("leftshoulder", input) || config->isMappedTo("rightshoulder", input)))
	{
		mActiveTab = (mActiveTab == 0) ? 1 : 0;
		updateTab();
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
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));

	if (mFile != nullptr)
		prompts.push_back(HelpPrompt("x", _("LAUNCH")));

	return prompts;
}
