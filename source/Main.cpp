#include "ourCommon.h"
#include "plugin.h"
#include <filesystem>
#include <map>
#include <unordered_map>

#include "AudioManager.h"

#include <CAudioEngine.h>
#include <eSurfaceType.h>

#include "CWaterLevel.h"

#include "CClock.h"
#include "CCollision.h"
#include "CCullZones.h"
#include "CCutsceneMgr.h"
#include "CAEExplosionAudioEntity.h"
#include "CAEWeatherAudioEntity.h"
#include "CExplosion.h"
#include "CFireManager.h"
#include "CGame.h"
#include "CGameLogic.h"
#include "CKeyGen.h"
#include "CMenuManager.h"
#include "CGeneral.h"
#include "CTheZones.h"
#include "CWeather.h"
#include "CWorld.h"
#include "debugmenu_public.h"
#include "include/subhook-0.8.2/subhook.h"
#include "logging.h"
#include "CEntryExitManager.h"
#include "IniReader.h"
#include <unordered_set>
#include "Loaders.h"

#ifdef QUAKE_KILLSOUNDS_TEST
#include "CDarkel.h"
#include "CPedGroup.h"
#include "CPedGroups.h"
#endif
std::unordered_map<CPed*, eWeaponType> g_pedLastFiredWeaponType;
// useles ans not neded
//#include <libsndfile/include/sndfile.h>
using namespace plugin;
namespace fs = std::filesystem;
DebugMenuAPI gDebugMenuAPI;
Buffers g_Buffers;

#ifdef QUAKE_KILLSOUNDS_TEST
static int32_t killCounter = 0;
uint32_t lastTimePedKilled = 0;
static bool wasHeadShotted = false;
ALuint buffer = 0;
void __cdecl HookedRegisterKillByPlayer(const CPed* killedPed, eWeaponType damageWeaponId, bool headShotted, int32_t playerId) {

	const uint32_t now = CTimer::m_snTimeInMilliseconds;
	if (killedPed) {
		if (headShotted) {
			ALuint buff = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/headshot.wav");
			AudioManager.PlaySource2D(buff, true, AEAudioHardware.m_fEffectMasterScalingFactor * 0.3f, CTimer::ms_fTimeScale);
			wasHeadShotted = true;
		}
		// The player that killed the ped
		CPlayerPed* player = CWorld::Players[playerId].m_pPed;
		// For streaks it has to be the same weapon (Quake behaviour)
		if (!headShotted && player)
		{
			eWeaponType weaponType = player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType;
			if (weaponType == damageWeaponId) {
				killCounter++; // increment streak
				lastTimePedKilled = now;
				wasHeadShotted = false;
				ALuint newBuffer = 0;
				switch (killCounter) {
				case 1:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/firstblood.wav");
					break;
				case 3:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/triplekill.wav");
					break;
				case 5:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/multikill.wav");
					break;
				case 6:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/rampage.wav");
					break;
				case 7:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/killingspree.wav");
					break;
				case 8:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/dominating.wav");
					break;
				case 9:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/impressive.wav");
					break;
				case 10:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/unstoppable.wav");
					break;
				case 11:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/outstanding.wav");
					break;
				case 12:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/megakill.wav");
					break;
				case 13:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/ultrakill.wav");
					break;
				case 14:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/eagleeye.wav");
					break;
				case 15:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/ownage.wav");
					break;
				case 16:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/comboking.wav");
					break;
				case 17:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/maniac.wav");
					break;
				case 18:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/ludicrouskill.wav");
					break;
				case 19:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/bullseye.wav");
					break;
				case 20:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/excellent.wav");
					break;
				case 21:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/pancake.wav");
					break;
				case 22:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/headhunter.wav");
					break;
				case 23:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/unreal.wav");
					break;
				case 24:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/assassin.wav");
					break;
				case 25:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/wickedsick.wav");
					break;
				case 26:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/massacre.wav");
					break;
				case 27:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/killingmachine.wav");
					break;
				case 28:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/monsterkill.wav");
					break;
				case 29:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/holyshit.wav");
					break;
				case 30:
					newBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/godlike.wav");
					break;
				default:
					break;
				}
				LOG("Killstreak: %d", killCounter);
				if (newBuffer != 0) {
					buffer = newBuffer;
					AudioManager.PlaySource2D(buffer, true, AEAudioHardware.m_fEffectMasterScalingFactor * 0.3f, CTimer::ms_fTimeScale);
				}
			}
		}
	}
	CDarkel::RegisterKillByPlayer(killedPed, damageWeaponId, headShotted, playerId);
}
#endif

void __fastcall HookedCAEWeaponAudioEntity__WeaponFire(
	CAEWeaponAudioEntity* thispointer, void* unused,
	eWeaponType weaponType, CPhysical* victim, int audioEventId
) {
	LOG("audioEvent %d weaponType %d", audioEventId, weaponType);
	CWeapon* weap = nullptr;
	CWeaponInfo* info = nullptr;
	if (thispointer->m_pPed && IsPedPointerValid(thispointer->m_pPed) && thispointer->m_pPed->GetWeapon())
	{
		weap = thispointer->m_pPed->GetWeapon();
		info = CWeaponInfo::GetWeaponInfo(weap->m_eWeaponType, thispointer->m_pPed->GetWeaponSkill());
	}

	if (!thispointer) {
		LOG("thispointer is null, falling back.");
		subhook_remove(subhookCAEWeaponAudioEntity__WeaponFire);
		thispointer->WeaponFire(weaponType, victim, audioEventId);
		subhook_install(subhookCAEWeaponAudioEntity__WeaponFire);
		return;
	}

	// Choose a valid entity.
	// Because AI heli's only have valid "victim" physical entity, we gotta consider that too.
	// So look for a valid one.
	CPhysical* entity = nullptr;
	if (thispointer->m_pPed && IsPedPointerValid(thispointer->m_pPed)) {
		entity = thispointer->m_pPed;
	}
	else if (victim && IsEntityPointerValid(victim)) {
		entity = victim;
	}
	if (!IsEntityPointerValid(entity)) {
		LOG("no valid entity found, falling back.");
		subhook_remove(subhookCAEWeaponAudioEntity__WeaponFire);
		thispointer->WeaponFire(weaponType, victim, audioEventId);
		subhook_install(subhookCAEWeaponAudioEntity__WeaponFire);
		return;
	}

	float dist = CVector::Distance(cameraposition, entity->GetPosition());
	bool farAway = dist >= distanceForDistantGunshot;
	bool emptyPlayed = false, dryFirePlayed = false;
	bool alternatePlayed = false;

	// Play low ammo and dryfire sounds
	if (!farAway && entity && IsEntityPointerValid(entity) && entity->m_nType == ENTITY_TYPE_PED && weap && info) {
		unsigned int ammo = weap->m_nAmmoInClip;
		unsigned short ammoClip = info->m_nAmmoClip;
		int left = (ammoClip / 3);
		if (ammo < left) {
			if (AUDIOCALL(AUDIOLOWAMMO)) {
				emptyPlayed = true;
			}
			if (ammo < 2) {
				if (AUDIOCALL(AUDIODRYFIRE)) {
					emptyPlayed = true;
				}
			}
		}
	}

	if (farAway) {
		if (AUDIOCALL(AUDIODISTANT))
		{
			alternatePlayed = true;
		}
		if (alternatePlayed) return;
	}

	// Try alternate shoot + after sounds
	for (int i = 0; i < MAX_SOUND_ALTERNATIVES; ++i) {
		std::string altShoot = "shoot" + std::to_string(i);
		std::string altAfter = "after" + std::to_string(i);
		if (AUDIOPLAY(entity->m_nModelIndex, altShoot)) {
			if (CGame::currArea <= 0) 
			{
				AUDIOPLAY(entity->m_nModelIndex, altAfter);
			}
			alternatePlayed = true;
			break;
		}
	}

	// If no alt sounds played, play singular shooting sounds
	if (!alternatePlayed) {
		if (AUDIOCALL(AUDIOSHOOT)) alternatePlayed = true;
		if (CGame::currArea <= 0 && AUDIOCALL(AUDIOAFTER)) alternatePlayed = true;
		if (alternatePlayed) return;
	}

	if (emptyPlayed || alternatePlayed) {
		return;
	}
	// Fallback to OG func if failed to play something
	LOG("fallback");
	subhook_remove(subhookCAEWeaponAudioEntity__WeaponFire);
	thispointer->WeaponFire(weaponType, victim, audioEventId);
	subhook_install(subhookCAEWeaponAudioEntity__WeaponFire);
}

void __fastcall HookedCAEWeaponAudioEntity__WeaponReload(CAEWeaponAudioEntity* thispointer, void* unusedpointer,
	eWeaponType weaponType, CPhysical* entity, int audioEventId
) {
	LOG("HookedCAEWeaponAudioEntity__WeaponReload: weaponType: %d, audioEventId: %d", weaponType, audioEventId);
	if (thispointer && entity)
	{
		switch (audioEventId) {
		// Some weapons have two reload sounds, take them in account too
		case AE_WEAPON_RELOAD_A: // reload one
			if (AUDIOCALL(AUDIORELOAD1))
			{
				return;
			}
			break;
		case AE_WEAPON_RELOAD_B: // reload two
			if (AUDIOCALL(AUDIORELOAD2))
			{
				return;
			}
			break;
		default: // single reload
			if (AUDIOCALL(AUDIORELOAD))
			{
				return;
			}
			break;
		}
	}
	LOG("fallback");
	subhook_remove(subhookCAEWeaponAudioEntity__WeaponReload);
	thispointer->WeaponReload(weaponType, entity, audioEventId);
	subhook_install(subhookCAEWeaponAudioEntity__WeaponReload);
}
// a2 is the audioevent
void __fastcall HookedCAEPedAudioEntity__HandlePedHit(CAEPedAudioEntity* thispointer, void* unusedpointer,
	int AudioEvent, CPhysical* victim, uint8_t Surface, float volume, uint32_t maxVolume
) {
	//	if (!thispointer || !thispointer->m_pPed)
	//		return;
	CPhysical* entity = victim;
	CPed* ped = thispointer->m_pPed;
	eWeaponType weaponType = ped->m_aWeapons[ped->m_nSelectedWepSlot].m_eWeaponType;
	gentInfo[ped] = Surface;
	//auto task = entity->m_pIntelligence->GetTaskFighting();
	LOG("AudioEvent: %d, Surface: %d, volume: %.f, maxVolume: %d, weaponType: %d, victim model ID: %d",
		AudioEvent, Surface, volume, maxVolume, weaponType, victim ? victim->m_nModelIndex : -1);
	// door surface(?): 43
	// special door model: 1532 (has default surface by default, why?)
	// Sometimes the function can return the surface higher than 179 (max surfaces in the game), so check if it's within the valid range
	bool metalSurface = Surface <= TOTAL_NUM_SURFACE_TYPES ? IsAudioMetal(eSurfaceType(Surface)) || Surface == SURFACE_GLASS : false/*(Surface == 53 || Surface == 63)*/;
	bool woodSurface = Surface <= TOTAL_NUM_SURFACE_TYPES ? IsAudioWood(eSurfaceType(Surface)) || (victim && (victim->m_nModelIndex == 1532 || victim->m_nModelIndex == 1500 || victim->m_nModelIndex == 1478 || victim->m_nModelIndex == 1432) || IsWood(Surface)) : false;
	bool handled = false;
	if (entity) {
		switch (AudioEvent) {
		case AE_PED_HIT_HIGH:
		case AE_PED_HIT_LOW:
		case AE_PED_HIT_GROUND: case AE_PED_HIT_GROUND_KICK:
		case AE_PED_HIT_HIGH_UNARMED: case AE_PED_HIT_LOW_UNARMED:
		case AE_PED_HIT_MARTIAL_PUNCH: case AE_PED_HIT_MARTIAL_KICK:
		{
			// Martial attacks
			if (AudioEvent == AE_PED_HIT_MARTIAL_PUNCH && AUDIOCALL(AUDIOMARTIALPUNCH)) {
				LOG("AUDIOMARTIALPUNCH success");
				handled = true;
			}
			if (!handled && AudioEvent == AE_PED_HIT_MARTIAL_KICK && AUDIOCALL(AUDIOMARTIALKICK)) {
				LOG("AUDIOMARTIALKICK success");
				handled = true;
			}

			// Surface-specific
			if (!handled && metalSurface && AUDIOCALL(AUDIOHITMETALT)) {
				LOG("AUDIOHITMETALT success");
				handled = true;
			}
			if (!handled && woodSurface && AUDIOCALL(AUDIOHITWOOD)) {
				LOG("AUDIOHITWOOD success");
				handled = true;
			}
			if (!handled && (AudioEvent == AE_PED_HIT_GROUND || AudioEvent == AE_PED_HIT_GROUND_KICK) && AUDIOCALL(AUDIOHITGROUND)) {
				LOG("AUDIOHITGROUND success");
				handled = true;
			}

			// Fallback for hitting flesh
			if (!handled && !woodSurface && (victim && victim->m_nType == ENTITY_TYPE_PED) && IsFleshy(Surface) && AUDIOCALL(AUDIOHIT)) {
				LOG("AUDIOHIT fallback success");
				handled = true;
			}

			break;
		}

		default:
			LOG("Unhandled AudioEvent %d", AudioEvent);
			break;
		}
	}


	if (!handled) {
		LOG("Falling back to original HandlePedHit");
		subhook_remove(subhookCAEPedAudioEntity__HandlePedHit);
		plugin::CallMethod<0x4E1CC0, CAEPedAudioEntity*, int, CPhysical*, uint8_t, float, uint32_t>(
			thispointer, AudioEvent, victim, Surface, volume, maxVolume);
		subhook_install(subhookCAEPedAudioEntity__HandlePedHit);
	}
}

// Custom ricochet sounds
int __fastcall HookedCAudioEngine__ReportBulletHit(CAudioEngine* engine, int, CEntity* victim, uint8_t surface, const CVector& posn, float angleWithColPointNorm)
{
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;
	// cast to uint8_t to avoid garbage values like "-60030976" or "309067776", etc.
	//uint8_t actualSurface = static_cast<uint8_t>(surface);

	if (engine && surface >= 0 && surface <= TOTAL_NUM_SURFACE_TYPES /*&& victim*/) {
		LOG("surface %d", surface);
		std::string surfaceType = "default";
		bool surfaceResolved = false;

		switch (surface) {
		case SURFACE_PED:
		case SURFACE_GORE:
			surfaceType = "flesh";
			surfaceResolved = true;
			break;
		case SURFACE_GLASS:
		case SURFACE_GLASS_WINDOWS_LARGE:
		case SURFACE_GLASS_WINDOWS_SMALL:
		case SURFACE_UNBREAKABLE_GLASS:
			surfaceType = "glass";
			surfaceResolved = true;
			break;
		default:
			break; // keep checking
		}

		if (!surfaceResolved) {
			// If it's a metal surface OR it's a vehicle and a gravel surface, choose metal surface anyway
			// Because vehicles can have gravel surfaces on them for some reason
			if (IsAudioMetal(surface) || (victim && victim->m_nType == ENTITY_TYPE_VEHICLE && surface == SURFACE_GRAVEL)) {
				surfaceType = "metal"; surfaceResolved = true;
			}
			else if (IsAudioWater(surface) || IsWater(surface) || IsShallowWater(surface)) {
				surfaceType = "water"; surfaceResolved = true;
			}
			else if (IsAudioWood(surface)) {
				surfaceType = "wood"; surfaceResolved = true;
			}
			else if (IsAudioSand(surface)) {
				surfaceType = "sand"; surfaceResolved = true;
			}
			else if (IsAudioConcrete(surface)) {
				surfaceType = "stone"; surfaceResolved = true;
			}
			else if (IsAudioGrass(surface) ||
				IsAudioLongGrass(surface) ||
				IsAudioGravel(surface)) {
				surfaceType = "dirt"; surfaceResolved = true;
			}
		}

		auto& buffers = g_Buffers.ricochetBuffersPerMaterial[surfaceType];

		if (!surfaceResolved && buffers.empty()) {
			auto& defBuf = g_Buffers.ricochetBuffersPerMaterial["default"];
			if (!defBuf.empty()) {
				buffers = defBuf;
				LOG("Fallback to default ricochet sounds");
			}
		}

		if (!buffers.empty()) {
			RandomIntegers rnd(buffers.size());

			int index = rnd.next();
			//int index = CGeneral::GetRandomNumber() % buffers.size();
			ALuint ricochetBuf = buffers[index];
			SoundInstanceSettings opts;

			opts.maxDist = gAttenuationSettings.ricochet[surface].maxDist; // 50.0f
			opts.gain = gameVol;
			opts.airAbsorption = gAttenuationSettings.ricochet[surface].airAbsorption; // 3.0f
			opts.refDist = gAttenuationSettings.ricochet[surface].refDist; // 3.5f
			opts.rollOffFactor = gAttenuationSettings.ricochet[surface].rolloffFactor; // 5.0f
			if (gPitches.ricochet[surface].has_value())
			{
				opts.readPitchFromFile = gPitches.ricochet[surface].has_value();
				opts.readPitch = *gPitches.ricochet[surface];
			}
			opts.pos = posn;
			AudioManager.PlaySource(ricochetBuf, opts);

			return int(-1);
		}
	}

	//if (surface <= TOTAL_NUM_SURFACE_TYPES) 
//	{
	LOG("fallback to vanilla for surface %d", surface);
	subhook_remove(subhookCAudioEngine__ReportBulletHit);
	auto returnvalue = CallMethodAndReturn<int, 0x506EC0, CAudioEngine*, CEntity*, uint8_t, const CVector&, float>(
		engine, victim, surface, posn, angleWithColPointNorm);
	subhook_install(subhookCAudioEngine__ReportBulletHit);

	return returnvalue;
	//	}
}


char __fastcall HookedCAEPedAudioEntity__HandlePedSwing(CAEPedAudioEntity* thispointer, void* unusedpointer,
	int a2, int a3, int a4
) {
	LOG("audio event %d", a2);
	if (
		thispointer
		//&&	
		) {
		auto ped = thispointer->m_pPed;
		auto entity = (CPhysical*)ped;
		if (ped && ped->GetWeapon()) {
			auto weaponType = ped->GetWeapon()->m_eWeaponType;
			if (AUDIOCALL(AUDIOSWING))
			{
				return char(-1);
			}
		}
	}

	subhook_remove(subhookCAEPedAudioEntity__HandlePedSwing);
	auto returnvalue = plugin::CallMethodAndReturn<char, 0x4E1A40, CAEPedAudioEntity*, int, int, int>(thispointer, a2, a3, a4);
	subhook_install(subhookCAEPedAudioEntity__HandlePedSwing);

	return returnvalue;
}
void __fastcall HookedCAEExplosionAudioEntity_AddAudioEvent(
	CAEExplosionAudioEntity* t,
	void* unusedpointer,
	int Aevent,
	CVector* posn,
	float volume
) {
	LOG("event %d volume %.2f", Aevent, volume);

	float dist = CVector::Distance(cameraposition, *posn);
	constexpr float SPEED_OF_SOUND = 343.3f;
	float soundDelay = dist / SPEED_OF_SOUND;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	float gain = AEAudioHardware.m_fEffectMasterScalingFactor;
	gain *= fader;
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	// We only process those that aren't really far away (except for distant explosions)
	bool farAway = dist >= distanceForDistantExplosion;
	bool handled = false;
	float waterLevel = 0.0f;
	int lastExplosionType = 0;

	if (!g_lastExplosionType.empty()) {
		lastExplosionType = g_lastExplosionType.begin()->second;
	}
	bool notAMolotov = lastExplosionType != EXPLOSION_MOLOTOV;
	bool isUnderWater = CWaterLevel::GetWaterLevelNoWaves(posn->x, posn->y, posn->z, &waterLevel);
	auto OG = [&]()
		{
			LOG("fallback to vanilla");
			subhook_remove(subhookCAEExplosionAudioEntity__AddAudioEvent);
			plugin::CallMethod<0x4DCBE0, CAEExplosionAudioEntity*, int, CVector*, float>(t, Aevent, posn, volume);
			subhook_install(subhookCAEExplosionAudioEntity__AddAudioEvent);
		};

	auto chooseBuffers = [&](auto& explosionContainer,
		const std::vector<ALuint>* genericFallback, const char* what)
		-> const std::vector<ALuint>*
		{
			auto it = explosionContainer.find(lastExplosionType);
			if (it != explosionContainer.end() && !it->second.empty()) {
				LOG("Using ExplosionType: %d for: %s", lastExplosionType, what);
				return &it->second;
			}

			// Only suppress generic fallback if THIS specific category
			// has a dedicated entry for this type (even if empty after loading)
			bool hasCategoryEntry = explosionContainer.count(lastExplosionType) > 0;

			if (!hasCategoryEntry && genericFallback && !genericFallback->empty() && notAMolotov) {
				LOG("Using generic fallback for %s", what);
				return genericFallback;
			}

			LOG("No buffers for %s (%s), ignoring...", what,
				hasCategoryEntry ? "category entry exists" : "no category entry");
			return nullptr;
		};

	auto* explosionUnderwaterBuffers =
		chooseBuffers(g_Buffers.ExplosionTypeUnderwaterBuffers,
			&g_Buffers.explosionUnderwaterBuffers, "underwater explosion");

	if (!farAway) {
		// Underwater explosion
		if (isUnderWater && posn->z < waterLevel - 3.0f && explosionUnderwaterBuffers) {
			RandomIntegers rnd(explosionUnderwaterBuffers->size());
			auto inst = std::make_shared<SoundInstance>();
			int id = rnd.next();
			ALuint buff = (*explosionUnderwaterBuffers)[id];
			SoundInstanceSettings opts;

			opts.maxDist = gAttenuationSettings.underwater[lastExplosionType].maxDist; // 4000.0f
			opts.gain = gain;
			opts.airAbsorption = gAttenuationSettings.underwater[lastExplosionType].airAbsorption; // 3.0f
			opts.refDist = gAttenuationSettings.underwater[lastExplosionType].refDist; // 15.0f
			opts.rollOffFactor = gAttenuationSettings.underwater[lastExplosionType].rolloffFactor; // 0.20f
			opts.pitch = pitch;
			if (gPitches.underwater[lastExplosionType].has_value())
			{
				opts.readPitchFromFile = gPitches.underwater[lastExplosionType].has_value();
				opts.readPitch = *gPitches.underwater[lastExplosionType];
			}
			opts.pos = *posn;
			handled = AudioManager.PlaySource(buff, opts) != nullptr;
		}
		// Main explosion
		else if (auto* explosionBuffers =
			chooseBuffers(g_Buffers.ExplosionTypeExplosionBuffers,
			&g_Buffers.explosionBuffers, "main explosion"))
		{
			RandomIntegers rnd(explosionBuffers->size());

			int id = rnd.next();
			ALuint buff = (*explosionBuffers)[id];
			SoundInstanceSettings opts;
			opts.maxDist = gAttenuationSettings.explosion[lastExplosionType].maxDist; // notAMolotov ? distanceForDistantExplosion : FLT_MAX
			opts.gain = gain;
			opts.airAbsorption = gAttenuationSettings.explosion[lastExplosionType].airAbsorption; // 0.6f
			opts.refDist = gAttenuationSettings.explosion[lastExplosionType].refDist;// notAMolotov ? 10.0f : 1.5f;
			opts.rollOffFactor = gAttenuationSettings.explosion[lastExplosionType].rolloffFactor; // notAMolotov ? 0.7f : 1.3f;
			opts.pitch = pitch;
			if (gPitches.explosion[lastExplosionType].has_value())
			{
				opts.readPitchFromFile = gPitches.explosion[lastExplosionType].has_value();
				opts.readPitch = *gPitches.explosion[lastExplosionType];
			}
			opts.pos = *posn;
			handled = AudioManager.PlaySource(buff, opts) != nullptr;
		}

		// Debris
		if (auto* debrisBuffers =
			chooseBuffers(g_Buffers.ExplosionTypeDebrisBuffers,
				&g_Buffers.explosionsDebrisBuffers, "debris"))
		{
			RandomIntegers rnd(debrisBuffers->size());

			int id = rnd.next();
			ALuint buff = (*debrisBuffers)[id];
			SoundInstanceSettings opts;
			opts.maxDist = gAttenuationSettings.debris[lastExplosionType].maxDist; // 100.0f
			opts.gain = gain;
			opts.airAbsorption = gAttenuationSettings.debris[lastExplosionType].airAbsorption; // 0.8f
			opts.refDist = gAttenuationSettings.debris[lastExplosionType].refDist; // 7.0f
			opts.rollOffFactor = gAttenuationSettings.debris[lastExplosionType].rolloffFactor; // 1.0f
			opts.pitch = pitch;
			if (gPitches.debris[lastExplosionType].has_value())
			{
				opts.readPitchFromFile = gPitches.debris[lastExplosionType].has_value();
				opts.readPitch = *gPitches.debris[lastExplosionType];
			}
			opts.pos = *posn;
			handled = AudioManager.PlaySource(buff, opts) != nullptr;
		}
	}
	else {
		// Distant explosion
		if (auto* distantBuffers =
			chooseBuffers(g_Buffers.ExplosionTypeDistantBuffers,
				/*g_Buffers.WeaponTypeDistantBuffers,*/
				&g_Buffers.explosionDistantBuffers, "distant explosion"))
		{
			RandomIntegers rnd(distantBuffers->size());

			int id = rnd.next();
			ALuint buff = (*distantBuffers)[id];
			SoundInstanceSettings opts;
			opts.maxDist = gAttenuationSettings.distexplosion[lastExplosionType].maxDist; // 200.0f
			opts.gain = gain;
			opts.airAbsorption = gAttenuationSettings.distexplosion[lastExplosionType].airAbsorption; // 0.6f
			opts.refDist = gAttenuationSettings.distexplosion[lastExplosionType].refDist; // 20.0f
			opts.rollOffFactor = gAttenuationSettings.distexplosion[lastExplosionType].rolloffFactor; // 0.5f
			opts.pitch = pitch;
			if (gPitches.distexplosion[lastExplosionType].has_value())
			{
				opts.readPitchFromFile = gPitches.distexplosion[lastExplosionType].has_value();
				opts.readPitch = *gPitches.distexplosion[lastExplosionType];
			}
			opts.pos = *posn;
			ALuint filter;
			alGenFilters(1, &filter);
			alFilteri(filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
			alFilterf(filter, AL_LOWPASS_GAIN, 1.2f);
			alFilterf(filter, AL_LOWPASS_GAINHF, 0.20f); // aggressive HF cut
			opts.filter = filter;
			AudioManager.ScheduleDelayedSound(buff, opts, soundDelay);			
			handled = true;
		}
	}


	if (!handled)
	{
		// fallback to vanilla
		OG();
	}

}

void __fastcall HookedCAEFireAudioEntity__AddAudioEvent(CAEFireAudioEntity* ts, int, int eventId, CVector* posn) {
	// Keep track of CAEFireAudioEntity to check FX existence later
	if (std::find(g_Buffers.ent.begin(), g_Buffers.ent.end(), ts) == g_Buffers.ent.end()) {
		g_Buffers.ent.push_back(ts);
	}
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);

	auto SafeDeleteInstanceSource = [&](std::shared_ptr<SoundInstance> inst) {
		if (!inst) return;
		if (inst->source != 0) {
			alDeleteSources(1, &inst->source);
			inst->source = 0;
		}

		inst->entity = nullptr;
		inst->shooter = nullptr;
		inst->firePtr = nullptr;
		inst->paused = false;
		inst->isFire = false;
		inst.reset();
		};

	auto EnsureNonFireInstanceValid = [&](int evt) -> bool {
		auto it = g_Buffers.nonFireSounds.find(evt);
		if (it == g_Buffers.nonFireSounds.end()) return false;

		auto inst = it->second;
		if (!inst) { g_Buffers.nonFireSounds.erase(it); return false; }

		ALint state = AudioManager.GetSourceState(inst->source);
		if (state == AL_STOPPED || state == -1) {
			SafeDeleteInstanceSource(inst);
			g_Buffers.nonFireSounds.erase(it);
			return false;
		}

		return true;
		};

	auto GetOrCleanupFireInstance = [&](CFire* fire) -> std::shared_ptr<SoundInstance> {
		auto it = g_Buffers.fireSounds.find(fire);
		if (it == g_Buffers.fireSounds.end()) return nullptr;

		auto inst = it->second;
		if (!inst) { g_Buffers.fireSounds.erase(it); return nullptr; }

		if (!fire->m_nFlags.bActive || !fire->m_nFlags.bMakesNoise) {
			SafeDeleteInstanceSource(inst);
			g_Buffers.fireSounds.erase(it);
			return nullptr;
		}

		ALint state = AudioManager.GetSourceState(inst->source);
		if (state == AL_STOPPED || state == -1) {
			SafeDeleteInstanceSource(inst);
			g_Buffers.fireSounds.erase(it);
			return nullptr;
		}


		if (inst->entity || inst->shooter) {
			inst->entity = nullptr;
			inst->shooter = nullptr;
		}

		return inst;
		};

	auto PlayOrUpdatePositional = [&](int evt, const std::vector<ALuint>& buffers, const CVector& position) -> bool {
		if (buffers.empty()) return false;

		if (EnsureNonFireInstanceValid(evt)) {
			auto& inst = g_Buffers.nonFireSounds[evt];
			if (inst && inst->source != 0) {
				alSource3f(inst->source, AL_POSITION, position.x, position.y, position.z);

				ALint state = AudioManager.GetSourceState(inst->source);
				if (state == AL_PAUSED) {
					if (inst->paused) {
						AudioManager.ResumeSource(inst.get());
					}
					else {
						alSourcePlay(inst->source);
						inst->paused = false;
					}
				}
				else if (state == AL_STOPPED) {
					LOG("Played cuz stopped");
					alSourcePlay(inst->source);
				}
				return true;
			}
		}

		RandomIntegers rnd(buffers.size());
		ALuint buf = buffers[rnd.next()];
		if (buf == 0) return false;

		SoundInstanceSettings opts;
		opts.maxDist = gAttenuationSettings.nonfire.maxDist; // 200.0f
		opts.gain = gameVol;
		opts.airAbsorption = gAttenuationSettings.nonfire.airAbsorption; // 4.0f
		opts.refDist = gAttenuationSettings.nonfire.refDist; // 1.0f
		opts.rollOffFactor = gAttenuationSettings.nonfire.rolloffFactor; // 1.5f
		opts.pitch = pitch;
		if (gPitches.nonfire.has_value())
		{
			opts.readPitchFromFile = gPitches.nonfire.has_value();
			opts.readPitch = *gPitches.nonfire;
		}
		opts.pos = position;
		opts.isFire = false;
		opts.fireFX = ts->field_84;
		opts.firePtr = nullptr;
		opts.fireEventID = evt;
		opts.looping = true;
		bool ok = AudioManager.PlaySource(buf, opts) != nullptr;

		return ok;
		};

	auto PlayOrUpdateFireLoop = [&](CFire* fire, const std::vector<ALuint>& buffers) -> bool {
		if (!fire->m_nFlags.bActive || !fire->m_nFlags.bMakesNoise || buffers.empty()) return false;

		CVector pos = fire->m_vecPosition;
		auto inst = GetOrCleanupFireInstance(fire);
		if (inst) {
			if (inst->source != 0) {
				alSource3f(inst->source, AL_POSITION, pos.x, pos.y, pos.z);
				ALint state = AudioManager.GetSourceState(inst->source);

				if (state == AL_PAUSED) {
					if (inst->paused) AudioManager.ResumeSource(inst.get());
					else {
						alSourcePlay(inst->source);
						inst->paused = false;
					}
				}
				else if (state == AL_STOPPED) {
					alSourcePlay(inst->source);
					inst->paused = false;
				}
				return true;
			}
		}

		RandomIntegers rnd(buffers.size());
		ALuint buf = buffers[rnd.next()];
		if (buf == 0) return false;

		SoundInstanceSettings opts;
		opts.maxDist = gAttenuationSettings.fire.maxDist; // 200.0f
		opts.gain = gameVol;
		opts.airAbsorption = gAttenuationSettings.fire.airAbsorption; // 4.0f
		opts.refDist = gAttenuationSettings.fire.refDist; // 1.0f
		opts.rollOffFactor = gAttenuationSettings.fire.rolloffFactor; // 1.5f
		opts.pitch = pitch;
		if (gPitches.fire.has_value())
		{
			opts.readPitchFromFile = gPitches.fire.has_value();
			opts.readPitch = *gPitches.fire;
		}
		opts.pos = pos;
		opts.isFire = true;
		opts.firePtr = fire;
		bool ok = AudioManager.PlaySource(buf, opts) != nullptr;
		if (ok) {
			auto it = g_Buffers.fireSounds.find(fire);
			if (it != g_Buffers.fireSounds.end()) {
				auto newInst = it->second;
				if (newInst) {
					newInst->entity = nullptr;
					newInst->shooter = nullptr;
					newInst->isFire = true;
				}
			}
		}
		return ok;
		};

	bool handled = false;

	if (posn) {
		switch (eventId) {
		case AE_FIRE_CAR:
			handled |= PlayOrUpdatePositional(AE_FIRE_CAR, g_Buffers.fireLoopBuffersCar, *posn);
			break;
		case AE_FIRE_BIKE:
			handled |= PlayOrUpdatePositional(AE_FIRE_BIKE, g_Buffers.fireLoopBuffersBike, *posn);
			break;
		case AE_FIRE_FLAME:
			handled |= PlayOrUpdatePositional(AE_FIRE_FLAME, g_Buffers.fireLoopBuffersFlame, *posn);
			break;
		case AE_FIRE_MOLOTOV_FLAME:
			handled |= PlayOrUpdatePositional(AE_FIRE_MOLOTOV_FLAME, g_Buffers.fireLoopBuffersMolotov, *posn);
			break;
		default: break;
		}
	}

	for (int i = 0; i < MAX_NUM_FIRES; i++) {
		CFire* fire = &gFireManager.m_aFires[i];

		switch (eventId) {
		case AE_FIRE:
			handled |= PlayOrUpdateFireLoop(fire, g_Buffers.fireLoopBuffersSmall);
			break;
		case AE_FIRE_MEDIUM:
			handled |= PlayOrUpdateFireLoop(fire, g_Buffers.fireLoopBuffersMedium);
			break;
		case AE_FIRE_LARGE:
			handled |= PlayOrUpdateFireLoop(fire, g_Buffers.fireLoopBuffersLarge);
			break;
		default: break;
		}
	}

	// Something hasn't worked out, call OG
	if (!handled) {
		subhook_remove(subhookCAEFireAudioEntity__AddAudioEvent);
		plugin::CallMethod<0x4DD3C0, CAEFireAudioEntity*, int, CVector*>(ts, eventId, posn);
		subhook_install(subhookCAEFireAudioEntity__AddAudioEvent);
	}
}

void __fastcall PlayMinigunBarrelStopSound(CAEWeaponAudioEntity* ts, int, CPed* ped)
{
	// play the stop sound ONCE
	if (ped)
	{
		eWeaponType weaponType = ped->GetWeapon()->m_eWeaponType;
		bool foundSound = AudioManager.findWeapon(&weaponType, eModelID(-1), "minigun_barrelspinend", ped, false);
		if (foundSound) 
		{
			AudioManager.PlayOrStopBarrelSpinSound(ped, &weaponType, false, true, true);
			return;
		}
	}
	ts->PlayMiniGunStopSound(ped);
}

char __fastcall CAEPedAudioEntity__HandlePedJacked(CAEPedAudioEntity* ts, void*, int AudioEvent)
{
	LOG("AudioEvent is %d", AudioEvent);
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;
	if (/*!cameraposition ||*/ !ts || !ts->m_pPed) {
		subhook_remove(subhookCAEPedAudioEntity__HandlePedJacked);
		char returnvalue = CallMethodAndReturn<char, 0x4E2350, CAEPedAudioEntity*, int>(ts, AudioEvent);
		subhook_install(subhookCAEPedAudioEntity__HandlePedJacked);
		return returnvalue;
	}

	bool handled = false;
	CVector PedPos = ts->m_pPed->GetPosition();

	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);

	auto PlayJackingSound = [&](const std::vector<ALuint>& bufferList) {
		if (bufferList.empty()) return false;
		RandomIntegers rnd(bufferList.size());

		int idx = rnd.next();
		//int idx = CGeneral::GetRandomNumber() % bufferList.size();
		ALuint buf = bufferList[idx];
		// recheck vol
		SoundInstanceSettings opts;
		opts.maxDist = gAttenuationSettings.jacked.maxDist; // FLT_MAX
		opts.gain = gameVol;
		opts.airAbsorption = gAttenuationSettings.jacked.airAbsorption; // 0.8f
		opts.refDist = gAttenuationSettings.jacked.refDist; // 3.0f
		opts.rollOffFactor = gAttenuationSettings.jacked.rolloffFactor; // 1.5f
		opts.pitch = pitch;
		if (gPitches.jacked.has_value())
		{
			opts.readPitchFromFile = gPitches.jacked.has_value();
			opts.readPitch = *gPitches.jacked;
		}
		opts.pos = PedPos;
		if (AudioManager.PlaySource(buf, opts))
		{
			return true;
		}
		return false;
		};

	switch (AudioEvent) {
	case AE_PED_JACKED_CAR_PUNCH: // Jacked: punch
		LOG("Playing jacked punch sound");
		handled = PlayJackingSound(g_Buffers.carJackBuff);
		break;
	case AE_PED_JACKED_CAR_HEAD_BANG: // Head bang
		LOG("Playing jacked headbang sound");
		handled = PlayJackingSound(g_Buffers.carJackHeadBangBuff);
		break;
	case AE_PED_JACKED_CAR_KICK: // Kick
		LOG("Playing jacked kick sound");
		handled = PlayJackingSound(g_Buffers.carJackKickBuff);
		break;
	case AE_PED_JACKED_BIKE: // Bike
		LOG("Playing jacked bike sound");
		handled = PlayJackingSound(g_Buffers.carJackBikeBuff);
		break;
	case AE_PED_JACKED_DOZER: // Dozer
		LOG("Playing jacked dozer sound");
		handled = PlayJackingSound(g_Buffers.carJackBulldozerBuff);
		break;
	}

	if (!handled) {
		subhook_remove(subhookCAEPedAudioEntity__HandlePedJacked);
		char returnvalue = CallMethodAndReturn<char, 0x4E2350, CAEPedAudioEntity*, int>(ts, AudioEvent);
		subhook_install(subhookCAEPedAudioEntity__HandlePedJacked);
		return returnvalue;
	}
}

// footsteps
void __fastcall HookedCAEPedAudioEntity__AddAudioEvent(CAEPedAudioEntity* ts, void*, eAudioEvents audioEvent, float volume, float speed, CPhysical* ped, uint8_t surfaceId, int32_t a7, uint32_t maxVol)
{
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float FinalPitch = pitch;
	bool PitchReadFromFile = false;
	if (ts && ts->m_pPed) {
		CPed* pedPtr = ts->m_pPed;
		eSurfaceType actualSurface = eSurfaceType(pedPtr->m_nContactSurface);

		if (actualSurface <= TOTAL_NUM_SURFACE_TYPES && (audioEvent == AE_PED_FOOTSTEP_LEFT || audioEvent == AE_PED_FOOTSTEP_RIGHT)) {
			std::string surfaceType = "default";

			switch (actualSurface) {
			case SURFACE_PED:
			case SURFACE_GORE: surfaceType = "flesh"; break;
			case SURFACE_GLASS:
			case SURFACE_GLASS_WINDOWS_LARGE: surfaceType = "glass"; break;
			default:
				if (IsAudioGrass(actualSurface)) surfaceType = "grass";
				else if (IsAudioWood(actualSurface)) surfaceType = "wood";
				else if (IsAudioMetal(actualSurface)) surfaceType = "metal";
				else if (IsAudioSand(actualSurface)) surfaceType = "sand";
				else if (IsAudioGravel(actualSurface)) surfaceType = "dirt";
				else if (IsAudioConcrete(actualSurface)) surfaceType = "pavement";
				else if (IsAudioWater(actualSurface) || IsWater(actualSurface) || IsShallowWater(actualSurface)) surfaceType = "water";
				else if (IsAudioTile(actualSurface)) surfaceType = "tile";
				break;
			}
			LOG("Current surface type for footsteps: %s, ID %d", surfaceType.c_str(), actualSurface);
			std::string shoeType = "default";
			std::string shoeTexture = "default";
			CPedClothesDesc* clothesDesc = pedPtr->m_pPlayerData ? pedPtr->m_pPlayerData->m_pPedClothesDesc : nullptr;

			if (clothesDesc) {
				unsigned int modelId = clothesDesc->m_anModelKeys[CLOTHES_MODEL_SHOES];
				unsigned int texture = clothesDesc->m_anTextureKeys[CLOTHES_TEXTURE_SHOES];
				for (const auto& [folder, surfaceMap] : g_Buffers.footstepShoeBuffers) {
					if (CKeyGen::GetUppercaseKey(folder.c_str()) == modelId) {
						shoeType = folder;
						break;
					}
				}
				for (const auto& [folder, textureMap] : g_Buffers.footstepShoeByTextureBuffers) {
					if (CKeyGen::GetUppercaseKey(folder.c_str()) == texture) {
						shoeTexture = folder;
						break;
					}
				}
			}

			std::vector<ALuint>* selected = nullptr;

			// Try to find a special shoe sound
			auto shoeIt = g_Buffers.footstepShoeBuffers.find(shoeType);
			if (shoeIt != g_Buffers.footstepShoeBuffers.end()) {
				auto surfIt = shoeIt->second.find(surfaceType);
				if (surfIt != shoeIt->second.end() && !surfIt->second.empty()) {
					selected = &surfIt->second;
				}
			}

			// Try to find a special shoe sound but by a texture this time
			auto shoeTexIt = g_Buffers.footstepShoeByTextureBuffers.find(shoeTexture);
			if (shoeTexIt != g_Buffers.footstepShoeByTextureBuffers.end()) {
				auto surfIt = shoeTexIt->second.find(surfaceType);
				if (surfIt != shoeTexIt->second.end() && !surfIt->second.empty()) {
					selected = &surfIt->second;
				}
			}

			// If none, we search for generic sounds
			if (!selected) {
				auto surfIt = g_Buffers.footstepSurfaceBuffers.find(surfaceType);
				if (surfIt != g_Buffers.footstepSurfaceBuffers.end() && !surfIt->second.empty()) {
					selected = &surfIt->second;
				}
			}

			// Fallback to default
			if (!selected) {
				auto surfIt = g_Buffers.footstepSurfaceBuffers.find("default");
				if (surfIt != g_Buffers.footstepSurfaceBuffers.end() && !surfIt->second.empty()) {
					selected = &surfIt->second;
				}
			}
			// Play them
			if (selected && !selected->empty()) {

				//int index = CGeneral::GetRandomNumber() % selected->size();
				RandomIntegers rnd(selected->size());

				int index = rnd.next();
				ALuint buffer = (*selected)[index];
				//CVector position = pedPtr->GetPosition();
				float referenceDistance, maxDist, airAbs, rollOff;

				if (pedPtr->IsPlayer()) {
					if (pedPtr->bIsDucking) {
						referenceDistance = gAttenuationSettings.footstepsPlayerDuck.refDist; // 0.1f
						maxDist = gAttenuationSettings.footstepsPlayerDuck.maxDist;
						airAbs = gAttenuationSettings.footstepsPlayerDuck.airAbsorption;
						rollOff = gAttenuationSettings.footstepsPlayerDuck.rolloffFactor;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_SPRINT) {
						referenceDistance = gAttenuationSettings.footstepsPlayerSprint.refDist; // 1.0f
						maxDist = gAttenuationSettings.footstepsPlayerSprint.maxDist;
						airAbs = gAttenuationSettings.footstepsPlayerSprint.airAbsorption;
						rollOff = gAttenuationSettings.footstepsPlayerSprint.rolloffFactor;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_WALK) {
						referenceDistance = gAttenuationSettings.footstepsPlayerWalk.refDist; // 0.3f
						maxDist = gAttenuationSettings.footstepsPlayerWalk.maxDist;
						airAbs = gAttenuationSettings.footstepsPlayerWalk.airAbsorption;
						rollOff = gAttenuationSettings.footstepsPlayerWalk.rolloffFactor;
					}
					else {
						LOG("after else if walk for player");
						referenceDistance = gAttenuationSettings.footstepsPlayer.refDist; // 0.5f
						maxDist = gAttenuationSettings.footstepsPlayer.maxDist;
						airAbs = gAttenuationSettings.footstepsPlayer.airAbsorption;
						rollOff = gAttenuationSettings.footstepsPlayer.rolloffFactor;
					}
					if (gPitches.footstepsPlayer.has_value())
					{
						PitchReadFromFile = gPitches.footstepsPlayer.has_value();
						FinalPitch = *gPitches.footstepsPlayer;
					}
					LOG("Player move state: %d", pedPtr->m_nMoveState);
				}
				else {
					if (pedPtr->bIsDucking) {
						referenceDistance = gAttenuationSettings.footstepsNPCDuck.refDist; // 0.1f
						maxDist = gAttenuationSettings.footstepsNPCDuck.maxDist;
						airAbs = gAttenuationSettings.footstepsNPCDuck.airAbsorption;
						rollOff = gAttenuationSettings.footstepsNPCDuck.rolloffFactor;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_SPRINT) {
						referenceDistance = gAttenuationSettings.footstepsNPCSprint.refDist; // 0.7f
						maxDist = gAttenuationSettings.footstepsNPCSprint.maxDist;
						airAbs = gAttenuationSettings.footstepsNPCSprint.airAbsorption;
						rollOff = gAttenuationSettings.footstepsNPCSprint.rolloffFactor;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_WALK) {
						referenceDistance = gAttenuationSettings.footstepsNPCWalk.refDist; // 0.2f
						maxDist = gAttenuationSettings.footstepsNPCWalk.maxDist;
						airAbs = gAttenuationSettings.footstepsNPCWalk.airAbsorption;
						rollOff = gAttenuationSettings.footstepsNPCWalk.rolloffFactor;
					}
					else {
						LOG("after else if walk for npc");
						referenceDistance = gAttenuationSettings.footstepsNPC.refDist; // 0.3f
						maxDist = gAttenuationSettings.footstepsNPC.maxDist;
						airAbs = gAttenuationSettings.footstepsNPC.airAbsorption;
						rollOff = gAttenuationSettings.footstepsNPC.rolloffFactor;
					}
					if (gPitches.footstepsNPC.has_value())
					{
						PitchReadFromFile = gPitches.footstepsNPC.has_value();
						FinalPitch = *gPitches.footstepsNPC;
					}
					LOG("NPC move state: %d", pedPtr->m_nMoveState);
				}
				SoundInstanceSettings opts;
				opts.pos = pedPtr->GetPosition();
				opts.maxDist = maxDist;//pedPtr->IsPlayer() ? 140.0f : 150.0f; (FLT_MAX prev)
				opts.gain = gameVol;
				opts.airAbsorption = airAbs; // pedPtr->IsPlayer() ? 1.5f : 3.0f;
				opts.refDist = referenceDistance;
				opts.rollOffFactor = rollOff; // pedPtr->IsPlayer() ? 1.5f : 2.5f;
				opts.pitch = pitch;
				opts.readPitchFromFile = PitchReadFromFile;
				opts.readPitch = FinalPitch;
				opts.entity = pedPtr;
				AudioManager.PlaySource(buffer, opts);
				return;
			}
		}
	}

	// Fallback to original
	subhook_remove(subhookCAEPedAudioEntity__AddAudioEvent);
	CallMethod<0x4E2BB0, CAEPedAudioEntity*, eAudioEvents, float, float, CPhysical*, uint8_t, int32_t, uint32_t>(
		ts, audioEvent, volume, speed, ped, surfaceId, a7, maxVol);
	subhook_install(subhookCAEPedAudioEntity__AddAudioEvent);
}

void __fastcall HookedCAudioEngine__ReportWeaponEvent(CAudioEngine* engine, void*,
	int32_t audioEvent, eWeaponType weaponType, CPhysical* physical)
{
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;

	// get the vehicle that fired it's weapon
	CVehicle* vehicle = nullptr;
	for (int i = 0; i < CPools::ms_pVehiclePool->m_nSize; i++) {
		CVehicle* veh = CPools::ms_pVehiclePool->GetAt(i);
		if (veh && veh->m_nVehicleWeaponInUse) {
			vehicle = veh;
			break;
		}
	}

	// store info about the vehicle firing the weapon, if not yet
	if (vehicle)
	{
		vehInfo info{};
		info.model = vehicle->m_nModelIndex;
		info.weap = weaponType;
		gvehInfo[vehicle] = info;
		LOG("in a veh, model index: %d", vehicle->m_nModelIndex);
	}
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	bool handled = false;
	LOG("event: %d, weapon type: %d physical model ID %d", audioEvent, weaponType, physical ? physical->m_nModelIndex : -1);
	if (engine && physical) {
		switch (audioEvent) {
		case AE_WEAPON_FIRE: // gun ambience in LS

			if (AudioManager.PlayAmbienceSFX(physical->GetPosition(), weaponType, false))
			{
				LOG("playing ambient sound");
				handled = true;
			}
			break;

		case AE_PROJECTILE_FIRE: // projectile fire event


			if (vehicle && AudioManager.findWeapon(&weaponType, eModelID(vehicle->m_nModelIndex), "projfire", vehicle))
			{
				LOG("playing rocket fire sound for vehicle model '%d'", vehicle->m_nModelIndex);
				//return char(-1);
				handled = true;
			}
			// Custom missile fly sound
			if (physical)
			{
				if (physical->m_nModelIndex == 345) { // missile model id
					auto inst = std::make_shared<SoundInstance>();
					if (inst->missileSource == 0 && !g_Buffers.missileSoundBuffers.empty())
					{
						RandomIntegers rnd(g_Buffers.missileSoundBuffers.size());

						int index = rnd.next();
						ALuint buff = g_Buffers.missileSoundBuffers[index];
						SoundInstanceSettings opts;
						opts.maxDist = gAttenuationSettings.missile.maxDist; // 250.0f
						opts.gain = gameVol;
						opts.airAbsorption = gAttenuationSettings.missile.airAbsorption; // 1.0f
						opts.refDist = gAttenuationSettings.missile.refDist; // 6.0f
						opts.rollOffFactor = gAttenuationSettings.missile.rolloffFactor; // 1.0f
						opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
						if (gPitches.missile.has_value())
						{
							opts.readPitchFromFile = gPitches.missile.has_value();
							opts.readPitch = *gPitches.missile;
						}
						opts.isMissile = true;
						opts.weaponType = weaponType;
						opts.pos = physical->GetPosition();
						opts.entity = physical;
						AudioManager.PlaySource(buff, opts);
						//return char(-1);
						handled = true;
					}
				}
			}
			break;

		}
	}
	if (!handled)
	{
		LOG("fallback");
		subhook_remove(subhookCAudioEngine__ReportWeaponEvent);
		CallMethod<0x506F40, CAudioEngine*, int32_t, eWeaponType, CPhysical*>(engine, audioEvent, weaponType, physical);
		subhook_install(subhookCAudioEngine__ReportWeaponEvent);
	}
}

#if 0
// Missile flying sound
char __fastcall HookedFireProjectile(
	void* ts, int,
	eWeaponType weaponType, CPhysical* physical, eAudioEvents aEvent)
{
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;
	//g_lastWeaponType[physical] = (int)weaponType;
	LOG("weaponType '%d', audio event '%d', physical model index '%d'", weaponType, aEvent, physical ? physical->m_nModelIndex : -1);
	CAEWeaponAudioEntity* entity = reinterpret_cast<CAEWeaponAudioEntity*>(ts);
	CAEWeaponAudioEntity* entity2 = reinterpret_cast<CAEWeaponAudioEntity*>(AudioEngine.m_pWeaponAudio);
	if (entity == entity2) {
		if (entity2->m_pPed)
		{
			LOG("entity ped model index: %d", entity2->m_pPed ? entity2->m_pPed->m_nModelIndex : -1);
		}
		if (entity2->m_pPed && entity2->m_pPed->m_pVehicle)
		{
			LOG("in a veh, model index: %d", entity2->m_pPed->m_pVehicle->m_nModelIndex);
		}
		LOG("ts is not null");
	}
	CVehicle* playaVehicle = FindPlayerVehicle();
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	bool handled = false;
	if (ts) {

		if (aEvent == 0x94) // projectile fire event
		{

			if (playaVehicle && AudioManager.findWeapon(&weaponType, eModelID(playaVehicle->m_nModelIndex), "projfire", playaVehicle))
			{
				LOG("playing rocket fire sound for vehicle model '%d'", playaVehicle->m_nModelIndex);
				//return char(-1);
				handled = true;
			}
			// Custom missile fly sound
			if (physical)
			{
				if (physical->m_nModelIndex == 345) { // missile model id
					auto inst = std::make_shared<SoundInstance>();
					if (inst->missileSource == 0 && !g_Buffers.missileSoundBuffers.empty())
					{
						RandomIntegers rnd(g_Buffers.missileSoundBuffers.size());

						int index = rnd.next();
						ALuint buff = g_Buffers.missileSoundBuffers[index];
						SoundInstanceSettings opts;
						opts.maxDist = gAttenuationSettings.missile.maxDist; // 250.0f
						opts.gain = gameVol;
						opts.airAbsorption = gAttenuationSettings.missile.airAbsorption; // 1.0f
						opts.refDist = gAttenuationSettings.missile.refDist; // 6.0f
						opts.rollOffFactor = gAttenuationSettings.missile.rolloffFactor; // 1.0f
						opts.pitch = pitch;
						opts.pos = physical->GetPosition();
						opts.isMissile = true;
						opts.weaponType = weaponType;
						opts.entity = physical;
						AudioManager.PlaySource(buff, opts);
						//return char(-1);
						handled = true;
					}
				}
			}
		}
		//	g_lastWeaponType = -1;
	}
	if (!handled)
	{
		LOG("fallback");
		//	g_lastWeaponType = -1;
		return CallMethodAndReturn<char, 0x4DF060, void*, eWeaponType, CPhysical*, eAudioEvents>(ts, weaponType, physical, aEvent);
	}
}
#endif

// Custom thunders
void __fastcall HookedCAEWeatherAudioEntity__AddAudioEvent(CAEWeatherAudioEntity* ts, void*, int AudioEvent)
{
	LOG("event: %d", AudioEvent);
	if (ts && AudioEvent == 141) {
		if (!g_Buffers.ThunderBuffs.empty())
		{
			RandomIntegers rnd(g_Buffers.ThunderBuffs.size());

			int index = rnd.next();
			ALuint buffer = g_Buffers.ThunderBuffs[index];

			if (AudioManager.PlayAmbienceBuffer(buffer, cameraposition, false, true))
			{
				LOG("playing thunder sound");
				return;
			}
		}
	}
	LOG("fallback");
	CallMethod<0x506800, CAEWeatherAudioEntity*, int>(ts, AudioEvent);
}

// tank cannon fire sound (the explosion func hook)
bool __cdecl TriggerTankFireHooked(CEntity* victim, CEntity* creator, eExplosionType type, CVector pos, uint32_t lifetime, uint8_t usesSound, float cameraShake, uint8_t bInvisible)
{
	static bool isInside = false;
	g_lastExplosionType.clear();
	g_lastExplosionType[creator] = static_cast<int>(type);

	if (!isInside)
	{
		float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
		LOG("added explosion with type %d", type);

		float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
		float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
		gameVol *= fader;

		if (CPad::GetPad(0) && CPad::GetPad(0)->CarGunJustDown()) {
			if (!g_Buffers.tankCannonFireBuffers.empty() && creator && creator->m_nType == ENTITY_TYPE_PED && ((CPed*)creator)->bInVehicle && type == EXPLOSION_TANK_FIRE)
			{
				RandomIntegers rnd(g_Buffers.tankCannonFireBuffers.size());
				int index = rnd.next();
				ALuint buffer = g_Buffers.tankCannonFireBuffers[index];
				SoundInstanceSettings opts;
				opts.maxDist = gAttenuationSettings.tankcannon.maxDist;
				opts.gain = gameVol;
				opts.airAbsorption = gAttenuationSettings.tankcannon.airAbsorption;
				opts.refDist = gAttenuationSettings.tankcannon.refDist;
				opts.rollOffFactor = gAttenuationSettings.tankcannon.rolloffFactor;
				opts.pitch = pitch;
				if (gPitches.tankcannon.has_value())
				{
					opts.readPitchFromFile = gPitches.tankcannon.has_value();
					opts.readPitch = *gPitches.tankcannon;
				}
				opts.entity = (CPhysical*)creator;
				AudioManager.PlaySource(buffer, opts);
			}
		}

		isInside = true;
		subhook_remove(subhookCExplosion__AddExplosion);
		bool result = AddExplosion(victim, creator, type, pos, lifetime, usesSound, cameraShake, bInvisible);
		subhook_install(subhookCExplosion__AddExplosion);
		isInside = false;
		g_lastExplosionType.erase(creator);

		return result;
	}

	return AddExplosion(victim, creator, type, pos, lifetime, usesSound, cameraShake, bInvisible);
}

void __fastcall CAudioEngine__ReportFrontEndAudioHooked(CAudioEngine* eng, int, eAudioEvents eventId, float volumeChange, float speed)
{
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	float audiovolume = AEAudioHardware.m_fEffectMasterScalingFactor * fader;
	//LOG("CAudioEngine__ReportFrontEndAudioHooked: event '%d'", eventId);
	std::vector<ALuint>* bufferList = nullptr;

	switch (eventId) {
	case AE_FRONTEND_BULLET_PASS_LEFT_REAR:
		bufferList = &g_Buffers.bulletWhizzLeftRearBuffers;
		break;
	case AE_FRONTEND_BULLET_PASS_LEFT_FRONT:
		bufferList = &g_Buffers.bulletWhizzLeftFrontBuffers;
		break;
	case AE_FRONTEND_BULLET_PASS_RIGHT_REAR:
		bufferList = &g_Buffers.bulletWhizzRightRearBuffers;
		break;
	case AE_FRONTEND_BULLET_PASS_RIGHT_FRONT:
		bufferList = &g_Buffers.bulletWhizzRightFrontBuffers;
		break;
	default:
		break;
	}

	if (bufferList && !bufferList->empty()) {
		RandomIntegers rnd(bufferList->size());

		int index = rnd.next();
		//int index = CGeneral::GetRandomNumber() % bufferList->size();
		ALuint buffer = (*bufferList)[index];
		AudioManager.PlaySource2D(buffer, true, audiovolume, pitch);
		LOG("playing audio with event '%d'", eventId);
		//AudioManager.audiosplaying.push_back(std::move(inst));
		return;
	}

	subhook_remove(subhookCAudioEngine__ReportFrontEndAudioEvent);
	eng->ReportFrontendAudioEvent(eventId, volumeChange, speed);
	subhook_install(subhookCAudioEngine__ReportFrontEndAudioEvent);
}

#ifdef QUAKE_KILLSOUNDS_TEST
static int prevMembersExcludingLeader = -1;
static CPed* prevLastMember = nullptr;
static bool lastManPlayed = false;

void ManageLastManAndTeamKill() {
	CPlayerPed* local = FindPlayerPed();
	if (!local) return;

	CPedGroup* group = (CPedGroup*)CPedGroups::GetPedsGroup(local);
	if (!group) {
		prevMembersExcludingLeader = -1;
		prevLastMember = nullptr;
		lastManPlayed = false;
		return;
	}

	int curr = group->m_groupMembership.CountMembersExcludingLeader();

	// First-time init
	if (prevMembersExcludingLeader == -1) {
		prevMembersExcludingLeader = curr;
		prevLastMember = nullptr;
		if (curr > 0) {
			for (int i = curr - 1; i >= 0; --i) {
				CPed* p = group->m_groupMembership.m_apMembers[i];
				if (p) { prevLastMember = p; break; }
			}
		}
		lastManPlayed = false;
		return;
	}

	// Teamkills
	for (int i = 0; i < size(group->m_groupMembership.m_apMembers); i++) {
		CPed* ally = group->m_groupMembership.m_apMembers[i];
		if (!ally) continue;

		// detect if ally was killed by a player, then trigger teamkill.wav sound
		CPed* killer = (CPed*)ally->m_pDamageEntity;
		if (killer && ally->m_fHealth <= 0.0f) {
			if (killer->IsPlayer()) {
				static std::unordered_set<CPed*> alreadyReported;
				if (alreadyReported.find(ally) == alreadyReported.end()) {
					ALuint buf = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/teamkiller.wav");
					if (buf) AudioManager.PlaySource2D(buf, true, AEAudioHardware.m_fEffectMasterScalingFactor * 0.3f, CTimer::ms_fTimeScale);
					alreadyReported.insert(ally);
				}
			}
		}
	}

	// Last man standing would trigger when the last ally in the players group was killed
	if (prevMembersExcludingLeader > 0 && curr == 0) {
		if (prevLastMember && prevLastMember->m_fHealth <= 0.0f && !wasHeadShotted && !lastManPlayed) {
			lastManPlayed = true;
			ALuint buf = AudioManager.CreateOpenALBufferFromAudioFile(foldermod / "quake/youarethelastmanstanding.wav");
			if (buf) AudioManager.PlaySource2D(buf, true, AEAudioHardware.m_fEffectMasterScalingFactor * 0.3f, CTimer::ms_fTimeScale);
		}
	}
	else if (curr > 0) {
		lastManPlayed = false;
	}

	// Update last member when membership changes
	if (curr != prevMembersExcludingLeader) {
		prevLastMember = nullptr;
		if (curr > 0) {
			for (int i = curr - 1; i >= 0; --i) {
				CPed* p = group->m_groupMembership.m_apMembers[i];
				if (p) { prevLastMember = p; break; }
			}
		}
	}

	prevMembersExcludingLeader = curr;
}
#endif
void __fastcall PlayChainsawEvent(CAEWeaponAudioEntity* ts, int, CPed* ped, int Aevent)
{
	//subhook_remove(subhookPlayChainsawEvent);
	ts->ReportChainsawEvent(ped, Aevent);
	//subhook_install(subhookPlayChainsawEvent);
	//chainsaw = &ts->m_tempSound;
	if (ts->m_nChainsawSoundState != AE_WEAPON_CHAINSAW_STATE_CUTTING)
	{
		PlayStop(nullptr, false, true, false, 2);
		PlayStop(nullptr, false, true, false, 3);
		LOG("Stopped cutting sound");
	}

// get surface from gentInfo
	eSurfaceType surf = SURFACE_DEFAULT;
	auto it = gentInfo.find(ts->m_pPed);
	if (it != gentInfo.end())
	{
		surf = (eSurfaceType)it->second;
	}
	SoundInstanceSettings opts{};
	std::shared_ptr<SoundInstance> inst;
	auto SoundStateToStr = [&](int state) -> std::string
		{
			switch (state)
			{
			case AE_WEAPON_CHAINSAW_STATE_IDLE:
				return "IDLE";
			case AE_WEAPON_CHAINSAW_STATE_ACTIVE:
				return "ACTIVE";
			case AE_WEAPON_CHAINSAW_STATE_CUTTING:
				return "CUTTING";
			case AE_WEAPON_CHAINSAW_STATE_STOPPING:
				return "STOPPING";
			case AE_WEAPON_CHAINSAW_STATE_STOPPED:
				return "STOPPED";
			default:
				return "UNKNOWN";
			}
		};
	switch (ts->m_nChainsawSoundState) 
	{
	case AE_WEAPON_CHAINSAW_STATE_IDLE:
		if (AudioManager.m_apChainsawSounds[ped][0] == nullptr)
		{
			// set up attenuations
			opts.maxDist = gAttenuationSettings.chainsawIdle.maxDist;
			opts.refDist = gAttenuationSettings.chainsawIdle.refDist;
			opts.airAbsorption = gAttenuationSettings.chainsawIdle.airAbsorption;
			opts.rollOffFactor = gAttenuationSettings.chainsawIdle.rolloffFactor;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
			if (gPitches.chainsawIdle.has_value())
			{
				opts.readPitchFromFile = gPitches.chainsawIdle.has_value();
				opts.readPitch = *gPitches.chainsawIdle;
			}
			opts.pos = ped->GetPosition();
			opts.looping = true;
			opts.isChainsawSound = true;
			opts.shooter = ped;
			inst = AudioManager.PlaySource(g_Buffers.chainsawBuffers[0], opts);
			if (inst)
			{
				AudioManager.m_apChainsawSounds[ped][0] = inst;
			}
			break;
	case AE_WEAPON_CHAINSAW_STATE_ACTIVE:
		if (AudioManager.m_apChainsawSounds[ped][1] == nullptr)
		{
			opts.maxDist = gAttenuationSettings.chainsawActive.maxDist;
			opts.refDist = gAttenuationSettings.chainsawActive.refDist;
			opts.airAbsorption = gAttenuationSettings.chainsawActive.airAbsorption;
			opts.rollOffFactor = gAttenuationSettings.chainsawActive.rolloffFactor;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
			if (gPitches.chainsawActive.has_value()) 
			{
				opts.readPitchFromFile = gPitches.chainsawActive.has_value();
				opts.readPitch = *gPitches.chainsawActive;
			}
			opts.pos = ped->GetPosition();
			opts.looping = true;
			opts.isChainsawSound = true;
			opts.shooter = ped;
			inst = AudioManager.PlaySource(g_Buffers.chainsawBuffers[1], opts);
			if (inst)
			{
				AudioManager.m_apChainsawSounds[ped][1] = inst;
			}
		}
		break;
	case AE_WEAPON_CHAINSAW_STATE_CUTTING:
		if (AudioManager.m_apChainsawSounds[ped][2] == nullptr)
		{
			opts.maxDist = gAttenuationSettings.chainsawCutting.maxDist;
			opts.refDist = gAttenuationSettings.chainsawCutting.refDist;
			opts.airAbsorption = gAttenuationSettings.chainsawCutting.airAbsorption;
			opts.rollOffFactor = gAttenuationSettings.chainsawCutting.rolloffFactor;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
			if (gPitches.chainsawCutting.has_value())
			{
				opts.readPitchFromFile = gPitches.chainsawCutting.has_value();
				opts.readPitch = *gPitches.chainsawCutting;
			}
			opts.pos = ped->GetPosition();
			opts.looping = true;
			opts.isChainsawSound = true;
			opts.shooter = ped;
			ALuint bufferToPlay = 0;
			if (IsFleshy(surf))
			{
				bufferToPlay = g_Buffers.chainsawBuffers[2];
			}
			else if (IsAudioMetal(surf)) 
			{
				bufferToPlay = g_Buffers.chainsawBuffers[3];
			}
			inst = AudioManager.PlaySource(bufferToPlay, opts);
			if (inst)
			{
				AudioManager.m_apChainsawSounds[ped][2] = inst;
			}
		}
		break;
		}
	}
	LOG("chainsaw sound state: %s", SoundStateToStr(ts->m_nChainsawSoundState).c_str());
	LOG("Chainsaw event: %d | state: %d | time: %u",
		Aevent, ts ? ts->m_nChainsawSoundState : -1, ts ? ts->m_dwTimeChainsaw : 0);
}

void __fastcall PlayChainsawStopSound(CAEWeaponAudioEntity* ts, int, CPhysical* entity)
{
	subhook_remove(subhookPlayChainsawStopSound);
	ts->PlayChainsawStopSound(entity);
	subhook_install(subhookPlayChainsawStopSound);
	PlayStop(nullptr, false, true, false, 2);
	PlayStop(nullptr, false, true, false, 3);
	//PlayStop(entity);
	//LOG("Chainsaw stop sound played.");
}

// for some reason both sounds played at the same time, so we don't do it for now
#if 0
void __fastcall CWeaponAudio__PlayStealthEvent(
	CAEWeaponAudioEntity* ts, int,
	eWeaponType weapType,
	CPed* ped,
	int event)
{
	ts->ReportStealthKill(weapType, ped, event);
	LOG("Stealth event: %d for weap type: %d", event, weapType);
	//if (ts->m_tempSound) 
	//{
		LOG("SFX %d BANK %d", ts->m_tempSound.m_nSoundIdInSlot, ts->m_tempSound.m_nBankSlotId);
	//}
	// paranoid coding moment
	bool handled = false;
	if (ts)
	{
		CPed* entity = ts->m_pPed;
		if (entity) 
		{
			switch (event)
			{
			case AE_WEAPON_STEALTH_KILL:
				if (AudioManager.findWeapon(&weapType, eModelID(-1), "stealth_firstcut", ped, false))
					handled = true;
				break;

			case AE_FRONTEND_CAR_NO_CASH:
				if (AudioManager.findWeapon(&weapType, eModelID(-1), "stealth_secondcut", ped, false))
					handled = true;
				break;
			}
		}
	}
}
#endif
void __fastcall CAESound__Dummy(
	CAESound* ts, int,
	__int16 bankSlotId,
	__int16 sfxId,
	CAEAudioEntity* audio,
	float x,
	float y,
	float z,
	float volume,
	float maxDistance,
	float speed,
	float timeScale,
	char a12,
	__int16 environmentFlags,
	float a14,
	__int16 currPlayPosn)
{
	ts->Initialise(
		bankSlotId,
		sfxId,
		audio,
		CVector(
			x,
			y,
			z),
		volume,
		maxDistance,
		speed,
		timeScale,
		a12,
		environmentFlags,
		a14,
		currPlayPosn);
	CAEWeaponAudioEntity* weaponAudioEntity = reinterpret_cast<CAEWeaponAudioEntity*>(audio);
	//bool foundSound = false;
	std::string what;
	LOG("SFX ID: %d BANK ID: %d", sfxId, bankSlotId);
	// minigun barrel spin sfx
	if (audio && weaponAudioEntity && weaponAudioEntity->m_pPed && IsPedPointerValid(weaponAudioEntity->m_pPed) && weaponAudioEntity->m_pPed->GetWeapon()) {
		eWeaponType weaponType = weaponAudioEntity->m_pPed->GetWeapon()->m_eWeaponType;
		switch (sfxId) {
	
		case 28:
			if (bankSlotId == 5)
			what = "spraycan_sprayloop";

			break;
		case 9:
			if (bankSlotId == 5)
			what = "extinguisher_loop";
			else if (bankSlotId == 3)
			what = "chainsaw_cuttingflesh";
			break;
		case 7:
		case 8:
			if (bankSlotId == 3)
			{
				what = "chainsaw_cuttingflesh";
			}
			break;
		case 10:
		case 83:
		case 26:
			if (bankSlotId == 5 || bankSlotId == 19) {
				// flamethrower
				if (sfxId == 26)
					what = "flamethrower_fire";
				else if (sfxId == 83)
					what = "flamethrower_start";
				else if (sfxId == 10)
					what = "flamethrower_idlegasloop";
			}
			break;
			
		// minigun barrel spin & spin end
		case 14:
			if (bankSlotId == 5)
			what = "minigun_barrelspinloop";
			break;
		case 63:
			if (bankSlotId == 5)
			what = "minigun_barrelspinend";
			break;
		case 15:
		case 16:
		case 11:
		case 12:
		case 13:
			if (bankSlotId == 5)
			what = "minigun_fireloop";
			break;
			// chainsaw sounds
		case 1: // idle
			if (bankSlotId == 40) {
				what = "chainsaw_idle";
				//bankSlotId = 1337;
			}
			break;

		case 0: // active
			if (bankSlotId == 40)
			{
				what = "chainsaw_active";
				//bankSlotId = 1337;
			}
			break;

		case 2: // stop
			if (bankSlotId == 40) {
				what = "chainsaw_stop";
				//bankSlotId = 1337;
			}
			break;
#if 0
		case 33:
		case 53:
			if (bankSlotId == 5) {
				eWeaponType knife = WEAPONTYPE_KNIFE;
				AudioManager.findWeapon(&knife, eModelID(-1), "stealth_firstcut", weaponAudioEntity->m_pPed, true);
			}
			break;

		case 81:
			LOG("SFX 81 (stealth first cut) bank slot id: %d", bankSlotId);
			if (bankSlotId == 5)
				what = "stealth_firstcut";
			break;

		case 47:
			LOG("SFX 47 (stealth first cut) bank slot id: %d", bankSlotId);
			if (bankSlotId == 2)
				what = "stealth_secondcut";
			break;
#endif
		default: break;
		}
		LOG("weapontype %d", weaponType);
		if (!what.empty()) {
			if (what == "minigun_barrelspinloop")
			{
				AudioManager.PlayOrStopBarrelSpinSound(weaponAudioEntity->m_pPed, &weaponType, true);
			}
			else if (what == "minigun_barrelspinend")
			{
				AudioManager.PlayOrStopBarrelSpinSound(weaponAudioEntity->m_pPed, &weaponType, false, true);
			}
			else if (what == "chainsaw_stop")
			{
				PlayStop(weaponAudioEntity->m_pPed, true, true, false, 1);
				PlayStop(nullptr, false, true, false, 2);
				PlayStop(nullptr, false, true, false, 3);
			}
			/*else if (what == "stealth_firstcut")
			{
				AudioManager.findWeapon(&weaponType, eModelID(-1), what, weaponAudioEntity->m_pPed);
			}
			else if (what == "stealth_secondcut")
			{
				AudioManager.findWeapon(&weaponType, eModelID(-1), what, weaponAudioEntity->m_pPed);
			}*/
		}
	}
}
#include "CAEAudioHardware.h"
// 0x4D88C0
bool CAEAudioHardware__IsSoundBankLoaded(uint16_t bankId, int16_t bankSlotId) {
	return CallMethodAndReturn<bool, 0x4D88C0, void*, uint16_t, int16_t>((void*)0xB5F8B8, bankId, bankSlotId);
}

void __fastcall CAESound__CalculateVolume(CAESound* snd, int)
{
	bool skipVolumeCalc = false;
	std::string what;
	eWeaponType typeWeNeed = WEAPONTYPE_UNARMED;

	bool hasVehicleSiren = false;

	auto CheckVehicleSiren = [&]() {
		if (hasVehicleSiren) return; // already checked
		CAEVehicleAudioEntity* vehAudio = reinterpret_cast<CAEVehicleAudioEntity*>(snd->m_pBaseAudio);
		if (vehAudio)
		{
			CVehicle* veh = (CVehicle*)vehAudio->m_pEntity;
			if (veh)
			{
				int modelId = veh->m_nModelIndex;
				auto it = g_Buffers.g_VehicleHasSiren.find(modelId);
				if (it != g_Buffers.g_VehicleHasSiren.end())
					hasVehicleSiren = it->second;
			}
		}
	};
	switch (snd->m_nSoundIdInSlot) {
	case 28:
		if (snd->m_nBankSlotId == 5) {
			what = "spraycan_sprayloop"; typeWeNeed = WEAPONTYPE_SPRAYCAN;
		}
		break;
	case 9:
		if (snd->m_nBankSlotId == 5) 
		{ 
			what = "extinguisher_loop"; typeWeNeed = WEAPONTYPE_EXTINGUISHER;
		}
		else if (snd->m_nBankSlotId == 3) 
		{
			what = "chainsaw_cuttingflesh"; typeWeNeed = WEAPONTYPE_CHAINSAW;
		}
		break;
	case 7:
	case 8:
		if (snd->m_nBankSlotId == 3)
		{
			what = "chainsaw_cuttingflesh"; typeWeNeed = WEAPONTYPE_CHAINSAW;
		}
		break;
	case 10:
	case 83:
	case 26:
		if (snd->m_nBankSlotId == 5 || snd->m_nBankSlotId == 19) 
		{
			if (snd->m_nSoundIdInSlot == 26)
			what = "flamethrower_fire";
			else if (snd->m_nSoundIdInSlot == 83)
				what = "flamethrower_start";
			else if (snd->m_nSoundIdInSlot == 10)
				what = "flamethrower_idlegasloop";

			typeWeNeed = WEAPONTYPE_FTHROWER;
		}
		// first check if we have a sound for x model
		if (snd->m_nBankSlotId == 17 && snd->m_nSoundIdInSlot == 10)
		{
			CheckVehicleSiren();
			if (hasVehicleSiren) skipVolumeCalc = true;
		}
		break;
	case 14:
		if (snd->m_nBankSlotId == 5) 
		{
			what = "minigun_barrelspinloop"; typeWeNeed = WEAPONTYPE_MINIGUN;
		}
		break;
	case 63:
		if (snd->m_nBankSlotId == 5) 
		{
			what = "minigun_barrelspinend"; typeWeNeed = WEAPONTYPE_MINIGUN;
		}
		break;
	case 15: case 16: case 11: case 12: case 13:
		if (snd->m_nBankSlotId == 5) {
			what = "minigun_fireloop"; typeWeNeed = WEAPONTYPE_MINIGUN;
		}
		if (snd->m_nBankSlotId == 17 && snd->m_nSoundIdInSlot == 11)
		{
			CheckVehicleSiren();
			if (hasVehicleSiren) skipVolumeCalc = true;
		}
		break;
	case 1: // chainsaw idle
		if (snd->m_nBankSlotId == 40 && CAEAudioHardware__IsSoundBankLoaded(36, 40)) {
			what = "chainsaw_idle"; typeWeNeed = WEAPONTYPE_CHAINSAW; 
		}
		break;
	case 0: // chainsaw active
		if (snd->m_nBankSlotId == 40 && CAEAudioHardware__IsSoundBankLoaded(36, 40)) {
			what = "chainsaw_active"; typeWeNeed = WEAPONTYPE_CHAINSAW; 
		}
		break;
	case 2: // chainsaw stop
		if (snd->m_nBankSlotId == 40 && CAEAudioHardware__IsSoundBankLoaded(36, 40))
		{ 
			what = "chainsaw_stop"; typeWeNeed = WEAPONTYPE_CHAINSAW;
		}
		break;
	/*case 50:
		// grenade boucing sounds
		if (snd->m_nBankSlotId == 2)
		{
			skipVolumeCalc = true;
		}
		break;*/
#if 0
	case 81:
		if (snd->m_nBankSlotId == 5) 
		{
			what = "stealth_firstcut"; typeWeNeed = WEAPONTYPE_KNIFE;
		}
		break;

	case 47:
		if (snd->m_nBankSlotId == 2)
		{
			what = "stealth_secondcut"; typeWeNeed = WEAPONTYPE_KNIFE;
		}
		break;
#endif
	default:
		break;
	}
	if (!what.empty() && AudioManager.findWeapon(&typeWeNeed, eModelID(-1), what, (CPhysical*)snd->m_pPhysicalEntity, false)) {
		//if (snd->m_pPhysicalEntity) {
			// found a replacement for vanilla sfx, mute it
			//if (found) {
				skipVolumeCalc = true;
				//LOG("SFX ID: %d, BANK ID %d", snd->m_nSoundIdInSlot, snd->m_nBankSlotId);
			//}
		//}
	}

	if (!skipVolumeCalc) {
		snd->CalculateVolume();
	}
}


void __fastcall CAEWeaponAudioEntity__PlayFlameThrowerSounds(
	CAEWeaponAudioEntity* ts, int,
	CPhysical* entity,
	__int16 sfx1,
	__int16 sfx2,
	int audioEventId,
	float audability,
	float speed)
{
	subhook_remove(subhookCAEWeaponAudioEntity__PlayFlameThrowerSounds);
	ts->PlayFlameThrowerSounds(entity, sfx1, sfx2, audioEventId, audability, speed);
	subhook_install(subhookCAEWeaponAudioEntity__PlayFlameThrowerSounds);
	LOG("Flamethrower SFX1: %d, SFX2: %d", sfx1, sfx2);

	CPed* ped = ts->m_pPed;
	eWeaponType weaponType = ped ? ped->GetWeapon()->m_eWeaponType : WEAPONTYPE_UNARMED;
	if (sfx1 == 83) {
		if (AudioManager.m_apFlamethrowerSounds[ped][2] == nullptr) {
			SoundInstanceSettings opts{};
			opts.maxDist = gAttenuationSettings.flamethrowerStart.maxDist;
			opts.refDist = gAttenuationSettings.flamethrowerStart.refDist;
			opts.airAbsorption = gAttenuationSettings.flamethrowerStart.airAbsorption;
			opts.rollOffFactor = gAttenuationSettings.flamethrowerStart.rolloffFactor;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
			if (gPitches.flamethrowerStart.has_value())
			{
				opts.readPitchFromFile = gPitches.flamethrowerStart.has_value();
				opts.readPitch = *gPitches.flamethrowerStart;
			}
			opts.pos = ped->GetPosition();
			opts.shooter = ped;
			auto inst = AudioManager.PlaySource(g_Buffers.flamethrowerBuffers[1], opts);
			if (inst) {
				AudioManager.m_apFlamethrowerSounds[ped][2] = inst;
				LOG("Firing flamethrower: start (custom)");
			}
		}
	}

	if (sfx2 == 26) {
		if (AudioManager.m_apFlamethrowerSounds[ped][0] == nullptr) {
			SoundInstanceSettings opts{};
			opts.maxDist = gAttenuationSettings.flamethrowerFireLoop.maxDist; // 100.0f
			opts.refDist = gAttenuationSettings.flamethrowerFireLoop.refDist; // 1.0f
			opts.airAbsorption = gAttenuationSettings.flamethrowerFireLoop.airAbsorption; // 1.0f
			opts.rollOffFactor = gAttenuationSettings.flamethrowerFireLoop.rolloffFactor; // 1.0f
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
			if (gPitches.flamethrowerFireLoop.has_value())
			{
				opts.readPitchFromFile = gPitches.flamethrowerFireLoop.has_value();
				opts.readPitch = *gPitches.flamethrowerFireLoop;
			}
			opts.pos = ped->GetPosition();
			opts.looping = true;
			opts.shooter = ped;
			auto inst = AudioManager.PlaySource(g_Buffers.flamethrowerBuffers[2], opts);
			if (inst) {
				AudioManager.m_apFlamethrowerSounds[ped][0] = inst;
				LOG("Firing flamethrower: main loop (custom)");
			}
		}
	}
}


// 0x503870
void __fastcall CAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop(CAEWeaponAudioEntity* ts, int, CPhysical* entity) {
	subhook_remove(subhookCAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop);
	ts->PlayFlameThrowerIdleGasLoop(entity);
	subhook_install(subhookCAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop);

	// play the gas loop sound
	if (ts->m_pPed && AudioManager.m_apFlamethrowerSounds[ts->m_pPed][1] == nullptr) {
		SoundInstanceSettings opts{};
		opts.maxDist = gAttenuationSettings.flamethrowerGasLoop.maxDist; // 100.0f
		opts.refDist = gAttenuationSettings.flamethrowerGasLoop.refDist; // 1.0f
		opts.airAbsorption = gAttenuationSettings.flamethrowerGasLoop.airAbsorption; // 1.0f
		opts.rollOffFactor = gAttenuationSettings.flamethrowerGasLoop.rolloffFactor; // 1.0f
		opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
		opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
		if (gPitches.flamethrowerGasLoop.has_value())
		{
			opts.readPitchFromFile = gPitches.flamethrowerGasLoop.has_value();
			opts.readPitch = *gPitches.flamethrowerGasLoop;
		}
		opts.pos = ts->m_pPed->GetPosition();
		opts.looping = true;
		opts.shooter = ts->m_pPed;
		auto inst = AudioManager.PlaySource(g_Buffers.flamethrowerBuffers[0], opts);
		if (inst)
		{
			AudioManager.m_apFlamethrowerSounds[ts->m_pPed][1] = inst;
		}
	}
}

// 0x5034E0
void __fastcall CAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop(CAEWeaponAudioEntity* ts, int) {
	subhook_remove(subhookCAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop);
	ts->StopFlameThrowerIdleGasLoop();
	subhook_install(subhookCAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop);
	CPed* owner = ts->m_pPed;
	if (owner) {
		auto it = AudioManager.m_apFlamethrowerSounds.find(owner);
		if (it != AudioManager.m_apFlamethrowerSounds.end()) {
			auto& inst = it->second[1];
			if (inst) {
				alSourceStop(inst->source);
				alDeleteSources(1, &inst->source);
				inst->source = 0;
				it->second[1] = nullptr;
			}
		}
	}
}

void __fastcall StopFlamethrowerFireSound(CAESound* snd, int) 
{
	LOG("Stopping flamethrower fire sfx");
	snd->StopSoundAndForget();
	//bool handled = false;
	for (auto& entry : AudioManager.m_apFlamethrowerSounds) {
		CPed* shooter = entry.first;
		if (!shooter) continue;
		auto& inst0 = entry.second[0]; // flamethrower fire loop sound is at index 0
		auto& inst2 = entry.second[2]; // flamethrower start sound is at index 2
		if (inst0) {
			// stop and delete the sound
			alSourceStop(inst0->source);
			alDeleteSources(1, &inst0->source);
			inst0->source = 0;
			AudioManager.m_apFlamethrowerSounds[shooter][0] = nullptr;
		}
		if (inst2) {
			// stop and delete the sound
			alSourceStop(inst2->source);
			alDeleteSources(1, &inst2->source);
			inst2->source = 0;
			AudioManager.m_apFlamethrowerSounds[shooter][2] = nullptr;
		}
	}
	LOG("Stopping flamethrower fire sfx");
}

void __fastcall StopSpraycanSound(CAESound* snd, int)
{
	snd->StopSoundAndForget();
	bool handled = false;
	for (auto& entry : AudioManager.m_apSpraycanSounds) {
		CPed* shooter = entry.first;
		if (!shooter) continue;
		auto& inst = entry.second; // flamethrower fire sound is at index 0
		if (inst) {
			// stop and delete the sound
			alSourceStop(inst->source);
			alDeleteSources(1, &inst->source);
			inst->source = 0;
			AudioManager.m_apSpraycanSounds[shooter] = nullptr;
		}
	}
	LOG("Stopped spraycan sfx");
}

void __fastcall StopFireExtinguisherSound(CAESound* snd, int)
{
	snd->StopSoundAndForget();
	for (auto& entry : AudioManager.m_apFireextinguisherSounds) {
		CPed* shooter = entry.first;
		if (!shooter) continue;
		auto& inst = entry.second; // flamethrower fire sound is at index 0
		if (inst) {
			// stop and delete the sound
			alSourceStop(inst->source);
			alDeleteSources(1, &inst->source);
			inst->source = 0;
			AudioManager.m_apFireextinguisherSounds[shooter] = nullptr;
		}
	}
	LOG("Stopped fire extinguisher sfx");
}

void __fastcall StopMinigunSounds(CAESound* snd, int) 
{
	LOG("Stopping minigun sfx %d", snd->m_nSoundIdInSlot);
	snd->StopSoundAndForget();
	for (auto& entry : AudioManager.m_apMinigunSound) {
		CPed* shooter = entry.first;
		if (!shooter) continue;
		if (snd->m_nBankSlotId != 5) continue;
		auto& inst = entry.second[0];
		auto& inst1 = entry.second[1];
		switch (snd->m_nSoundIdInSlot) {
		case 15:
		case 16:
		case 11:
		case 12:
		case 13:
			if (inst) {
				// stop and delete the sound
				alSourceStop(inst->source);
				alDeleteSources(1, &inst->source);
				inst->source = 0;
				AudioManager.m_apMinigunSound[shooter][0] = nullptr;
				LOG("Stopped minigun fire loop sfx");
			}
			break;
		case 14:
		case 63:
			if (inst1) {
				// stop and delete the sound
				alSourceStop(AudioManager.barrelSpinSource);
				alDeleteSources(1, &AudioManager.barrelSpinSource);
				AudioManager.barrelSpinSource = 0;
				AudioManager.m_apMinigunSound[shooter][1] = nullptr;
				LOG("Stopped minigun spin loop sfx");
			}
			break;
		}
	}
	LOG("Stopped minigun sfx with event %u", snd->m_nEvent);
}

void __fastcall StopChainsawSounds(CAESound* snd, int) 
{
	snd->StopSoundAndForget();
	switch (snd->m_nSoundIdInSlot) 
	{
	case 1: //idle
		PlayStop(nullptr, false, true, false, 0);
		break;
	case 0: //active
		PlayStop(nullptr, false, true, false, 1);
		PlayStop(nullptr, false, true, false, 2);
		PlayStop(nullptr, false, true, false, 3);
		//PlayStop(nullptr, false, true, false, 2);
		break;
	//case 2: //cutting
	//	PlayStop(nullptr, false, true, false, 2);
	//	break;
	}
	LOG("cleared all chainsaw sfx, sfx %d", snd->m_nSoundIdInSlot);
}

//#endif

void __fastcall CAEWeaponAudioEntity__PlayWeaponLoopSound(
	CAEWeaponAudioEntity* ts, int,
	CPhysical* entity,
	__int16 sfxId,
	int audioEventId,
	float audability,
	float speed,
	unsigned __int32 finalEvent)
{
	ts->PlayWeaponLoopSound(entity, sfxId, audioEventId, audability, speed, finalEvent);
	auto weaponType = ts->m_pPed ? ts->m_pPed->GetWeapon()->m_eWeaponType : WEAPONTYPE_UNARMED;
	bool foundSound = false;
	if (AudioManager.findWeapon(&weaponType, eModelID(-1), sfxId == 28 ? "spraycan_sprayloop" : sfxId == 9 ? "extinguisher_loop" : "", ts->m_pPed, false)) 
	{
		foundSound = true;
	}
	if (foundSound) {
		switch (sfxId) {
		case 28: // spraycan sfxId
			if (AudioManager.m_apSpraycanSounds[ts->m_pPed] == nullptr) {
				SoundInstanceSettings opts{};
				opts.maxDist = gAttenuationSettings.sprayCan.maxDist; // 100.0f
				opts.refDist = gAttenuationSettings.sprayCan.refDist; // 1.0f
				opts.airAbsorption = gAttenuationSettings.sprayCan.airAbsorption; // 1.0f
				opts.rollOffFactor = gAttenuationSettings.sprayCan.rolloffFactor; // 1.0f
				opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
				opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
				if (gPitches.sprayCan.has_value())
				{
					opts.readPitchFromFile = gPitches.sprayCan.has_value();
					opts.readPitch = *gPitches.sprayCan;
				}
				opts.pos = ts->m_pPed->GetPosition();
				opts.looping = true;
				opts.shooter = ts->m_pPed;
				auto inst = AudioManager.PlaySource(g_Buffers.sprayCanLoopBuffer, opts);
				if (inst)
				{
					AudioManager.m_apSpraycanSounds[ts->m_pPed] = inst;
				}
			}
			break;

		case 9: // fireextinguisher sfxId
			if (AudioManager.m_apFireextinguisherSounds[ts->m_pPed] == nullptr) {
				SoundInstanceSettings opts{};
				opts.airAbsorption = gAttenuationSettings.fireExtinguisher.airAbsorption; // 1.0f
				opts.rollOffFactor = gAttenuationSettings.fireExtinguisher.rolloffFactor; // 1.0f
				opts.maxDist = gAttenuationSettings.fireExtinguisher.maxDist; // 100.0f
				opts.refDist = gAttenuationSettings.fireExtinguisher.refDist; // 1.0f
				opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
				opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
				if (gPitches.fireExtinguisher.has_value())
				{
					opts.readPitchFromFile = gPitches.fireExtinguisher.has_value();
					opts.readPitch = *gPitches.fireExtinguisher;
				}
				opts.pos = ts->m_pPed->GetPosition();
				opts.looping = true;
				opts.shooter = ts->m_pPed;
				auto inst = AudioManager.PlaySource(g_Buffers.fireExtinguisherLoopBuffer, opts);
				if (inst)
				{
					AudioManager.m_apFireextinguisherSounds[ts->m_pPed] = inst;
				}
			}

			break;
		}
	}
}

void __fastcall CAEWeaponAudioEntity__PlayGunSounds(
	CAEWeaponAudioEntity* ts, int,
	CPhysical* a2,
	__int16 emptySfxId,
	__int16 farSfxId2,
	__int16 highPitchSfxId3,
	__int16 lowPitchSfxId4,
	__int16 echoSfxId5,
	int nAudioEventId,
	float volumeChange,
	float speed1,
	float speed2)
{
	LOG("PlayGunSounds SFX: empty sfx %d, farSfx %d,  highPitchSfx %d, lowPitchSfx %d, echoSfx %d", emptySfxId, farSfxId2, highPitchSfxId3, lowPitchSfxId4, echoSfxId5);
	ts->PlayGunSounds(
		a2,
		emptySfxId,
		farSfxId2,
		highPitchSfxId3,
		lowPitchSfxId4,
		echoSfxId5,
		nAudioEventId,
		volumeChange,
		speed1,
		speed2);
	if (AudioManager.m_apMinigunSound[ts->m_pPed][0] == nullptr) {
		SoundInstanceSettings opts{};
		opts.airAbsorption = gAttenuationSettings.minigunShoot.airAbsorption; // 1.0f
		opts.rollOffFactor = gAttenuationSettings.minigunShoot.rolloffFactor; // 1.0f
		opts.maxDist = gAttenuationSettings.minigunShoot.maxDist; // 100.0f
		opts.refDist = gAttenuationSettings.minigunShoot.refDist; // 1.0f
		opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
		opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
		if (gPitches.minigunShoot.has_value())
		{
			opts.readPitchFromFile = gPitches.minigunShoot.has_value();
			opts.readPitch = *gPitches.minigunShoot;
		}
		opts.pos = ts->m_pPed->GetPosition();
		opts.shooter = ts->m_pPed;
		opts.looping = true;
		auto inst = AudioManager.PlaySource(g_Buffers.minigunBuffers[0], opts);
		if (inst)
		{
			AudioManager.m_apMinigunSound[ts->m_pPed][0] = inst;
		}
	}
}

// mostly copy and paste from the footsteps func
void __fastcall CAEPedAudioEntity__HandleLandingEvent(CAEPedAudioEntity* audio, int, int event)
{
	LOG("Event sound played: %d", event);
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float FinalPitch = pitch;
	bool PitchReadFromFile = false;
	if (audio && audio->m_pPed) {
		CPed* pedPtr = audio->m_pPed;
		eSurfaceType actualSurface = eSurfaceType(pedPtr->m_nContactSurface);

		if (actualSurface <= TOTAL_NUM_SURFACE_TYPES) {
			std::string surfaceType = "default";

			switch (actualSurface) {
			case SURFACE_PED:
			case SURFACE_GORE: surfaceType = "flesh"; break;
			case SURFACE_GLASS:
			case SURFACE_GLASS_WINDOWS_LARGE: surfaceType = "glass"; break;
			default:
				if (IsAudioGrass(actualSurface)) surfaceType = "grass";
				else if (IsAudioWood(actualSurface)) surfaceType = "wood";
				else if (IsAudioMetal(actualSurface)) surfaceType = "metal";
				else if (IsAudioSand(actualSurface)) surfaceType = "sand";
				else if (IsAudioGravel(actualSurface)) surfaceType = "dirt";
				else if (IsAudioConcrete(actualSurface)) surfaceType = "pavement";
				else if (IsAudioWater(actualSurface) || IsWater(actualSurface) || IsShallowWater(actualSurface)) surfaceType = "water";
				else if (IsAudioTile(actualSurface)) surfaceType = "tile";
				break;
			}

			std::string shoeType = "default";
			std::string shoeTextureType = "default";
			CPedClothesDesc* clothesDesc = pedPtr->m_pPlayerData ? pedPtr->m_pPlayerData->m_pPedClothesDesc : nullptr;

			if (clothesDesc) {
				unsigned int modelId = clothesDesc->m_anModelKeys[CLOTHES_MODEL_SHOES];
				unsigned int textureId = clothesDesc->m_anTextureKeys[CLOTHES_TEXTURE_SHOES];
				for (const auto& [folder, surfaceMap] : g_Buffers.landingBuffers) {
					if (CKeyGen::GetUppercaseKey(folder.c_str()) == modelId) {
						shoeType = folder;
						break;
					}
				}
				for (const auto& [folder, surfaceMap] : g_Buffers.landingByShoeTextureBuffers) {
					if (CKeyGen::GetUppercaseKey(folder.c_str()) == textureId) {
						shoeTextureType = folder;
						break;
					}
				}
			}

			// Landing or collapse?
			bool isCollapse = (event == AE_PED_COLLAPSE_AFTER_FALL);

			std::vector<ALuint>* selected = nullptr;

			// Try to find a special shoe sound
			if (!isCollapse) {
				auto shoeIt = g_Buffers.landingBuffers.find(shoeType);
				if (shoeIt != g_Buffers.landingBuffers.end()) {
					auto surfIt = shoeIt->second.find(surfaceType);
					if (surfIt != shoeIt->second.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
				// texture ones
				auto textureIt = g_Buffers.landingByShoeTextureBuffers.find(shoeTextureType);
				if (textureIt != g_Buffers.landingByShoeTextureBuffers.end()) {
					auto surfIt = textureIt->second.find(surfaceType);
					if (surfIt != textureIt->second.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
			}
			else {
				auto shoeIt = g_Buffers.collapseBuffers.find(shoeType);
				if (shoeIt != g_Buffers.collapseBuffers.end()) {
					auto surfIt = shoeIt->second.find(surfaceType);
					if (surfIt != shoeIt->second.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
				// texture ones
				auto textureIt = g_Buffers.collapseByShoeTextureBuffers.find(shoeTextureType);
				if (textureIt != g_Buffers.collapseByShoeTextureBuffers.end()) {
					auto surfIt = textureIt->second.find(surfaceType);
					if (surfIt != textureIt->second.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
			}

			// If none, we search for generic sounds
			if (!selected) {
				if (!isCollapse) {
					auto surfIt = g_Buffers.landingPerSurfaceBuffers.find(surfaceType);
					if (surfIt != g_Buffers.landingPerSurfaceBuffers.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
				else {
					auto surfIt = g_Buffers.collapsePerSurfaceBuffers.find(surfaceType);
					if (surfIt != g_Buffers.collapsePerSurfaceBuffers.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
			}

			// Fallback to default
			if (!selected) {
				if (!isCollapse) {
					auto surfIt = g_Buffers.landingPerSurfaceBuffers.find("default");
					if (surfIt != g_Buffers.landingPerSurfaceBuffers.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
				else {
					auto surfIt = g_Buffers.collapsePerSurfaceBuffers.find("default");
					if (surfIt != g_Buffers.collapsePerSurfaceBuffers.end() && !surfIt->second.empty()) {
						selected = &surfIt->second;
					}
				}
			}
			// Play them
			if (selected && !selected->empty()) {

				//int index = CGeneral::GetRandomNumber() % selected->size();
				RandomIntegers rnd(selected->size());

				int index = rnd.next();
				ALuint buffer = (*selected)[index];
				//CVector position = pedPtr->GetPosition();
				float referenceDistance, maxDist, airAbs, rollOff;

				if (pedPtr->IsPlayer()) {
					if (!isCollapse) {
						referenceDistance = gAttenuationSettings.landingPlayer.refDist; // 0.5f
						maxDist = gAttenuationSettings.landingPlayer.maxDist;
						airAbs = gAttenuationSettings.landingPlayer.airAbsorption;
						rollOff = gAttenuationSettings.landingPlayer.rolloffFactor;
						if (gPitches.landingPlayer.has_value())
						{
							PitchReadFromFile = gPitches.landingPlayer.has_value();
							FinalPitch = *gPitches.landingPlayer;
						}
					}
					else {
						referenceDistance = gAttenuationSettings.collapsePlayer.refDist;
						maxDist = gAttenuationSettings.collapsePlayer.maxDist;
						airAbs = gAttenuationSettings.collapsePlayer.airAbsorption;
						rollOff = gAttenuationSettings.collapsePlayer.rolloffFactor;
						if (gPitches.collapsePlayer.has_value())
						{
							PitchReadFromFile = gPitches.collapsePlayer.has_value();
							FinalPitch = *gPitches.collapsePlayer;
						}
					}
					LOG("Player move state: %d", pedPtr->m_nMoveState);
				}
				else {
					if (!isCollapse) {
						referenceDistance = gAttenuationSettings.landingNPC.refDist; // 0.3f
						maxDist = gAttenuationSettings.landingNPC.maxDist;
						airAbs = gAttenuationSettings.landingNPC.airAbsorption;
						rollOff = gAttenuationSettings.landingNPC.rolloffFactor;
						if (gPitches.landingNPC.has_value()) 
						{
							PitchReadFromFile = gPitches.landingNPC.has_value();
							FinalPitch = *gPitches.landingNPC;
						}
					}
					else {
						referenceDistance = gAttenuationSettings.collapseNPC.refDist;
						maxDist = gAttenuationSettings.collapseNPC.maxDist;
						airAbs = gAttenuationSettings.collapseNPC.airAbsorption;
						rollOff = gAttenuationSettings.collapseNPC.rolloffFactor;
						if (gPitches.collapseNPC.has_value())
						{
							PitchReadFromFile = gPitches.collapseNPC.has_value();
							FinalPitch = *gPitches.collapseNPC;
						}
					}
					LOG("NPC move state: %d", pedPtr->m_nMoveState);
				}
				SoundInstanceSettings opts;
				opts.pos = pedPtr->GetPosition();
				opts.maxDist = maxDist;//pedPtr->IsPlayer() ? 140.0f : 150.0f; (FLT_MAX prev)
				opts.gain = gameVol;
				opts.airAbsorption = airAbs; // pedPtr->IsPlayer() ? 1.5f : 3.0f;
				opts.refDist = referenceDistance;
				opts.rollOffFactor = rollOff; // pedPtr->IsPlayer() ? 1.5f : 2.5f;
				opts.pitch = pitch;
				opts.readPitchFromFile = PitchReadFromFile;
				opts.readPitch = FinalPitch;
				opts.entity = pedPtr;
				AudioManager.PlaySource(buffer, opts);
				return;
			}
		}
	}
	CallMethod<0x4E18E0, CAEPedAudioEntity*, int>(audio, event);
}
void __fastcall CAEWeaponAudioEntity__PlayGoggleSound(CAEWeaponAudioEntity* ts, int, __int16 sfxId, int audioEventId)
{
	float volume = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	volume *= fader;
	eWeaponType weaponType = ts->m_pPed ? ts->m_pPed->GetWeapon()->m_eWeaponType : WEAPONTYPE_UNARMED;
	bool foundSound = AudioManager.findWeapon(&weaponType, eModelID(-1), "goggles_on", ts->m_pPed, false);
	if (foundSound) 
	{
		AudioManager.PlaySource2D(g_Buffers.gogglesBuffer[0], true, volume, 0.0f);
		return;
	}
	ts->PlayGoggleSound(sfxId, audioEventId);
}

void __fastcall CPed__RemoveGogglesModel(CPed* ts, int) 
{
	float volume = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	volume *= fader;
	eWeaponType weaponType = ts ? ts->GetWeapon()->m_eWeaponType : WEAPONTYPE_UNARMED;
	bool foundSound = AudioManager.findWeapon(&weaponType, eModelID(-1), "goggles_off", ts, false);
	if (foundSound)
	{
		AudioManager.PlaySource2D(g_Buffers.gogglesBuffer[1], true, volume, 0.0f);
	}
	ts->RemoveGogglesModel();
}

void __fastcall CAEWeaponAudioEntity__PlayCameraSound(
	CAEWeaponAudioEntity* ts, int,
	CPhysical* entity,
	int audioEventId,
	float audability)
{
	float volume = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	volume *= fader;
	eWeaponType weaponType = ts->m_pPed ? ts->m_pPed->GetWeapon()->m_eWeaponType : WEAPONTYPE_UNARMED;
	bool foundSound = AudioManager.findWeapon(&weaponType, eModelID(-1), "camera_shutter", ts->m_pPed, false);
	if (foundSound)
	{
		AudioManager.PlaySource2D(g_Buffers.cameraShutterBuffer, true, volume, 0.0f);
		return;
	}
	ts->PlayCameraSound(entity, audioEventId, audability);
}

void __fastcall CAESoundManager__CancelSoundsOwnedByAudioEntity(void* ts, int, CAEAudioEntity* entity, uint8_t a3)
{
	if (entity)
	{
		CAEFireAudioEntity* fireEntity = reinterpret_cast<CAEFireAudioEntity*>(entity);
		// stop all active non-fire sounds from this entity
		if (fireEntity) {
			auto SafeDeleteInstanceSource = [&](std::shared_ptr<SoundInstance> inst) {
				if (!inst) return;
				if (inst->source != 0) {
					alDeleteSources(1, &inst->source);
					inst->source = 0;
				}

				inst->entity = nullptr;
				inst->shooter = nullptr;
				inst->firePtr = nullptr;
				inst->paused = false;
				inst->isFire = false;
				inst.reset();
				};

			for (auto it = g_Buffers.ent.begin(); it != g_Buffers.ent.end();) {
				CAEFireAudioEntity* entity = *it;
				if (entity != fireEntity)
				{
					++it;
					continue;
				}
				if (entity->field_84) {
					for (auto nsIt = g_Buffers.nonFireSounds.begin(); nsIt != g_Buffers.nonFireSounds.end();) {
						auto& instShared = nsIt->second;
						bool erased = false;
						// we gotta really make sure it's a non-harming fire particle...
						if (instShared && !instShared->isFire && instShared->fireFX && fireEntity->field_84 && instShared->fireFX == fireEntity->field_84) {

							LOG("Stopping non-fire sound instance tied to entity %p", (void*)entity);
							// Stop playback
							if (instShared->source != 0) {
								alSourceStop(instShared->source);
								ALint state = AudioManager.GetSourceState(instShared->source);
								// Delete source only when it is stopped and not paused (in case it will get resumed later)
								if (state == AL_STOPPED && !instShared->paused) {
									SafeDeleteInstanceSource(instShared);
								}
							}

							nsIt = g_Buffers.nonFireSounds.erase(nsIt);
							erased = true;
						}

						if (!erased) {
							++nsIt;
						}
					}
				}

				it = g_Buffers.ent.erase(it);
			}
		}
	}
	CallMethod<0x4EFCD0, void*, CAEAudioEntity*, uint8_t>(ts, entity, a3);
}

void __fastcall CAESound__DummyVeh(
	CAESound* ts, int,
	__int16 bankSlotId,
	__int16 sfxId,
	CAEAudioEntity* audio,
	float x,
	float y,
	float z,
	float volume,
	float maxDistance,
	float speed,
	float timeScale,
	char a12,
	__int16 environmentFlags,
	float a14,
	__int16 currPlayPosn)
{
	ts->Initialise(
		bankSlotId,
		sfxId,
		audio,
		CVector(
			x,
			y,
			z),
		volume,
		maxDistance,
		speed,
		timeScale,
		a12,
		environmentFlags,
		a14,
		currPlayPosn);
	CAEVehicleAudioEntity* vehAudio = reinterpret_cast<CAEVehicleAudioEntity*>(audio);
	CVehicle* veh = (CVehicle*)vehAudio->m_pEntity;
	int modelId = veh ? veh->m_nModelIndex : -1;
	CVector pos = { x, y, z };
	if (vehAudio && veh) {
		switch (sfxId) {
		case 11:
			if (AudioManager.m_apSirens[veh][0] == nullptr) {
				SoundInstanceSettings opts{};
				opts.maxDist = gAttenuationSettings.siren[modelId].maxDist;
				opts.refDist = gAttenuationSettings.siren[modelId].refDist;
				opts.airAbsorption = gAttenuationSettings.siren[modelId].airAbsorption;
				opts.rollOffFactor = gAttenuationSettings.siren[modelId].rolloffFactor;
				opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
				if (gPitches.siren[modelId].has_value())
				{
					opts.readPitchFromFile = gPitches.siren[modelId].has_value();
					opts.readPitch = *gPitches.siren[modelId];
				}
				opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
				opts.pos = pos;
				opts.entity = veh;
				opts.looping = true;
				AudioManager.m_apSirens[veh][0] = AudioManager.PlaySource(g_Buffers.sirenBuffers[modelId][0], opts);
				LOG("Siren");
			}
			break;

		case 10:
			if (AudioManager.m_apSirens[veh][1] == nullptr) {
				SoundInstanceSettings opts{};
				opts.maxDist = gAttenuationSettings.sirenidle[modelId].maxDist;
				opts.refDist = gAttenuationSettings.sirenidle[modelId].refDist;
				opts.airAbsorption = gAttenuationSettings.sirenidle[modelId].airAbsorption;
				opts.rollOffFactor = gAttenuationSettings.sirenidle[modelId].rolloffFactor;
				opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
				opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
				if (gPitches.sirenidle[modelId].has_value())
				{
					opts.readPitchFromFile = gPitches.sirenidle[modelId].has_value();
					opts.readPitch = *gPitches.sirenidle[modelId];
				}
				opts.pos = pos;
				opts.entity = veh;
				opts.looping = true;
				AudioManager.m_apSirens[veh][1] = AudioManager.PlaySource(g_Buffers.sirenBuffers[modelId][1], opts);
				LOG("Siren 2");
				break;
			}
		}
	}
}
CVehicle* targetVeh = nullptr;
char __fastcall CAEVehicleAudioEntity__PlayHornOrSiren(
	CAEVehicleAudioEntity* ts, int,
	char counter,
	char sirenOrAlarm,
	char mrWhoopie,
	cVehicleParams* data)
{
	if (data->m_pVehicle) {
		targetVeh = data->m_pVehicle;
	}
	return CallMethodAndReturn<char, 0x4F99D0, CAEVehicleAudioEntity*, char, char, char, cVehicleParams*>(ts, counter, sirenOrAlarm, mrWhoopie, data);
}

void __fastcall CAESound__StopSirenSound(CAESound* ts, int)
{
	subhook_remove(subhookCAESound__StopSoundAndForget);
	ts->StopSoundAndForget();
	subhook_install(subhookCAESound__StopSoundAndForget);
	LOG("STOPPED SOUND: %d BANK: %d", ts->m_nSoundIdInSlot, ts->m_nBankSlotId);

	auto RemoveSource = [&](int slot)
		{
			// If we know the vehicle, stop only its source
			if (targetVeh)
			{
				auto it = AudioManager.m_apSirens.find(targetVeh);
				if (it == AudioManager.m_apSirens.end()) return;
				auto& source = it->second[slot];
				if (!source || source->source == 0) return;
				LOG("Stopped in slot %d for known vehicle %d", slot, targetVeh->m_nModelIndex);
				alSourceStop(source->source);
				alDeleteSources(1, &source->source);
				source->source = 0;
				source = nullptr;
				return;
			}
			// otherwise just iterate through em all
			for (auto& data : AudioManager.m_apSirens)
			{
				auto& source = data.second[slot];
				if (!source || source->source == 0) continue;
				LOG("Removed all");
				alSourceStop(source->source);
				alDeleteSources(1, &source->source);
				source->source = 0;
				source = nullptr;
				break;
			}
		};
	if (ts->m_nBankSlotId == 17 && CAEAudioHardware__IsSoundBankLoaded(0x4Au, 17)) {
		switch (ts->m_nSoundIdInSlot)
		{
		case 11: RemoveSource(0); break;
		case 10: RemoveSource(1); break;
		}
	}
}

bool lostFocus = false;

LRESULT CALLBACK
MainWndProcHOOK(HWND window, UINT message, UINT wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATE:
		lostFocus = LOWORD(wParam) == WA_INACTIVE;
		break;

	case WM_ACTIVATEAPP:
		lostFocus = (wParam == FALSE);
		if (lostFocus || FrontEndMenuManager.m_bMenuActive)
			AudioManager.PauseAllSources();
		else
			AudioManager.ResumeAllSources();
		break;

	case WM_KILLFOCUS:
		lostFocus = true;
		break;

	case WM_SETFOCUS:
		lostFocus = false;
		break;
	}
	subhook_remove(subhookMainWndProc);
	LRESULT result = CallStdAndReturn<LRESULT, 0x747EB0, HWND, UINT, WPARAM, LPARAM>(window, message, wParam, lParam);
	subhook_install(subhookMainWndProc);
	return result;
}
#if 0
static DWORD rnsStackPtr;

void __declspec(naked) TraceRequestNewSound()
{
	_asm
	{
		mov     eax, [esp]       // grab return address before anything changes
		mov     rnsStackPtr, eax
		pushad
	}
	// ECX = CAESoundManager* (this), [esp+4] = CAESound* after pushad offsets it
	LOG("CAESoundManager::RequestNewSound called from 0x%X", rnsStackPtr);
	_asm
	{
		popad
		push    esi              // replicate: push esi  (0x4EFB10)
		push    edi              // replicate: push edi  (0x4EFB11)
		mov     eax, 4EFB12h    // jump past the two instructions we already ran
		jmp     eax
	}
}
#endif

// grenade bounce
void __fastcall CAudioEngine__ReportCollision(CAudioEngine* eng, int, CEntity* entity1, CEntity* entity2, eSurfaceType surf1, eSurfaceType surf2, CVector& point, CVector* normal, float fCollisionImpact1, float fCollisionImpact2, bool playOnlyOneShotCollisionSound, bool unknown)
{
	if (entity1 && (entity1->m_nModelIndex == MODEL_GRENADE || entity1->m_nModelIndex == MODEL_TEARGAS))
	{
		CObject* grenadeEntity = static_cast<CObject*>(entity1);
		// cuz when the grenade is rolling on the ground it's constantly colliding, resulting in spamming SFX
		// so play it only when it bounces off a surface
		const float normalDominance = max({ fabsf(normal->x), fabsf(normal->y), fabsf(normal->z) });
		const bool bouncing = fCollisionImpact1 >= 0.025f && normal && normalDominance >= 0.5f;
		if (bouncing)
		{
			std::string surfaceType = "default";
			switch (surf2)
			{
			case SURFACE_PED:
			case SURFACE_GORE:
				surfaceType = "flesh";    break;
			case SURFACE_GLASS:
			case SURFACE_GLASS_WINDOWS_LARGE:
				surfaceType = "glass";    break;
			default:
				if (IsAudioGrass(surf2))                                                      surfaceType = "grass";
				else if (IsAudioWood(surf2))                                                       surfaceType = "wood";
				else if (IsAudioMetal(surf2))                                                      surfaceType = "metal";
				else if (IsAudioSand(surf2))                                                       surfaceType = "sand";
				else if (IsAudioGravel(surf2))                                                     surfaceType = "dirt";
				else if (IsAudioConcrete(surf2))                                                   surfaceType = "pavement";
				else if (IsAudioWater(surf2) || IsWater(surf2) || IsShallowWater(surf2)) surfaceType = "water";
				else if (IsAudioTile(surf2))                                                       surfaceType = "tile";
				break;
			}

			auto it = g_Buffers.grenadeBounceBufferPerSurface.find(surfaceType);
			if (it == g_Buffers.grenadeBounceBufferPerSurface.end() || it->second.empty())
			{
				// No buffer for this surface — try "default" as explicit fallback
				it = g_Buffers.grenadeBounceBufferPerSurface.find("default");
			}

			if (it != g_Buffers.grenadeBounceBufferPerSurface.end() && !it->second.empty())
			{
				ALuint buffer = it->second[rand() % it->second.size()];
				SoundInstanceSettings opts{};
				opts.maxDist = gAttenuationSettings.grenade_bounce.maxDist;
				opts.refDist = gAttenuationSettings.grenade_bounce.refDist;
				opts.airAbsorption = gAttenuationSettings.grenade_bounce.airAbsorption;
				opts.rollOffFactor = gAttenuationSettings.grenade_bounce.rolloffFactor;
				opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
				opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
				if (gPitches.grenade_bounce.has_value())
					{
						opts.readPitchFromFile = gPitches.grenade_bounce.has_value();
						opts.readPitch = *gPitches.grenade_bounce;
				}
				opts.pos = point;
				AudioManager.PlaySource(buffer, opts);
				return;
			}
		}
	}
	subhook_remove(subhookCAudioEngine__ReportCollision);
	CallMethod<0x506EB0, CAudioEngine*, CEntity*, CEntity*, eSurfaceType, eSurfaceType, CVector&, CVector*, float, float, bool, bool>(eng, entity1, entity2, surf1, surf2, point, normal, fCollisionImpact1, fCollisionImpact2, playOnlyOneShotCollisionSound, unknown);
	subhook_install(subhookCAudioEngine__ReportCollision);
}
class EarShot {
public:
	EarShot() {
		// If you remove this, you are automatically getting no more support from me.
		if (_strcmpi(PLUGIN_FILENAME, "EarShot.asi") > 0) {
			if (!Error("Renaming of this plugin is not allowed. Please, keep it 'EarShot.asi'!"))
			{
				exit(0);
			}
		}
		else {
			LOG("Not renamed.");
		}
		// if there's a folder in data folder from root, use that instead
		if (fs::exists(folderdata)) foldermod = folderdata;
		else fs::create_directories(foldermod); // else create a new "EarShot" folder if wasn't found
		//logfile.open(foldermod / fs::path(modname).replace_extension(".log"), fstream::out);
		// Add some debug menu entries
		if (DebugMenuLoad())
		{
			DebugMenuAddCmd("EarShot", "Reload all audio folders", Loaders::ReloadAudioFolders);
			DebugMenuAddUInt64("EarShot", "Max bytes that can be written into log", &maxBytesInLog, nullptr, 10, 0, std::numeric_limits<uint64_t>::max(), nullptr);
			DebugMenuAddVarBool8("EarShot", "Toggle debug log", (int8_t*)&Logging, nullptr);
			//	DebugMenuAddVarBool8("EarShot", "Toggle reverb type (EAX or EFX)", (int8_t*)&EAXOrNot, nullptr);
		}
		// Init everything
		Events::initRwEvent.after += []()
			{
				Loaders::InitializeIniFile(1);
				AudioManager.Initialize();
			};

		Events::initGameEvent.after += []()
			{
				Loaders::RegisterAllWeapons();
				Loaders::InitializeIniFile(2);
				Loaders::InstallHooks();
			};

		Events::gameProcessEvent += []() {
			// Don't update any fire sound if the game is paused
			if (!FrontEndMenuManager.m_bMenuActive)
			{
				AudioManager.UpdateFireSoundCleanup();
				AudioManager.ProcessScheduledSounds();
				// Play the ambience when it's not rainy and the screen didn't fade out yet
				if (CWeather::Rain <= 0.0f && TheCamera.GetScreenFadeStatus() == 0)
				{
					AudioManager.PlayAmbienceSFX(cameraposition, eWeaponType(0), true);
				}
			}
			float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
			bool isNight = (CClock::ms_nGameClockHours >= 20 || CClock::ms_nGameClockHours < 6);
			bool isRiot = CGameLogic::LaRiotsActiveHere();
			for (auto& inst : AudioManager.audiosplaying)
			{
				// We update each source's gain so when the screen fades, the sound can fade smoothly as well
				if (inst) {
					ALint state = AudioManager.GetSourceState(inst->source);
					ALint buffer;
					alGetSourcei(inst->source, AL_BUFFER, &buffer);
					ALint fmt = AudioManager.GetBufferFormat((ALuint)buffer);

					float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
					float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
					gameVol *= fader;

					if (state == AL_PLAYING)
					{
						if (inst->isAmbience) {
							if (fmt == AL_FORMAT_STEREO_FLOAT32)
							{
								// They are loud as hell, decrease the gain a bit
								alSourcei(inst->source, AL_SOURCE_RELATIVE, AL_TRUE);
								alSource3f(inst->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
								gameVol *= stereoAmbienceVol;

							}
							else if (fmt == AL_FORMAT_MONO_FLOAT32)
							{
								if (CCutsceneMgr::ms_running || CGame::currArea > 0)
									gameVol = 0.0f; // mute to not interrupt anything
							}
							if (!inst->isGunfireAmbience && !inst->isManualAmbience && inst->source != 0)
							{
								static auto GetAmbienceTimeOfDay = [&](const std::string& stem) -> EAmbienceTime {
									//LOG("GetAmbienceTimeOfDay: stem='%s'", stem.c_str());
									if (stem.ends_with("_night") || NameEndsWithIndexedSuffix(stem, "_night")) return EAmbienceTime::Night;
									if (stem.ends_with("_riot") || NameEndsWithIndexedSuffix(stem, "_riot")) return EAmbienceTime::Riot;
									return EAmbienceTime::Day;
									};
								// we stop ambience sounds that don't fit the time of day or riot state
								std::string stem = inst->path.stem().string();
								bool isReallyAAmbience =
									NameStartsWithIndexedSuffix(stem.c_str(), "ambience")
									|| IsMatchingName(stem.c_str(), "ambience") ||
									(stem.ends_with("_night") || NameEndsWithIndexedSuffix(stem, "_night")) ||
									(stem.ends_with("_riot") || NameEndsWithIndexedSuffix(stem, "_riot"));
								if (isReallyAAmbience) {
									//if (buff.second == 0) continue;
									auto timeOfDay = GetAmbienceTimeOfDay(stem);
									//LOG("Time of day %d", timeOfDay);
									bool cantPlay = (timeOfDay == EAmbienceTime::Night && !isNight) || (timeOfDay == EAmbienceTime::Riot && !isRiot);

									if (cantPlay)
									{
										//LOG("Stopped for time of day %d", timeOfDay);
										alSourceStop(inst->source);
										alDeleteSources(1, &inst->source);
										inst->source = 0;
									}
								}

							}
						}

						// they compute fadings for themselves, don't mess with those sounds
						bool Ok = NameStartsWithIndexedSuffix(inst->nameBuffer.c_str(), { "minigun_barrelspinloop", "minigun_barrelspinend", "low_ammo" })
							|| IsMatchingName(inst->nameBuffer.c_str(), { "minigun_barrelspinloop", "minigun_barrelspinend", "low_ammo" });
						if (!Ok)
						{
							AudioManager.SetSourceGain(inst->source, gameVol);
						}
						//LOG("inst->pitch: %.2f, pitch: %.2f", inst->pitch, pitch);
						//AudioManager.SetSourcePitch(inst->source, inst->readPitch ? inst->pitch : pitch);
					}


					// clear all chainsaw sources when the shooter is in a vehicle
					if (inst->shooter && IsPedPointerValid(inst->shooter) && inst->isChainsawSound)
					{
						if (inst->shooter->bInVehicle) {
							LOG("Cleared chainsaw sound for ped in vehicle");
							PlayStop(nullptr, false, true, true);
						}
					}


					// Pause audio's when it's time to do so
					bool shouldPause = FrontEndMenuManager.m_bMenuActive ||
						CTimer::m_UserPause ||
						CTimer::m_CodePause ||
						CTimer::ms_fTimeScale <= 0.0f;
					if (shouldPause)
					{
						AudioManager.PauseSource(inst.get());
					}
					else
					{
						AudioManager.ResumeSource(inst.get());
					}
					AudioManager.AttachReverbToSource(inst->source);
				}
			}

			// The listener is the player, set appropriate values...
			CVector pos = *TheCamera.GetGameCamPosition();
			cameraposition = pos;
			alListener3f(AL_POSITION, cameraposition.x, cameraposition.y, cameraposition.z);
			// For correct sound panning we use camera's heading, ensuring convincing 3D audio
			CVector vecCamDir = TheCamera.m_mCameraMatrix.GetForward();
			CVector vecCamUpDir = TheCamera.m_mCameraMatrix.GetUp();
			vecCamDir.Normalize();
			vecCamUpDir.Normalize();
			ALfloat orientation[6] =
			{
			vecCamDir.x, vecCamDir.y, vecCamDir.z, // forward vector
			vecCamUpDir.x, vecCamUpDir.y, vecCamUpDir.z // up vector
			};

			alListenerfv(AL_ORIENTATION, orientation);
			AudioManager.audiosplaying.erase(remove_if(AudioManager.audiosplaying.begin(), AudioManager.audiosplaying.end(),
				[&](std::shared_ptr<SoundInstance>& inst) {
					//float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
					ALint state = AudioManager.GetSourceState(inst->source);
					// Update missile sound position and velocity
					if (!inst->isFire && (!inst->firePtr || !inst->fireFX) && state == AL_PLAYING)
					{
						if (inst->entity && IsEntityPointerValid(inst->entity) && inst->bIsMissile && inst->missileSource != 0 && inst->source == inst->missileSource) {
							alSource3f(inst->missileSource, AL_POSITION,
								inst->entity->GetPosition().x,
								inst->entity->GetPosition().y,
								inst->entity->GetPosition().z);
							alSource3f(inst->missileSource, AL_VELOCITY,
								inst->entity->m_vecMoveSpeed.x,
								inst->entity->m_vecMoveSpeed.y,
								inst->entity->m_vecMoveSpeed.z);

							CVector direction = inst->entity->GetForward();
							alSourcef(inst->missileSource, AL_CONE_INNER_ANGLE, 60.0f);
							alSourcef(inst->missileSource, AL_CONE_OUTER_ANGLE, 180.0f);
							alSourcef(inst->missileSource, AL_CONE_OUTER_GAIN, 0.3f);
							alSource3f(inst->missileSource, AL_DIRECTION, direction.x, direction.y, direction.z);

						}
						// Update regular sound position and velocity if it's attached to some entity
						if (inst->entity && IsEntityPointerValid(inst->entity)) {
							alSource3f(inst->source, AL_POSITION,
								inst->entity->GetPosition().x,
								inst->entity->GetPosition().y,
								inst->entity->GetPosition().z);
							alSource3f(inst->source, AL_VELOCITY,
								inst->entity->m_vecMoveSpeed.x,
								inst->entity->m_vecMoveSpeed.y,
								inst->entity->m_vecMoveSpeed.z);
							CVector direction = inst->entity->GetForward();
							alSource3f(inst->source, AL_DIRECTION, direction.x, direction.y, direction.z);
						}
						else if (inst->shooter && IsPedPointerValid(inst->shooter))
						{
							alSource3f(inst->source, AL_POSITION,
								inst->shooter->GetPosition().x,
								inst->shooter->GetPosition().y,
								inst->shooter->GetPosition().z);
							alSource3f(inst->source, AL_VELOCITY,
								inst->shooter->m_vecMoveSpeed.x,
								inst->shooter->m_vecMoveSpeed.y,
								inst->shooter->m_vecMoveSpeed.z);
							CVector direction = inst->shooter->GetForward();
							alSource3f(inst->source, AL_DIRECTION, direction.x, direction.y, direction.z);
						}
					}

					// To prevent overflow, we erase any sources that are no longer used
					if (state != AL_PLAYING && state != AL_PAUSED) {
						//LOG("Removed source '%d'", inst->source);
						alDeleteSources(1, &inst->source);
						alDeleteFilters(1, &inst->filter);
						inst->filter = 0;
						inst->source = 0;
						inst.reset();
						return true;
					}
					return false;
				}), AudioManager.audiosplaying.end());

#ifdef QUAKE_KILLSOUNDS_TEST
			CPlayerPed* playa = FindPlayerPed();
			ManageLastManAndTeamKill();
			// We reset the killstreak counter here, when needed (Replicated Quake behaviour).
			// When we die OR
			// 10 seconds passed since last kill, reset counter
			if (killCounter > 0 && !wasHeadShotted) {
				uint32_t now = CTimer::m_snTimeInMilliseconds;
				bool playerDead = (playa && playa->m_fHealth <= 0.0f);
				if (playerDead || (now - lastTimePedKilled > 10000))
				{
					LOG("Counter reset");
					killCounter = 0;
				}
			}
#endif

		};

		Events::vehicleRenderEvent += [](CVehicle* veh)
			{
				auto& inst = AudioManager.m_apSirens[veh][2];
				int modelId = veh->m_nModelIndex;
				float m_fVelocityChange = 0.0f;
				if (veh->m_nStatus == STATUS_SIMPLE)
					m_fVelocityChange = veh->m_autoPilot.m_fMaxTrafficSpeed * 0.02f;
				else
					m_fVelocityChange = CVector::Dot(veh->m_vecMoveSpeed, veh->GetForward());
				if (veh->bEngineOn && veh->m_fGasPedal < 0.0f)
				{
					// play reverse warning beep
					if (inst == nullptr) {
						SoundInstanceSettings opts{};
						opts.maxDist = gAttenuationSettings.reverse_beep[modelId].maxDist;
						opts.refDist = gAttenuationSettings.reverse_beep[modelId].refDist;
						opts.airAbsorption = gAttenuationSettings.reverse_beep[modelId].airAbsorption;
						opts.rollOffFactor = gAttenuationSettings.reverse_beep[modelId].rolloffFactor;
						opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
						opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
						if (gPitches.reverse_beep[modelId].has_value())
						{
							opts.readPitchFromFile = gPitches.reverse_beep[modelId].has_value();
							opts.readPitch = *gPitches.reverse_beep[modelId];
						}
						opts.pos = veh->GetPosition();
						opts.entity = veh;
						opts.looping = true;
						inst = AudioManager.PlaySource(g_Buffers.sirenBuffers[modelId][2], opts);
					}
				}
				else {
					if (inst != nullptr) {
						alDeleteSources(1, &inst->source);
						inst->source = 0;
						inst.reset();
						inst = nullptr;
					}
				}
				//LOG("Velocity change for audio: %.3f, m_fVelocityChange: %.3f", g_Buffers.m_fVelocityChangeForAudio[veh], m_fVelocityChange);
				//play air brakes
				if (veh->bEngineOn && (g_Buffers.m_fVelocityChangeForAudio[veh] >= 0.025f && m_fVelocityChange < 0.025f ||
					g_Buffers.m_fVelocityChangeForAudio[veh] <= -0.025f && m_fVelocityChange > 0.025f)) {
					SoundInstanceSettings opts{};
					opts.maxDist = gAttenuationSettings.air_brake[modelId].maxDist;
					opts.refDist = gAttenuationSettings.air_brake[modelId].refDist;
					opts.airAbsorption = gAttenuationSettings.air_brake[modelId].airAbsorption;
					opts.rollOffFactor = gAttenuationSettings.air_brake[modelId].rolloffFactor;
					opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
					opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
					if (gPitches.air_brake[modelId].has_value())
					{
						opts.readPitchFromFile = gPitches.air_brake[modelId].has_value();
						opts.readPitch = *gPitches.air_brake[modelId];
					}
					opts.pos = veh->GetPosition();
					opts.entity = veh;
					AudioManager.PlaySource(g_Buffers.sirenBuffers[modelId][3], opts);
					LOG("Air brake played for model %d", modelId);
				}

				g_Buffers.m_fVelocityChangeForAudio[veh] = m_fVelocityChange;

				//wrecked vehicles get their sound removed too
				if (veh->m_nStatus == STATUS_WRECKED || veh->m_fHealth <= 0.0f) {
					for (int i = 0; i < 3; i++) {
						auto& source = AudioManager.m_apSirens[veh][i];

						if (!source || source->source == 0)
							continue;
						LOG("Cleared siren sound for disappeared car");
						alSourceStop(source->source);
						alDeleteSources(1, &source->source);
						source->source = 0;
						source = nullptr;
						break; // found and cleaned up, no need to keep iterating
					}
					g_Buffers.m_fVelocityChangeForAudio.erase(veh);
					//g_Buffers.g_VehicleHasSiren.erase(veh->m_nModelIndex);
				}
			};

		Events::vehicleDtorEvent += [](CVehicle* veh)
			{
				// remove siren sounds for the car that suddenly disappeared (became null)
				for (int i = 0; i < 3; i++) {
					auto& source = AudioManager.m_apSirens[veh][i];

					if (!source || source->source == 0)
						continue;
					LOG("Cleared siren sound for disappeared car");
					alSourceStop(source->source);
					alDeleteSources(1, &source->source);
					source->source = 0;
					source = nullptr;
					break; // found and cleaned up, no need to keep iterating
				}
				LOG("Vehicle with model %d yeeted from the world", veh->m_nModelIndex);
				g_Buffers.m_fVelocityChangeForAudio.erase(veh);
				//g_Buffers.g_VehicleHasSiren.erase(veh->m_nModelIndex);
			};

			

		// Shut down everything
		shutdownGameEvent += []()
			{
				AudioManager.Shutdown();
			};

		ClearForRestartEvent += []()
			{
				AudioManager.scheduledSounds.clear();
				// Reset ambience stuff on reload to prevent "never playing" issues
				nextZoneAmbienceTime = 0;
				nextFireAmbienceTime = 0;
				//AudioManager.UnloadManualAmbiences();

				// FIX: game would crash on reload when the non-fire fire was active
				g_Buffers.ent.clear();

				for (auto& inst : AudioManager.audiosplaying)
				{
					if (inst->bIsMissile) {
						if (inst->source == inst->missileSource)
						{
							LOG("removing missile source during reload %d", inst->source);
							alDeleteSources(1, &inst->missileSource);
							inst->missileSource = 0;
						}
					}
				}
			};

		ClearExcitingStuffFromAreaEvent += []()
			{
				for (auto& inst : AudioManager.audiosplaying)
				{
					if (inst->bIsMissile) {
						if (inst->source == inst->missileSource)
						{
							alDeleteSources(1, &inst->missileSource);
							inst->missileSource = 0;
						}
					}
				}
			};

		MakePlayerSafeEvent += []()
			{
				for (auto& inst : AudioManager.audiosplaying)
				{
					if (inst->bIsMissile) {
						if (inst->source == inst->missileSource)
						{
							alDeleteSources(1, &inst->missileSource);
							inst->missileSource = 0;
						}
					}
				}
			};

		WorldRemoveProjEvent += [](CEntity* ent)
			{
				for (auto& inst : AudioManager.audiosplaying) {
					if (inst->bIsMissile) {
						if (inst->entity == ent && inst->source == inst->missileSource)
						{
							LOG("Removing missile source %d", inst->missileSource);
							alDeleteSources(1, &inst->missileSource);
							inst->missileSource = 0;
						}
					}
				}
			};

	};
} earShot;

// So, why this exists?
// This is for the case if some other plugin uses OpenAL and it can conflict with this plugin.
// Resulting in no sounds being played because this is a thread safe plugin.
// So that plugin can use the context from here resulting in compatibility.
// Trust me, if it was a .exe this wouldn't have been here.
// Hope that clears it up.
extern "C" __declspec(dllexport) ALCcontext* GetContext()
{
	return AudioManager.GetContext();
}

extern "C" __declspec(dllexport) ALCdevice* GetDevice()
{
	return AudioManager.GetDevice();
}

extern "C" __declspec(dllexport) void PlayWeaponSound(eWeaponType type, eModelID id, std::string filename, CPhysical* ent)
{
	AudioManager.findWeapon(&type, id, filename, ent);
}

extern "C" __declspec(dllexport) void PlayGunshellSound(int type, eSurfaceType surface, const CVector& pos)
{
	AudioManager.PlayGunshellSound(type, surface, pos);
}
