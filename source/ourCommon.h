#pragma once
#include "plugin.h"
#include "CCamera.h"
#include "CAEAudioHardware.h"
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/efx.h"
#include <eSurfaceType.h>
#include <filesystem>
#include "logging.h"
#include <unordered_set>
using namespace plugin;
using namespace std;
namespace fs = std::filesystem;
static CdeclEvent <AddressList<0x748E6B, H_CALL>, PRIORITY_BEFORE, ArgPickNone, void()> shutdownGameEvent;
static CdeclEvent <AddressList<0x564372, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> ClearForRestartEvent;
static CdeclEvent <AddressList<0x56A46E, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> ClearExcitingStuffFromAreaEvent;
static CdeclEvent <AddressList<0x56E975, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> MakePlayerSafeEvent;
static CdeclEvent <AddressList<0x738AFF, H_CALL, 0x73997A, H_CALL, 0x739A17, H_CALL, 0x739AD0, H_CALL>, PRIORITY_AFTER, ArgPickN<CEntity*, 0>, void(CEntity* ent)> WorldRemoveProjEvent;
static uint32_t interiorIntervalMin = 5000;  // ms
static uint32_t interiorIntervalMax = 10000; // ms
static uint32_t fireIntervalMin = 5000;
static uint32_t fireIntervalMax = 10000;
static uint32_t zoneIntervalMin = 5000;
static uint32_t zoneIntervalMax = 10000;
// Store audios data
struct SoundInstance
{
	// The source per instance
	ALuint source = 0;
	// Entity the sounds is attached too
	CPhysical* entity = nullptr;
	// These two below are only used when the game get's paused
	float pauseOffset = 0.0f;
	bool paused = false;
	// Name's and path's data
	std::string nameBuffer;
	fs::path path;
	const char* name;
	// The sound pos in the world
	CVector pos;
	bool isPossibleGunFire = false;
	ALuint filter = 0;
	ALuint missileSource = 0;
	bool bIsMissile = false;
	// Minigun barrel stuff
	bool spinEndStarted = false;
	bool minigunBarrelSpin = false;
	// Ambience stuff
	bool isAmbience = false;
	bool isGunfireAmbience = false;
	bool isInteriorAmbience = false;
	eWeaponType WeaponType;
	CPed* shooter = nullptr;
	ALenum spinningstatus;
	float baseGain = 1.0f;
	bool isFire = false;
	FxSystem_c* fireFX = nullptr;
	CFire* firePtr = nullptr;
};

// Main array to manage ALL currently playing sounds
static std::vector<std::shared_ptr<SoundInstance>> audiosplaying;
//static std::unordered_set<ALuint> pausedFireSources;
static void PauseAudio(SoundInstance* audio) {
	if (!audio->paused) {
		//if (audio->isFire) pausedFireSources.insert(audio->source);
		alGetSourcef(audio->source, AL_SEC_OFFSET, &audio->pauseOffset);
		alSourcePause(audio->source);
		audio->paused = true;
	}
}

static void ResumeAudio(SoundInstance* audio) {
	if (audio->paused) {
		//if (audio->isFire) pausedFireSources.erase(audio->source);
		alSourcef(audio->source, AL_SEC_OFFSET, audio->pauseOffset);
		alSourcePlay(audio->source);
		audio->paused = false;
	}
}

class AudioStream;
struct SoundInstance;
struct ThrownWeapon {
	int32_t explosionType;
	int32_t weaponType;
};
// Define them all in a structure for a better readability
struct Buffers
{
	//std::vector<AudioStream> registeredExplosions;
	//std::vector<AudioStream> registeredExplosionsDebris;
	std::unordered_map<CFire*, std::shared_ptr<SoundInstance>> fireSounds;
	std::unordered_map<int, std::shared_ptr<SoundInstance>> nonFireSounds;
	std::vector<CAEFireAudioEntity*> ent;
	std::vector<ALuint> explosionBuffers;
	std::vector<ALuint> molotovExplosionBuffers;
	std::vector<ALuint> explosionsDebrisBuffers;
	std::vector<ALuint> explosionDistantBuffers;
	std::vector<ALuint> ricochetBuffers;
	//fs::path* weaponpath;
	std::unordered_map<std::string, std::vector<ALuint>> ricochetBuffersPerMaterial;
	std::unordered_map<std::string, std::vector<ALuint>> footstepBuffersPerSurface;
	std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>> footstepShoeBuffers;
	std::unordered_map<std::string, std::vector<ALuint>> footstepSurfaceBuffers;
	std::vector<ALuint> fireLoopBuffers;
	std::vector<ALuint> fireBurstBuffers;
	std::vector<ALuint> fireLoopBuffersSmall;
	std::vector<ALuint> fireLoopBuffersMedium;
	std::vector<ALuint> fireLoopBuffersLarge;
	std::vector<ALuint> fireLoopBuffersCar;
	std::vector<ALuint> fireLoopBuffersBike;
	std::vector<ALuint> fireLoopBuffersFlame;
	std::vector<ALuint> fireLoopBuffersMolotov;
	std::vector<ALuint> carJackBuff;
	std::vector<ALuint> carJackHeadBangBuff;
	std::vector<ALuint> carJackKickBuff;
	std::vector<ALuint> carJackBikeBuff;
	std::vector<ALuint> carJackBulldozerBuff;
	std::vector<ALuint> NightAmbienceBuffs;
	std::vector<ALuint> RiotAmbienceBuffs;
	std::vector<ALuint> ThunderBuffs;
	std::unordered_map<std::string, std::vector<ALuint>> ZoneAmbienceBuffers_Day;
	std::unordered_map<std::string, std::vector<ALuint>> ZoneAmbienceBuffers_Night;
	std::unordered_map<std::string, std::vector<ALuint>> ZoneAmbienceBuffers_Riot;
	std::unordered_map<std::string, std::vector<ALuint>> GlobalZoneAmbienceBuffers_Day;
	std::unordered_map<std::string, std::vector<ALuint>> GlobalZoneAmbienceBuffers_Night;
	std::unordered_map<std::string, std::vector<ALuint>> GlobalZoneAmbienceBuffers_Riot;
	std::vector<ALuint> AmbienceBuffs; // old generic ambience buffers
	// New map for weapon-specific ambience sounds by weapon type enum
	std::unordered_map<eWeaponType, ALuint> WeaponTypeAmbienceBuffers;
	std::vector<ALuint> missileSoundBuffers;
	std::vector<ALuint> tankCannonFireBuffers;
	std::vector<ALuint> bulletWhizzLeftRearBuffers;
	std::vector<ALuint> bulletWhizzLeftFrontBuffers;
	std::vector<ALuint> bulletWhizzRightRearBuffers;
	std::vector<ALuint> bulletWhizzRightFrontBuffers;
	//std::unordered_map<int, std::vector<ALuint>> WeaponTypeExplosionBuffers;
	std::unordered_map<int, std::vector<ALuint>> ExplosionTypeExplosionBuffers;
	//std::unordered_map<int, std::vector<ALuint>> ExplosionTypeMolotovBuffers;
	std::unordered_map<int, std::vector<ALuint>> ExplosionTypeDistantBuffers;
	std::unordered_map<int, std::vector<ALuint>> ExplosionTypeDebrisBuffers;
	//std::unordered_map<int, std::vector<ALuint>> WeaponTypeMolotovBuffers;
	//std::unordered_map<int, std::vector<ALuint>> WeaponTypeDistantBuffers;
	//std::unordered_map<int, std::vector<ALuint>> WeaponTypeDebrisBuffers;
	//std::unordered_map<CEntity*, int32_t> g_WeaponTypes;
	std::unordered_map<CEntity*, int32_t> g_ExplosionTypes;
};

extern Buffers g_Buffers;


// Utility: safely delete an AL buffer
inline void SafeDeleteBuffer(ALuint& buf) {
	if (alIsBuffer(buf) && buf != 0) {
		Log("Freeing buffer %u", buf);
		alDeleteBuffers(1, &buf);
		buf = 0;
	}
}

// Utility: delete all buffers in a vector
inline void DeleteBufferVector(std::vector<ALuint>& vec) {
	for (auto& b : vec) SafeDeleteBuffer(b);
	vec.clear();
}

// Utility: delete all buffers in map<string, vector<ALuint>>
inline void DeleteBufferMapVec(std::unordered_map<std::string, std::vector<ALuint>>& map) {
	for (auto& kv : map) DeleteBufferVector(kv.second);
	map.clear();
}

// Utility: delete all buffers in map<int, vector<ALuint>>
inline void DeleteBufferMapVec(std::unordered_map<int, std::vector<ALuint>>& map) {
	for (auto& kv : map) DeleteBufferVector(kv.second);
	map.clear();
}

// Utility: delete all buffers in nested map<string, map<string, vector<ALuint>>>
inline void DeleteBufferMapNested(std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>>& map) {
	for (auto& kv1 : map) {
		for (auto& kv2 : kv1.second) {
			DeleteBufferVector(kv2.second);
		}
	}
	map.clear();
}

// Utility: delete buffers in map<eWeaponType, ALuint>
inline void DeleteBufferMapSingle(std::unordered_map<eWeaponType, ALuint>& map) {
	for (auto& kv : map) SafeDeleteBuffer(kv.second);
	map.clear();
}

static void DeleteAllBuffers(Buffers& b) {
	DeleteBufferVector(b.explosionBuffers);
	DeleteBufferVector(b.molotovExplosionBuffers);
	DeleteBufferVector(b.explosionsDebrisBuffers);
	DeleteBufferVector(b.explosionDistantBuffers);
	DeleteBufferVector(b.ricochetBuffers);

	DeleteBufferMapVec(b.ricochetBuffersPerMaterial);
	DeleteBufferMapVec(b.footstepBuffersPerSurface);
	DeleteBufferMapNested(b.footstepShoeBuffers);
	DeleteBufferMapVec(b.footstepSurfaceBuffers);

	DeleteBufferVector(b.fireLoopBuffers);
	DeleteBufferVector(b.fireBurstBuffers);
	DeleteBufferVector(b.fireLoopBuffersSmall);
	DeleteBufferVector(b.fireLoopBuffersMedium);
	DeleteBufferVector(b.fireLoopBuffersLarge);
	DeleteBufferVector(b.fireLoopBuffersCar);
	DeleteBufferVector(b.fireLoopBuffersBike);
	DeleteBufferVector(b.fireLoopBuffersFlame);
	DeleteBufferVector(b.fireLoopBuffersMolotov);

	DeleteBufferVector(b.carJackBuff);
	DeleteBufferVector(b.carJackHeadBangBuff);
	DeleteBufferVector(b.carJackKickBuff);
	DeleteBufferVector(b.carJackBikeBuff);
	DeleteBufferVector(b.carJackBulldozerBuff);

	DeleteBufferVector(b.NightAmbienceBuffs);
	DeleteBufferVector(b.RiotAmbienceBuffs);
	DeleteBufferVector(b.ThunderBuffs);

	DeleteBufferMapVec(b.ZoneAmbienceBuffers_Day);
	DeleteBufferMapVec(b.ZoneAmbienceBuffers_Night);
	DeleteBufferMapVec(b.ZoneAmbienceBuffers_Riot);

	DeleteBufferMapVec(b.GlobalZoneAmbienceBuffers_Day);
	DeleteBufferMapVec(b.GlobalZoneAmbienceBuffers_Night);
	DeleteBufferMapVec(b.GlobalZoneAmbienceBuffers_Riot);

	DeleteBufferVector(b.AmbienceBuffs);

	DeleteBufferMapSingle(b.WeaponTypeAmbienceBuffers);

	DeleteBufferVector(b.missileSoundBuffers);
	DeleteBufferVector(b.tankCannonFireBuffers);
	DeleteBufferVector(b.bulletWhizzLeftRearBuffers);
	DeleteBufferVector(b.bulletWhizzLeftFrontBuffers);
	DeleteBufferVector(b.bulletWhizzRightRearBuffers);
	DeleteBufferVector(b.bulletWhizzRightFrontBuffers);

	DeleteBufferMapVec(b.ExplosionTypeExplosionBuffers);
	DeleteBufferMapVec(b.ExplosionTypeDistantBuffers);
	DeleteBufferMapVec(b.ExplosionTypeDebrisBuffers);
	// cleanup any per-fire / non-fire instances so they don't reference deleted sources/buffers
	for (auto& kv : g_Buffers.fireSounds) {
		auto inst = kv.second.get();
		if (inst && inst->source != 0) {
			alSourceStop(inst->source);
			alSourcei(inst->source, AL_BUFFER, AL_NONE);
			alDeleteSources(1, &inst->source);
			inst->source = 0;
		}
	}
	g_Buffers.fireSounds.clear();

	for (auto& kv : g_Buffers.nonFireSounds) {
		auto inst = kv.second.get();
		if (inst && inst->source != 0) {
			alSourceStop(inst->source);
			alSourcei(inst->source, AL_BUFFER, AL_NONE);
			alDeleteSources(1, &inst->source);
			inst->source = 0;
		}
	}
	g_Buffers.nonFireSounds.clear();

	// and clear ent vector if it holds CAEFireAudioEntity* references
	g_Buffers.ent.clear();

	// g_ExplosionTypes only stores ints, no ALuints, so just clear
	b.g_ExplosionTypes.clear();
}

// To avoid constant OpenAL blocks, we use this func for everything.
// Used to play 3D sounds in a 3D space.
// For 2D sounds we use the other func called "playBuffer2D".
// Note that this plays the sound only ONCE and looping is only done for missiles/fire/minigun barrel here and managed separately.
// @returns true on success, false otherwise.
static auto playBuffer = [&](ALuint buff/*const std::vector<ALuint>* buffersInVector = nullptr, ALuint singleBuff = 0*/,
	float maxDist = FLT_MAX,
	float gain = 1.0f,
	float airAbsorption = 1.0f,
	float refDist = 1.0f,
	float rollOffFactor = 1.0f, float pitch = 1.0f,
	CVector pos = CVector(0.0f, 0.0f, 0.0f), bool isFire = false,
	CFire* firePtr = nullptr,
	int fireEventID = 0, std::shared_ptr<SoundInstance>* outInst = nullptr,
	CPhysical* ent = nullptr, bool isPossibleGunFire = false,
	fs::path* path = nullptr, std::string nameBuff = "",
	bool isMinigunBarrelSpin = false, CPed* shooter = nullptr, eWeaponType weapType = eWeaponType(0), bool isGunfire = false,
	bool isInteriorAmbience = false, bool isMissile = false, bool looping = false, uint32_t delay = 0, bool isAmbience = false, FxSystem_c* fireFX = nullptr) -> bool
	{
		float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
		float audiovolume = gain * fader;
		// No point in continuing, there's no valid buffers!
		if (!alIsBuffer(buff)) return false;

		auto inst = std::make_shared<SoundInstance>();
		alGenSources(1, &inst->source);
		// Is it a valid source?
		if (!alIsSource(inst->source)) 
		{
			return false;
		}
		bool useLooping = false;
		if (isFire || isMissile || looping) 
		{
			useLooping = true;
		}
		alSourcei(inst->source, AL_BUFFER, buff);
		alSource3f(inst->source, AL_POSITION, pos.x, pos.y, pos.z);
		alSourcef(inst->source, AL_GAIN, audiovolume);
		alSourcef(inst->source, AL_AIR_ABSORPTION_FACTOR, airAbsorption);
		alSourcef(inst->source, AL_PITCH, pitch);
		alSourcei(inst->source, AL_LOOPING, useLooping);
		alSourcef(inst->source, AL_REFERENCE_DISTANCE, refDist);
		alSourcef(inst->source, AL_MAX_DISTANCE, maxDist);
		alSourcef(inst->source, AL_ROLLOFF_FACTOR, rollOffFactor);
		//AttachReverbToSource(inst->source);
		static uint32_t playStartTime = 0;

		if (ent && ent->m_nType == ENTITY_TYPE_PED) {
			auto ped = reinterpret_cast<CPed*>(ent);
			eWeaponType weaponType = ped->m_aWeapons[ped->m_nSelectedWepSlot].m_eWeaponType;
			CWeaponInfo* weapInfo = CWeaponInfo::GetWeaponInfo(weaponType, WEAPSKILL_STD);

			if (weapInfo) {
				unsigned int anim = weapInfo->m_nAnimToPlay;
				bool isMinigun = (weaponType == WEAPONTYPE_MINIGUN ||
					(anim == ANIM_GROUP_FLAME && weapInfo->m_nWeaponFire == WEAPON_FIRE_INSTANT_HIT));

				if (isMinigun) {
					if (CTimer::m_snTimeInMilliseconds - playStartTime > delay) {
						playStartTime = CTimer::m_snTimeInMilliseconds;
						alSourcePlay(inst->source);
					}
				}
				else {
					alSourcePlay(inst->source);
				}
			}
		}
		else {
			alSourcePlay(inst->source);
		}
		if (path && !path->empty()) {
			inst->path = *path;
			if (!nameBuff.empty())
				inst->nameBuffer = nameBuff;
		}
		if (!inst->nameBuffer.empty()) 
		{
			inst->name = inst->nameBuffer.c_str();
		}

		inst->entity = ent;
		inst->pos = pos;
		inst->isPossibleGunFire = isPossibleGunFire;
		inst->minigunBarrelSpin = isMinigunBarrelSpin;
		inst->shooter = shooter;
		inst->WeaponType = weapType;
		inst->pos = pos;
		inst->isAmbience = isAmbience;
		inst->isGunfireAmbience = isGunfire;
		inst->isInteriorAmbience = isInteriorAmbience;
		inst->bIsMissile = isMissile;
		inst->baseGain = audiovolume;
		inst->isFire = isFire;
		inst->fireFX = fireFX;
		inst->firePtr = firePtr;

		if (inst->bIsMissile) 
		{
			inst->missileSource = inst->source;
		}

		if (!inst->path.empty() && !inst->nameBuffer.empty() && inst->name)
		{
			Log("playBuffer: path: %s, nameBuffer: %s, name: %s", inst->path.string().c_str(), inst->nameBuffer.c_str(), inst->name);
		}
		if (outInst) 
		{
			*outInst = inst;
		}
		if (isFire) {
			if (firePtr) {
				g_Buffers.fireSounds[firePtr] = inst;
			}
		}
		else if (fireEventID != 0) {
			g_Buffers.nonFireSounds[fireEventID] = inst;
		}
		if (inst) 
		{
			audiosplaying.push_back(std::move(inst));
			return true;
		}
		return false;
	};

static auto setSourceGain = [&](ALuint source, float gain) -> bool
	{
		// Is it a valid source?
		if (!alIsSource(source)) return false;
		alSourcef(source, AL_GAIN, gain);
		return true;
	};

static auto playBuffer2D = [&](ALuint buff, bool relative, float volume, float pitch) -> bool
{
		// No point in continuing, there's no valid buffers!
		if (!alIsBuffer(buff)) return false;

		auto inst = std::make_shared<SoundInstance>();
		alGenSources(1, &inst->source);
		// Is it a valid source?
		if (!alIsSource(inst->source))
		{
			return false;
		}
		alSourcei(inst->source, AL_BUFFER, buff);
		alSourcei(inst->source, AL_SOURCE_RELATIVE, relative);
		alSourcef(inst->source, AL_GAIN, volume);
		alSourcef(inst->source, AL_PITCH, pitch);
		alSourcePlay(inst->source);
		return true;
};

// Surface helpers
class SurfaceInfos_c;
static SurfaceInfos_c& g_surfaceInfos = *reinterpret_cast<SurfaceInfos_c*>(0xB79538);
// 0x55EA30
inline bool IsAudioConcrete(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EA30, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EA50
inline bool IsAudioGrass(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EA50, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EA70
inline bool IsAudioSand(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EA70, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EA90
inline bool IsAudioGravel(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EA90, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EAB0
inline bool IsAudioWood(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EAB0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EAD0
inline bool IsAudioWater(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EAD0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EAF0
inline bool IsAudioMetal(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EAF0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EB10
inline bool IsAudioLongGrass(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EB10, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// 0x55EB30
inline bool IsAudioTile(uint32_t id)
{
	return CallMethodAndReturn<bool, 0x55EB30, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Bullet FX
// 0x55E670
uint32_t GetBulletFx(uint32_t id) {
	return CallMethodAndReturn<uint32_t, 0x55E670, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Soft landing
// 0x55E690
inline bool IsSoftLanding(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E690, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// See-through
// 0x55E6B0
inline bool IsSeeThrough(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E6B0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Shoot-through
// 0x55E6D0
inline bool IsShootThrough(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E6D0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Sand
// 0x55E6F0
inline bool IsSand(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E6F0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Is wood
inline bool IsWood(uint32_t id) {
	switch (id) {
	case SURFACE_WOOD_CRATES:
	case SURFACE_WOOD_SOLID:
	case SURFACE_WOOD_THIN:
	case SURFACE_WOOD_BENCH:
	case SURFACE_FLOORBOARD:
	case SURFACE_STAIRSWOOD:
	case SURFACE_P_WOODLAND:
	case SURFACE_P_WOODDENSE:
	case SURFACE_P_FORESTSTUMPS:
	case SURFACE_P_FORESTSTICKS:
	case SURFACE_P_FORRESTLEAVES:
	case SURFACE_P_FORRESTDRY:
	case SURFACE_WOOD_PICKET_FENCE:
	case SURFACE_WOOD_SLATTED_FENCE:
	case SURFACE_WOOD_RANCH_FENCE:
		return true;
		break;

	default:
		return false;
		break;
	}
}

// Is a fleshy surface
inline bool IsFleshy(uint32_t id) {
	switch (id) {
	case SURFACE_PED:
	case SURFACE_GORE:
	case SURFACE_DEFAULT: // yeah, default sometimes can be fleshy
		return true;
		break;
	default:
		return false;
		break;
	}
}

// Water
// 0x55E710
inline bool IsWater(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E710, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Shallow water
// 0x55E730
inline bool IsShallowWater(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E730, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Beach
// 0x55E750
inline bool IsBeach(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E750, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Steep slope
// 0x55E770
inline bool IsSteepSlope(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E770, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Glass
// 0x55E790
inline bool IsGlass(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E790, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Stairs
// 0x55E7B0
inline bool IsStairs(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E7B0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Skateable
// 0x55E7D0
inline bool IsSkateable(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E7D0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Pavement
// 0x55E7F0
inline bool IsPavement(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E7F0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Roughness
// 0x55E810
uint32_t GetRoughness(uint32_t id) {
	return CallMethodAndReturn<uint32_t, 0x55E810, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Flammability
// 0x55E830
uint32_t GetFlammability(uint32_t id) {
	return CallMethodAndReturn<uint32_t, 0x55E830, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Sparks
// 0x55E850
inline bool CreatesSparks(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E850, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Can't sprint
// 0x55E870
inline bool CantSprintOn(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E870, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Footsteps
// 0x55E890
inline bool LeavesFootsteps(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E890, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Foot dust
// 0x55E8B0
inline bool ProducesFootDust(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E8B0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Makes car dirty
// 0x55E8D0
inline bool MakesCarDirty(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E8D0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Makes car clean
// 0x55E8F0
inline bool MakesCarClean(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E8F0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Wheel grass
// 0x55E910
inline bool CreatesWheelGrass(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E910, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Wheel gravel
// 0x55E930
inline bool CreatesWheelGravel(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E930, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Wheel mud
// 0x55E950
inline bool CreatesWheelMud(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E950, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Wheel dust
// 0x55E970
inline bool CreatesWheelDust(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E970, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Wheel sand
// 0x55E990
inline bool CreatesWheelSand(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E990, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Wheel spray
// 0x55E9B0
inline bool CreatesWheelSpray(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E9B0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Plants
// 0x55E9D0
inline bool CreatesPlants(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E9D0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Objects
// 0x55E9F0
inline bool CreatesObjects(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55E9F0, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}

// Climb
// 0x55EA10
inline bool CanClimb(uint32_t id) {
	return CallMethodAndReturn<bool, 0x55EA10, SurfaceInfos_c*, uint32_t>(&g_surfaceInfos, id);
}


inline float sq(float x)
{
	return x * x;
}

#define SQR(x) ((x) * (x))

#include <numeric>   // std::iota
#include <random>    // std::mt19937, std::random_device

class RandomUnique {
public:
	RandomUnique(size_t n)
		: rng(std::random_device{}()), current(0)
	{
		reset(n);
	}

	// Reset with new size
	void reset(size_t n) {
		indices.resize(n);
		std::iota(indices.begin(), indices.end(), 0); // fill 0..n-1
		std::shuffle(indices.begin(), indices.end(), rng);
		current = 0;
	}

	// Get next unique number
	int next() {
		if (current >= indices.size()) {
			// auto-reset if exhausted (optional behavior)
			reset(indices.size());
		}
		return indices[current++];
	}

	// Check if we've used all numbers
	bool empty() const {
		return current >= indices.size();
	}

private:
	std::vector<int> indices;
	size_t current;
	std::mt19937 rng;
};


static uint8_t ComputeVolume(uint8_t emittingVolume, float maxDistance, float distance)
{
	float minDistance;
	uint8_t newEmittingVolume;

	if (AEAudioHardware.m_fEffectMasterScalingFactor <= 0.0f)
		return 0;

	if (distance > maxDistance)
		return 0;

	if (maxDistance <= 0.0f)
		return 0;

	minDistance = maxDistance / 5.0f;

	if (minDistance > distance)
		newEmittingVolume = emittingVolume;
	else {
		float fadeFactor = (maxDistance - minDistance - (distance - minDistance)) / (maxDistance - minDistance);
		newEmittingVolume = emittingVolume * SQR(fadeFactor);
	}

	float finalVolume = newEmittingVolume * AEAudioHardware.m_fEffectMasterScalingFactor;

	return static_cast<uint8_t>(std::min(127.0f, finalVolume));
}


inline float GetDistanceSquared(const CVector& v)
{
	const CVector& c = TheCamera.GetPosition();
	return sq(v.x - c.x) + sq(v.y - c.y) + sq((v.z - c.z) * 0.2f);
}

inline bool IsMatchingName(const char* name, std::initializer_list<const char*> values) {
	for (auto val : values) {
		if (_strcmpi(name, val) == 0)
			return true;
	}
	return false;
}

inline bool NameStartsWithIndexedSuffix(const char* name, const std::string& prefix, int maxIndex = 10) {
	for (int i = 0; i < maxIndex; ++i) {
		std::string full = prefix + std::to_string(i);
		if (_strcmpi(name, full.c_str()) == 0)
			return true;
	}
	return false;
}
#define Clamp2(v, center, radius) ((v) > (center) ? min(v, center + radius) : max(v, center - radius))
static uint32_t ComputeDopplerEffectedFrequency(uint32_t oldFreq, float pos1, float pos2, float speedMultiplier, float timeStep, float speedOfSound) {
	uint32_t newFreq = oldFreq;
	if (!TheCamera.Get_Just_Switched_Status() && speedMultiplier != 0.0f) {
		float dist = pos2 - pos1;
		if (dist != 0.0f) {
			float speedOfSource = (dist / timeStep) * speedMultiplier;
			if (speedOfSound > fabs(speedOfSource)) {
				speedOfSource = Clamp2(speedOfSource, 0.0f, 1.5f);
				newFreq = static_cast<uint32_t>((oldFreq * speedOfSound) / (speedOfSource + speedOfSound));
			}
		}
	}
	return newFreq;
}

inline CVector MultiplyInverse(const CMatrix& mat, const CVector& vec)
{
	CVector v(vec.x - mat.pos.x, vec.y - mat.pos.y, vec.z - mat.pos.z);
	return CVector(
		mat.right.x * v.x + mat.right.y * v.y + mat.right.z * v.z,
		mat.GetForward().x * v.x + mat.GetForward().y * v.y + mat.GetForward().z * v.z,
		mat.up.x * v.x + mat.up.y * v.y + mat.up.z * v.z);
}
static void TranslateEntity(const CVector* in, CVector* out)
{
	*out = MultiplyInverse(*TheCamera.GetMatrix(), *in);
}

static bool AddExplosion(CEntity* victim, CEntity* creator, int32_t explosionType, CVector& posn, unsigned int time, unsigned char makeSound, float camShake, unsigned char visibility) {
	return plugin::CallAndReturn<bool, 0x736A50, CEntity*, CEntity*, int32_t, CVector, unsigned int, unsigned char, float, unsigned char>(victim, creator, explosionType, posn, time, makeSound, camShake, visibility);
}
bool PlayAmbienceSFX(const CVector& origin, eWeaponType weaponType = eWeaponType(0), bool useOldAmbience = false);

inline CVector operator/(const CVector& left, float right)
{
	return CVector(left.x / right, left.y / right, left.z / right);
}

inline CVector
CrossProduct(const CVector& v1, const CVector& v2)
{
	return CVector(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

//extern std::string version, NewVersion;

//static int g_lastExplosionType = -1;
//static int g_lastWeaponType    = -1;
