#include "guis/GuiGameAchievements.h"
#include "components/WebImageComponent.h"
#include "components/TextComponent.h"
#include "components/ButtonComponent.h"
#include "components/MultiLineMenuEntry.h"
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
	GuiComponent(window), mGrid(window, Vector2i(1, 4)), mBackground(window, ":/frame.png")
{
	mRaInfo = ra;
	mActiveTab = 0;
	mFile = game != nullptr ? game : GuiRetroAchievements::getFileData(std::to_string(ra.ID));

	addChild(&mBackground);
	addChild(&mGrid);

	auto theme = ThemeData::getMenuTheme();
	mBackground.setImagePath(theme->Background.path);
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);
	mBackground.setPostProcessShader(theme->Background.menuShader);

	// Row 0: Header Grid (2x3) - title, subtitle, progress bar | game image
	mHeaderGrid = std::make_shared<ComponentGrid>(mWindow, Vector2i(2, 3));

	std::string titleText = ra.Title;
	
	mTitle = std::make_shared<TextComponent>(mWindow, titleText, theme->Title.font, theme->Title.color, ALIGN_LEFT);

	int totalPoints = 0;
	int userPoints = 0;
	for (auto game : mRaInfo.Achievements)
	{
		if (!game.DateEarned.empty() || !game.DateEarnedHardcore.empty())
			userPoints += Utils::String::toInteger(game.Points);
		totalPoints += Utils::String::toInteger(game.Points);
	}

	std::string subtitleText = "";
	if (mRaInfo.Achievements.size() == 0)
		subtitleText = _("THIS GAME HAS NO ACHIEVEMENTS YET");
	else
	{
		subtitleText = std::to_string(mRaInfo.NumAwardedToUser) + "/" + std::to_string(mRaInfo.NumAchievements) + " " + _("achievements");
		subtitleText += _U(" · ") + std::to_string(userPoints) + "/" + std::to_string(totalPoints) + " " + _("points");
	}

	mSubtitle = std::make_shared<TextComponent>(mWindow, subtitleText, theme->TextSmall.font, theme->Text.color, ALIGN_LEFT);
	
	mHeaderGrid->setEntry(mTitle, Vector2i(0, 0), false, true, Vector2i(1, 1));
	mHeaderGrid->setEntry(mSubtitle, Vector2i(0, 1), false, true, Vector2i(1, 1));

	mTitleImage = std::make_shared<WebImageComponent>(mWindow);
	mTitleImage->setImage(ra.getImageUrl());
	mHeaderGrid->setEntry(mTitleImage, Vector2i(1, 0), false, false, Vector2i(1, 3));

	if (ra.Achievements.size() > 0)
	{
		int percent = Math::round(ra.NumAwardedToUser * 100.0f / ra.Achievements.size());
		char trstring[256];
		snprintf(trstring, 256, _("%d%% complete").c_str(), percent);
		mProgress = std::make_shared<RetroAchievementProgress>(mWindow, ra.NumAwardedToUser, ra.NumAwardedToUserHardcore, ra.Achievements.size(), Utils::String::trim(trstring));
		mHeaderGrid->setEntry(mProgress, Vector2i(0, 2), false, true, Vector2i(1, 1));
	}

	mGrid.setEntry(mHeaderGrid, Vector2i(0, 0), false, true);

	// Row 1: Tabs
	mTabs = std::make_shared<ComponentTab>(mWindow);
	mTabs->addTab(_("ACHIEVEMENTS"));
	mTabs->addTab(_("ACTIVITY"));
	mTabs->addTab(_("INFO"));
	mTabs->addTab(_("OPTIONS"));

	if (mFile != nullptr)
		mOptionsUI = std::make_shared<GuiGameOptions>(mWindow, mFile, true);

	mTabs->setCursorChangedCallback([this](const CursorState& state) {
		if (mActiveTab != mTabs->getCursorIndex()) {
			mActiveTab = mTabs->getCursorIndex();
			populateTabContent();
		}
	});

	mGrid.setEntry(mTabs, Vector2i(0, 1), false, true);

	// Row 2: Content List
	mList = std::make_shared<ComponentList>(mWindow);
	mList->setUpdateType(ComponentListFlags::UPDATE_ALWAYS);
	mGrid.setEntry(mList, Vector2i(0, 2), true, true);

	// Row 3: Buttons
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



	centerWindow();
	populateTabContent();
}

void GuiGameAchievements::onSizeChanged()
{
	GuiComponent::onSizeChanged();

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	
	mGrid.setSize(mSize);

	const float titleHeight = mTitle->getFont()->getLetterHeight() * 1.5f;
	const float subtitleHeight = mSubtitle->getFont()->getLetterHeight() * 1.5f;
	const float progressHeight = Renderer::getScreenHeight() * 0.05f;

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
	}

	centerWindow();
}

void GuiGameAchievements::populateAchievementsTab()
{
	for (auto game : mRaInfo.Achievements)
	{
		ComponentListRow row;
		auto itstring = std::make_shared<GameAchievementEntry>(mWindow, game);
		row.addElement(itstring, true);
		mList->addRow(row);
	}
}

void GuiGameAchievements::populatePlayHistoryTab()
{
	if (mFile == nullptr) return;
	auto theme = ThemeData::getMenuTheme();
	
	// Play Time
	ComponentListRow rowTime;
	auto lblTime = std::make_shared<TextComponent>(mWindow, _("PLAY TIME"), theme->Text.font, theme->Text.color);
	auto valTime = std::make_shared<TextComponent>(mWindow, Utils::Time::secondsToString(Utils::String::toInteger(mFile->getMetadata(MetaDataId::GameTime))), theme->Text.font, theme->Text.color);
	valTime->setHorizontalAlignment(ALIGN_RIGHT);
	rowTime.addElement(lblTime, true);
	rowTime.addElement(valTime, false);
	mList->addRow(rowTime);
	
	// Play Count
	ComponentListRow rowCount;
	auto lblCount = std::make_shared<TextComponent>(mWindow, _("PLAY COUNT"), theme->Text.font, theme->Text.color);
	auto valCount = std::make_shared<TextComponent>(mWindow, mFile->getMetadata(MetaDataId::PlayCount), theme->Text.font, theme->Text.color);
	valCount->setHorizontalAlignment(ALIGN_RIGHT);
	rowCount.addElement(lblCount, true);
	rowCount.addElement(valCount, false);
	mList->addRow(rowCount);
	
	// Last Played
	ComponentListRow rowLast;
	auto lblLast = std::make_shared<TextComponent>(mWindow, _("LAST PLAYED"), theme->Text.font, theme->Text.color);
	auto valLast = std::make_shared<TextComponent>(mWindow, mFile->getMetadata(MetaDataId::LastPlayed), theme->Text.font, theme->Text.color);
	valLast->setHorizontalAlignment(ALIGN_RIGHT);
	rowLast.addElement(lblLast, true);
	rowLast.addElement(valLast, false);
	mList->addRow(rowLast);
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
		auto valDesc = std::make_shared<TextComponent>(mWindow, desc, theme->TextSmall.font, theme->Text.color);
		valDesc->setSize(mList->getSize().x(), 0);
		rowDesc.addElement(valDesc, true);
		mList->addRow(rowDesc);
	}
}

void GuiGameAchievements::render(const Transform4x4f& parentTrans)
{
	GuiComponent::render(parentTrans);
}

bool GuiGameAchievements::input(InputConfig* config, Input input)
{
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



	if (mTabs->input(config, input))
		return true;

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
