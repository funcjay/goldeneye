/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "shake.h"


LINK_ENTITY_TO_CLASS(weapon_glock, CP345);
LINK_ENTITY_TO_CLASS(weapon_9mmhandgun, CP345);
LINK_ENTITY_TO_CLASS(weapon_p345, CP345);

void CP345::Spawn()
{
	pev->classname = MAKE_STRING("weapon_p345");
	Precache();
	m_iId = WEAPON_P345;
	SET_MODEL(ENT(pev), "models/weapons/w_p345.mdl");
	m_iDefaultAmmo = P345_DEFAULT_GIVE;

	m_flMeleeWait = 0.0f;

	FallInit(); // get ready to fall down.
}

void CP345::Precache()
{
	PRECACHE_MODEL("models/weapons/v_p345.mdl");
	PRECACHE_MODEL("models/weapons/w_p345.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");

	PRECACHE_SOUND("weapons/p345/shoot.wav");
	PRECACHE_SOUND("weapons/pistol_swing.wav");
	PRECACHE_SOUND("weapons/pistol_smack.wav");

	m_usP345 = PRECACHE_EVENT(1, "events/p345.sc");
	m_usP345Melee = PRECACHE_EVENT(1, "events/p345_melee.sc");
}

bool CP345::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P345_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_NOAUTORELOAD;
	p->iId = m_iId = WEAPON_P345;
	p->iWeight = P345_WEIGHT;

	return true;
}

bool CP345::Deploy()
{
	return DefaultDeploy("models/weapons/v_p345.mdl", "models/p_9mmhandgun.mdl", m_iClip != 0 ? P345_DRAW : P345_DRAW_EMPTY, "onehanded");
}

void CP345::Holster()
{
	m_flMeleeWait = 0.0f;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	SendWeaponAnim(m_iClip != 0 ? P345_HOLSTER : P345_HOLSTER_EMPTY);
}

void CP345::PrimaryAttack()
{
	if ((m_pPlayer->m_afButtonPressed & IN_ATTACK) == 0)
		return;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.25f);
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			SendWeaponAnim(P345_DRYFIRE);
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.25f);
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(0.01f, 0.01f, 0.01f), 8192, BULLET_PLAYER_9MM, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usP345, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip == 0) ? 1 : 0, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.15f);
	m_flNextSecondaryAttack = GetNextAttackDelay(0.5f);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);	// HEV suit - indicate out of ammo condition

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5.0f, 10.0f);
}

void CP345::SecondaryAttack()
{
	m_flMeleeWait = gpGlobals->time + 0.33f;
	SendWeaponAnim(m_iClip != 0 ? P345_MELEE : P345_MELEE_EMPTY);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(1.0f);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5.0f, 10.0f);
}

void FindHullIntersection(const Vector& vecSrc, TraceResult& tr, const Vector& mins, const Vector& maxs, edict_t* pEntity);

void CP345::P345Bash()
{
	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0f)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos; // This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usP345Melee, 0.0f, g_vecZero, g_vecZero, 0, 0, 0, 0.0f, 0, 0.0f);

#ifndef CLIENT_DLL
	if (tr.flFraction < 1.0f)
	{
		// hit
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB);
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pistol_smack.wav", VOL_NORM, ATTN_NORM);
		m_pPlayer->m_iWeaponVolume = 256;

		UTIL_ScreenShake(m_pPlayer->pev->origin, 2.0f, 1.0f, 0.4f, 0.1f);
	}
#endif
}

void CP345::ItemPostFrame()
{
	if (m_flMeleeWait != 0.0f && m_flMeleeWait <= gpGlobals->time)
	{
		m_flMeleeWait = 0.0f;
		P345Bash();
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CP345::Reload()
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	bool iResult;

	if (m_iClip == 0)
		iResult = DefaultReload(P345_MAX_CLIP, P345_RELOAD_EMPTY, 3.16f);
	else
		iResult = DefaultReload(P345_MAX_CLIP, P345_RELOAD, 2.66f);

	if (iResult)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 5.0f, 10.0f);
}

void CP345::WeaponIdle()
{
	ResetEmptySound();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	switch (RANDOM_LONG(0, 6))
	{
	default:
		SendWeaponAnim(m_iClip != 0 ? P345_IDLE : P345_IDLE_EMPTY);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0f;
		break;
	case 0:
		SendWeaponAnim(m_iClip != 0 ? P345_FIDGET1 : P345_FIDGET1_EMPTY);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		break;
	case 1:
		SendWeaponAnim(m_iClip != 0 ? P345_FIDGET2 : P345_FIDGET2_EMPTY);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		break;
	case 2:
		SendWeaponAnim(m_iClip != 0 ? P345_FIDGET3 : P345_FIDGET3_EMPTY);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
		break;
	}
}



class CP345Ammo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/items/ammo_p345.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/items/ammo_p345.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(AMMO_P345_GIVE, "9mm", _9MM_MAX_CARRY) != -1)
		{
			if (!gEvilPlayerEquip)
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_glockclip, CP345Ammo);
LINK_ENTITY_TO_CLASS(ammo_9mmclip, CP345Ammo);
LINK_ENTITY_TO_CLASS(ammo_p345, CP345Ammo);
