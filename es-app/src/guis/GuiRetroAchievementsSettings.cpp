#include "GuiRetroAchievementsSettings.h"
#include "SystemConf.h"
#include "Settings.h"
#include "ApiSystem.h"
#include "RetroAchievements.h"
#include "utils/FileSystemUtil.h"
#include "utils/Platform.h"
#include "HttpReq.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"
#include "components/TextComponent.h"
#include "LocaleES.h"

GuiRetroAchievementsSettings::GuiRetroAchievementsSettings(Window* window) : GuiSettings(window, _("RETROACHIEVEMENT PROFILES").c_str())
{
	addGroup(_("GENERAL SETTINGS"));

	// We are now in NATIVE MODE. We scan the SD card directly.
	auto hubPath = RetroAchievements::getRetroAchievementsHubPath();
	bool isIntegrated = !hubPath.empty();

	// Master Toggle - Always available
	addSwitch(_("ENABLE RETROACHIEVEMENTS"), _("Enable local achievement tracking for supported systems."), "global.retroachievements", true, nullptr);
	addSwitch(_("HARDCORE MODE"), _("Disable save states and cheats to earn hardcore achievements."), "global.retroachievements.hardcore", false, nullptr);

	if (isIntegrated)
	{
		addGroup(_("ACTIVE PROFILE"));
		
		auto userDir = hubPath + "/User";
		auto currentUsername = SystemConf::getInstance()->get("global.retroachievements.username");
		if (currentUsername.empty() || currentUsername == "0") currentUsername = "Player";

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
				SystemConf::getInstance()->saveSystemConf(); // WRITE TO DISK IMMEDIATELY
				
				// LAHEE handles the actual file creation on next heartbeat/switch
				HttpReqOptions options;
				HttpReq request("http://127.0.0.1:8000/laheer/dorequest.php?r=laheeswitchuser&u=" + HttpReq::urlEncode(name), &options);
				request.wait();
			}, false));
		});

		addGroup(_("TOOLS"));
		
		addEntry(_("PATCH RETROARCH"), true, [this, hubPath]
		{
			// Use executeScript for shell expansion and pipe logging to Hub
			std::string cmd = "mount -o remount,rw / ; python3 \"" + hubPath + "/Server/lahee_patch_ra.py\" > \"" + hubPath + "/patch_ra.log\" 2>&1 ; mount -o remount,ro /";
			std::string fullCmd = "sh -c \"" + cmd + "\"";
			int ret = Utils::Platform::ProcessStartInfo(fullCmd).run();
			
			std::string msg = (ret == 0) ? _("RETROARCH PATCHED. PLEASE RESTART.") : _("PATCH FAILED. CHECK patch_ra.log IN HUB.");
			mWindow->pushGui(new GuiMsgBox(mWindow, msg, _("OK"), nullptr));
		});

		addEntry(_("UNPATCH RETROARCH"), true, [this, hubPath]
		{
			std::string cmd = "mount -o remount,rw / ; python3 \"" + hubPath + "/Server/lahee_unpatch_ra.py\" > \"" + hubPath + "/unpatch_ra.log\" 2>&1 ; mount -o remount,ro /";
			std::string fullCmd = "sh -c \"" + cmd + "\"";
			int ret = Utils::Platform::ProcessStartInfo(fullCmd).run();
			
			std::string msg = (ret == 0) ? _("RETROARCH UNPATCHED.") : _("UNPATCH FAILED. CHECK unpatch_ra.log IN HUB.");
			mWindow->pushGui(new GuiMsgBox(mWindow, msg, _("OK"), nullptr));
		});

		addGroup(_("ENGINE STATUS"));
		
		// Heartbeat check
		bool isOnline = RetroAchievements::isLocalEngineActive();
		auto statusText = std::make_shared<TextComponent>(mWindow, isOnline ? _("ONLINE") : _("OFFLINE"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
		addWithLabel(_("LAHEE ENGINE"), statusText);
		
		if (!isOnline)
		{
			addEntry(_("RESTART ENGINE"), true, [this, hubPath]
			{
				// Kill any stuck PID and try again
				Utils::Platform::ProcessStartInfo("killall LAHEE").run();
				
				std::string launcherPath = hubPath + "/Server/lahee_startup.sh";
				std::string romsRoot = Utils::FileSystem::getParent(hubPath);
				std::string cmd = "bash \"" + launcherPath + "\" --roms \"" + romsRoot + "\" --hub \"" + hubPath + "\" --trusted";
				Utils::Platform::ProcessStartInfo(cmd).run();

				delete this;
			});
		}

		// Save handler for profile switching
		addSaveFunc([profile_choices] 
		{ 
			std::string selected = profile_choices->getSelected();
			SystemConf::getInstance()->set("global.retroachievements.username", selected);
			SystemConf::getInstance()->saveSystemConf();
			
			// NATIVE INJECTION: Force overwrite in RetroArch's real config file
			RetroAchievements::updateRetroArchConfig();
			
			// Notify the engine
			HttpReqOptions options;
			HttpReq request("http://127.0.0.1:8000/laheer/dorequest.php?r=laheeswitchuser&u=" + HttpReq::urlEncode(selected), &options);
			request.wait(); 
		});
	}
	else
	{
		// Fallback for remote/legacy mode
		addGroup(_("REMOTE SERVER SETTINGS"));
		addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
		addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);
	}

	addSwitch(_("SHOW IN MENU"), _("Show RetroAchievements in the main menu."), "RetroachievementsMenuitem", true, nullptr);
}
