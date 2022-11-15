//
//	SoLoud music manager, written by Jay - 2022
//	You are free to use and modify this code as long as credit is given.
//

#include "extdll.h"
#include "util.h"
#include "hud.h"

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif
#define HSPRITE HSPRITE_WINDOWS
#include "soloud.h"
#include "soloud_wav.h"
#undef HSPRITE
SoLoud::Soloud gSoLoud;
SoLoud::Wav gSample;

#include <sys/stat.h>


float flVolumeCVar = 1.0;
float flVolume = 1.0;
int handle = 0;
bool isPaused = false;
bool isFading = false;


void MusicMan_DoStop()
{
	gSoLoud.stop(handle);
	gSoLoud.setPause(handle, false);
	handle = 0;
}

void MusicMan_Stop(bool fade)
{
	if (handle == 0)
		return;

#ifdef _DEBUG
	gEngfuncs.Con_Printf("SoLoud: Stopping, %s\n", fade == true ? "fading" : "not fading");
#endif

	gSoLoud.setLooping(handle, false);

	if (isFading == true)
	{
		MusicMan_DoStop();

		isFading = false;
		flVolume = flVolumeCVar;

#ifdef _DEBUG
		gEngfuncs.Con_Printf("SoLoud: Fade cancelled\n");
#endif
	}
	else
	{
		if (fade == true)
			isFading = true;
		else
			MusicMan_DoStop();
	}
}

void MusicMan_Play(const char* file, bool loop, bool fade)
{
	if (strncmp(file, "stop", sizeof(file)) == 0)
	{
		MusicMan_Stop(fade);
		return;
	}
	else
		MusicMan_Stop(false);

	if (flVolumeCVar <= 0.0)
		return;

	char bufFile[64];
	snprintf(bufFile, sizeof(bufFile), "%s\\sound\\%s", gEngfuncs.pfnGetGameDirectory(), file);

	struct stat checkfile;
	if (0 > stat(bufFile, &checkfile))
	{
#ifdef _DEBUG
		gEngfuncs.Con_Printf("SoLoud: File %s doesn't exist!\n", bufFile);
#endif
		return;
	}

#ifdef _DEBUG
	gEngfuncs.Con_Printf("SoLoud: Playing %s, %s\n", file, loop != false ? "looping" : "not looping");
#endif

	gSample.load(bufFile);
	handle = gSoLoud.play(gSample, flVolume);
	gSoLoud.setLooping(handle, loop);
}

void MusicMan_Paused(bool pause) { isPaused = pause; }


void MusicMan_Start() { gSoLoud.init(); }

void MusicMan_Update()
{
	flVolumeCVar = gEngfuncs.pfnGetCvarFloat("MP3Volume");
	if (handle != 0 && isFading == false)
		flVolume = flVolumeCVar;

	if (handle != 0)
	{
		if (isFading == true)
		{
			if (isPaused == false)
				flVolume -= CVAR_GET_FLOAT("MP3Fade") >= 0.0 ? CVAR_GET_FLOAT("MP3Fade") : 0.005;

			if (flVolume <= 0.0)
			{
				MusicMan_DoStop();
				
#ifdef _DEBUG
				gEngfuncs.Con_Printf("SoLoud: Fade ended\n");
#endif

				isFading = false;
			}
		}

		gSoLoud.setVolume(handle, flVolume);
		gSoLoud.setPause(handle, isPaused);
	}
}

void MusicMan_Shutdown()
{
	MusicMan_Stop(false);
	gSoLoud.deinit();
}
