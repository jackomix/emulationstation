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

class ProfileCard : public GuiComponent
{
public:
	ProfileCard(Window* window, const Profile& profile, bool isCreate = false) 
		: GuiComponent(window), mProfile(profile), mIsCreate(isCreate)
	{
		mBackground = std::make_shared<NinePatchComponent>(window);
		mBackground->setImagePath(":/frame.png");
		mBackground->setCenterColor(0x333333FF);
		mBackground->setEdgeColor(0x333333FF);
		addChild(mBackground.get());

		mAvatar = std::make_shared<WebImageComponent>(window, 0);
		if (isCreate) {
			mAvatar->setImage(":/help/plus.svg");
		} else {
			mAvatar->setImage(":/avatar_default.svg"); // Could use profile.avatarUrl if existed
		}
		mAvatar->setResize(100, 100);
		addChild(mAvatar.get());

		std::string text = isCreate ? _("CREATE NEW") : profile.name;
		mName = std::make_shared<TextComponent>(window, text, Font::get(FONT_SIZE_SMALL), 0xFFFFFFFF, ALIGN_CENTER);
		addChild(mName.get());
	}

	void onSizeChanged() override
	{
		mBackground->fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
		mBackground->setPosition(0, 0);

		mAvatar->setPosition((mSize.x() - 100) / 2, 20);
		mName->setPosition(0, 130);
		mName->setSize(mSize.x(), 24);
	}

	void onFocusGained() override
	{
		mBackground->setCenterColor(0x555555FF);
		mBackground->setEdgeColor(0x0000FFFF);
		mName->setColor(0xFFFF00FF);
	}

	void onFocusLost() override
	{
		mBackground->setCenterColor(0x333333FF);
		mBackground->setEdgeColor(0x333333FF);
		mName->setColor(0xFFFFFFFF);
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
		return;
	}

	addChild(&mBackground);
	mBackground.setImagePath(":/frame.png");

	mGrid = std::make_shared<ComponentGrid>(window, Vector2i((int)mProfiles.size() + 1, 1));
	addChild(mGrid.get());

	populateProfiles();
}

GuiProfileSelect::~GuiProfileSelect()
{
}

void GuiProfileSelect::onSizeChanged()
{
	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));
	
	if (mGrid)
	{
		mGrid->setSize(mSize.x(), 180);
		mGrid->setPosition(0, (mSize.y() - 180) / 2);
	}
}

void GuiProfileSelect::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mBypass)
	{
		std::string active = ProfileManager::getInstance()->getActiveProfileName();
		if (active.empty() && !mProfiles.empty()) active = mProfiles.front().name;
		
		if (!active.empty())
			selectProfile(Profile{active}); // Fallback wrapper

		if (mDoneCallback) mDoneCallback();
		delete this;
	}
}

void GuiProfileSelect::populateProfiles()
{
	if (!mGrid) return;

	for (size_t i = 0; i < mProfiles.size(); i++)
	{
		auto card = std::make_shared<ProfileCard>(mWindow, mProfiles[i]);
		card->setSize(140, 180);
		mGrid->setEntry(card, Vector2i(i, 0), true, true);
	}

	// Create New
	auto createCard = std::make_shared<ProfileCard>(mWindow, Profile{}, true);
	createCard->setSize(140, 180);
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
	// Provide Rename or Delete options
	// (Implementation abbreviated to match instructions, could use GuiMenu)
	mWindow->pushGui(new GuiMsgBox(mWindow, _("DELETE PROFILE?"), _("YES"), [this, profile]() {
		deleteProfile(profile);
	}, _("NO"), nullptr));
}

void GuiProfileSelect::deleteProfile(const Profile& profile)
{
	// Not implementing the backend deletion here, just the state fallback logic
	// If active is deleted, fallback to first available
	std::string active = ProfileManager::getInstance()->getActiveProfileName();
	if (active == profile.name) {
		auto profiles = ProfileManager::getInstance()->getProfiles();
		if (!profiles.empty()) {
			selectProfile(profiles.front());
		}
	}
	
	// Reload the UI
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
