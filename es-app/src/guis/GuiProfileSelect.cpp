#include "guis/GuiProfileSelect.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiMsgBox.h"
#include "Window.h"
#include "LocaleES.h"
#include "Settings.h"
#include "Log.h"
#include "ThemeData.h"

GuiProfileSelect::GuiProfileSelect(Window* window, const std::function<void()>& doneCallback)
	: GuiSettings(window, _("SELECT PROFILE")), mDoneCallback(doneCallback)
{
	populateProfiles();
}

GuiProfileSelect::~GuiProfileSelect()
{
}

void GuiProfileSelect::populateProfiles()
{
	mMenu.clear();

	mProfiles = ProfileManager::getInstance()->getProfiles();

	for (const auto& profile : mProfiles)
	{
		addEntry(profile.name, true, [this, profile]() {
			ProfileManager::getInstance()->setActiveProfile(profile.name);

			if (mDoneCallback)
			{
				mWindow->postToUiThread([cb = mDoneCallback]() {
					cb();
				});
			}

			close();
		});
	}

	addEntry(_("CREATE NEW PROFILE"), true, [this]() {
		createNewProfilePrompt();
	});
}

void GuiProfileSelect::createNewProfilePrompt()
{
	auto updateVal = [this](std::string val) {
		if (val.empty())
			return;

		bool success = ProfileManager::getInstance()->createProfile(val);
		if (success)
		{
			ProfileManager::getInstance()->setActiveProfile(val);

			if (mDoneCallback)
			{
				mWindow->postToUiThread([cb = mDoneCallback]() {
					cb();
				});
			}
			
			close();
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
