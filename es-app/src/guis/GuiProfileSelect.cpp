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
#include "components/ImageComponent.h"
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

		if (!isCreate) {
			mBackground = std::make_shared<NinePatchComponent>(window);
			mBackground->setImagePath(":/frame.png");
			mBackground->setCornerSize(6, 6);
			mBackground->setCenterColor(0x222222FF);
			mBackground->setEdgeColor(0x888888FF);
			addChild(mBackground.get());
		}

		mAvatar = std::make_shared<ImageComponent>(window);
		if (isCreate) {
			mAvatar->setImage(":/fav_add.svg");
			mAvatar->setColorShift(0x888888FF);
		} else {
			mAvatar->setImage(":/cartridge.svg"); 
		}
		
		float avatarSize = isCreate ? (Renderer::getScreenWidth() * 0.13f) : (Renderer::getScreenWidth() * 0.1f);
		mAvatar->setResize(avatarSize, avatarSize);
		addChild(mAvatar.get());

		if (!isCreate) {
			mName = std::make_shared<TextComponent>(window, profile.name, Font::get(FONT_SIZE_SMALL), theme->Text.color, ALIGN_CENTER);
			addChild(mName.get());
		}
	}

	void onSizeChanged() override
	{
		if (!mIsCreate) {
			mBackground->fitTo(mSize, Vector3f::Zero(), Vector2f::Zero());
			mBackground->setPosition(0, 0);
		}

		float avatarSize = mIsCreate ? (Renderer::getScreenWidth() * 0.13f) : (Renderer::getScreenWidth() * 0.1f);
		float textHeight = mName ? mName->getFont()->getLetterHeight() : 0;
		float totalHeight = avatarSize + (mName ? 10 + textHeight : 0);
		float startY = (mSize.y() - totalHeight) / 2;

		mAvatar->setPosition((mSize.x() - avatarSize) / 2, startY);
		
		if (mName) {
			float pad = mSize.x() * 0.10f;
			mName->setSize(mSize.x() - pad * 2, textHeight);
			mName->setPosition(pad, startY + avatarSize + 10);
		}
	}

	void onFocusGained() override
	{
		if (mIsCreate) {
			mAvatar->setColorShift(0xFFFFFFFF);
		} else {
			mBackground->setEdgeColor(0xFFFFFFFF);
		}
	}

	void onFocusLost() override
	{
		if (mIsCreate) {
			mAvatar->setColorShift(0x888888FF);
		} else {
			mBackground->setEdgeColor(0x888888FF);
		}
	}

	Profile getProfile() const { return mProfile; }
	bool isCreate() const { return mIsCreate; }

private:
	Profile mProfile;
	bool mIsCreate;
	std::shared_ptr<NinePatchComponent> mBackground;
	std::shared_ptr<ImageComponent> mAvatar;
	std::shared_ptr<TextComponent> mName;
};

GuiProfileSelect::GuiProfileSelect(Window* window, const std::function<void()>& doneCallback)
	: GuiComponent(window), mDoneCallback(doneCallback), mBypass(false)
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

	mGrid = std::make_shared<ComponentGrid>(window, Vector2i((int)mProfiles.size() + 1, 1));
	
	populateProfiles();

	mTitle = std::make_shared<TextComponent>(window, _("SELECT PROFILE"), Font::get(FONT_SIZE_LARGE), theme->Text.color, ALIGN_CENTER);

	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	setPosition(0, 0);

	addChild(mGrid.get());
	addChild(mTitle.get());
}

GuiProfileSelect::~GuiProfileSelect()
{
}

void GuiProfileSelect::onSizeChanged()
{
	if (mGrid && mTitle)
	{
		float titleH = mTitle->getFont()->getLetterHeight() + 8;
		float gap    = Renderer::getScreenHeight() * 0.08f;
		float gridH  = Renderer::getScreenHeight() * 0.30f;
		float blockH = titleH + gap + gridH;
		float startY = (mSize.y() - blockH) / 2.0f;

		mTitle->setSize(mSize.x(), titleH);
		mTitle->setPosition(0, startY);

		mGrid->setSize(mSize.x() * 0.90f, gridH);
		mGrid->setPosition(mSize.x() * 0.05f, startY + titleH + gap);
	}
}

void GuiProfileSelect::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
}

void GuiProfileSelect::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();
	Renderer::setMatrix(trans);
	Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0x000000CC);
	GuiComponent::render(parentTrans);
}

void GuiProfileSelect::populateProfiles()
{
	if (!mGrid) return;

	int cols = (int)mProfiles.size() + 1;
	float colW = 1.0f / cols;
	for (size_t i = 0; i <= mProfiles.size(); i++)
		mGrid->setColWidthPerc(i, colW);

	float cardW = (Renderer::getScreenWidth() * 0.90f / cols) * 0.85f;
	float cardH = Renderer::getScreenHeight() * 0.28f;

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
