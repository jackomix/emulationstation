#include "guis/GuiProfileManager.h"
#include "ProfileManager.h"
#include "Window.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiMenu.h"
#include "HttpReq.h"
#include "CollectionSystemManager.h"
#include "views/ViewController.h"

GuiProfileManager::GuiProfileManager(Window* window) : GuiSettings(window, _("PROFILE MANAGER").c_str())
{
	populateList();
}

GuiProfileManager::~GuiProfileManager()
{
}

void GuiProfileManager::populateList()
{
	mMenu.clear();
	auto profiles = ProfileManager::getInstance()->getAvailableProfiles();
	std::string activeId = ProfileManager::getInstance()->getActiveProfileId();

	for (const auto& profile : profiles)
	{
		bool isActive = (profile.id == activeId);
		std::string displayName = profile.name + (isActive ? " [ACTIVE]" : "");
		
		mMenu.addEntry(displayName, false, [this, profile] {
			// NON-BLOCKING SWITCH: Show a modal so user knows it is working
			auto msgBox = new GuiMsgBox(mWindow, _("SWITCHING USER..."), "", nullptr);
			mWindow->pushGui(msgBox);

			// Notify server in background thread to prevent UI hang
			ProfileManager::getInstance()->switchProfileAsync(mWindow, profile.id, [this, profile, msgBox]() {
				// 1. Full Native Reload (Exactly like changing a theme)
				ViewController::reloadAllGames(mWindow, true);
			});
		}, isActive ? "iconFavorite" : "", false, false, profile.id);
	}

	// Highlight current profile
	mMenu.getList()->setCursor(activeId);
}

void GuiProfileManager::createNewProfile()
{
	mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("CREATE NEW PROFILE"), "", [this](std::string name) {
		if (ProfileManager::getInstance()->createProfile(name)) {
			populateList();
		} else {
			mWindow->pushGui(new GuiMsgBox(mWindow, _("FAILED TO CREATE PROFILE OR ALREADY EXISTS"), _("OK")));
		}
	}, false));
}

void GuiProfileManager::openOptions(const std::string& id)
{
	auto s = new GuiSettings(mWindow, _("PROFILE OPTIONS"));
	s->addEntry(_("RENAME"), true, [this, s, id] { renameProfile(id); delete s; });
	s->addEntry(_("DELETE"), true, [this, s, id] { deleteProfile(id); delete s; });
	mWindow->pushGui(s);
}

void GuiProfileManager::deleteProfile(const std::string& id)
{
	if (id == "1" && ProfileManager::getInstance()->getAvailableProfiles().size() == 1) {
		mWindow->pushGui(new GuiMsgBox(mWindow, _("CANNOT DELETE THE LAST PROFILE"), _("OK")));
		return;
	}

	mWindow->pushGui(new GuiMsgBox(mWindow, _("THIS WILL DELETE ALL SAVES FOR THIS USER. ARE YOU SURE?"), _("YES"), [this, id] {
		if (ProfileManager::getInstance()->deleteProfile(id)) populateList();
	}, _("NO"), nullptr));
}

void GuiProfileManager::renameProfile(const std::string& id)
{
	mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("RENAME PROFILE"), "", [this, id](std::string newName) {
		if (ProfileManager::getInstance()->renameProfile(id, newName)) populateList();
	}, false));
}

bool GuiProfileManager::input(InputConfig* config, Input input)
{
	if (config->isMappedTo("x", input) && input.value)
	{
		createNewProfile();
		return true;
	}

	if (config->isMappedTo("y", input) && input.value)
	{
		auto selectedId = mMenu.getSelected();
		if (!selectedId.empty()) openOptions(selectedId);
		return true;
	}

	return GuiSettings::input(config, input);
}

std::vector<HelpPrompt> GuiProfileManager::getHelpPrompts()
{
	auto prompts = GuiSettings::getHelpPrompts();
	prompts.push_back(HelpPrompt("x", _("CREATE")));
	prompts.push_back(HelpPrompt("y", _("OPTIONS")));
	return prompts;
}
