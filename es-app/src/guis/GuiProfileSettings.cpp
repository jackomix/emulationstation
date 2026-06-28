#include "guis/GuiProfileSettings.h"
#include "guis/GuiProfileSelect.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiMsgBox.h"
#include "components/SwitchComponent.h"
#include "Window.h"
#include "LocaleES.h"
#include "Settings.h"
#include "views/ViewController.h"
#include "Paths.h"

GuiProfileSettings::GuiProfileSettings(Window* window)
	: GuiSettings(window, _("PROFILES"))
{
	refreshMenu();
}

GuiProfileSettings::~GuiProfileSettings()
{
}

void GuiProfileSettings::refreshMenu()
{
	// Clear all rows
	mMenu.clear();

	auto pm = ProfileManager::getInstance();

	// Switch profile entry
	if (pm->isProfilesEnabled())
	{
		std::string activeName = pm->getActiveProfileName();
		addEntry(_("ACTIVE PROFILE: ") + activeName, true, [this]() {
			switchProfile();
		});
	}

	// Enable profiles switch
	auto enableProfiles = std::make_shared<SwitchComponent>(mWindow);
	enableProfiles->setState(pm->isProfilesEnabled());
	addSaveFunc([this, enableProfiles]() {
		bool wasEnabled = ProfileManager::getInstance()->isProfilesEnabled();
		bool nowEnabled = enableProfiles->getState();
		if (wasEnabled != nowEnabled)
		{
			ProfileManager::getInstance()->setProfilesEnabled(nowEnabled);
			Paths::recalculateProfilePaths();

			// If enabled and no profiles, trigger creation
			if (nowEnabled && ProfileManager::getInstance()->getProfiles().empty())
			{
				createProfile();
			}
			else
			{
				mWindow->postToUiThread([this]() {
					refreshMenu();
				});
			}
		}
	});
	addWithLabel(_("ENABLE MULTI-USER PROFILES"), enableProfiles);

	// Profile management entries
	addEntry(_("CREATE NEW PROFILE"), true, [this]() {
		createProfile();
	});

	if (!pm->getProfiles().empty())
	{
		addEntry(_("RENAME PROFILE"), true, [this]() {
			renameProfilePrompt();
		});

		addEntry(_("DELETE PROFILE"), true, [this]() {
			deleteProfilePrompt();
		});
	}
}

void GuiProfileSettings::switchProfile()
{
	close();
	
	mWindow->pushGui(new GuiProfileSelect(mWindow, nullptr));
}

void GuiProfileSettings::createProfile()
{
	auto updateVal = [this](std::string val) {
		if (val.empty())
			return;

		bool success = ProfileManager::getInstance()->createProfile(val);
		if (success)
		{
			// Refresh settings menu
			refreshMenu();
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

void GuiProfileSettings::renameProfilePrompt()
{
	auto pm = ProfileManager::getInstance();
	auto profiles = pm->getProfiles();

	auto s = new GuiSettings(mWindow, _("SELECT PROFILE TO RENAME"));
	for (const auto& p : profiles)
	{
		s->addEntry(p.name, true, [this, s, p]() {
			auto updateVal = [this, s, p](std::string val) {
				if (val.empty())
					return;

				bool success = ProfileManager::getInstance()->renameProfile(p.name, val);
				if (success)
				{
					s->close();
					refreshMenu();
				}
				else
				{
					mWindow->pushGui(new GuiMsgBox(mWindow, _("RENAME FAILED. NAME MIGHT BE IN USE OR INVALID."), _("OK")));
				}
			};

			if (Settings::getInstance()->getBool("UseOSK"))
				mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("Enter New Name"), p.name, updateVal, false));
			else
				mWindow->pushGui(new GuiTextEditPopup(mWindow, _("Enter New Name"), p.name, updateVal, false));
		});
	}
	mWindow->pushGui(s);
}

void GuiProfileSettings::deleteProfilePrompt()
{
	auto pm = ProfileManager::getInstance();
	auto profiles = pm->getProfiles();

	auto s = new GuiSettings(mWindow, _("SELECT PROFILE TO DELETE"));
	for (const auto& p : profiles)
	{
		s->addEntry(p.name, true, [this, s, p]() {
			mWindow->pushGui(new GuiMsgBox(mWindow, _("CONFIRM DELETE PROFILE: ") + p.name + _("? ALL SAVES AND DATA WILL BE LOST PERMANENTLY!"),
				_("YES"), [this, s, p]() {
					ProfileManager::getInstance()->deleteProfile(p.name);
					s->close();
					refreshMenu();
				},
				_("NO"), nullptr));
		});
	}
	mWindow->pushGui(s);
}
