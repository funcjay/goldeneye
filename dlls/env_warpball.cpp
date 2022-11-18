/*

28.08.18

"env_warpball" entity for ps2hl

Combination of "monstermaker" and this code:
https://github.com/FWGS/hlsdk-xash3d/blob/blueshift/dlls/effects.cpp

Modified by Jay - added compatibility with HL Updated & more

*/

// Header files
#include "extdll.h"		// Required for KeyValueData
#include "util.h"		// Required Consts & Macros
#include "cbase.h"		// Required for CPointEntity
#include "monsters.h"	// Needed for "monstermaker"
#include "effects.h"	// CBeam

//=========================================================
// env_warpball
//=========================================================
#define SF_REMOVE_ON_FIRE	0x0001
#define SF_KILL_CENTER		0x0002
#define SF_DLIGHT			0x0004	// jay - dlight


class CEnvWarpBall : public CBaseEntity
{
public:
	// From Xash BShift
	void Precache() override;
	void Spawn() override { Precache(); }
	void Think() override;
	bool KeyValue(KeyValueData *pkvd) override;
	void Telefrag(bool PlayerOnly);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int ObjectCaps() override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	Vector vecOrigin;

	// From "monstermaker"
	void FinalBeams();
	void MakeMonster();

	string_t m_iszMonsterClassname;// classname of the monster(s) that will be created.
	int	 m_cNumMonsters;// max number of monsters this ent can create
	int  m_cLiveChildren;// how many monsters made by this monster maker that are currently alive
	int	 m_iMaxLiveChildren;// max number of monsters that this maker may have out at one time.
	float m_flGround; // z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child



	static TYPEDESCRIPTION m_SaveData[];
	bool Save(CSave &save);
	bool Restore(CRestore &restore);
};
LINK_ENTITY_TO_CLASS(env_warpball, CEnvWarpBall)

// Save/restore from "monstermaker"
TYPEDESCRIPTION	CEnvWarpBall::m_SaveData[] =
{
	DEFINE_FIELD(CEnvWarpBall, m_iszMonsterClassname, FIELD_STRING),
	DEFINE_FIELD(CEnvWarpBall, m_cNumMonsters, FIELD_INTEGER),
	DEFINE_FIELD(CEnvWarpBall, m_cLiveChildren, FIELD_INTEGER),
	DEFINE_FIELD(CEnvWarpBall, m_flGround, FIELD_FLOAT),
	DEFINE_FIELD(CEnvWarpBall, m_iMaxLiveChildren, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CEnvWarpBall, CBaseEntity);

bool CEnvWarpBall::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->button = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "warp_target"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "monstercount"))
	{
		m_cNumMonsters = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_imaxlivechildren"))
	{
		m_iMaxLiveChildren = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "monstertype"))
	{
		m_iszMonsterClassname = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CEnvWarpBall::Precache()
{
	PRECACHE_MODEL("sprites/xflare1.spr");
	PRECACHE_MODEL("sprites/fexplo1.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/beamstart2.wav");
	PRECACHE_SOUND("debris/beamstart7.wav");

	// PS2HL: precache monster (if set)
	if (m_iszMonsterClassname != NULL)
		UTIL_PrecacheOther(STRING(m_iszMonsterClassname));
}

void CEnvWarpBall::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int iTimes = 0;
	int iDrawn = 0;
	TraceResult tr;
	Vector vecDest;
	CBeam *pBeam;
	CBaseEntity *pEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
	edict_t *pos;

	if (pEntity)//target found ?
	{
		vecOrigin = pEntity->pev->origin;
		pos = pEntity->edict();
	}
	else
	{
		//use as center
		vecOrigin = pev->origin;
		pos = edict();
	}

	// jay - dlight
	if ((pev->spawnflags & SF_DLIGHT) != 0)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(vecOrigin.x);	// origin
		WRITE_COORD(vecOrigin.y);
		WRITE_COORD(vecOrigin.z);
		WRITE_BYTE(pev->button * 0.15f); // radius
		WRITE_BYTE(0);					 // R
		WRITE_BYTE(255);				 // G
		WRITE_BYTE(0);					 // B
		WRITE_BYTE(15);					 // life in seconds * 10
		WRITE_BYTE(pev->button * 0.175f);// decay
		MESSAGE_END();
	}

	EMIT_SOUND(pos, CHAN_BODY, "debris/beamstart2.wav", VOL_NORM, ATTN_NORM);
	UTIL_ScreenShake(vecOrigin, 4.0f, 160.0f, 0.75f, pev->button * 1.75f);

	CSprite* pSpr1 = CSprite::SpriteCreate("sprites/xflare1.spr", vecOrigin, true);
	pSpr1->AnimateAndDie(10.0f);
	pSpr1->SetTransparency(kRenderGlow, 184, 250, 214, 255, kRenderFxNoDissipation);
	CSprite *pSpr2 = CSprite::SpriteCreate("sprites/fexplo1.spr", vecOrigin, true);
	pSpr2->AnimateAndDie(10.0f);
	pSpr2->SetTransparency(kRenderGlow, 77, 210, 130, 255, kRenderFxNoDissipation);

	int iBeams = RANDOM_LONG(20, 40);
	while (iDrawn < iBeams && iTimes < (iBeams * 3))
	{
		vecDest = pev->button * (Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1)).Normalize());
		UTIL_TraceLine(vecOrigin, vecOrigin + vecDest, ignore_monsters, NULL, &tr);
		if (tr.flFraction != 1.0f)
		{
			// we hit something.
			iDrawn++;
			pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 200);
			pBeam->PointsInit(vecOrigin, tr.vecEndPos);
			pBeam->SetColor(197, 243, 169);
			pBeam->SetNoise(65);
			pBeam->SetBrightness(150);
			pBeam->SetWidth(18);
			pBeam->SetScrollRate(35);
			pBeam->SetThink(&CBeam::SUB_Remove);
			pBeam->pev->nextthink = gpGlobals->time + 0.5f;
		}
		iTimes++;
	}
	
	// PS2HL: if monster isn't specified then kill all only once
	if (m_iszMonsterClassname == NULL)
		Telefrag(false);

	pev->nextthink = gpGlobals->time + 0.5f;
}

void CEnvWarpBall::Think()
{
	SUB_UseTargets(this, USE_TOGGLE, 0.0f);

	// PS2HL: if monster isn't specified then kill only the player
	if (m_iszMonsterClassname != NULL)
		Telefrag(false);
	else
		Telefrag(true);

	// PS2HL: spawn monster
	if (m_iszMonsterClassname != NULL)
	{
		FinalBeams();
		MakeMonster();
	}

	if ((pev->spawnflags & SF_REMOVE_ON_FIRE) != 0)
		UTIL_Remove(this);
}

void CEnvWarpBall::Telefrag(bool PlayerOnly)
{
	if ((pev->spawnflags & SF_KILL_CENTER) != 0)
	{
		CBaseEntity *pMonster = NULL;

		while ((pMonster = UTIL_FindEntityInSphere(pMonster, vecOrigin, 72.0f)) != NULL)
		{
			// PS2HL: kill only the player
			if (PlayerOnly && !pMonster->IsPlayer())
				continue;

			// PS2HL: increased dmg to guarantee kill
			if (FBitSet(pMonster->pev->flags, FL_MONSTER | FL_CLIENT))
				pMonster->TakeDamage(pev, pev, 1000.0f, DMG_GENERIC);
		}
	}
}

void CEnvWarpBall::FinalBeams()
{
	int iTimes = 0;
	int iDrawn = 0;
	TraceResult tr;
	Vector vecDest;
	CBeam* pBeam;
	CBaseEntity* pEntity = UTIL_FindEntityByTargetname(NULL, STRING(pev->message));
	edict_t* pos;

	if (pEntity) // target found ?
	{
		vecOrigin = pEntity->pev->origin;
		pos = pEntity->edict();
	}
	else
	{
		// use as center
		vecOrigin = pev->origin;
		pos = edict();
	}

	EMIT_SOUND(pos, CHAN_ITEM, "debris/beamstart7.wav", VOL_NORM, ATTN_NORM);

	int iBeams = RANDOM_LONG(20, 40);
	while (iDrawn < iBeams && iTimes < (iBeams * 3))
	{
		vecDest = pev->button * (Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1)).Normalize());
		UTIL_TraceLine(vecOrigin, vecOrigin + vecDest, ignore_monsters, NULL, &tr);
		if (tr.flFraction != 1.0f)
		{
			// we hit something.
			iDrawn++;
			pBeam = CBeam::BeamCreate("sprites/lgtning.spr", 200);
			pBeam->PointsInit(vecOrigin, tr.vecEndPos);
			pBeam->SetColor(197, 243, 169);
			pBeam->SetNoise(65);
			pBeam->SetBrightness(150);
			pBeam->SetWidth(18);
			pBeam->SetScrollRate(35);
			pBeam->SetThink(&CBeam::SUB_Remove);
			pBeam->pev->nextthink = gpGlobals->time + 0.5f;
		}
		iTimes++;
	}
}

// From "monstermaker"
void CEnvWarpBall::MakeMonster()
{
	edict_t* pent;
	entvars_t* pevCreate;

	if (m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren)
	{ // not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	if (0 == m_flGround)
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me.
		TraceResult tr;

		UTIL_TraceLine(pev->origin, pev->origin - Vector(0.0f, 0.0f, 2048.0f), ignore_monsters, ENT(pev), &tr);
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = pev->origin - Vector(34.0f, 34.0f, 0.0f);
	Vector maxs = pev->origin + Vector(34.0f, 34.0f, 0.0f);
	maxs.z = pev->origin.z;
	mins.z = m_flGround;

	pent = CREATE_NAMED_ENTITY(m_iszMonsterClassname);

	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in MonsterMaker!\n");
		return;
	}

	// If I have a target, fire!
	if (!FStringNull(pev->target))
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0.0f);
	}

	pevCreate = VARS(pent);
	pevCreate->origin = pev->origin;
	pevCreate->angles = pev->angles;
	SetBits(pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND);

	DispatchSpawn(ENT(pevCreate));
	pevCreate->owner = edict();

	if (!FStringNull(pev->netname))
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++;// count this monster
	m_cNumMonsters--;

	if (m_cNumMonsters == 0)
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink(NULL);
		SetUse(NULL);
	}
}
