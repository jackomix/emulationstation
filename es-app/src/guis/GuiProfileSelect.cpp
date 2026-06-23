#include "guis/GuiProfileSelect.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "Window.h"
#include "LocaleES.h"
#include "Settings.h"
#include "Log.h"
#include "Paths.h"
#include "resources/Font.h"
#include "renderers/Renderer.h"
#include "utils/StringUtil.h"

GuiProfileSelect::GuiProfileSelect(Window* window, const std::function<void()>& doneCallback)
	: GuiComponent(window), mBackground(window, ":/frame.png")
{
	mDoneCallback = doneCallback;
	mSelectedIndex = 0;

	// Set size to full screen
	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	auto theme = ThemeData::getMenuTheme();
	
	mBackground.setImagePath(theme->Background.path);
	mBackground.setEdgeColor(theme->Background.color);
	mBackground.setCenterColor(theme->Background.centerColor);
	mBackground.setCornerSize(theme->Background.cornerSize);

	mTitleText = std::make_shared<TextComponent>(mWindow, _("WHO'S PLAYING?"), theme->Title.font, 0xFFFFFFFF, ALIGN_CENTER);
	mInstructionsText = std::make_shared<TextComponent>(mWindow, _("SELECT A PROFILE TO LOAD YOUR SAVES"), theme->TextSmall.font, 0x888888FF, ALIGN_CENTER);

	addChild(mTitleText.get());
	addChild(mInstructionsText.get());

	populateProfiles();
}

GuiProfileSelect::~GuiProfileSelect()
{
}

void GuiProfileSelect::populateProfiles()
{
	mProfiles = ProfileManager::getInstance()->getProfiles();
	
	// Reset selection index if it went out of bounds
	if (mSelectedIndex < 0)
		mSelectedIndex = 0;
	if (mSelectedIndex > (int)mProfiles.size())
		mSelectedIndex = (int)mProfiles.size();

	updateSelection();
}

void GuiProfileSelect::updateSelection()
{
	// Can animate or update hints if needed
}

void GuiProfileSelect::onSizeChanged()
{
	GuiComponent::onSizeChanged();

	float screenW = mSize.x();
	float screenH = mSize.y();

	mBackground.fitTo(mSize, Vector3f::Zero(), Vector2f(-32, -32));

	mTitleText->setSize(screenW, 0);
	mTitleText->setPosition(0, screenH * 0.15f);

	mInstructionsText->setSize(screenW, 0);
	mInstructionsText->setPosition(0, screenH * 0.85f);
}

bool GuiProfileSelect::input(InputConfig* config, Input input)
{
	if (input.value == 0)
		return false;

	int totalTiles = (int)mProfiles.size() + 1;

	if (config->isMappedLike("left", input))
	{
		mSelectedIndex = (mSelectedIndex - 1 + totalTiles) % totalTiles;
		updateSelection();
		return true;
	}
	else if (config->isMappedLike("right", input))
	{
		mSelectedIndex = (mSelectedIndex + 1) % totalTiles;
		updateSelection();
		return true;
	}
	else if (config->isMappedTo(BUTTON_OK, input))
	{
		if (mSelectedIndex == (int)mProfiles.size())
		{
			// Clicked "+" (New Profile)
			createNewProfilePrompt();
		}
		else
		{
			// Selected profile
			std::string name = mProfiles[mSelectedIndex].name;
			
			ProfileManager::getInstance()->setActiveProfile(name);

			if (mDoneCallback)
			{
				mWindow->postToUiThread([cb = mDoneCallback]() {
					cb();
				});
			}

			delete this;
		}
		return true;
	}

	return false;
}

void GuiProfileSelect::createNewProfilePrompt()
{
	auto updateVal = [this](std::string val) {
		if (val.empty())
			return;

		bool success = ProfileManager::getInstance()->createProfile(val);
		if (success)
		{
			populateProfiles();
		}
		else
		{
			// Could show error msg, but just re-populate is fine
			populateProfiles();
		}
	};

	if (Settings::getInstance()->getBool("UseOSK"))
		mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("Enter Profile Name"), "", updateVal, false));
	else
		mWindow->pushGui(new GuiTextEditPopup(mWindow, _("Enter Profile Name"), "", updateVal, false));
}

void GuiProfileSelect::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	// Render background and title
	renderChildren(trans);

	float screenW = mSize.x();
	float screenH = mSize.y();

	// Calculate tile sizing based on screen size
	float tileW = screenW * 0.16f;
	if (tileW < 80.0f) tileW = 80.0f;
	float tileH = tileW;

	int totalTiles = (int)mProfiles.size() + 1;
	float spacing = screenW * 0.03f;
	float totalWidth = (totalTiles * tileW) + ((totalTiles - 1) * spacing);
	float startX = (screenW - totalWidth) / 2.0f;
	float startY = (screenH - tileH) / 2.0f;

	std::shared_ptr<Font> nameFont = Font::get(FONT_SIZE_MEDIUM);
	std::shared_ptr<Font> letterFont = Font::get(tileH * 0.5f);

	// Curated profile colors matching classic flat profile look
	unsigned int profileColors[] = {
		0x3498DBFF, // Blue
		0x2ECC71FF, // Green
		0xE74C3CFF, // Red
		0xF1C40FFF, // Yellow
		0x9B59B6FF, // Purple
		0x1ABC9CFF  // Teal
	};
	int numColors = sizeof(profileColors) / sizeof(profileColors[0]);

	for (int i = 0; i < totalTiles; i++)
	{
		float x = startX + i * (tileW + spacing);
		float y = startY;

		bool isSelected = (i == mSelectedIndex);
		unsigned int tileColor = 0x222222FF;
		unsigned int borderColor = isSelected ? 0xE50914FF : 0x444444FF; // Netflix red border on select
		float borderThickness = isSelected ? 4.0f : 1.5f;

		if (i < (int)mProfiles.size())
		{
			// Profile tile
			tileColor = profileColors[i % numColors];
			Renderer::drawSolidRectangle(x, y, tileW, tileH, tileColor, borderColor, borderThickness, 8.0f);

			// Draw initials in center
			std::string initStr = "";
			if (!mProfiles[i].name.empty())
				initStr = Utils::String::toUpper(mProfiles[i].name.substr(0, 1));
			
			Vector2f letterSize = letterFont->sizeText(initStr);
			TextCache* cacheLetter = letterFont->buildTextCache(initStr, x + (tileW - letterSize.x()) / 2.0f, y + (tileH - letterSize.y()) / 2.0f, 0xFFFFFFFF);
			if (cacheLetter) {
				letterFont->renderTextCache(cacheLetter);
				delete cacheLetter;
			}

			// Draw name below tile
			std::string nameStr = mProfiles[i].name;
			Vector2f nameSize = nameFont->sizeText(nameStr);
			TextCache* cacheName = nameFont->buildTextCache(nameStr, x + (tileW - nameSize.x()) / 2.0f, y + tileH + screenH * 0.02f, isSelected ? 0xFFFFFFFF : 0x888888FF);
			if (cacheName) {
				nameFont->renderTextCache(cacheName);
				delete cacheName;
			}
		}
		else
		{
			// "+" (Create Profile) tile
			Renderer::drawSolidRectangle(x, y, tileW, tileH, tileColor, borderColor, borderThickness, 8.0f);

			// Draw "+" in center
			float cx = x + tileW / 2.0f;
			float cy = y + tileH / 2.0f;
			float thickness = 4.0f;
			float length = tileW * 0.35f;

			// Horizontal bar
			Renderer::drawRect(cx - length / 2.0f, cy - thickness / 2.0f, length, thickness, isSelected ? 0xFFFFFFFF : 0x888888FF);
			// Vertical bar
			Renderer::drawRect(cx - thickness / 2.0f, cy - length / 2.0f, thickness, length, isSelected ? 0xFFFFFFFF : 0x888888FF);

			// Draw name below tile
			std::string labelStr = _("NEW PROFILE");
			Vector2f labelSize = nameFont->sizeText(labelStr);
			TextCache* cacheLabel = nameFont->buildTextCache(labelStr, x + (tileW - labelSize.x()) / 2.0f, y + tileH + screenH * 0.02f, isSelected ? 0xFFFFFFFF : 0x888888FF);
			if (cacheLabel) {
				nameFont->renderTextCache(cacheLabel);
				delete cacheLabel;
			}
		}
	}
}
