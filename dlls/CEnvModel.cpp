/***
*
*	Copyright (c) 2020, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"
#include "player.h"

//=================================================================
// env_model: like env_sprite, except you can specify a sequence.
//=================================================================
#define SF_ENVMODEL_OFF			1
#define SF_ENVMODEL_DROPTOFLOOR	2
#define SF_ENVMODEL_SOLID		4

class CEnvModel : public CBaseAnimating
{
	void Spawn(void) override;
	void Precache(void) override;
	void EXPORT Think(void);
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	int ObjectCaps(void) override { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void SetSequence(void);

	string_t m_iszSequence_On;
	string_t m_iszSequence_Off;
	int m_iAction_On;
	int m_iAction_Off;
};

TYPEDESCRIPTION CEnvModel::m_SaveData[] =
{
	DEFINE_FIELD(CEnvModel, m_iszSequence_On, FIELD_STRING),
	DEFINE_FIELD(CEnvModel, m_iszSequence_Off, FIELD_STRING),
	DEFINE_FIELD(CEnvModel, m_iAction_On, FIELD_INTEGER),
	DEFINE_FIELD(CEnvModel, m_iAction_Off, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CEnvModel, CBaseAnimating);
LINK_ENTITY_TO_CLASS(env_model, CEnvModel);

bool CEnvModel::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszSequence_On"))
	{
		m_iszSequence_On = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszSequence_Off"))
	{
		m_iszSequence_Off = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_On"))
	{
		m_iAction_On = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iAction_Off"))
	{
		m_iAction_Off = atoi(pkvd->szValue);
		return true;
	}

	return CBaseAnimating::KeyValue(pkvd);
}

void CEnvModel::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), (char*)STRING(pev->model));
	UTIL_SetOrigin(pev, pev->origin);

	if ((pev->spawnflags & SF_ENVMODEL_SOLID) != 0)
	{
		pev->solid = SOLID_SLIDEBOX;
		UTIL_SetSize(pev, Vector(-10, -10, -10), Vector(10, 10, 10));	//LRCT
	}

	if ((pev->spawnflags & SF_ENVMODEL_DROPTOFLOOR) != 0)
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR(ENT(pev));
	}

	SetBoneController(0, 0);
	SetBoneController(1, 0);

	SetSequence();

	pev->nextthink = gpGlobals->time + 0.1;

	pev->fuser1 = 12;
	ALERT(at_console, "%f is the scale\n", pev->scale);
}

void CEnvModel::Precache(void)
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

void CEnvModel::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, (pev->spawnflags & SF_ENVMODEL_OFF) == 0))
	{
		if ((pev->spawnflags & SF_ENVMODEL_OFF) != 0)
			pev->spawnflags &= ~SF_ENVMODEL_OFF;
		else
			pev->spawnflags |= SF_ENVMODEL_OFF;

		SetSequence();
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CEnvModel::Think(void)
{
	int iTemp;

	StudioFrameAdvance(); // set m_fSequenceFinished if necessary

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		if ((pev->spawnflags & SF_ENVMODEL_OFF) != 0)
			iTemp = m_iAction_Off;
		else
			iTemp = m_iAction_On;

		switch (iTemp)
		{
		case 2: // change state
			if ((pev->spawnflags & SF_ENVMODEL_OFF) != 0)
				pev->spawnflags &= ~SF_ENVMODEL_OFF;
			else
				pev->spawnflags |= SF_ENVMODEL_OFF;
			SetSequence();
			break;
		default: //remain frozen
			return;
		}
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CEnvModel::SetSequence(void)
{
	int iszSeq;

	if ((pev->spawnflags & SF_ENVMODEL_OFF) != 0)
		iszSeq = m_iszSequence_Off;
	else
		iszSeq = m_iszSequence_On;

	if (iszSeq == 0)
		return;
	pev->sequence = LookupSequence((char*)STRING(iszSeq));

	if (pev->sequence == -1)
	{
		if ((pev->targetname) == 0)
			ALERT(at_error, "env_model %s: unknown sequence \"%s\"\n", STRING(pev->targetname), STRING(iszSeq));
		else
			ALERT(at_error, "env_model: unknown sequence \"%s\"\n", STRING(pev->targetname), STRING(iszSeq));
		pev->sequence = 0;
	}

	pev->frame = 0;
	ResetSequenceInfo();

	if ((pev->spawnflags & SF_ENVMODEL_OFF) != 0)
	{
		if (m_iAction_Off == 1)
			m_fSequenceLoops = true;
		else
			m_fSequenceLoops = false;
	}
	else
	{
		if (m_iAction_On == 1)
			m_fSequenceLoops = true;
		else
			m_fSequenceLoops = false;
	}
}
