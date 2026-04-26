#include "GuiRetroAchievementsSettings.h"
#include "ThreadedHasher.h"
#include "GuiHashStart.h"
#include "SystemConf.h"
#include "ApiSystem.h"
#include "RetroAchievements.h"
#include "HttpReq.h"
#include "Settings.h"

#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditPopup.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"

GuiRetroAchievementsSettings::GuiRetroAchievementsSettings(Window* window) : GuiSettings(window, _("RETROACHIEVEMENT SETTINGS").c_str())
{
	addGroup(_("SETTINGS"));

	bool retroachievementsEnabled = SystemConf::getInstance()->getBool("global.retroachievements");
	std::string username = SystemConf::getInstance()->get("global.retroachievements.username");
	std::string password = SystemConf::getInstance()->get("global.retroachievements.password");

	std::string serverUrl = Settings::getInstance()->getString("RetroAchievementsServerURL");
	bool isLocal = serverUrl.find("127.0.0.1") != std::string::npos;

	// retroachievements_enable
	auto retroachievements_enabled = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_enabled->setState(retroachievementsEnabled);
	addWithLabel(_("RETROACHIEVEMENTS"), retroachievements_enabled);

	if (isLocal)
	{
		auto hubPath = RetroAchievements::getRetroAchievementsHubPath();
		auto userDir = hubPath + "/User";

		auto profile_choices = std::make_shared<OptionListComponent<std::string>>(mWindow, _("ACTIVE PROFILE"), false);
		
		auto files = Utils::FileSystem::getDirContent(userDir);
		bool profileFound = false;
		for (auto file : files)
		{
			if (Utils::FileSystem::getExtension(file) == ".json")
			{
				std::string name = Utils::FileSystem::getStem(file);
				profile_choices->add(name, name, username == name);
				if (username == name) profileFound = true;
			}
		}

		if (!profileFound)
		{
			profile_choices->add("Player", "Player", true);
			if (username.empty()) username = "Player";
		}

		addWithLabel(_("ACTIVE PROFILE"), profile_choices);

		addEntry(_("ADD NEW PROFILE"), true, [this, profile_choices, userDir]
		{
			mWindow->pushGui(new GuiTextEditPopup(mWindow, _("NEW PROFILE NAME"), "", [this, profile_choices, userDir](std::string name)
			{
				if (name.empty()) return;
				// Create a dummy user file so it shows up in the list next time
				// Actually, LAHEE will create it, but we want it in the UI now.
				profile_choices->add(name, name, true);
				SystemConf::getInstance()->set("global.retroachievements.username", name);
			}, false));
		});

		addSaveFunc([profile_choices] 
		{ 
			std::string selected = profile_choices->getSelected();
			SystemConf::getInstance()->set("global.retroachievements.username", selected);
			
			// Notify LAHEE immediately
			HttpReqOptions options;
			HttpReq request("http://127.0.0.1:8000/laheer/dorequest.php?r=laheeswitchuser&u=" + HttpReq::urlEncode(selected), &options);
			request.wait(); // Fast on localhost
		});
	}
	else
	{
		// retroachievements, username, password
		addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
		addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);
	}

	addSwitch(_("AUTO-START LAHEE"), _("Automatically launch the local LAHEE server at startup."), "AutoStartLAHEE", true, nullptr);

	bool isOnline = false;
	if (Settings::getInstance()->getBool("AutoStartLAHEE"))
	{
		HttpReqOptions options;
		HttpReq request("http://127.0.0.1:8000/laheer/dorequest.php?r=laheeinfo", &options);
		isOnline = request.wait() && request.status() == HttpReq::REQ_SUCCESS;
	}

	addEntry(_("SERVER STATUS"), false, nullptr, isOnline ? _("Online") : _("Offline"));

	addGroup(_("OPTIONS"));

	addSwitch(_("HARDCORE MODE"), _("Disable loading states, rewind and cheats for more points."), "global.retroachievements.hardcore", false, nullptr);
	addSwitch(_("LEADERBOARDS"), _("Compete in high-score and best time leaderboards (requires hardcore)."), "global.retroachievements.leaderboards", false, nullptr);
	addSwitch(_("VERBOSE MODE"), _("Show achievement progression on game launch and other notifications."), "global.retroachievements.verbose", false, nullptr);
	addSwitch(_("RICH PRESENCE"), "global.retroachievements.richpresence", false);
	addSwitch(_("ENCORE MODE"), _("Unlocked achievements can be earned again."), "global.retroachievements.encore", false, nullptr);
	addSwitch(_("AUTOMATIC SCREENSHOT"), _("Automatically take a screenshot when an achievement is earned."), "global.retroachievements.screenshot", false, nullptr);
	addSwitch(_("CHALLENGE INDICATORS"), _("Shows icons in the bottom right corner when eligible achievements can be earned."), "global.retroachievements.challenge_indicators", false, nullptr);

	// Unlock sound
	auto installedRSounds = ApiSystem::getInstance()->getRetroachievementsSoundsList();
	if (installedRSounds.size() > 0)
	{
		std::string currentSound = SystemConf::getInstance()->get("global.retroachievements.sound");

		auto rsounds_choices = std::make_shared<OptionListComponent<std::string> >(mWindow, _("RETROACHIEVEMENT UNLOCK SOUND"), false);
		rsounds_choices->add(_("none"), "none", currentSound.empty() || currentSound == "none");

		for (auto snd : installedRSounds)
			rsounds_choices->add(_(Utils::String::toUpper(snd).c_str()), snd, currentSound == snd);

		if (!rsounds_choices->hasSelection())
			rsounds_choices->selectFirstItem();

		addWithLabel(_("UNLOCK SOUND"), rsounds_choices);
		addSaveFunc([rsounds_choices] { SystemConf::getInstance()->set("global.retroachievements.sound", rsounds_choices->getSelected()); });
	}

	addSwitch(_("SHOW RETROACHIEVEMENTS ENTRY IN MAIN MENU"), _("View your RetroAchievement stats right from the main menu!"), "RetroachievementsMenuitem", true, nullptr);

	addGroup(_("GAME INDEXES"));
	addSwitch(_("INDEX NEW GAMES AT STARTUP"), "CheevosCheckIndexesAtStart", true);
	addEntry(_("INDEX GAMES"), true, [this]
	{
		if (ThreadedHasher::checkCloseIfRunning(mWindow))
			mWindow->pushGui(new GuiHashStart(mWindow, ThreadedHasher::HASH_CHEEVOS_MD5));
	});

	addSaveFunc([retroachievementsEnabled, retroachievements_enabled, username, password, window]
	{
		bool newState = retroachievements_enabled->getState();
		std::string newUsername = SystemConf::getInstance()->get("global.retroachievements.username");
		std::string newPassword = SystemConf::getInstance()->get("global.retroachievements.password");
		std::string token = SystemConf::getInstance()->get("global.retroachievements.token");

		std::string serverUrl = Settings::getInstance()->getString("RetroAchievementsServerURL");
		bool isLocal = serverUrl.find("127.0.0.1") != std::string::npos;

		if (newState && (!retroachievementsEnabled || username != newUsername || password != newPassword || token.empty()))
		{
			if (isLocal)
			{
				SystemConf::getInstance()->set("global.retroachievements.token", "local_token");
			}
			else
			{
				std::string tokenOrError;
				if (RetroAchievements::testAccount(newUsername, newPassword, tokenOrError))
				{
					SystemConf::getInstance()->set("global.retroachievements.token", tokenOrError);
				}
				else
				{
					SystemConf::getInstance()->set("global.retroachievements.token", "");

					window->pushGui(new GuiMsgBox(window, _("UNABLE TO ACTIVATE RETROACHIEVEMENTS:") + "\n" + tokenOrError, _("OK"), nullptr, GuiMsgBoxIcon::ICON_ERROR));
					retroachievements_enabled->setState(false);
					newState = false;
				}
			}
		}
		else if (!newState)
			SystemConf::getInstance()->set("global.retroachievements.token", "");

		if (SystemConf::getInstance()->setBool("global.retroachievements", newState))
			if (!ThreadedHasher::isRunning() && newState)
				ThreadedHasher::start(window, ThreadedHasher::HASH_CHEEVOS_MD5, false, true);
	});
}
