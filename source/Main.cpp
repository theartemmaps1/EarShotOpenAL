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

// useles ans not neded
//#include <libsndfile/include/sndfile.h>
using namespace plugin;
using namespace std;
DebugMenuAPI gDebugMenuAPI;
std::unordered_map<CEntity*, int> g_lastExplosionType;
namespace fs = std::filesystem;
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
				Log("Killstreak: %d", killCounter);
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

auto __fastcall HookedCAEWeaponAudioEntity__WeaponFire(
	CAEWeaponAudioEntity* thispointer, void* unused,
	eWeaponType weaponType, CPhysical* victim, int audioEventId
) {
	Log("HookedCAEWeaponAudioEntity__WeaponFire: audioEvent %d weaponType %d", audioEventId, weaponType);
	CWeapon* weap = nullptr;
	CWeaponInfo* info = nullptr;
	if (thispointer->m_pPed && IsPedPointerValid(thispointer->m_pPed))
	{
		weap = thispointer->m_pPed->GetWeapon();
		info = CWeaponInfo::GetWeaponInfo(weap->m_eWeaponType, thispointer->m_pPed->GetWeaponSkill());
	}
	if (!thispointer) {
		Log("WeaponFire: thispointer is null, falling back.");
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
		Log("WeaponFire: no valid entity found, falling back.");
		subhook_remove(subhookCAEWeaponAudioEntity__WeaponFire);
		thispointer->WeaponFire(weaponType, victim, audioEventId);
		subhook_install(subhookCAEWeaponAudioEntity__WeaponFire);
		return;
	}

	// Handle minigun barrel spinning
	if (audioEventId == 151) {
		if (AudioManager.findWeapon(&weaponType, eModelID(-1), "spin", thispointer->m_pPed, false)) {
			AudioManager.PlayOrStopBarrelSpinSound(thispointer->m_pPed, &weaponType, true);
			return;
		}
		else {
			Log("HookedCAEWeaponAudioEntity__WeaponFire: spin not found");
			subhook_remove(subhookCAEWeaponAudioEntity__WeaponFire);
			thispointer->WeaponFire(weaponType, victim, 151);
			subhook_install(subhookCAEWeaponAudioEntity__WeaponFire);
			return;
		}
	}

	float dist = DistanceBetweenPoints(cameraposition, entity->GetPosition());
	bool farAway = dist > distanceForDistantGunshot;
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
	for (int i = 0; i < 10; ++i) {
		std::string altShoot = "shoot" + std::to_string(i);
		std::string altAfter = "after" + std::to_string(i);
		if (AUDIOPLAY(entity->m_nModelIndex, altShoot) &&
			AUDIOPLAY(entity->m_nModelIndex, altAfter)) {
			alternatePlayed = true;
			break;
		}
	}

	// If no alt sounds played, play singular shooting sounds
	if (!alternatePlayed) {
		if (AUDIOCALL(AUDIOSHOOT)) alternatePlayed = true;
		if (AUDIOCALL(AUDIOAFTER)) alternatePlayed = true;
		if (alternatePlayed) return;
	}

	if (emptyPlayed || alternatePlayed) {
		return;
	}
	// Fallback to OG func if failed to play something
	Log("HookedCAEWeaponAudioEntity__WeaponFire: fallback");
	subhook_remove(subhookCAEWeaponAudioEntity__WeaponFire);
	thispointer->WeaponFire(weaponType, victim, audioEventId);
	subhook_install(subhookCAEWeaponAudioEntity__WeaponFire);
}

auto __fastcall HookedCAEWeaponAudioEntity__WeaponReload(CAEWeaponAudioEntity* thispointer, void* unusedpointer,
	eWeaponType weaponType, CPhysical* entity, int audioEventId
) {
	Log("HookedCAEWeaponAudioEntity__WeaponReload: weaponType: %d, audioEventId: %d", weaponType, audioEventId);
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
	Log("HookedCAEWeaponAudioEntity__WeaponReload: fallback");
	subhook_remove(subhookCAEWeaponAudioEntity__WeaponReload);
	thispointer->WeaponReload(weaponType, entity, audioEventId);
	subhook_install(subhookCAEWeaponAudioEntity__WeaponReload);
}
// a2 is the audioevent
auto __fastcall HookedCAEPedAudioEntity__HandlePedHit(CAEPedAudioEntity* thispointer, void* unusedpointer,
	int AudioEvent, CPhysical* victim, uint8_t Surface, float volume, uint32_t maxVolume
) {
	//	if (!thispointer || !thispointer->m_pPed)
	//		return;
	CPhysical* entity = victim;
	CPed* ped = thispointer->m_pPed;
	eWeaponType weaponType = ped->m_aWeapons[ped->m_nSelectedWepSlot].m_eWeaponType;
	//auto task = entity->m_pIntelligence->GetTaskFighting();
	Log("HookedCAEPedAudioEntity__HandlePedHit: AudioEvent: %d, Surface: %d, volume: %.f, maxVolume: %d, weaponType: %d, victim model ID: %d",
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
			if (AudioEvent == 0x43 && AUDIOCALL(AUDIOMARTIALPUNCH)) {
				Log("AUDIOMARTIALPUNCH success");
				handled = true;
			}
			if (!handled && AudioEvent == 0x44 && AUDIOCALL(AUDIOMARTIALKICK)) {
				Log("AUDIOMARTIALKICK success");
				handled = true;
			}

			// Surface-specific
			if (!handled && metalSurface && AUDIOCALL(AUDIOHITMETALT)) {
				Log("AUDIOHITMETALT success");
				handled = true;
			}
			if (!handled && woodSurface && AUDIOCALL(AUDIOHITWOOD)) {
				Log("AUDIOHITWOOD success");
				handled = true;
			}
			if (!handled && (AudioEvent == 63 || AudioEvent == 64) && AUDIOCALL(AUDIOHITGROUND)) {
				Log("AUDIOHITGROUND success");
				handled = true;
			}

			// Fallback for hitting flesh
			if (!handled && !woodSurface && (victim && victim->m_nType == ENTITY_TYPE_PED) && IsFleshy(Surface) && AUDIOCALL(AUDIOHIT)) {
				Log("AUDIOHIT fallback success");
				handled = true;
			}

			break;
		}

		default:
			Log("Unhandled AudioEvent %d", AudioEvent);
			break;
		}
	}


	if (!handled) {
		Log("Falling back to original HandlePedHit");
		subhook_remove(subhookCAEPedAudioEntity__HandlePedHit);
		plugin::CallMethod<0x4E1CC0, CAEPedAudioEntity*, int, CPhysical*, uint8_t, float, uint32_t>(
			thispointer, AudioEvent, victim, Surface, volume, maxVolume);
		subhook_install(subhookCAEPedAudioEntity__HandlePedHit);
	}
}

// Custom ricochet sounds
auto __fastcall HookedCAudioEngine__ReportBulletHit(CAudioEngine* engine, int, CEntity* victim, uint8_t surface, const CVector& posn, float angleWithColPointNorm)
{
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	//float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	//float audiovolume = AEAudioHardware.m_fEffectMasterScalingFactor * fader;
	// cast to uint8_t to avoid garbage values like "-60030976" or "309067776", etc.
	//uint8_t actualSurface = static_cast<uint8_t>(surface);
	if (engine && surface >= 0 && surface <= TOTAL_NUM_SURFACE_TYPES /*&& victim*/) {
		Log("HookedCAudioEngine__ReportBulletHit: surface %d", surface);
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
				Log("Fallback to default ricochet sounds");
			}
		}

		if (!buffers.empty()) {
			RandomIntegers rnd(buffers.size());

			int index = rnd.next();
			//int index = CGeneral::GetRandomNumber() % buffers.size();
			ALuint ricochetBuf = buffers[index];
			SoundInstanceSettings opts;

			opts.maxDist = 50.0f;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.airAbsorption = 3.0f;
			opts.refDist = 3.5f;
			opts.rollOffFactor = 5.0f;
			opts.pitch = pitch;
			opts.pos = posn;
			AudioManager.PlaySource(ricochetBuf, opts);

			return int(-1);
		}
	}

	//if (surface <= TOTAL_NUM_SURFACE_TYPES) 
//	{
	Log("HookedCAudioEngine__ReportBulletHit: fallback to vanilla for surface %d", surface);
	subhook_remove(subhookCAudioEngine__ReportBulletHit);
	auto returnvalue = CallMethodAndReturn<int, 0x506EC0, CAudioEngine*, CEntity*, uint8_t, const CVector&, float>(
		engine, victim, surface, posn, angleWithColPointNorm);
	subhook_install(subhookCAudioEngine__ReportBulletHit);

	return returnvalue;
	//	}
}


auto __fastcall HookedCAEPedAudioEntity__HandlePedSwing(CAEPedAudioEntity* thispointer, void* unusedpointer,
	int a2, int a3, int a4
) {
	Log("HookedCAEPedAudioEntity__HandlePedSwing: audio event %d", a2);
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
auto __fastcall HookedCAEExplosionAudioEntity_AddAudioEvent(
	CAEExplosionAudioEntity* t,
	void* unusedpointer,
	int Aevent,
	CVector* posn,
	float volume
) {
	Log("HookedCAEExplosionAudioEntity_AddAudioEvent: event %d volume %.2f", Aevent, volume);

	float dist = DistanceBetweenPoints(cameraposition, *posn);
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	// We only process those that aren't really far away (except for distant sounds)
	bool farAway = dist > distanceForDistantExplosion;
	bool handled = false;
	float waterLevel = 0.0f;
	bool isUnderWater = CWaterLevel::GetWaterLevelNoWaves(posn->x, posn->y, posn->z, &waterLevel);
	auto OG = [&]()
		{
			Log("HookedCAEExplosionAudioEntity_AddAudioEvent: fallback to vanilla");
			subhook_remove(subhookCAEExplosionAudioEntity__AddAudioEvent);
			plugin::CallMethod<0x4DCBE0, CAEExplosionAudioEntity*, int, CVector*, float>(t, Aevent, posn, volume);
			subhook_install(subhookCAEExplosionAudioEntity__AddAudioEvent);
		};


	auto chooseBuffers = [](auto& explosionContainer,
		/*auto& weaponContainer,*/
		const std::vector<ALuint>* genericFallback, const char* what)
		-> const std::vector<ALuint>*
		{
			// --- Explosion types ---
			if (!g_lastExplosionType.empty()) {
				for (auto& data : g_lastExplosionType) {
					int expType = data.second;
					auto it = explosionContainer.find(expType);
					if (it != explosionContainer.end() && !it->second.empty()) {
						Log("HookedCAEExplosionAudioEntity_AddAudioEvent: Using ExplosionType: %d for: %s", expType, what);
						return &it->second;
					}
				}
			}

			// --- Generic fallback ---
			if (genericFallback && !genericFallback->empty()) {
				Log("HookedCAEExplosionAudioEntity_AddAudioEvent: Using generic fallback for %s", what);
				return genericFallback;
			}
			if (what != "debris" && what != "distant explosion")
			{
				Log("No buffers at all for main explosion, falling back to vanilla");
			}
			else {
				Log("No buffers at all for %s, ignoring...", what);
			}
			return nullptr;
		};

	auto* explosionUnderwaterBuffers =
		chooseBuffers(g_Buffers.ExplosionTypeUnderwaterBuffers,
			/*g_Buffers.WeaponTypeExplosionBuffers,*/
			&g_Buffers.explosionUnderwaterBuffers, "underwater explosion");

	if (!farAway) {
		// Underwater explosion
		if (isUnderWater && posn->z < waterLevel && explosionUnderwaterBuffers) {
			RandomIntegers rnd(explosionUnderwaterBuffers->size());
			auto inst = std::make_shared<SoundInstance>();
			int id = rnd.next();
			ALuint buff = (*explosionUnderwaterBuffers)[id];
			SoundInstanceSettings opts;

			opts.maxDist = 4000.0f;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor * 0.9f;
			opts.airAbsorption = 3.0f;
			opts.refDist = 15.0f;
			opts.rollOffFactor = 0.20f;
			opts.pitch = pitch;
			opts.pos = *posn;
			handled = AudioManager.PlaySource(buff, opts) != nullptr;
		}
		// Main explosion
		else if (auto* explosionBuffers =
			chooseBuffers(g_Buffers.ExplosionTypeExplosionBuffers,
				/*g_Buffers.WeaponTypeExplosionBuffers,*/
				&g_Buffers.explosionBuffers, "main explosion"))
		{
			RandomIntegers rnd(explosionBuffers->size());

			int id = rnd.next();
			ALuint buff = (*explosionBuffers)[id];
			SoundInstanceSettings opts;
			opts.maxDist = distanceForDistantExplosion;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.airAbsorption = 0.6f;
			opts.refDist = 9.0f;
			opts.rollOffFactor = 0.7f;
			opts.pitch = pitch;
			opts.pos = *posn;
			handled = AudioManager.PlaySource(buff, opts) != nullptr;
		}
		else {
			OG();
		}

		// Debris
		if (auto* debrisBuffers =
			chooseBuffers(g_Buffers.ExplosionTypeDebrisBuffers,
				/*g_Buffers.WeaponTypeDebrisBuffers,*/
				&g_Buffers.explosionsDebrisBuffers, "debris"))
		{
			RandomIntegers rnd(debrisBuffers->size());

			int id = rnd.next();
			ALuint buff = (*debrisBuffers)[id];
			SoundInstanceSettings opts;
			opts.maxDist = distanceForDistantExplosion;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.airAbsorption = 0.8f;
			opts.refDist = 7.0f;
			opts.rollOffFactor = 1.0f;
			opts.pitch = pitch;
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
			opts.maxDist = 200.0f;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.airAbsorption = 0.6f;
			opts.refDist = 20.0f;
			opts.rollOffFactor = 0.5f;
			opts.pitch = pitch;
			opts.pos = *posn;
			handled = AudioManager.PlaySource(buff, opts) != nullptr;
		}
	}


	if (!handled)
	{
		// fallback to vanilla
		OG();
	}

}

auto __fastcall HookedCAEFireAudioEntity__AddAudioEvent(CAEFireAudioEntity* ts, int, int eventId, CVector* posn) {
	// Keep track of CAEFireAudioEntity to check FX existence later
	if (std::find(g_Buffers.ent.begin(), g_Buffers.ent.end(), ts) == g_Buffers.ent.end()) {
		g_Buffers.ent.push_back(ts);
	}

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
		};

	auto EnsureNonFireInstanceValid = [&](int evt) -> bool {
		auto it = g_Buffers.nonFireSounds.find(evt);
		if (it == g_Buffers.nonFireSounds.end()) return false;

		auto inst = it->second;
		if (!inst) { g_Buffers.nonFireSounds.erase(it); return false; }

		int state = AudioManager.GetSourceState(inst->source);
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
					alSourcePlay(inst->source);
				}
				return true;
			}
		}

		RandomIntegers rnd(buffers.size());
		ALuint buf = buffers[rnd.next()];
		if (buf == 0) return false;

		SoundInstanceSettings opts;
		opts.maxDist = 200.0f;
		opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
		opts.airAbsorption = 4.0f;
		opts.refDist = 1.0f;
		opts.rollOffFactor = 1.5f;
		opts.pitch = pitch;
		opts.pos = position;
		opts.isFire = false;
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
		opts.maxDist = 200.0f;
		opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
		opts.airAbsorption = 4.0f;
		opts.refDist = 1.0f;
		opts.rollOffFactor = 1.5f;
		opts.pitch = pitch;
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

	// Nothing worked out, call OG
	if (!handled) {
		subhook_remove(subhookCAEFireAudioEntity__AddAudioEvent);
		plugin::CallMethod<0x4DD3C0, CAEFireAudioEntity*, int, CVector*>(ts, eventId, posn);
		subhook_install(subhookCAEFireAudioEntity__AddAudioEvent);
	}
}


// Forgive my retarded method, but at least it works just how i wanted it to :(
void __fastcall PlayMinigunBarrelStopSound(CAEWeaponAudioEntity* ts, int, CPed* ped)
{
	//	ts->PlayMiniGunStopSound(ped);
	if (ts->m_pPed) {
		eWeaponType weapon = ts->m_pPed->m_aWeapons[ts->m_pPed->m_nSelectedWepSlot].m_eWeaponType;
		CWeaponInfo* weapInfo = CWeaponInfo::GetWeaponInfo(weapon, WEAPSKILL_STD);
		unsigned int anim = weapInfo->m_nAnimToPlay;
		bool isMinigun = (weapon == WEAPONTYPE_MINIGUN ||
			(anim == ANIM_GROUP_FLAME && weapInfo->m_nWeaponFire == WEAPON_FIRE_INSTANT_HIT));
		if (!isMinigun) {
			// Not minigun, block vanilla stop sound entirely
			Log("PlayMinigunBarrelStopSound: Ignored, not minigun");
			ts->PlayMiniGunStopSound(ped);
			return;
		}

		if (AudioManager.findWeapon(&weapon, eModelID(-1), "spin_end", ts->m_pPed, false)) {
			AudioManager.PlayOrStopBarrelSpinSound(ts->m_pPed, &weapon, false, false);
			//	ts->PlayMiniGunStopSound(ped);
			Log("PlayMinigunBarrelStopSound: Custom spin_end found, skipping vanilla");
			return;
		}
	}
	Log("PlayMinigunBarrelStopSound fallback: no custom spin_end, using vanilla");
	ts->PlayMiniGunStopSound(ped);
}

auto __fastcall CAEPedAudioEntity__HandlePedJacked(CAEPedAudioEntity* ts, void*, int AudioEvent)
{
	Log("CAEPedAudioEntity__HandlePedJacked: AudioEvent is %d", AudioEvent);

	if (/*!cameraposition ||*/ !ts || !ts->m_pPed) {
		subhook_remove(subhookCAEPedAudioEntity__HandlePedJacked);
		char returnvalue = CallMethodAndReturn<char, 0x4E2350, CAEPedAudioEntity*, int>(ts, AudioEvent);
		subhook_install(subhookCAEPedAudioEntity__HandlePedJacked);
		return returnvalue;
	}

	bool handled = false;
	CVector PedPos = ts->m_pPed->GetPosition();

	float dist = DistanceBetweenPoints(cameraposition, PedPos);
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);

	auto PlayJackingSound = [&](const std::vector<ALuint>& bufferList) {
		if (bufferList.empty()) return false;
		RandomIntegers rnd(bufferList.size());

		int idx = rnd.next();
		//int idx = CGeneral::GetRandomNumber() % bufferList.size();
		ALuint buf = bufferList[idx];
		// recheck vol
		SoundInstanceSettings opts;
		opts.maxDist = FLT_MAX;
		opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
		opts.airAbsorption = 0.8f;
		opts.refDist = 3.0f;
		opts.rollOffFactor = 1.5f;
		opts.pitch = pitch;
		opts.pos = PedPos;
		if (AudioManager.PlaySource(buf, opts))
		{
			return true;
		}
		return false;
		};

	switch (AudioEvent) {
	case AE_PED_JACKED_CAR_PUNCH: // Jacked: punch
		Log("Playing jacked punch sound");
		handled = PlayJackingSound(g_Buffers.carJackBuff);
		break;
	case AE_PED_JACKED_CAR_HEAD_BANG: // Head bang
		Log("Playing jacked headbang sound");
		handled = PlayJackingSound(g_Buffers.carJackHeadBangBuff);
		break;
	case AE_PED_JACKED_CAR_KICK: // Kick
		Log("Playing jacked kick sound");
		handled = PlayJackingSound(g_Buffers.carJackKickBuff);
		break;
	case AE_PED_JACKED_BIKE: // Bike
		Log("Playing jacked bike sound");
		handled = PlayJackingSound(g_Buffers.carJackBikeBuff);
		break;
	case AE_PED_JACKED_DOZER: // Dozer
		Log("Playing jacked dozer sound");
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


auto __fastcall HookedCAEPedAudioEntity__AddAudioEvent(CAEPedAudioEntity* ts, void*, eAudioEvents audioEvent, float volume, float speed, CPhysical* ped, uint8_t surfaceId, int32_t a7, uint32_t maxVol)
{
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	//float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	//float audiovolume = AEAudioHardware.m_fEffectMasterScalingFactor * fader;
	if (ts && ts->m_pPed) {
		auto pedPtr = ts->m_pPed;
		eSurfaceType actualSurface = eSurfaceType(pedPtr->m_nContactSurface);

		if (actualSurface <= TOTAL_NUM_SURFACE_TYPES && (audioEvent == 54 || audioEvent == 55)) {
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
			auto* clothesDesc = pedPtr->m_pPlayerData ? pedPtr->m_pPlayerData->m_pPedClothesDesc : nullptr;

			if (clothesDesc) {
				int modelId = clothesDesc->m_anModelKeys[4]; // 4 for shoes
				for (const auto& [folder, surfaceMap] : g_Buffers.footstepShoeBuffers) {
					if (CKeyGen::GetUppercaseKey(folder.c_str()) == modelId) {
						shoeType = folder;
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
				float referenceDistance;

				if (pedPtr->IsPlayer()) {
					if (pedPtr->bIsDucking) {
						referenceDistance = 0.1f;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_SPRINT) {
						referenceDistance = 1.0f;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_WALK) {
						referenceDistance = 0.3f;
					}
					else {
						referenceDistance = 0.5f;
					}
				}
				else {
					if (pedPtr->bIsDucking) {
						referenceDistance = 0.1f;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_SPRINT) {
						referenceDistance = 0.7f;
					}
					else if (pedPtr->m_nMoveState == PEDMOVE_WALK) {
						referenceDistance = 0.2f;
					}
					else {
						referenceDistance = 0.3f;
					}
				}
				SoundInstanceSettings opts;
				opts.maxDist = FLT_MAX;//pedPtr->IsPlayer() ? 140.0f : 150.0f;
				opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
				opts.airAbsorption = pedPtr->IsPlayer() ? 1.5f : 3.0f;
				opts.refDist = referenceDistance;
				opts.rollOffFactor = pedPtr->IsPlayer() ? 1.5f : 2.5f;
				opts.pitch = pitch;
				opts.entity = pedPtr;
				//opts.pos = position;
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
	Log("HookedCAudioEngine__ReportWeaponEvent: event: %d, weapon type: %d physical model ID %d", audioEvent, weaponType, physical ? physical->m_nModelIndex : -1);
	if (engine && physical) {
		if (audioEvent == 145) // gun ambience in LS
		{
			if (AudioManager.PlayAmbienceSFX(physical->GetPosition(), weaponType, false))
			{
				Log("HookedCAudioEngine__ReportWeaponEvent: playing ambient sound");
				return;
			}
		}
	}
	Log("HookedCAudioEngine__ReportWeaponEvent: fallback");
	CallMethod<0x506F40, CAudioEngine*, int32_t, eWeaponType, CPhysical*>(engine, audioEvent, weaponType, physical);
}


char __fastcall HookedFireProjectile(
	void* ts, int,
	eWeaponType weaponType, CPhysical* physical, eAudioEvents aEvent)
{
	//g_lastWeaponType[physical] = (int)weaponType;
	Log("HookedFireProjectile: weaponType '%d', audio event '%d', physical model index '%d'", weaponType, aEvent, physical ? physical->m_nModelIndex : -1);
	CVehicle* playaVehicle = FindPlayerVehicle();
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	bool handled = false;
	//	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	//	float audiovolume = AEAudioHardware.m_fEffectMasterScalingFactor * fader;
	if (ts) {

		//CVector pos = playaVehicle ? playaVehicle->GetPosition() : (playa ? playa->GetPosition() : CVector(0.0f,0.0f,0.0f));
		if (aEvent == 0x94) // projectile fire event
		{

			if (playaVehicle && AudioManager.findWeapon(&weaponType, eModelID(playaVehicle->m_nModelIndex), "projfire", playaVehicle))
			{
				Log("HookedFireProjectile: playing rocket fire sound for vehicle model '%d'", playaVehicle->m_nModelIndex);
				//return char(-1);
				handled = true;
			}
			// Custom missile fly sound
			if (physical/* && (weaponType == WEAPONTYPE_ROCKET || weaponType == WEAPONTYPE_ROCKET_HS)*/)
			{
				if (physical->m_nModelIndex == 345) { // missile model id
					auto inst = std::make_shared<SoundInstance>();
					if (inst->missileSource == 0 && !g_Buffers.missileSoundBuffers.empty())
					{
						RandomIntegers rnd(g_Buffers.missileSoundBuffers.size());

						int index = rnd.next();
						ALuint buff = g_Buffers.missileSoundBuffers[index];
						SoundInstanceSettings opts;
						opts.maxDist = 250.0f;
						opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
						opts.airAbsorption = 1.0f;
						opts.refDist = 6.0f;
						opts.rollOffFactor = 1.0f;
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
		Log("HookedFireProjectile: fallback");
		//	g_lastWeaponType = -1;
		return CallMethodAndReturn<char, 0x4DF060, void*, eWeaponType, CPhysical*, eAudioEvents>(ts, weaponType, physical, aEvent);
	}
}

// Custom thunders
void __fastcall HookedCAEWeatherAudioEntity__AddAudioEvent(CAEWeatherAudioEntity* ts, void*, int AudioEvent)
{
	Log("HookedCAEWeatherAudioEntity__AddAudioEvent: event: %d", AudioEvent);
	if (ts && AudioEvent == 141) {
		if (!g_Buffers.ThunderBuffs.empty())
		{
			RandomIntegers rnd(g_Buffers.ThunderBuffs.size());

			int index = rnd.next();
			ALuint buffer = g_Buffers.ThunderBuffs[index];

			if (AudioManager.PlayAmbienceBuffer(buffer, cameraposition, false, true))
			{
				Log("HookedCAudioEngine__ReportWeaponEvent: playing thunder sound");
				return;
			}
		}
	}
	Log("HookedCAEWeatherAudioEntity__AddAudioEvent: fallback");
	CallMethod<0x506800, CAEWeatherAudioEntity*, int>(ts, AudioEvent);
}

// tank cannon fire sound (the explosion func hook)
bool __cdecl TriggerTankFireHooked(CEntity* victim, CEntity* creator, eExplosionType type, CVector pos, uint32_t lifetime, uint8_t usesSound, float cameraShake, uint8_t bInvisible)
{
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	Log("CExplosion::AddExplosion: added explosion with type %d", type);
	// We'll reuse this later for explosion-type specific explosion sounds
	g_lastExplosionType[creator] = (int)type;
	// Just to be sure
	if (CPad::GetPad(0)->CarGunJustDown()) {
		if (!g_Buffers.tankCannonFireBuffers.empty() && creator && creator->m_nType == ENTITY_TYPE_PED && ((CPed*)creator)->bInVehicle && type == EXPLOSION_TANK_FIRE)
		{
			RandomIntegers rnd(g_Buffers.tankCannonFireBuffers.size());

			int index = rnd.next();
			//int index = CGeneral::GetRandomNumber() % g_Buffers.tankCannonFireBuffers.size();
			ALuint buffer = g_Buffers.tankCannonFireBuffers[index];
			CVector pos = creator->GetPosition();
			SoundInstanceSettings opts;
			opts.maxDist = 125.0f;
			opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
			opts.airAbsorption = 0.3f;
			opts.refDist = 3.5f;
			opts.rollOffFactor = 0.3f;
			opts.pitch = pitch;
			opts.pos = pos;
			AudioManager.PlaySource(buffer, opts);
		}
	}
	subhook_remove(subhookCExplosion__AddExplosion);
	bool result = AddExplosion(victim, creator, type, pos, lifetime, usesSound, cameraShake, bInvisible);
	subhook_install(subhookCExplosion__AddExplosion);
	g_lastExplosionType[creator] = -1;
	return result;
}

void __fastcall CAudioEngine__ReportFrontEndAudioHooked(CAudioEngine* eng, int, eAudioEvents eventId, float volumeChange, float speed)
{
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	float audiovolume = AEAudioHardware.m_fEffectMasterScalingFactor * fader;
	//Log("CAudioEngine__ReportFrontEndAudioHooked: event '%d'", eventId);
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
		Log("CAudioEngine__ReportFrontEndAudioHooked: playing audio with event '%d'", eventId);
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

class EarShot {
public:
	EarShot() {
		if (_strcmpi(PLUGIN_FILENAME, "EarShot.asi") > 0) {
			if (!Error("Renaming of this plugin is not allowed. Please, keep it 'EarShot.asi'!"))
			{
				exit(0);
			}
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
		Events::initRwEvent += []
			{
				AudioManager.Initialize();
			};
		patch::RedirectCall({ 0x504D11, 0x504CD2 }, PlayMinigunBarrelStopSound);
		patch::RedirectCall(0x4DF81B, HookedCAudioEngine__ReportWeaponEvent); // LS gunfire ambience
		patch::RedirectCall(0x4DFAE6, HookedFireProjectile);
		patch::RedirectCall(0x72BB37, HookedCAEWeatherAudioEntity__AddAudioEvent);
#ifdef QUAKE_KILLSOUNDS_TEST
		patch::RedirectCall(0x4B93AA, HookedRegisterKillByPlayer);
#endif
		Events::processScriptsEvent += [] {
			if (initializationstatus != -2) return;
			initializationstatus = 0;

			Events::gameProcessEvent += [] {
				//	Log("CWeather::WeatherRegion: %d", CWeather::WeatherRegion);
				if (initializationstatus == 0) {
					if (!AudioManager.GetDevice() || !AudioManager.GetContext()) {
						modMessage("Could not initialize OpenAL audio engine.");
						initializationstatus = -1;
					}
				}

				// Don't update any fire sound if the game is paused
				if (!FrontEndMenuManager.m_bMenuActive)
				{
					AudioManager.UpdateFireSoundCleanup();
					// Play the ambience when it's not rainy and the screen didn't fade out yet
					if (CWeather::Rain <= 0.0f && TheCamera.GetScreenFadeStatus() == 0)
					{
						AudioManager.PlayAmbienceSFX(cameraposition, eWeaponType(0), true);
					}
				}
				// Install hooks and load weapon sounds
				if (initializationstatus != -1) {
					if (initializationstatus == 0) {

						Loaders::LoadMainWeaponsFolder();

						subhookCAEWeaponAudioEntity__WeaponFire = subhook_new((void*)(originalCAEWeaponAudioEntity__WeaponFire)0x504F80, HookedCAEWeaponAudioEntity__WeaponFire, subhook_flags_t(0));
						subhookCAEWeaponAudioEntity__WeaponReload = subhook_new((void*)(originalCAEWeaponAudioEntity__WeaponReload)0x503690, HookedCAEWeaponAudioEntity__WeaponReload, subhook_flags_t(0));
						subhookCAEPedAudioEntity__HandlePedHit = subhook_new((void*)(originalCAEPedAudioEntity__HandlePedHit)0x4E1CC0, HookedCAEPedAudioEntity__HandlePedHit, subhook_flags_t(0));
						subhookCAEPedAudioEntity__HandlePedSwing = subhook_new((void*)(originalCAEPedAudioEntity__HandlePedSwing)0x4E1A40, HookedCAEPedAudioEntity__HandlePedSwing, subhook_flags_t(0));
						subhookCAEExplosionAudioEntity__AddAudioEvent = subhook_new((void*)(originalCAEExplosionAudioEntity__AddAudioEvent)0x4DCBE0, HookedCAEExplosionAudioEntity_AddAudioEvent, subhook_flags_t(0));
						subhookCAEPedAudioEntity__HandlePedJacked = subhook_new((void*)(originalCAEPedAudioEntity__HandlePedJacked)0x4E2350, CAEPedAudioEntity__HandlePedJacked, subhook_flags_t(0));
						subhookCAEFireAudioEntity__AddAudioEvent = subhook_new((void*)(originalCAEFireAudioEntity__AddAudioEvent)0x4DD3C0, HookedCAEFireAudioEntity__AddAudioEvent, subhook_flags_t(0));
						subhookCAudioEngine__ReportBulletHit = subhook_new((void*)(originalCAudioEngine__ReportBulletHit)0x506EC0, HookedCAudioEngine__ReportBulletHit, subhook_flags_t(0));
						subhookCAEPedAudioEntity__AddAudioEvent = subhook_new((void*)(originalCAEPedAudioEntity__AddAudioEvent)0x4E2BB0, HookedCAEPedAudioEntity__AddAudioEvent, subhook_flags_t(0));
						subhookCExplosion__AddExplosion = subhook_new((void*)(originalCExplosion__AddExplosion)0x736A50, TriggerTankFireHooked, subhook_flags_t(0));
						subhookCAudioEngine__ReportFrontEndAudioEvent = subhook_new((void*)(originalCAudioEngine__ReportFrontEndAudioEvent)0x506EA0, CAudioEngine__ReportFrontEndAudioHooked, subhook_flags_t(0));
						//	subhookReportCollision = subhook_new((void*)(tReportCollision)0x506EB0, HookedReportCollision, subhook_flags_t(0));


						subhook_install(subhookCAEWeaponAudioEntity__WeaponFire);
						subhook_install(subhookCAEWeaponAudioEntity__WeaponReload);
						subhook_install(subhookCAEPedAudioEntity__HandlePedHit);
						subhook_install(subhookCAEPedAudioEntity__HandlePedSwing);
						subhook_install(subhookCAEExplosionAudioEntity__AddAudioEvent);
						subhook_install(subhookCAEPedAudioEntity__HandlePedJacked);
						subhook_install(subhookCAEFireAudioEntity__AddAudioEvent);
						subhook_install(subhookCAudioEngine__ReportBulletHit);
						subhook_install(subhookCAEPedAudioEntity__AddAudioEvent);
						subhook_install(subhookCExplosion__AddExplosion);
						subhook_install(subhookCAudioEngine__ReportFrontEndAudioEvent);
						//subhook_install(subhookReportCollision);
						Events::gameProcessEvent += [] 
							{
								float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
								for (auto& inst : AudioManager.audiosplaying)
								{
									// This gave me PTSD, it created one bug to another, and i managed to get it fixed
									// So i prefer to not touch it anymore and leave it as it is
									if (inst->source == barrelSpinSource && inst->shooter && inst->minigunBarrelSpin)
									{
										ALint spinState;
										spinState = AudioManager.GetSourceState(inst->source);
										eWeaponType weaponType = inst->shooter->m_aWeapons[inst->shooter->m_nSelectedWepSlot].m_eWeaponType;
										CTaskSimpleUseGun* shooting = inst->shooter->m_pIntelligence->GetTaskUseGun();
										bool gunTaskFinished = shooting && shooting->m_bIsFinished;
										bool shouldStop = (weaponType != inst->WeaponType || (gunTaskFinished) || inst->shooter->m_ePedState == PEDSTATE_DEAD || inst->shooter->m_fHealth <= 0.0f);
										if (spinState == AL_PLAYING && shouldStop)
										{
											//	inst->shooter->m_weaponAudio.PlayMiniGunStopSound(inst->shooter);
											AudioManager.PlayOrStopBarrelSpinSound(inst->shooter, &weaponType, false, gunTaskFinished ? true : false);
											//	Log("Gargle");
										}
									}
									// We update each source's gain so when the screen fades, the sound can fade smoothly as well
									ALint state;
									state = AudioManager.GetSourceState(inst->source);
									ALint buffer;
									alGetSourcei(inst->source, AL_BUFFER, &buffer);
									ALint fmt = AudioManager.GetBufferFormat((ALuint)buffer);

									float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
									float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
									float gain = gameVol * fader;
									if ((inst->isAmbience/* || inst->isManualAmbience*/) && state == AL_PLAYING && (fmt == AL_FORMAT_STEREO_FLOAT32 || fmt == AL_FORMAT_MONO_FLOAT32))
									{
										// They are loud as hell, decrease the gain a bit
										if (fmt == AL_FORMAT_STEREO_FLOAT32) {
											alSourcei(inst->source, AL_SOURCE_RELATIVE, AL_TRUE);
											alSource3f(inst->source, AL_POSITION, 0.0f, 0.0f, 0.0f);
											gain *= stereoAmbienceVol;
										}

										if (CCutsceneMgr::ms_running || CGame::currArea > 0)
											gain = 0.0f; // mute to not interrupt anything

										AudioManager.SetSourceGain(inst->source, gain);
									}
									else if (state == AL_PLAYING)
									{
										bool notOk = NameStartsWithIndexedSuffix(inst->nameBuffer.c_str(), { "spin", "spin_end", "low_ammo" }) 
											|| IsMatchingName(inst->nameBuffer.c_str(), { "spin", "spin_end", "low_ammo" });
										if (!notOk) 
										{
											AudioManager.SetSourceGain(inst->source, gain);
										}
									}

									// Doppler effect (never for ambience sounds tho)
									// Cut cuz useless
									if (inst->bIsMissile && inst->source == inst->missileSource) {
										static CVector prevPos = *TheCamera.GetGameCamPosition();
										static CVector smoothedVel{ 0.0f, 0.0f, 0.0f };
										static bool prevSkipFrame = false;

										bool skipFrame = TheCamera.m_bJust_Switched || TheCamera.m_bCameraJustRestored || CPad::GetPad(0)->JustOutOfFrontEnd;
										CVector position = *TheCamera.GetGameCamPosition();

										float timeStep = 0.001f * (CTimer::m_snTimeInMillisecondsNonClipped - CTimer::m_snPreviousTimeInMillisecondsNonClipped);

										if (!skipFrame && timeStep > 0.0001f) {
											CVector vel = (position - prevPos) / timeStep;

											if (!prevSkipFrame) {
												smoothedVel = (smoothedVel * 2.0f + vel) / 3.0f;
											}
											else {
												smoothedVel = vel;
											}
											alListener3f(AL_VELOCITY, smoothedVel.x, smoothedVel.y, smoothedVel.z);
										}

										prevPos = position;
										prevSkipFrame = skipFrame;
									}
									else {
										alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
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
									AudioManager.AttachReverbToSource(inst.get()->source);
								}
							};
					}
					// The listener is the player, set appropriate values...
					CVector pos = *TheCamera.GetGameCamPosition();
					cameraposition = pos;
					alListener3f(AL_POSITION, cameraposition.x, cameraposition.y, cameraposition.z);
					// For correct sound panning we use camera's heading, ensuring convincing 3D audio
					CVector vecCamDir = TheCamera.m_mCameraMatrix.GetForward();
					CVector vecCamUpDir = TheCamera.m_mCameraMatrix.GetUp();
					vecCamDir.Normalise();
					vecCamUpDir.Normalise();
					ALfloat orientation[6] =
					{
					vecCamDir.x, vecCamDir.y, vecCamDir.z, // forward vector
					vecCamUpDir.x, vecCamUpDir.y, vecCamUpDir.z // up vector
					};

					alListenerfv(AL_ORIENTATION, orientation);
					AudioManager.audiosplaying.erase(remove_if(AudioManager.audiosplaying.begin(), AudioManager.audiosplaying.end(),
						[&](const std::shared_ptr<SoundInstance>& inst) {
							//float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
							ALint state;
							state = AudioManager.GetSourceState(inst->source);
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
							// Update minigun spin pos relative to the owner
							if (inst->shooter && inst->minigunBarrelSpin && inst->source != 0)
							{
								alSource3f(inst->source, AL_POSITION, inst->shooter->GetPosition().x, inst->shooter->GetPosition().y, inst->shooter->GetPosition().z);
							}

							// To prevent overflow, we erase any sources that are no longer used
							if (state != AL_PLAYING && state != AL_PAUSED) {
								//Log("Removed source '%d'", inst->source);
								alDeleteSources(1, &inst->source);
								inst->source = 0;
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
							Log("Counter reset");
							killCounter = 0;
						}
					}
#endif

				}

				if (initializationstatus == 0) initializationstatus = 1;
				};
			};

		// Shut down everything
		shutdownGameEvent += []()
			{
				AudioManager.Shutdown();
			};

		ClearForRestartEvent += []()
			{
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
							Log("removing missile source during reload %d", inst->source);
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
							Log("Removing missile source %d", inst->missileSource);
							alDeleteSources(1, &inst->missileSource);
							inst->missileSource = 0;
						}
					}
				}
			};

	}
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
