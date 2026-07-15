#include "guis/GuiProfileSelect.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiMenu.h"
#include "Window.h"
#include "LocaleES.h"
#include "Settings.h"
#include "Log.h"
#include "ThemeData.h"
#include "views/ViewController.h"
#include "components/TextComponent.h"
#include "components/WebImageComponent.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "renderers/Renderer.h"

class ProfileCard : public GuiComponent
{
public:
	ProfileCard(Window* window, const Profile& profile, bool isCreate = false) 
		: GuiComponent(window), mProfile(profile), mIsCreate(isCreate)
	{
		auto theme = ThemeData::getMenuTheme();

		mBackground = std::make_shared<NinePatchComponent>(window);
		mBackground->setImagePath(theme->Background.path.empty() ? ":/frame.png" : theme->Background.path);
		mBackground->setCenterColor(theme->Background.color);
		mBackground->setEdgeColor(theme->Background.color);
		addChild(mBackground.get());

		mAvatar = std::make_shared<WebImageComponent>(window, 0);
		if (isCreate) {
			mAvatar->setImage(":/help/plus.svg");
		} else {
			mAvatar->setImage(":/avatar_default.svg"); // Could use profile.avatarUrl if existed
		}
		
		float avatarSize = Renderer::getScreenWidth() * 0.1f;
		mAvatar->setResize(avatarSize, avatarSize);
		addChild(mAvatar.get());

		std::string text = isCreate ? _("CREATE NEW") : profile.name;
		mName = std::make_shared<TextComponent>(window, text, theme->Text.font, theme->Text.color, ALIGN_CENTER);
		addChild(mName.get());
	}

	void onSizeChanged() override
	{
		mBackground->fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
		mBackground->setPosition(0, 0);

		float avatarSize = Renderer::getScreenWidth() * 0.1f;
		mAvatar->setPosition((mSize.x() - avatarSize) / 2, 20);
		mName->setPosition(0, 20 + avatarSize + 10);
		mName->setSize(mSize.x(), mName->getFont()->getLetterHeight());
	}

	void onFocusGained() override
	{
		auto theme = ThemeData::getMenuTheme();
		mBackground->setEdgeColor(theme->Text.color);
	}

	void onFocusLost() override
	{
		auto theme = ThemeData::getMenuTheme();
		mBackground->setEdgeColor(theme->Background.color);
	}

	Profile getProfile() const { return mProfile; }
	bool isCreate() const { return mIsCreate; }

private:
	Profile mProfile;
	bool mIsCreate;
	std::shared_ptr<NinePatchComponent> mBackground;
	std::shared_ptr<WebImageComponent> mAvatar;
	std::shared_ptr<TextComponent> mName;
};

GuiProfileSelect::GuiProfileSelect(Window* window, const std::function<void()>& doneCallback)
	: GuiComponent(window), mDoneCallback(doneCallback), mBackground(window), mBypass(false)
{
	mProfiles = ProfileManager::getInstance()->getProfiles();

	bool autoLogin = Settings::getInstance()->getBool("AutoLoginProfile"); // Assuming this is the key
	if (mProfiles.size() <= 1 || autoLogin)
	{
		mBypass = true;
		mWindow->postToUiThread([this]() {
			std::string active = ProfileManager::getInstance()->getActiveProfileName();
			if (active.empty() && !mProfiles.empty()) active = mProfiles.front().name;
			
			if (!active.empty()) selectProfile(Profile{active});

			if (mDoneCallback) mDoneCallback();
			delete this;
		});
		return;
	}

	auto theme = ThemeData::getMenuTheme();

	mBackground.setImagePath(theme->Background.path.empty() ? ":/frame.png" : theme->Background.path);
	mBackground.setCenterColor(theme->Background.color);
	mBackground.setEdgeColor(theme->Background.color);

	mGrid = std::make_shared<ComponentGrid>(window, Vector2i((int)mProfiles.size() + 1, 1));
	
	populateProfiles();

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	setPosition(0, 0);

	addChild(&mBackground);
	addChild(mGrid.get());
}

GuiProfileSelect::~GuiProfileSelect()
{
}

void GuiProfileSelect::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	
	if (mGrid)
	{
		float gridH = Renderer::getScreenHeight() * 0.3f;
		mGrid->setSize(mSize.x(), gridH);
		mGrid->setPosition(0, (mSize.y() - gridH) / 2);
	}
}

void GuiProfileSelect::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
}

void GuiProfileSelect::populateProfiles()
{
	if (!mGrid) return;

	float colW = 1.0f / (mProfiles.size() + 1);
	for (size_t i = 0; i <= mProfiles.size(); i++)
		mGrid->setColWidthPerc(i, colW);

	float cardW = Renderer::getScreenWidth() * 0.2f;
	float cardH = Renderer::getScreenHeight() * 0.3f;

	for (size_t i = 0; i < mProfiles.size(); i++)
	{
		auto card = std::make_shared<ProfileCard>(mWindow, mProfiles[i]);
		card->setSize(cardW, cardH);
		mGrid->setEntry(card, Vector2i(i, 0), true, true);
	}

	// Create New
	auto createCard = std::make_shared<ProfileCard>(mWindow, Profile{}, true);
	createCard->setSize(cardW, cardH);
	mGrid->setEntry(createCard, Vector2i(mProfiles.size(), 0), true, true);
}

void GuiProfileSelect::selectProfile(const Profile& profile)
{
	ProfileManager::getInstance()->setActiveProfile(profile.name);
	
	// Reset view state (clear filters, go to system view)
	ViewController::get()->goToSystemView(ViewController::get()->getState().getSystem());
	ViewController::get()->reloadAll(mWindow);
}

void GuiProfileSelect::showProfileOptions(const Profile& profile)
{
	mWindow->pushGui(new GuiMsgBox(mWindow, _("DELETE PROFILE?"), _("YES"), [this, profile]() {
		deleteProfile(profile);
	}, _("NO"), nullptr));
}

void GuiProfileSelect::deleteProfile(const Profile& profile)
{
	std::string active = ProfileManager::getInstance()->getActiveProfileName();
	if (active == profile.name) {
		auto profiles = ProfileManager::getInstance()->getProfiles();
		if (!profiles.empty()) {
			selectProfile(profiles.front());
		}
	}
	
	mProfiles = ProfileManager::getInstance()->getProfiles();
	if (mGrid) mGrid->removeEntry(mGrid->getSelectedComponent());
}

void GuiProfileSelect::createNewProfilePrompt()
{
	auto updateVal = [this](std::string val) {
		if (val.empty()) return;

		bool success = ProfileManager::getInstance()->createProfile(val);
		if (success)
		{
			selectProfile(Profile{val});

			if (mDoneCallback)
			{
				mWindow->postToUiThread([cb = mDoneCallback]() {
					cb();
				});
			}
			delete this;
		}
		else
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, _("PROFILE CREATION FAILED. NAME MIGHT BE IN USE OR INVALID."), _("OK")));
		}
	};

	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("Enter Profile Name"), "", updateVal, false));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, _("Enter Profile Name"), "", updateVal, false));
}

bool GuiProfileSelect::input(InputConfig* config, Input input)
{
	if (mBypass) return true;

	if (config->isMappedTo("a", input) && input.value != 0)
	{
		auto comp = mGrid->getSelectedComponent();
		auto card = std::static_pointer_cast<ProfileCard>(comp);
		if (card->isCreate())
		{
			createNewProfilePrompt();
		}
		else
		{
			selectProfile(card->getProfile());
			if (mDoneCallback) mDoneCallback();
			delete this;
		}
		return true;
	}
	
	if (config->isMappedTo("x", input) && input.value != 0)
	{
		auto comp = mGrid->getSelectedComponent();
		auto card = std::static_pointer_cast<ProfileCard>(comp);
		if (!card->isCreate())
		{
			showProfileOptions(card->getProfile());
		}
		return true;
	}
	
	return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiProfileSelect::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("left/right", _("SELECT")));
	prompts.push_back(HelpPrompt("a", _("CHOOSE")));
	prompts.push_back(HelpPrompt("x", _("OPTIONS")));
	return prompts;
}
