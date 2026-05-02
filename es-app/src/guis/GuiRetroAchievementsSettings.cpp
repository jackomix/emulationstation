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
#include "ProfileManager.h"

GuiRetroAchievementsSettings::GuiRetroAchievementsSettings(Window* window) : GuiSettings(window, _("RETROACHIEVEMENTS").c_str())
{
	addGroup(_("GENERAL SETTINGS"));

	auto hubPath = RetroAchievements::getRetroAchievementsHubPath();
	bool isIntegrated = !hubPath.empty();

	addSwitch(_("ENABLE RETROACHIEVEMENTS"), _("Enable local achievement tracking for supported systems."), "global.retroachievements", true, nullptr);
	addSwitch(_("HARDCORE MODE"), _("Disable save states and cheats to earn hardcore achievements."), "global.retroachievements.hardcore", false, nullptr);

	if (isIntegrated)
	{
		addGroup(_("TOOLS"));
		
		addEntry(_("PATCH RETROARCH"), true, [this, hubPath]
		{
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
		
		bool isOnline = RetroAchievements::isLocalEngineActive();
		auto statusText = std::make_shared<TextComponent>(mWindow, isOnline ? _("ONLINE") : _("OFFLINE"), ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color);
		addWithLabel(_("LAHEE ENGINE"), statusText);
		
		if (!isOnline)
		{
			addEntry(_("RESTART ENGINE"), true, [this, hubPath]
			{
				Utils::Platform::ProcessStartInfo("killall LAHEE").run();
				
				std::string launcherPath = hubPath + "/Server/lahee_startup.sh";
				std::string romsRoot = Utils::FileSystem::getParent(hubPath);
				std::string cmd = "bash \"" + launcherPath + "\" --roms \"" + romsRoot + "\" --hub \"" + hubPath + "\" --trusted";
				Utils::Platform::ProcessStartInfo(cmd).run();

				delete this;
			});
		}
	}
	else
	{
		addGroup(_("REMOTE SERVER SETTINGS"));
		addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
		addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);
	}

	addSwitch(_("SHOW IN MENU"), _("Show RetroAchievements in the main menu."), "RetroachievementsMenuitem", true, nullptr);
}
