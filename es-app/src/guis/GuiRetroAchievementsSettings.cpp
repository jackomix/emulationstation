#include "GuiRetroAchievementsSettings.h"
#include "ThreadedHasher.h"
#include "GuiHashStart.h"
#include "SystemConf.h"
#include "ApiSystem.h"
#include "RetroAchievements.h"
#include "ProfileManager.h"
#include "ThemeData.h"
#include "components/TextComponent.h"

#include "guis/GuiMsgBox.h"
#include "components/SwitchComponent.h"
#include "components/OptionListComponent.h"

GuiRetroAchievementsSettings::GuiRetroAchievementsSettings(Window* window) : GuiSettings(window, _("RETROACHIEVEMENT SETTINGS").c_str())
{
	addGroup(_("SETTINGS"));

	bool retroachievementsEnabled = SystemConf::getInstance()->getBool("global.retroachievements");
	std::string username = SystemConf::getInstance()->get("global.retroachievements.username");
	std::string password = SystemConf::getInstance()->get("global.retroachievements.password");

	auto retroachievements_enabled = std::make_shared<SwitchComponent>(mWindow);
	retroachievements_enabled->setState(retroachievementsEnabled);

	std::string mode = Settings::getInstance()->getString("RetroachievementsOfflineMode");
	bool isOffline = (mode == "always_offline") || (mode == "auto" && ApiSystem::getInstance()->getIpAddress() == "NOT CONNECTED") || (mode.empty() && ApiSystem::getInstance()->getIpAddress() == "NOT CONNECTED");

	if (isOffline) {
		std::string profileName = ProfileManager::getInstance()->getActiveProfileName();
		if (profileName.empty()) profileName = "Player";
		auto usernameComp = std::make_shared<TextComponent>(mWindow, profileName, ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color, ALIGN_RIGHT);
		addWithLabel(_("USERNAME"), usernameComp);
	} else {
		addWithLabel(_("RETROACHIEVEMENTS"), retroachievements_enabled);
		addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
		addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);
	}

	addGroup(_("OPTIONS"));

	addSwitch(_("HARDCORE MODE"), _("Disable loading states, rewind and cheats for more points."), "global.retroachievements.hardcore", false, nullptr);
	
	if (!isOffline) {
		addSwitch(_("LEADERBOARDS"), _("Compete in high-score and best time leaderboards (requires hardcore)."), "global.retroachievements.leaderboards", false, nullptr);
	}
	
	addSwitch(_("VERBOSE MODE"), _("Show achievement progression on game launch and other notifications."), "global.retroachievements.verbose", false, nullptr);
	
	if (!isOffline) {
		addSwitch(_("RICH PRESENCE"), "global.retroachievements.richpresence", false);
		addSwitch(_("ENCORE MODE"), _("Unlocked achievements can be earned again."), "global.retroachievements.encore", false, nullptr);
	}
	
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

	auto online_mode = std::make_shared<SwitchComponent>(mWindow);
	online_mode->setState(!isOffline);
	addWithLabel(_("ONLINE MODE"), online_mode);
	addSaveFunc([online_mode] { Settings::getInstance()->setString("RetroachievementsOfflineMode", online_mode->getState() ? "none" : "always_offline"); });

	addGroup(_("APPEARANCE / UI"));

	auto anchor_choices = std::make_shared<OptionListComponent<std::string>>(mWindow, _("POPUP POSITION"), false);
	std::string currentAnchor = SystemConf::getInstance()->get("global.retroachievements.ui.anchor");
	if (currentAnchor.empty()) currentAnchor = "0"; // Top Left
	anchor_choices->add(_("TOP LEFT"), "0", currentAnchor == "0");
	anchor_choices->add(_("TOP CENTER"), "1", currentAnchor == "1");
	anchor_choices->add(_("TOP RIGHT"), "2", currentAnchor == "2");
	anchor_choices->add(_("BOTTOM LEFT"), "3", currentAnchor == "3");
	anchor_choices->add(_("BOTTOM CENTER"), "4", currentAnchor == "4");
	anchor_choices->add(_("BOTTOM RIGHT"), "5", currentAnchor == "5");
	addWithLabel(_("POPUP POSITION"), anchor_choices);
	addSaveFunc([anchor_choices] { SystemConf::getInstance()->set("global.retroachievements.ui.anchor", anchor_choices->getSelected()); });

	auto summary_choices = std::make_shared<OptionListComponent<std::string>>(mWindow, _("STARTUP SUMMARY"), false);
	std::string currentSummary = SystemConf::getInstance()->get("global.retroachievements.ui.summary");
	if (currentSummary.empty()) currentSummary = "1"; // All
	summary_choices->add(_("HIDE"), "0", currentSummary == "0");
	summary_choices->add(_("SHOW ALL"), "1", currentSummary == "1");
	summary_choices->add(_("GAMES ONLY"), "2", currentSummary == "2");
	addWithLabel(_("STARTUP SUMMARY"), summary_choices);
	addSaveFunc([summary_choices] { SystemConf::getInstance()->set("global.retroachievements.ui.summary", summary_choices->getSelected()); });

	addSwitch(_("SHOW ACHIEVEMENT BADGES"), _("Display achievement icons inside notifications."), "global.retroachievements.ui.badges", true, nullptr);
	addSwitch(_("LOGIN NOTIFICATIONS"), _("Show 'Logged in' notification when a game starts."), "global.retroachievements.ui.login", true, nullptr);
	addSwitch(_("UNLOCK NOTIFICATIONS"), _("Show a notification when an achievement is earned."), "global.retroachievements.ui.unlock", true, nullptr);
	addSwitch(_("MASTERY NOTIFICATIONS"), _("Show a notification when all achievements are earned."), "global.retroachievements.ui.mastery", true, nullptr);

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

		if (newState && (!retroachievementsEnabled || username != newUsername || password != newPassword || token.empty()))
		{
			std::string mode = Settings::getInstance()->getString("RetroachievementsOfflineMode");
			bool isOffline = (mode == "always_offline") || (mode == "auto" && ApiSystem::getInstance()->getIpAddress() == "NOT CONNECTED") || (mode.empty() && ApiSystem::getInstance()->getIpAddress() == "NOT CONNECTED");

			if (isOffline) {
				SystemConf::getInstance()->set("global.retroachievements.token", "offline_token");
			}
			else {
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
