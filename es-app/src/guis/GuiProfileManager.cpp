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
	std::string active = ProfileManager::getInstance()->getActiveProfile();

	for (const auto& name : profiles)
	{
		bool isActive = (name == active);
		std::string displayName = name + (isActive ? " [ACTIVE]" : "");
		
		mMenu.addEntry(displayName, false, [this, name] {
			// Notify server and update state
			// We do the HTTP call on the main thread for simplicity since we're going to 
			// show a splash screen / reload anyway.
			
			ProfileManager::getInstance()->setActiveProfile(name);
			
			// Show a simple message then reload everything
			mWindow->pushGui(new GuiMsgBox(mWindow, _("SWITCHING TO PROFILE: ") + name, _("OK"), [this, name] {
				// We use postToUiThread to escape the current callback stack
				mWindow->postToUiThread([this]() {
					// 1. Notify LAHEE (sync call is okay here because we are in a reload state)
					std::string active = ProfileManager::getInstance()->getActiveProfile();
					HttpReqOptions options;
					HttpReq request("http://127.0.0.1:8000/laheer/dorequest.php?r=laheeswitchuser&u=" + HttpReq::urlEncode(active), &options);
					request.wait();

					// 2. Full Native Reload (Exactly like changing a theme)
					ViewController::get()->reloadAll();

					// 3. Return to Main Menu
					Window* window = mWindow;
					window->pushGui(new GuiMenu(window, false));
				});
			}));
		}, isActive ? "iconFavorite" : "", false, false, name);
	}

	// Highlight current profile
	mMenu.getList()->setCursor(active);
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

void GuiProfileManager::openOptions(const std::string& profile)
{
	auto s = new GuiSettings(mWindow, profile + " " + _("OPTIONS"));
	s->addEntry(_("RENAME"), true, [this, s, profile] { renameProfile(profile); delete s; });
	s->addEntry(_("DELETE"), true, [this, s, profile] { deleteProfile(profile); delete s; });
	mWindow->pushGui(s);
}

void GuiProfileManager::deleteProfile(const std::string& profile)
{
	if (profile == "Player" && ProfileManager::getInstance()->getAvailableProfiles().size() == 1) {
		mWindow->pushGui(new GuiMsgBox(mWindow, _("CANNOT DELETE THE LAST PROFILE"), _("OK")));
		return;
	}

	mWindow->pushGui(new GuiMsgBox(mWindow, _("THIS WILL DELETE ALL SAVES FOR THIS USER. ARE YOU SURE?"), _("YES"), [this, profile] {
		if (ProfileManager::getInstance()->deleteProfile(profile)) populateList();
	}, _("NO"), nullptr));
}

void GuiProfileManager::renameProfile(const std::string& profile)
{
	mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("RENAME PROFILE"), profile, [this, profile](std::string newName) {
		if (ProfileManager::getInstance()->renameProfile(profile, newName)) populateList();
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
		auto selected = mMenu.getSelected();
		if (!selected.empty()) openOptions(selected);
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
