/*
	Effect by Lucas7yoshi
*/

#include <stdafx.h>

static int m_alpha = 0;

static void OnStart()
{
	REQUEST_ADDITIONAL_TEXT("CREDIT", 0);
	while (!HAS_ADDITIONAL_TEXT_LOADED(0))
	{
		WAIT(0);
	}

	PLAY_END_CREDITS_MUSIC(true);
	SET_CREDITS_ACTIVE(true);
	SET_MOBILE_PHONE_RADIO_STATE(true);
	SET_RADIO_TO_STATION_NAME("RADIO_16_SILVERLAKE");

	auto song = Random::GetRandomInt(0, 2);
	if (song == 0)
	{
		SET_CUSTOM_RADIO_TRACK_LIST("RADIO_16_SILVERLAKE", "END_CREDITS_SAVE_MICHAEL_TREVOR", 1);
	}
	else if (song == 1)
	{
		SET_CUSTOM_RADIO_TRACK_LIST("RADIO_16_SILVERLAKE", "END_CREDITS_KILL_MICHAEL", 1);
	}
	else
	{
		SET_CUSTOM_RADIO_TRACK_LIST("RADIO_16_SILVERLAKE", "END_CREDITS_KILL_TREVOR", 1);
	}
}

static void OnTick()
{
	SET_ENTITY_INVINCIBLE(PLAYER_PED_ID(), true);

	DISABLE_ALL_CONTROL_ACTIONS(0);
	SET_RADIO_TO_STATION_NAME("RADIO_16_SILVERLAKE");

	DRAW_RECT(.5f, .5f, 1.f, 1.f, 0, 0, 0, m_alpha, false);

	if (m_alpha < 255)
	{
		m_alpha++;
	}
}

static void OnStop()
{
	m_alpha = 0;

	SET_ENTITY_INVINCIBLE(PLAYER_PED_ID(), false);

	SET_CREDITS_ACTIVE(false);

	PLAY_END_CREDITS_MUSIC(false);
	SET_MOBILE_PHONE_RADIO_STATE(false);
}

static RegisterEffect registerEffect(EFFECT_MISC_CREDITS, OnStart, OnStop, OnTick);