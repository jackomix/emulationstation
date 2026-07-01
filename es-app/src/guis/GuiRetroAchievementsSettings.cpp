#include "GuiRetroAchievementsSettings.h"
#include "ThreadedHasher.h"
#include "GuiHashStart.h"
#include "SystemConf.h"
#include "ApiSystem.h"
#include "RetroAchievements.h"
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
		auto usernameComp = std::make_shared<TextComponent>(mWindow, username, ThemeData::getMenuTheme()->Text.font, ThemeData::getMenuTheme()->Text.color, ALIGN_RIGHT);
		addWithLabel(_("USERNAME"), usernameComp);
	} else {
		addWithLabel(_("RETROACHIEVEMENTS"), retroachievements_enabled);
		addInputTextRow(_("USERNAME"), "global.retroachievements.username", false);
		addInputTextRow(_("PASSWORD"), "global.retroachievements.password", true);
	}

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

	auto offline_mode = std::make_shared<OptionListComponent<std::string>>(mWindow, _("OFFLINE MODE"), false);
	std::string currentOfflineMode = Settings::getInstance()->getString("RetroachievementsOfflineMode");
	if (currentOfflineMode.empty()) currentOfflineMode = "auto";
	offline_mode->add(_("AUTO (IF NO NETWORK)"), "auto", currentOfflineMode == "auto");
	offline_mode->add(_("ALWAYS OFFLINE"), "always_offline", currentOfflineMode == "always_offline");
	addWithLabel(_("OFFLINE MODE"), offline_mode);
	addSaveFunc([offline_mode] { Settings::getInstance()->setString("RetroachievementsOfflineMode", offline_mode->getSelected()); });

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
