#include "stdafx.h"

#include "Main.h"
#include "EffectDispatcher.h"
#include "Memory.h"
#include "TwitchVoting.h"
#include "DebugMenu.h"
#include "OptionsFile.h"

std::array<int, 3> ParseColor(const std::string& colorText)
{
	// # <A> <R> <G> <B>

	std::array<int, 3> colors;

	int j = 0;
	for (int i = 3; i < 9; i += 2)
	{
		 TryParseInt(colorText.substr(i, 2), colors[j++], 16);
	}

	return colors;
}

void ParseConfigFile(int& effectSpawnTime, int& effectTimedDur, int& seed, int& effectTimedShortDur, bool& enableClearEffectsShortcut, bool& disableEffectsTwiceInRow,
	bool& disableTimerDrawing, bool& disableEffectTextDrawing, bool& enableToggleModShortcut, std::array<int, 3>& timerColor, std::array<int, 3>& textColor, std::array<int, 3>& effectTimerColor)
{
	OptionsFile configFile("chaosmod/config.ini");

	effectSpawnTime = configFile.ReadValueInt("NewEffectSpawnTime", 30);
	effectTimedDur = configFile.ReadValueInt("EffectTimedDur", 90);
	seed = configFile.ReadValueInt("Seed", 0);
	effectTimedShortDur = configFile.ReadValueInt("EffectTimedShortDur", 30);
	enableClearEffectsShortcut = configFile.ReadValueInt("EnableClearEffectsShortcut", true);
	disableEffectsTwiceInRow = configFile.ReadValueInt("DisableEffectTwiceInRow", false);
	disableTimerDrawing = configFile.ReadValueInt("DisableTimerBarDraw", false);
	disableEffectTextDrawing = configFile.ReadValueInt("DisableEffectTextDraw", false);
	enableToggleModShortcut = configFile.ReadValueInt("EnableToggleModShortcut", true);
	timerColor = ParseColor(configFile.ReadValue("EffectTimerColor", "#FF4040FF"));
	textColor = ParseColor(configFile.ReadValue("EffectTextColor", "#FFFFFFFF"));
	effectTimerColor = ParseColor(configFile.ReadValue("EffectTimedTimerColor", "#FFB4B4B4"));
}

void ParseTwitchFile(bool& enableTwitchVoting, int& twitchVotingNoVoteChance, int& twitchSecsBeforeVoting, bool& enableTwitchVoterIndicator, bool& enableTwitchVoteablesOnscreen)
{
	OptionsFile twitchFile("chaosmod/twitch.ini");

	enableTwitchVoting = twitchFile.ReadValueInt("EnableTwitchVoting", false);
	twitchVotingNoVoteChance = twitchFile.ReadValueInt("TwitchVotingNoVoteChance", 50);
	twitchSecsBeforeVoting = twitchFile.ReadValueInt("TwitchVotingSecsBeforeVoting", 0);
	enableTwitchVoterIndicator = twitchFile.ReadValueInt("TwitchVotingVoterIndicator", false);
	enableTwitchVoteablesOnscreen = twitchFile.ReadValueInt("TwitchVotingShowVoteablesOnscreen", false);
}

void ParseEffectsFile(std::map<EffectType, EffectData>& enabledEffects)
{
	OptionsFile effectsFile("chaosmod/effects.ini");

	for (int i = 0; i < _EFFECT_ENUM_MAX; i++)
	{
		EffectType effectType = static_cast<EffectType>(i);
		const EffectInfo& effectInfo = g_effectsMap.at(effectType);

		// Default EffectData values
		// Enabled, TimedType, CustomTime (-1 = Disabled), Weight, Permanent, ExcludedFromVoting
		std::vector<int> values { true, static_cast<int>(EffectTimedType::TIMED_DEFAULT), -1, 5, false, false };
		std::string value = effectsFile.ReadValue(effectInfo.Id);

		if (!value.empty())
		{
			int splitIndex = value.find(",");
			for (int i = 0; ; i++)
			{
				TryParseInt(value.substr(0, splitIndex), values[i]);
				value = value.substr(splitIndex + 1);

				if (splitIndex == value.npos)
				{
					break;
				}

				splitIndex = value.find(",");
			}
		}

		if (!values[0]) // enabled == false?
		{
			continue;
		}

		EffectData effectData;
		effectData.EffectTimedType = static_cast<EffectTimedType>(static_cast<EffectTimedType>(values[1]) == EffectTimedType::TIMED_DEFAULT ? effectInfo.IsShortDuration : values[1]); // Dang
		effectData.EffectCustomTime = values[2];
		effectData.EffectWeight = values[3];
		effectData.EffectPermanent = values[4];
		effectData.EffectExcludedFromVoting = values[5];

		enabledEffects.emplace(effectType, effectData);

		static std::ofstream log("chaosmod/enabledeffectslog.txt");
		log << effectInfo.Name << std::endl;
	}
}

void Main::Init()
{
	int effectSpawnTime, effectTimedDur, seed, effectTimedShortDur, twitchVotingNoVoteChance, twitchSecsBeforeChatVoting;
	bool disableEffectsTwiceInRow, enableTwitchVoting, enableTwitchVoterIndicator, enableTwitchVoteablesOnscreen;
	std::array<int, 3> timerColor, textColor, effectTimerColor;
	std::map<EffectType, EffectData> enabledEffects;

	ParseConfigFile(effectSpawnTime, effectTimedDur, seed, effectTimedShortDur, m_clearEffectsShortcutEnabled, disableEffectsTwiceInRow, m_disableDrawTimerBar,
		m_disableDrawEffectTexts, m_toggleModShortcutEnabled, timerColor, textColor, effectTimerColor);
	ParseTwitchFile(enableTwitchVoting, twitchVotingNoVoteChance, twitchSecsBeforeChatVoting, enableTwitchVoterIndicator, enableTwitchVoteablesOnscreen);
	ParseEffectsFile(enabledEffects);

	struct stat temp;
	if (enableTwitchVoting && stat("chaosmod/.twitchmode", &temp) == -1)
	{
		enableTwitchVoting = false;
	}

	Random::SetSeed(seed);

	g_effectDispatcher = std::make_unique<EffectDispatcher>(effectSpawnTime, effectTimedDur, enabledEffects, effectTimedShortDur, disableEffectsTwiceInRow,
		timerColor, textColor, effectTimerColor, enableTwitchVoteablesOnscreen);

#ifdef _DEBUG
	std::vector<EffectType> enabledEffectTypes;
	for (const auto& pair : enabledEffects)
	{
		enabledEffectTypes.push_back(pair.first);
	}

	m_debugMenu = std::make_unique<DebugMenu>(enabledEffectTypes);
#endif

	bool enableTwitchPollVoting = stat("chaosmod/.twitchpoll", &temp) != -1;
	m_twitchVoting = std::make_unique<TwitchVoting>(enableTwitchVoting, twitchVotingNoVoteChance, twitchSecsBeforeChatVoting, enableTwitchPollVoting, enableTwitchVoterIndicator,
		enableTwitchVoteablesOnscreen, enabledEffects);
}

void Main::MainLoop()
{
	int splashTextTime = 15000;
	int twitchVotingWarningTextTime = 15000;

	DWORD64 lastTick = GetTickCount64();

	while (true)
	{
		WAIT(0);

		if (IS_SCREEN_FADED_OUT() || m_disableMod)
		{
			WAIT(100);

			continue;
		}

		DWORD64 curTick = GetTickCount64();

		if (m_clearEffectsTextTime > 0)
		{
			BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
			ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Effects Cleared!");
			SET_TEXT_SCALE(.8f, .8f);
			SET_TEXT_COLOUR(255, 100, 100, 255);
			SET_TEXT_CENTRE(true);
			END_TEXT_COMMAND_DISPLAY_TEXT(.86f, .86f, 0);
			
			m_clearEffectsTextTime -= curTick - lastTick;
		}

		if (splashTextTime > 0)
		{
			BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
			ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Mod by pongo1231\n\nContributors:\nLucas7yoshi");
			SET_TEXT_SCALE(.65f, .65f);
			SET_TEXT_COLOUR(0, 255, 255, 255);
			SET_TEXT_CENTRE(true);
			END_TEXT_COMMAND_DISPLAY_TEXT(.2f, .5f, 0);

			splashTextTime -= curTick - lastTick;
		}

		if (m_twitchVoting && m_twitchVoting->IsEnabled() && twitchVotingWarningTextTime > 0)
		{
			BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
			ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Twitch Voting Enabled!");
			SET_TEXT_SCALE(.8f, .8f);
			SET_TEXT_COLOUR(255, 100, 100, 255);
			SET_TEXT_CENTRE(true);
			END_TEXT_COMMAND_DISPLAY_TEXT(.86f, .7f, 0);

			twitchVotingWarningTextTime -= curTick - lastTick;
		}

		lastTick = curTick;

		if (g_effectDispatcher)
		{
			if (!m_disableDrawTimerBar)
			{
				g_effectDispatcher->DrawTimerBar();
			}
			if (!m_disableDrawEffectTexts)
			{
				g_effectDispatcher->DrawEffectTexts();
			}
		}

#ifdef _DEBUG
		if (m_debugMenu && m_debugMenu->IsVisible())
		{
			// Arrow Up
			DISABLE_CONTROL_ACTION(1, 27, true);
			DISABLE_CONTROL_ACTION(1, 127, true);
			DISABLE_CONTROL_ACTION(1, 188, true);
			DISABLE_CONTROL_ACTION(1, 300, true);
			// Arrow Down
			DISABLE_CONTROL_ACTION(1, 173, true);
			DISABLE_CONTROL_ACTION(1, 187, true);
			DISABLE_CONTROL_ACTION(1, 299, true);
			// Enter
			DISABLE_CONTROL_ACTION(1, 18, true);
			DISABLE_CONTROL_ACTION(1, 176, true);
			DISABLE_CONTROL_ACTION(1, 191, true);
			DISABLE_CONTROL_ACTION(1, 201, true);
			DISABLE_CONTROL_ACTION(1, 215, true);
			// Backspace
			DISABLE_CONTROL_ACTION(1, 177, true);
			DISABLE_CONTROL_ACTION(1, 194, true);
			DISABLE_CONTROL_ACTION(1, 202, true);

			m_debugMenu->Tick();
		}
#endif
	}
}

void Main::RunEffectLoop()
{
	while (true)
	{
		WAIT(0);

		if (IS_SCREEN_FADED_OUT())
		{
			SET_TIME_SCALE(1.f); // Prevent potential softlock if Lag effect is running during screen fadeout

			WAIT(100);

			continue;
		}

		static bool justReenabled = false;
		if (m_disableMod)
		{
			if (!justReenabled)
			{
				justReenabled = true;

				g_effectDispatcher.reset();
#ifdef _DEBUG
				m_debugMenu.reset();
#endif
				m_twitchVoting.reset();
			}

			continue;
		}
		else if (justReenabled)
		{
			justReenabled = false;

			Init(); // Restart the main part of the mod completely
		}

		if (m_clearAllEffects)
		{
			m_clearAllEffects = false;

			g_effectDispatcher->Reset();
		}

		if (m_twitchVoting->IsEnabled())
		{
			m_twitchVoting->Tick();
		}

		if (!m_pauseTimer)
		{
			g_effectDispatcher->UpdateTimer();
		}

		g_effectDispatcher->UpdateEffects();
	}
}

void Main::OnKeyboardInput(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow)
{
	static bool CTRLpressed = false;

	if (key == VK_CONTROL)
	{
		CTRLpressed = !isUpNow;
	}
	else if (CTRLpressed && !wasDownBefore)
	{
		if (key == VK_OEM_MINUS && m_clearEffectsShortcutEnabled)
		{
			m_clearAllEffects = true;
			m_clearEffectsTextTime = 5000;
		}
#ifdef _DEBUG
		else if (key == VK_OEM_PERIOD)
		{
			m_pauseTimer = !m_pauseTimer;
		}
		else if (key == VK_OEM_COMMA && m_debugMenu)
		{
			m_debugMenu->SetVisible(!m_debugMenu->IsVisible());
		}
#endif
		else if (key == 0x4C && m_toggleModShortcutEnabled) // L
		{
			m_disableMod = !m_disableMod;
		}
	}

#ifdef _DEBUG
	if (m_debugMenu && m_debugMenu->IsVisible())
	{
		m_debugMenu->HandleInput(key, wasDownBefore);
	}
#endif
}