#include "GuiRetroAchievementsSettings.h"
#include "SystemConf.h"
#include "Settings.h"
#include "ApiSystem.h"
#include "RetroAchievements.h"
#include "utils/FileSystemUtil.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "LocaleES.h"

GuiRetroAchievementsSettings::GuiRetroAchievementsSettings(Window* window) : GuiSettings(window, _("RETROACHIEVEMENT PROFILES").c_str())
{
	// We are now in NATIVE MODE. We scan the SD card directly.
	auto hubPath = RetroAchievements::getRetroAchievementsHubPath();
	bool isIntegrated = !hubPath.empty();

	addGroup(_("GENERAL SETTINGS"));

	// Master Toggle - Always available
	addSwitch(_("ENABLE RETROACHIEVEMENTS"), _("Enable local achievement tracking for supported systems."), "global.retroachievements", true);
	addSwitch(_("HARDCORE MODE"), _("Disable save states and cheats to earn hardcore achievements."), "global.retroachievements.hardcore", false);

	if (isIntegrated)
	{
		addGroup(_("ACTIVE PROFILE"));
		
		auto userDir = hubPath + "/User";
		auto currentUsername = SystemConf::getInstance()->get("global.retroachievements.username");
		if (currentUsername.empty()) currentUsername = "Player";

		auto profile_choices = std::make_shared<OptionListComponent<std::string>>(mWindow, _("SELECT PROFILE"), false);
		
		// Direct file scan of the SD card
		auto files = Utils::FileSystem::getDirContent(userDir);
		bool currentFound = false;
		for (auto file : files)
		{
			if (Utils::FileSystem::getExtension(file) == ".json")
			{
				std::string name = Utils::FileSystem::getStem(file);
				profile_choices->add(name, name, currentUsername == name);
				if (currentUsername == name) currentFound = true;
			}
		}

		// Fallback for first-time use
		if (!currentFound)
		{
			profile_choices->add("Player", "Player", true);
			SystemConf::getInstance()->set("global.retroachievements.username", "Player");
		}

		addWithLabel(_("CURRENT USER"), profile_choices);

		addEntry(_("CREATE NEW PROFILE"), true, [this, profile_choices, userDir]
		{
			mWindow->pushGui(new GuiTextEditPopup(mWindow, _("NEW PROFILE NAME"), "", [this, profile_choices, userDir](std::string name)
			{
				if (name.empty()) return;
				// UI feedback: add to list immediately
				profile_choices->add(name, name, true);
				SystemConf::getInstance()->set("global.retroachievements.username", name);
				
				// LAHEE handles the actual file creation on next heartbeat/switch
				HttpReqOptions options;
				HttpReq request("http://127.0.0.1:8000/laheer/dorequest.php?r=laheeswitchuser&u=" + HttpReq::urlEncode(name), &options);
				request.wait();
			}, false));
		});

		addGroup(_("ENGINE STATUS"));
		
		// Heartbeat check
		bool isOnline = RetroAchievements::isLocalEngineActive();
		addEntry(_("LAHEE ENGINE"), false, nullptr, isOnline ? _("ONLINE") : _("STARTING..."));
		
		if (!isOnline)
		{
			addEntry(_("RESTART ENGINE"), true, [this]
			{
				// Kill any stuck PID and try again
				Utils::FileSystem::removeFile("/tmp/lahee.pid");
				ApiSystem::getInstance()->executeSystem("killall LAHEE");
				// The main loop will handle restart or we can trigger it here
				delete this;
			});
		}

		// Save handler for profile switching
		addSaveFunc([profile_choices] 
		{ 
			std::string selected = profile_choices->getSelected();
			if (SystemConf::getInstance()->get("global.retroachievements.username") != selected)
			{
				SystemConf::getInstance()->set("global.retroachievements.username", selected);
				
				// Notify the engine
				HttpReqOptions options;
				HttpReq request("http://127.0.0.1:8000/laheer/dorequest.php?r=laheeswitchuser&u=" + HttpReq::urlEncode(selected), &options);
				request.wait(); 
			}
		});
	}
	else
	{
		// Fallback for remote/legacy mode
		addGroup(_("REMOTE SERVER SETTINGS"));
		addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
		addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);
	}

	addSwitch(_("SHOW IN MENU"), _("Show RetroAchievements in the main menu."), "RetroachievementsMenuitem", true);
}
