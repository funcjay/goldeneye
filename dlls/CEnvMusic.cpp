//
//	SoLoud music manager, written by Jay - 2022
//	You are free to use and modify this code as long as credit is given.
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

extern int gmsgSoLoud;

#define SF_ENVMUSIC_REMOVE	1
#define SF_ENVMUSIC_LOOP	2
#define SF_ENVMUSIC_FADE	4

class CEnvMusic : public CPointEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	int ObjectCaps() override { return CPointEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};
LINK_ENTITY_TO_CLASS(env_music, CEnvMusic);

void CEnvMusic::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}

void CEnvMusic::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	const char* musFile = pev->target == NULL ? "stop" : STRING(pev->target);
	int loop = (pev->spawnflags & SF_ENVMUSIC_LOOP) != 0 ? 1 : 0;
	int fade = (pev->spawnflags & SF_ENVMUSIC_FADE) != 0 ? 1 : 0;

	MESSAGE_BEGIN(MSG_ALL, gmsgSoLoud);
	WRITE_STRING(musFile);
	WRITE_BYTE(loop);
	WRITE_BYTE(fade);
	MESSAGE_END();

	if ((pev->spawnflags & SF_ENVMUSIC_REMOVE) != 0)
		UTIL_Remove(this);
}
