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
#include <CAEWeatherAudioEntity.h>
#include <CAudioEngine.h>
#include "CAEPedAudioEntity.h"
using namespace plugin;
namespace fs = std::filesystem;
inline CdeclEvent <AddressList<0x748E6B, H_CALL>, PRIORITY_BEFORE, ArgPickNone, void()> shutdownGameEvent;
inline CdeclEvent <AddressList<0x564372, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> ClearForRestartEvent;
inline CdeclEvent <AddressList<0x56A46E, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> ClearExcitingStuffFromAreaEvent;
inline CdeclEvent <AddressList<0x56E975, H_CALL>, PRIORITY_AFTER, ArgPickNone, void()> MakePlayerSafeEvent;
inline CdeclEvent <AddressList<0x738AFF, H_CALL, 0x73997A, H_CALL, 0x739A17, H_CALL, 0x739AD0, H_CALL>, PRIORITY_AFTER, ArgPickN<CEntity*, 0>, void(CEntity* ent)> WorldRemoveProjEvent;
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

inline std::string trimStr(const std::string& s) {
	size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string::npos) return std::string();
	size_t b = s.find_last_not_of(" \t\r\n");
	return s.substr(a, b - a + 1);
}

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
inline auto caseLower(const std::string& s) {
	std::string out = s;
	std::transform(out.begin(), out.end(), out.begin(),
		[](unsigned char ch) { return std::tolower(ch); });
	return out;
}

inline auto caseUpper(const std::string& s) {
	std::string out = s;
	std::transform(out.begin(), out.end(), out.begin(),
		[](unsigned char ch) { return std::toupper(ch); });
	return out;
}
inline auto vecContains = [](const std::vector<std::string>& v, const std::string& s) -> bool 
{
	return std::find(v.begin(), v.end(), s) != v.end();
};

inline auto nameType = [&](string* weaponname, eWeaponType* weapontype)
	{
		char weaponchar[255]; sprintf(weaponchar, "%s", caseUpper(*weaponname).c_str());
		*weapontype = CWeaponInfo::FindWeaponType(weaponchar);
		return (*weapontype >= eWeaponType::WEAPONTYPE_UNARMED);
	};

typedef void(__thiscall* originalCAEWeaponAudioEntity__WeaponFire)(eWeaponType weaponType, CPhysical* entity, int audioEventId);
typedef void(__thiscall* originalCAEWeaponAudioEntity__WeaponReload)(eWeaponType weaponType, CPhysical* entity, int audioEventId);
typedef void(__thiscall* originalCAEPedAudioEntity__HandlePedHit)(int a2, CPhysical* a3, unsigned __int8 a4, float a5, unsigned int a6);
typedef char(__thiscall* originalCAEPedAudioEntity__HandlePedSwing)(int a2, int a3, int a4);
typedef void(__thiscall* originalCAEExplosionAudioEntity__AddAudioEvent)(int event, CVector* pos, float volume);
typedef char(__thiscall* originalCAEPedAudioEntity__HandlePedJacked)(int AudioEvent);
typedef void(__thiscall* originalCAEFireAudioEntity__AddAudioEvent)(int AudioEvent, CVector* posn);
typedef int(__thiscall* originalCAudioEngine__ReportBulletHit)(CEntity* entity, eSurfaceType surface, const CVector& posn, float angleWithColPointNorm);
typedef void(__thiscall* originalCAEPedAudioEntity__AddAudioEvent)(eAudioEvents event, float volume, float speed, CPhysical* ped, uint8_t surfaceId, int32_t a7, uint32_t maxVol);
typedef bool(__cdecl* originalCExplosion__AddExplosion)(CEntity* victim, CEntity* creator, eExplosionType type, CVector pos, uint32_t lifetime, uint8_t usesSound, float cameraShake, uint8_t bInvisible);
typedef void(__thiscall* originalCAudioEngine__ReportFrontEndAudioEvent)(eAudioEvents eventId, float volumeChange, float speed);
typedef void(__thiscall* originalCAudioEngine__ReportWeaponEvent)(eWeaponType weaponType, CPhysical* physical, eAudioEvents aEvent);
typedef void(__thiscall* originalPlayChainsawStopSound)(CPhysical* entity);
typedef void(__thiscall* originalPlayChainsawEvent)(CPed* ped, int Aevent);
typedef void(__thiscall* originalCAEWeaponAudioEntity__PlayFlameThrowerSounds)(
	CPhysical* entity,
	__int16 sfx1,
	__int16 sfx2,
	int audioEventId,
	float audability,
	float speed);
typedef void(__thiscall* originalCAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop)(CPhysical* entity);
typedef void(__thiscall* originalCAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop)();
typedef CAESound*(__thiscall* originalPlaySoundHook)(CAESound* sound);
typedef void (__thiscall* originalStopSoundAndForget)(CAESound* sound);
typedef LRESULT(__stdcall* originalMainWndProc)(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
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
inline auto subhookCAudioEngine__ReportWeaponEvent = subhook_t();
inline auto subhookPlayChainsawStopSound = subhook_t();
inline auto subhookPlayChainsawEvent = subhook_t();
inline auto subhookCAEWeaponAudioEntity__PlayFlameThrowerSounds = subhook_t();
inline auto subhookCAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop = subhook_t();
inline auto subhookCAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop = subhook_t();
inline auto subhookCAESound__StopSoundAndForget = subhook_t();
inline auto subhookMainWndProc = subhook_t();
#if 0
inline auto subhookPlaySoundHook = subhook_t();
inline void* AESoundManager = (void*)0xB62CB0;
CAESound* __fastcall HookedPlaySound(void* manager, int, CAESound* sound);
#endif
void __fastcall HookedCAEExplosionAudioEntity_AddAudioEvent(
	CAEExplosionAudioEntity* t,
	void* unusedpointer,
	int Aevent,
	CVector* posn,
	float volume
);
int __fastcall HookedCAudioEngine__ReportBulletHit(CAudioEngine* engine, int, CEntity* victim, uint8_t surface, const CVector& posn, float angleWithColPointNorm);
void __fastcall HookedCAEPedAudioEntity__HandlePedHit(CAEPedAudioEntity* thispointer, void* unusedpointer,
	int AudioEvent, CPhysical* victim, uint8_t Surface, float volume, uint32_t maxVolume
);
void __fastcall HookedCAEWeaponAudioEntity__WeaponReload(CAEWeaponAudioEntity* thispointer, void* unusedpointer,
	eWeaponType weaponType, CPhysical* entity, int audioEventId
);
void __fastcall HookedCAEWeaponAudioEntity__WeaponFire(
	CAEWeaponAudioEntity* thispointer, void* unused,
	eWeaponType weaponType, CPhysical* victim, int audioEventId
);
char __fastcall HookedCAEPedAudioEntity__HandlePedSwing(CAEPedAudioEntity* thispointer, void* unusedpointer,
	int a2, int a3, int a4
);
void __fastcall HookedCAEPedAudioEntity__HandlePedHit(CAEPedAudioEntity* thispointer, void* unusedpointer,
	int AudioEvent, CPhysical* victim, uint8_t Surface, float volume, uint32_t maxVolume
);
void __fastcall HookedCAEExplosionAudioEntity_AddAudioEvent(
	CAEExplosionAudioEntity* t,
	void* unusedpointer,
	int Aevent,
	CVector* posn,
	float volume
);
void __fastcall HookedCAEFireAudioEntity__AddAudioEvent(CAEFireAudioEntity* ts, int, int eventId, CVector* posn);
char __fastcall CAEPedAudioEntity__HandlePedJacked(CAEPedAudioEntity* ts, void*, int AudioEvent);
void __fastcall HookedCAEPedAudioEntity__AddAudioEvent(CAEPedAudioEntity* ts, void*, eAudioEvents audioEvent, float volume, float speed, CPhysical* ped, uint8_t surfaceId, int32_t a7, uint32_t maxVol);
void __fastcall HookedCAudioEngine__ReportWeaponEvent(CAudioEngine* engine, void*,
	int32_t audioEvent, eWeaponType weaponType, CPhysical* physical);
void __fastcall HookedCAEWeatherAudioEntity__AddAudioEvent(CAEWeatherAudioEntity* ts, void*, int AudioEvent);
bool __cdecl TriggerTankFireHooked(CEntity* victim, CEntity* creator, eExplosionType type, CVector pos, uint32_t lifetime, uint8_t usesSound, float cameraShake, uint8_t bInvisible);
void __fastcall CAudioEngine__ReportFrontEndAudioHooked(CAudioEngine* eng, int, eAudioEvents eventId, float volumeChange, float speed);
void __fastcall PlayMinigunBarrelStopSound(CAEWeaponAudioEntity* ts, int, CPed* ped);
void __fastcall PlayChainsawStopSound(CAEWeaponAudioEntity* ts, int, CPhysical* entity);
void __fastcall PlayChainsawEvent(CAEWeaponAudioEntity* ts, int, CPed* ped, int Aevent);
void __fastcall CAEWeaponAudioEntity__PlayFlameThrowerSounds(
	CAEWeaponAudioEntity* ts, int,
	CPhysical* entity,
	__int16 sfx1,
	__int16 sfx2,
	int audioEventId,
	float audability,
	float speed);
void __fastcall CAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop(CAEWeaponAudioEntity* ts, int);
void __fastcall CAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop(CAEWeaponAudioEntity* ts, int, CPhysical* entity);
void __fastcall StopFlamethrowerFireSound(CAESound* snd, int);
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
	__int16 currPlayPosn);
void __fastcall CAEWeaponAudioEntity__PlayWeaponLoopSound(
	CAEWeaponAudioEntity* ts, int,
	CPhysical* entity,
	__int16 sfxId,
	int audioEventId,
	float audability,
	float speed,
	unsigned __int32 finalEvent);
void __fastcall StopSpraycanSound(CAESound* snd, int);
void __fastcall StopFireExtinguisherSound(CAESound* snd, int);
void __fastcall StopMinigunSounds(CAESound* snd, int);
void __fastcall StopChainsawSounds(CAESound* snd, int);
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
	float speed2);
void __fastcall CAESound__CalculateVolume(CAESound* snd, int);
void __fastcall CAEPedAudioEntity__HandleLandingEvent(CAEPedAudioEntity* audio, int, int event);
void __fastcall CWeaponAudio__PlayStealthEvent(
	CAEWeaponAudioEntity* ts, int,
	eWeaponType weapType,
	CPed* ped,
	int event);
void __fastcall CAEWeaponAudioEntity__PlayGoggleSound(CAEWeaponAudioEntity* ts, int, __int16 sfxId, int audioEventId);
void __fastcall CAEWeaponAudioEntity__PlayCameraSound(
	CAEWeaponAudioEntity* ts, int,
	CPhysical* entity,
	int audioEventId,
	float audability);
void __fastcall CPed__RemoveGogglesModel(CPed* ts, int);
void __fastcall CAESoundManager__CancelSoundsOwnedByAudioEntity(void* ts, int, CAEAudioEntity* entity, uint8_t a3);
void __fastcall CAESound__StopSirenSound(CAESound* ts, int);
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
	__int16 currPlayPosn);
LRESULT CALLBACK
MainWndProcHOOK(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
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
	std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>> footstepShoeBuffers; // by model name
	std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>> footstepShoeByTextureBuffers; // by texture name
	std::unordered_map<std::string, std::vector<ALuint>> footstepSurfaceBuffers;
	std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>> landingBuffers;
	std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>> landingByShoeTextureBuffers;
	std::unordered_map<std::string, std::vector<ALuint>> landingPerSurfaceBuffers;
	std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>> collapseBuffers;
	std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>> collapseByShoeTextureBuffers;
	std::unordered_map<std::string, std::vector<ALuint>> collapsePerSurfaceBuffers;
	std::unordered_map<std::string, std::vector<ALuint>> gunshellBuffersPerSurface;
	std::unordered_map<std::string, std::vector<ALuint>> shotgunshellBuffersPerSurface;
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
	// minigun buffers: fire, spin, spin_end
	ALuint minigunBuffers[3] = { 0 };
	// chainsaw buffers: idle, active, cutting (flesh), stop
	ALuint chainsawBuffers[4] = { 0 };
	// flamethrower buffers: idlegasloop, firestart, fireloop
	ALuint flamethrowerBuffers[3] = { 0 };
	ALuint sprayCanLoopBuffer = 0, fireExtinguisherLoopBuffer = 0;
	// goggles: on, off
	ALuint cameraShutterBuffer = 0, gogglesBuffer[2] = { 0 };

	// siren buffers (0 - main loop, 1 - idle loop, 2 - reverse beep, 3 - air brakes)
	std::unordered_map<int, std::array<ALuint, 4>> sirenBuffers;
	// flag to mark the model as it has the sound
	std::unordered_map<int, bool> g_VehicleHasSiren;
	// for air brakes
	std::unordered_map<CVehicle*, float> m_fVelocityChangeForAudio;
};

extern Buffers g_Buffers;

inline void SafeDeleteBuffer(ALuint& buf) {
	if (alIsBuffer(buf) && buf != 0) {
		LOG("Freeing buffer %u", buf);
		alDeleteBuffers(1, &buf);
		buf = 0;
	}
}

inline void DeleteBufferVector(std::vector<ALuint>& vec) {
	for (auto& b : vec) SafeDeleteBuffer(b);
	vec.clear();
}

inline void DeleteBufferArray(std::array<ALuint, 4>& array) {
	for (auto& b : array) SafeDeleteBuffer(b);
}

inline void DeleteBufferMapVec(std::unordered_map<std::string, std::vector<ALuint>>& map) {
	for (auto& kv : map) DeleteBufferVector(kv.second);
	map.clear();
}

inline void DeleteBufferMapVec(std::unordered_map<int, std::vector<ALuint>>& map) {
	for (auto& kv : map) DeleteBufferVector(kv.second);
	map.clear();
}

inline void DeleteBufferMapNested(std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ALuint>>>& map) {
	for (auto& kv1 : map) {
		for (auto& kv2 : kv1.second) {
			DeleteBufferVector(kv2.second);
		}
	}
	map.clear();
}

inline void DeleteBufferMapSingle(std::unordered_map<eWeaponType, ALuint>& map) {
	for (auto& kv : map) SafeDeleteBuffer(kv.second);
	map.clear();
}

inline void DeleteBufferMapWithSharedPtr(std::unordered_map<CPed*, std::shared_ptr<SoundInstance>>& map) 
{
	for (auto& kv : map) {
		auto inst = kv.second.get();
		if (inst && inst->buffer != 0) {
			LOG("Freeing shared ptr buffer %u for key '%s'", inst->buffer, inst->name);
			SafeDeleteBuffer(inst->buffer);
		}
	}
	map.clear();
}

inline void DeleteBufferMapWithSharedPtrAndArray(
	std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 3>>& map)
{
	for (auto& kv : map) {
		auto& instArray = kv.second;

		for (size_t i = 0; i < 3; i++) {
			auto& inst = instArray[i];
			if (inst && inst->buffer != 0) {
				LOG(
					"Freeing shared ptr array buffer %u for key '%s' index %zu",
					inst->buffer,
					inst->name,
					i
				);
				SafeDeleteBuffer(inst->buffer);
				inst->buffer = 0;
			}
		}
	}
	map.clear();
}

inline void DeleteBufferMapWithSharedPtrAndArray(
	std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 5>>& map)
{
	for (auto& kv : map) {
		auto& instArray = kv.second;

		for (size_t i = 0; i < 4; i++) {
			auto& inst = instArray[i];
			if (inst && inst->buffer != 0) {
				LOG(
					"Freeing shared ptr array buffer %u for key '%s' index %zu",
					inst->buffer,
					inst->name,
					i
				);
				SafeDeleteBuffer(inst->buffer);
				inst->buffer = 0;
			}
		}
	}
	map.clear();
}

inline void DeleteBufferMapWithSharedPtrAndArray(
	std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 2>>& map)
{
	for (auto& kv : map) {
		auto& instArray = kv.second;

		for (size_t i = 0; i < 2; i++) {
			auto& inst = instArray[i];
			if (inst && inst->buffer != 0) {
				LOG(
					"Freeing shared ptr array buffer %u for key '%s' index %zu",
					inst->buffer,
					inst->name,
					i
				);
				SafeDeleteBuffer(inst->buffer);
				inst->buffer = 0;
			}
		}
	}
	map.clear();
}


inline void DeleteAllBuffers(Buffers& b) {
	DeleteBufferVector(b.explosionBuffers);
	DeleteBufferVector(b.molotovExplosionBuffers);
	DeleteBufferVector(b.explosionsDebrisBuffers);
	DeleteBufferVector(b.explosionDistantBuffers);
	DeleteBufferVector(b.ricochetBuffers);

	DeleteBufferMapVec(b.ricochetBuffersPerMaterial);
	DeleteBufferMapNested(b.landingBuffers);
	DeleteBufferMapVec(b.landingPerSurfaceBuffers);
	DeleteBufferMapNested(b.collapseBuffers);
	DeleteBufferMapVec(b.collapsePerSurfaceBuffers);
	DeleteBufferMapVec(b.gunshellBuffersPerSurface);
	DeleteBufferMapVec(b.shotgunshellBuffersPerSurface);

	DeleteBufferMapNested(b.footstepShoeBuffers);
	DeleteBufferMapVec(b.footstepSurfaceBuffers);
	DeleteBufferMapNested(b.landingByShoeTextureBuffers);
	DeleteBufferMapNested(b.collapseByShoeTextureBuffers);
	DeleteBufferMapNested(b.footstepShoeByTextureBuffers);

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
	DeleteBufferMapVec(b.ExplosionTypeUnderwaterBuffers);
	// remove siren buffers and clear siren flag map
	for (auto& kv : b.sirenBuffers) {
		DeleteBufferArray(kv.second);
		b.g_VehicleHasSiren[kv.first] = false;
	}

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
	for (int i = 0; i < 3; i++) {
		if (g_Buffers.minigunBuffers[i] != 0) {
			LOG("Freeing minigun buffer %d: %u", i, g_Buffers.minigunBuffers[i]);
			SafeDeleteBuffer(g_Buffers.minigunBuffers[i]);
		}
	}

	for (int i = 0; i < 4; i++) {
		if (g_Buffers.chainsawBuffers[i] != 0) {
			LOG("Freeing chainsaw buffer %d: %u", i, g_Buffers.chainsawBuffers[i]);
			SafeDeleteBuffer(g_Buffers.chainsawBuffers[i]);
		}
	}

	for (int i = 0; i < 3; i++) {
		if (g_Buffers.flamethrowerBuffers[i] != 0) {
			LOG("Freeing flamethrower buffer %d: %u", i, g_Buffers.flamethrowerBuffers[i]);
			SafeDeleteBuffer(g_Buffers.flamethrowerBuffers[i]);
		}
	}

	if (g_Buffers.sprayCanLoopBuffer != 0) {
		LOG("Freeing spray can loop buffer: %u", g_Buffers.sprayCanLoopBuffer);
		SafeDeleteBuffer(g_Buffers.sprayCanLoopBuffer);
	}

	if (g_Buffers.fireExtinguisherLoopBuffer != 0) {
		LOG("Freeing fire extinguisher loop buffer: %u", g_Buffers.fireExtinguisherLoopBuffer);
		SafeDeleteBuffer(g_Buffers.fireExtinguisherLoopBuffer);
	}
	if (g_Buffers.cameraShutterBuffer != 0) {
		LOG("Freeing camera shutter buffer: %u", g_Buffers.cameraShutterBuffer);
		SafeDeleteBuffer(g_Buffers.cameraShutterBuffer);
	}
	for (int i = 0; i < 2; i++) {
		if (g_Buffers.gogglesBuffer[i] != 0) {
			LOG("Freeing goggles buffers %d: %u", i, g_Buffers.gogglesBuffer[i]);
			SafeDeleteBuffer(g_Buffers.gogglesBuffer[i]);
		}
	}

	g_Buffers.nonFireSounds.clear();
	DeleteBufferMapWithSharedPtrAndArray(AudioManager.m_apChainsawSounds);
	DeleteBufferMapWithSharedPtrAndArray(AudioManager.m_apFlamethrowerSounds);
	DeleteBufferMapWithSharedPtr(AudioManager.m_apSpraycanSounds);
	DeleteBufferMapWithSharedPtr(AudioManager.m_apFireextinguisherSounds);
	DeleteBufferMapWithSharedPtrAndArray(AudioManager.m_apMinigunSound);

	// and clear ent vector if it holds CAEFireAudioEntity* references
	g_Buffers.ent.clear();
}

// PlayChainsawEvent
inline auto PlayStop = [&](CPhysical* entity, bool playSound = true, bool clearSources = false, bool all = true, int i = 0)
	{
		if (entity)
		{
			// Play one-shot stop sound
			if (playSound) {
				SoundInstanceSettings opts{};
				opts.maxDist = gAttenuationSettings.chainsawStop.maxDist; // 100.0f
				opts.refDist = gAttenuationSettings.chainsawStop.refDist; // 1.0f
				opts.airAbsorption = gAttenuationSettings.chainsawStop.airAbsorption; // 2.0f
				opts.rollOffFactor = gAttenuationSettings.chainsawStop.rolloffFactor; // 2.0f
				opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
				opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
				opts.readPitchFromFile = gPitches.chainsawStop.has_value();
				opts.readPitch = *gPitches.chainsawStop;
				LOG("chainsawStop pitch is %.2f", *gPitches.chainsawStop);
				opts.pos = entity->GetPosition();
				opts.isChainsawSound = true;
				opts.shooter = (CPed*)entity;
				AudioManager.PlaySource(g_Buffers.chainsawBuffers[3], opts);
			}
		}

		// Stop all looping sounds
		if (clearSources) {
			if (!all) {
				for (auto& pair : AudioManager.m_apChainsawSounds)
				{
					auto& inst = pair.second[i];
					CPed* ped = pair.first;
					if (inst)
					{
						alSourceStop(inst->source);
						alDeleteSources(1, &inst->source);
						inst->source = 0;
						AudioManager.m_apChainsawSounds[ped][i] = nullptr;
					}
				}
			}
			else {
				for (auto& pair : AudioManager.m_apChainsawSounds)
				{
					for (int j = 0; j < 4; j++) {
						{
							auto& inst = pair.second[j];
							CPed* ped = pair.first;
							// clear active source only
							if (inst && inst->source != 0 && AudioManager.GetSourceState(inst->source) == AL_PLAYING)
							{
								alSourceStop(inst->source);
								alDeleteSources(1, &inst->source);
								inst->source = 0;
								AudioManager.m_apChainsawSounds[ped][i] = nullptr;
							}
						}
					}
				}
			}
		}
	};

#define AUDIOPLAY(MODELID, FILESTEM) AudioManager.findWeapon(&weaponType, eModelID(MODELID), std::string(FILESTEM), entity, true)
#define AUDIOSHOOT(MODELID) AUDIOPLAY(MODELID, "shoot")
#define AUDIOAFTER(MODELID) AUDIOPLAY(MODELID, "after")
#define AUDIODISTANT(MODELID) AUDIOPLAY(MODELID, "distant")
#define AUDIODRYFIRE(MODELID) AUDIOPLAY(MODELID, "dryfire")
#define AUDIOLOWAMMO(MODELID) AUDIOPLAY(MODELID, "low_ammo")
#define AUDIOGUNSHELL(MODELID) AUDIOPLAY(MODELID, "gunshell")
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
#define AUDIOCHAINSAWSTOP(MODELID) AUDIOPLAY(MODELID, "stop")
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

// Custom simple class to generate random integers with less chance of having the same number in subsequent calls
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

inline const uint32_t GetExportedFunction(const char* szExportName, const char* moduleName)
{
	return reinterpret_cast<const uint32_t>(GetProcAddress(GetModuleHandleA(moduleName), szExportName));
}

// FLA:
// @returns weapon parent type
inline int __cdecl GetWeaponHighestParentType(int weaponType)
{
	auto Function = reinterpret_cast<int(__cdecl*)(int)>(GetExportedFunction("GetWeaponHighestParentType", "$fastman92limitAdjuster.asi"));
	return Function(weaponType);
}

inline bool IsShotgunType(eWeaponType type) {
	switch (type) {
	case WEAPONTYPE_SHOTGUN:
	case WEAPONTYPE_SPAS12:
	case WEAPONTYPE_SAWNOFF:
		return true;
	default:
		int typeCustom = GetWeaponHighestParentType(type);
		if (typeCustom == WEAPONTYPE_SHOTGUN || typeCustom == WEAPONTYPE_SPAS12 || typeCustom == WEAPONTYPE_SAWNOFF) {
			return true;
		}
		else {
			return false;
		}
	}
}


