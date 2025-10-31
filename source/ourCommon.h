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
#include <map>
#include "include/subhook-0.8.2/subhook.h"
#include <CExplosion.h>
#include <eAudioEvents.h>
#include "AudioManager.h"
using namespace plugin;
using namespace std;
namespace fs = std::filesystem;
static CdeclEvent <AddressList<0x748E6B, H_CALL>, PRIORITY_BEFORE, ArgPickNone, void()> shutdownGameEvent;
static CdeclEvent <AddressList<0x564372, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> ClearForRestartEvent;
static CdeclEvent <AddressList<0x56A46E, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> ClearExcitingStuffFromAreaEvent;
static CdeclEvent <AddressList<0x56E975, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> MakePlayerSafeEvent;
static CdeclEvent <AddressList<0x738AFF, H_CALL, 0x73997A, H_CALL, 0x739A17, H_CALL, 0x739AD0, H_CALL>, PRIORITY_AFTER, ArgPickN<CEntity*, 0>, void(CEntity* ent)> WorldRemoveProjEvent;
inline uint32_t interiorIntervalMin = 5000;  // ms
inline uint32_t interiorIntervalMax = 10000; // ms
inline uint32_t fireIntervalMin = 5000;
inline uint32_t fireIntervalMax = 10000;
inline uint32_t zoneIntervalMin = 5000;
inline uint32_t zoneIntervalMax = 10000;
inline float distanceForDistantExplosion = 100.0f;
inline float distanceForDistantGunshot = 50.0f;
inline float stereoAmbienceVol = 0.3f;
inline std::unordered_map<CEntity*, int> g_lastExplosionType;

// WIP of my fun idea made out of boredom, define to enable!
//#define QUAKE_KILLSOUNDS_TEST

inline auto modname = string("EarShot");
inline auto modMessage = [&](const string& messagetext, UINT messageflags = MB_OK) {
	if (!Error("%s: %s", modname.c_str(), messagetext.c_str()))
	{
		exit(0);
	}
	return 0;
	};
inline auto modextension = string(".earshot");

// Paths
inline auto folderroot = fs::path(GAME_PATH(""));
inline auto foldermod = fs::path(PLUGIN_PATH("")) / fs::path(modname);
inline auto folderdata = folderroot / fs::path("data") / fs::path(modname);
inline auto rootlength = folderroot.string().length();
inline auto outputPath = [&](fs::path* filepath)
	{
		string s = filepath->string();
		return s.erase(0, rootlength);
	};

// String cases converting
inline auto caseLower = [&](string casedstring)
	{
		string caseless;
		for (char ch : casedstring) {
			if (ch >= 'A' && ch <= 'Z') ch = tolower(ch);
			caseless += ch;
		}
		return caseless;
	};
inline auto caseUpper = [&](string casedstring)
	{
		string caseless;
		for (char ch : casedstring) {
			if (ch >= 'a' && ch <= 'z') ch = toupper(ch);
			caseless += ch;
		}
		return caseless;
	};

inline auto nameType = [&](string* weaponname, eWeaponType* weapontype)
	{
		char weaponchar[255]; sprintf(weaponchar, "%s", caseUpper(*weaponname).c_str());
		*weapontype = CWeaponInfo::FindWeaponType(weaponchar);
		return (*weapontype >= eWeaponType::WEAPONTYPE_UNARMED);
	};

inline int initializationstatus = -2;

typedef void(__thiscall* originalCAEWeaponAudioEntity__WeaponFire)(eWeaponType weaponType, CPhysical* entity, int audioEventId);
typedef void(__thiscall* originalCAEWeaponAudioEntity__WeaponReload)(eWeaponType weaponType, CPhysical* entity, int audioEventId);
typedef void(__thiscall* originalCAEPedAudioEntity__HandlePedHit)(int a2, CPhysical* a3, unsigned __int8 a4, float a5, unsigned int a6);
typedef char(__thiscall* originalCAEPedAudioEntity__HandlePedSwing)(int a2, int a3, int a4);
typedef void(__thiscall* originalCAEExplosionAudioEntity__AddAudioEvent)(int event, CVector* pos, float volume);
typedef char(__thiscall* originalCAEPedAudioEntity__HandlePedJacked)(int AudioEvent);
typedef void(__fastcall* originalCAEFireAudioEntity__AddAudioEvent)(int AudioEvent, CVector* posn);
typedef int(__fastcall* originalCAudioEngine__ReportBulletHit)(CEntity* entity, eSurfaceType surface, const CVector& posn, float angleWithColPointNorm);
typedef void(__thiscall* originalCAEPedAudioEntity__AddAudioEvent)(eAudioEvents event, float volume, float speed, CPhysical* ped, uint8_t surfaceId, int32_t a7, uint32_t maxVol);
typedef bool(__cdecl* originalCExplosion__AddExplosion)(CEntity* victim, CEntity* creator, eExplosionType type, CVector pos, uint32_t lifetime, uint8_t usesSound, float cameraShake, uint8_t bInvisible);
typedef void(__thiscall* originalCAudioEngine__ReportFrontEndAudioEvent)(eAudioEvents eventId, float volumeChange, float speed);
inline auto subhookCAEWeaponAudioEntity__WeaponFire = subhook_t();
inline auto subhookCAEWeaponAudioEntity__WeaponReload = subhook_t();
inline auto subhookCAEPedAudioEntity__HandlePedHit = subhook_t();
inline auto subhookCAEPedAudioEntity__HandlePedSwing = subhook_t();
inline auto subhookCAEExplosionAudioEntity__AddAudioEvent = subhook_t();
inline auto subhookCAEPedAudioEntity__HandlePedJacked = subhook_t();
inline auto subhookCAEFireAudioEntity__AddAudioEvent = subhook_t();
inline auto subhookCAudioEngine__ReportBulletHit = subhook_t();
inline auto subhookCAEPedAudioEntity__AddAudioEvent = subhook_t();
inline auto subhookCExplosion__AddExplosion = subhook_t();
inline auto subhookCAudioEngine__ReportFrontEndAudioEvent = subhook_t();

// Define them all in a structure for a better readability
struct Buffers
{
	std::unordered_map<CFire*, std::shared_ptr<SoundInstance>> fireSounds;
	std::unordered_map<int, std::shared_ptr<SoundInstance>> nonFireSounds;
	std::vector<CAEFireAudioEntity*> ent;
	std::vector<ALuint> explosionBuffers;
	std::vector<ALuint> molotovExplosionBuffers;
	std::vector<ALuint> explosionsDebrisBuffers;
	std::vector<ALuint> explosionDistantBuffers;
	std::vector<ALuint> explosionUnderwaterBuffers;
	std::vector<ALuint> ricochetBuffers;
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
	std::vector<ALuint> AmbienceBuffs;
	// map for weapon-specific ambience sounds by weapon type enum
	std::unordered_map<eWeaponType, ALuint> WeaponTypeAmbienceBuffers;
	std::vector<ALuint> missileSoundBuffers;
	std::vector<ALuint> tankCannonFireBuffers;
	std::vector<ALuint> bulletWhizzLeftRearBuffers;
	std::vector<ALuint> bulletWhizzLeftFrontBuffers;
	std::vector<ALuint> bulletWhizzRightRearBuffers;
	std::vector<ALuint> bulletWhizzRightFrontBuffers;
	std::unordered_map<int, std::vector<ALuint>> ExplosionTypeExplosionBuffers;
	std::unordered_map<int, std::vector<ALuint>> ExplosionTypeDistantBuffers;
	std::unordered_map<int, std::vector<ALuint>> ExplosionTypeDebrisBuffers;
	std::unordered_map<int, std::vector<ALuint>> ExplosionTypeUnderwaterBuffers;
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

inline void DeleteAllBuffers(Buffers& b) {
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

}

#define AUDIOPLAY(MODELID, FILESTEM) AudioManager.findWeapon(&weaponType, eModelID(MODELID), std::string(FILESTEM), entity, true)
#define AUDIOSHOOT(MODELID) AUDIOPLAY(MODELID, "shoot")
#define AUDIOAFTER(MODELID) AUDIOPLAY(MODELID, "after")
#define AUDIODISTANT(MODELID) AUDIOPLAY(MODELID, "distant")
#define AUDIODRYFIRE(MODELID) AUDIOPLAY(MODELID, "dryfire")
#define AUDIOLOWAMMO(MODELID) AUDIOPLAY(MODELID, "low_ammo")
#define AUDIORELOAD(MODELID, RETURNVALUE) AUDIOPLAY(MODELID, "reload", RETURNVALUE)
#define AUDIORELOAD1(MODELID, RETURNVALUE) AUDIOPLAY(MODELID, "reload_one", RETURNVALUE)
#define AUDIORELOAD2(MODELID, RETURNVALUE) AUDIOPLAY(MODELID, "reload_two", RETURNVALUE)
#define AUDIOHIT(MODELID) AUDIOPLAY(MODELID, "hit")
#define AUDIOMARTIALPUNCH(MODELID) AUDIOPLAY(MODELID, "martial_punch")
#define AUDIOMARTIALKICK(MODELID) AUDIOPLAY(MODELID, "martial_kick")
#define AUDIOHITMETALT(MODELID) AUDIOPLAY(MODELID, "hitmetal")
#define AUDIOHITWOOD(MODELID) AUDIOPLAY(MODELID, "hitwood")
#define AUDIOHITGROUND(MODELID) AUDIOPLAY(MODELID, "stomp")
#define AUDIOSWING(MODELID) AUDIOPLAY(MODELID, "swing")
#define AUDIOSPINEND(MODELID) AUDIOPLAY(MODELID, "spin_end")
#define AUDIOSTEALTHCUT1(MODELID) AUDIOPLAY(MODELID, "stealth_firstcut")
#define AUDIOSTEALTHCUT2(MODELID) AUDIOPLAY(MODELID, "stealth_secondcut")
#define AUDIOCALL(AUDIOMACRO) \
    ((entity->m_nType == eEntityType::ENTITY_TYPE_PED && AUDIOMACRO(MODELUNDEFINED)) || AUDIOMACRO(entity->m_nModelIndex))
#define MODELUNDEFINED eModelID(-1)

// Game camera pos
inline CVector cameraposition;

// Surface helpers
class SurfaceInfos_c;
inline SurfaceInfos_c& g_surfaceInfos = *reinterpret_cast<SurfaceInfos_c*>(0xB79538);
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

inline float sq(float x)
{
	return x * x;
}

#define SQR(x) ((x) * (x))


inline bool IsPointWithinSphere(const CSphere& sphere, const CVector& p) {
	return (p - sphere.m_vecCenter).MagnitudeSqr() <= sq(sphere.m_fRadius);
}

#include <random>    // std::mt19937, std::random_device

class RandomIntegers {
public:
	int value;  // store the generated random number

	RandomIntegers(size_t size) {
		if (size == 0) {
			throw std::invalid_argument("Size must be > 0");
		}

		static std::random_device rd;
		static std::mt19937 generator(rd());
		std::uniform_int_distribution<int> distribution(0, static_cast<int>(size) - 1);

		value = distribution(generator);
	}

	int next() const
	{
		return value;
	}
};

inline bool IsMatchingName(const char* name, std::initializer_list<const char*> values) {
	for (auto val : values) {
		if (_strcmpi(name, val) == 0)
			return true;
	}
	return false;
}

inline bool IsMatchingName(const char* name, const char* what) {

	if (_strcmpi(name, what) == 0)
		return true;

	return false;
}

inline bool NameStartsWithIndexedSuffix(const char* name, const std::string& prefix, int maxIndex = MAX_SOUND_ALTERNATIVES) {
	for (int i = 0; i < maxIndex; ++i) {
		std::string full = prefix + std::to_string(i);
		if (_strcmpi(name, full.c_str()) == 0)
			return true;
	}
	return false;
}

inline bool NameEndsWithIndexedSuffix(const std::string& name, const std::string& what, int maxIndex = MAX_AMBIENCE_ALTERNATIVES) {
	for (int i = 0; i < maxIndex; ++i) {
		std::string full = what + std::to_string(i);
		if (name.ends_with(full))
			return true;
	}
	return false;
}

inline bool NameStartsWithIndexedSuffix(const char* name, std::initializer_list<const char*> values, int maxIndex = MAX_SOUND_ALTERNATIVES) {
	for (int i = 0; i < maxIndex; ++i) {
		for (auto val : values) {
			std::string full = val + std::to_string(i);
			if (_strcmpi(name, full.c_str()) == 0)
				return true;
		}
	}
	return false;
}

inline bool AddExplosion(CEntity* victim, CEntity* creator, eExplosionType explosionType, CVector posn, unsigned int time, unsigned char makeSound, float camShake, unsigned char visibility) {
	return plugin::CallAndReturn<bool, 0x736A50, CEntity*, CEntity*, eExplosionType, CVector, unsigned int, unsigned char, float, unsigned char>(victim, creator, explosionType, posn, time, makeSound, camShake, visibility);
}


