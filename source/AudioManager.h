#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/alext.h>
#include <vector>
#include <CGeneral.h>
#include <filesystem>
#include <map>
#include "logging.h"
#include "eSurfaceType.h"
using namespace std;
namespace fs = std::filesystem;

#define MAX_SOUND_ALTERNATIVES (10) // weapon related
#define MAX_AMBIENCE_ALTERNATIVES (300)
#define MAX_EXPLOSIONTYPES (12)

inline const std::vector<std::string> extensions = { ".wav", ".mp3", ".flac", ".ogg" }; // all supported audio extensions
enum class EAmbienceTime { Any, Day, Night, Riot };
// thx to iFarbod
enum eChainsawState
{
	AE_WEAPON_CHAINSAW_STATE_IDLE,
	AE_WEAPON_CHAINSAW_STATE_ACTIVE,
	AE_WEAPON_CHAINSAW_STATE_CUTTING,
	AE_WEAPON_CHAINSAW_STATE_STOPPING,
	AE_WEAPON_CHAINSAW_STATE_STOPPED
};

struct Pitch {
	std::optional<float> base;
	std::optional<float> reload;
	std::optional<float> shoot;
	std::optional<float> after;
	std::optional<float> distant;
	std::optional<float> low_ammo;
	std::optional<float> vehicle;
	std::optional<float> quieter;
	std::optional<float> tankcannon;
	std::optional<float> missile;
	std::optional<float> minigunSpin, minigunSpinEnd, minigunShoot;
	std::optional<float> fireExtinguisher, sprayCan;
	std::optional<float> chainsawIdle, chainsawActive, chainsawCutting, chainsawStop;
	std::optional<float> flamethrowerStart, flamethrowerFireLoop, flamethrowerGasLoop;

	// Miscellaneous
	std::optional<float> footstepsPlayer;
	std::optional<float> footstepsNPC;
	std::optional<float> landingPlayer;
	std::optional<float> landingNPC;
	std::optional<float> collapsePlayer;
	std::optional<float> collapseNPC;
	std::optional<float> jacked;
	std::optional<float> fire, nonfire;

	// explosions
	std::optional<float> explosion[MAX_EXPLOSIONTYPES], distexplosion[MAX_EXPLOSIONTYPES], debris[MAX_EXPLOSIONTYPES], underwater[MAX_EXPLOSIONTYPES];

	std::optional<float> ricochet[TOTAL_NUM_SURFACE_TYPES];
};
inline std::unordered_map<std::string, Pitch> gWeaponPitches;
inline Pitch gPitches;
struct Attenuation 
{
	float maxDist;
	float refDist;
	float rolloffFactor;
	float airAbsorption;
};

struct AttenuationSet {
	// weapon related
	Attenuation base;
	Attenuation reload;
	Attenuation low_ammo;
	Attenuation distant;
	Attenuation vehicle;
	Attenuation quieter;
	Attenuation tankcannon;
	Attenuation missile;
	Attenuation minigunSpin, minigunSpinEnd, minigunShoot;
	Attenuation fireExtinguisher, sprayCan;
	Attenuation chainsawIdle, chainsawActive, chainsawCutting, chainsawStop;
	Attenuation flamethrowerStart, flamethrowerFireLoop, flamethrowerGasLoop;

	// Miscellaneous
	Attenuation footstepsPlayer;
	Attenuation footstepsPlayerDuck;
	Attenuation footstepsPlayerSprint;
	Attenuation footstepsPlayerWalk;
	Attenuation footstepsNPC;
	Attenuation footstepsNPCDuck;
	Attenuation footstepsNPCSprint;
	Attenuation footstepsNPCWalk;
	Attenuation landingPlayer;
	Attenuation landingNPC;
	Attenuation collapsePlayer;
	Attenuation collapseNPC;
	Attenuation jacked;
	Attenuation fire, nonfire;

	// explosions
	Attenuation explosion[MAX_EXPLOSIONTYPES], distexplosion[MAX_EXPLOSIONTYPES], debris[MAX_EXPLOSIONTYPES], underwater[MAX_EXPLOSIONTYPES];

	Attenuation ricochet[TOTAL_NUM_SURFACE_TYPES];

	AttenuationSet()
	{
		base.maxDist = FLT_MAX;
		base.refDist = 1.5f;
		base.rolloffFactor = 1.0f;
		base.airAbsorption = 2.0f;

		reload.maxDist = FLT_MAX;
		reload.refDist = 2.3f;
		reload.rolloffFactor = 6.0f;
		reload.airAbsorption = 2.0f;

		vehicle.maxDist = 125.0f;
		vehicle.airAbsorption = 1.5f;
		vehicle.refDist = 1.5f;
		vehicle.rolloffFactor = 0.5f;

		quieter.maxDist = 3000.0f;
		quieter.refDist = 2.0f;
		quieter.rolloffFactor = 3.0f;
		quieter.airAbsorption = 1.0f;

		// per surface
		for (int i = 0; i < TOTAL_NUM_SURFACE_TYPES; i++) {
			ricochet[i].maxDist = 50.0f;
			ricochet[i].refDist = 3.5f;
			ricochet[i].rolloffFactor = 5.0f;
			ricochet[i].airAbsorption = 3.0f;
		}

		// per explosion type
		for (int i = 0; i < MAX_EXPLOSIONTYPES; i++) {
			explosion[i] = Attenuation{ 100.0f, 10.0f, 0.7f, 0.6f };
			debris[i] = Attenuation{ 100.0f, 7.0f, 1.0f, 0.8f };
			distexplosion[i] = Attenuation{ 200.0f, 20.0f, 0.5f, 0.6f };
			underwater[i] = Attenuation{ 4000.0f, 15.0f, 0.20f, 3.0f };
		}
		
		// minigun
		minigunShoot.maxDist = 200.0f;
		minigunShoot.refDist = 2.0f;
		minigunShoot.rolloffFactor = 1.0f;
		minigunShoot.airAbsorption = 1.0f;
		minigunSpin.maxDist = FLT_MAX;
		minigunSpin.refDist = 3.0f;
		minigunSpin.rolloffFactor = 1.5f;
		minigunSpin.airAbsorption = 0.8f;
		minigunSpinEnd.maxDist = FLT_MAX;
		minigunSpinEnd.refDist = 3.0f;
		minigunSpinEnd.rolloffFactor = 1.5f;
		minigunSpinEnd.airAbsorption = 0.8f;

		// fire extinguisher and spray can
		fireExtinguisher.maxDist = 100.0f;
		fireExtinguisher.refDist = 1.0f;
		fireExtinguisher.rolloffFactor = 1.5f;
		fireExtinguisher.airAbsorption = 4.0f;
		sprayCan.maxDist = 100.0f;
		sprayCan.refDist = 1.0f;
		sprayCan.rolloffFactor = 1.5f;
		sprayCan.airAbsorption = 4.0f;

		// chainsaw
		chainsawIdle.maxDist = FLT_MAX;
		chainsawIdle.refDist = 2.0f;
		chainsawIdle.rolloffFactor = 1.5f;
		chainsawIdle.airAbsorption = 1.0f;
		chainsawActive.maxDist = FLT_MAX;
		chainsawActive.refDist = 2.0f;
		chainsawActive.rolloffFactor = 1.5f;
		chainsawActive.airAbsorption = 1.0f;
		chainsawCutting.maxDist = FLT_MAX;
		chainsawCutting.refDist = 2.0f;
		chainsawCutting.rolloffFactor = 1.5f;
		chainsawCutting.airAbsorption = 1.0f;
		chainsawStop.maxDist = FLT_MAX;
		chainsawStop.refDist = 2.0f;
		chainsawStop.rolloffFactor = 1.5f;
		chainsawStop.airAbsorption = 1.0f;

		// flamethrower
		flamethrowerStart.maxDist = 150.0f;
		flamethrowerStart.refDist = 1.0f;
		flamethrowerStart.rolloffFactor = 1.5f;
		flamethrowerStart.airAbsorption = 4.0f;
		flamethrowerFireLoop.maxDist = 200.0f;
		flamethrowerFireLoop.refDist = 1.0f;
		flamethrowerFireLoop.rolloffFactor = 1.5f;
		flamethrowerFireLoop.airAbsorption = 4.0f;
		flamethrowerGasLoop.maxDist = 200.0f;
		flamethrowerGasLoop.refDist = 1.0f;
		flamethrowerGasLoop.rolloffFactor = 1.5f;
		flamethrowerGasLoop.airAbsorption = 4.0f;

		fire.maxDist = 200.0f;
		fire.refDist = 1.0f;
		fire.rolloffFactor = 1.5f;
		fire.airAbsorption = 4.0f;

		nonfire.maxDist = 200.0f;
		nonfire.refDist = 1.0f;
		nonfire.rolloffFactor = 1.5f;
		nonfire.airAbsorption = 4.0f;

		jacked.maxDist = FLT_MAX;
		jacked.refDist = 3.0f;
		jacked.rolloffFactor = 1.5f;
		jacked.airAbsorption = 0.8f;

		low_ammo = Attenuation{ FLT_MAX, 4.5f, 1.0f, 2.0f };
		distant = Attenuation{ 500.0f, 2.0f, 1.0f, 2.0f };

		tankcannon.maxDist = 125.0f;
		tankcannon.refDist = 3.5f;
		tankcannon.rolloffFactor = 0.3f;
		tankcannon.airAbsorption = 0.3f;

		missile.maxDist = 250.0f;
		missile.refDist = 6.0f;
		missile.rolloffFactor = 1.0f;
		missile.airAbsorption = 1.0f;

		footstepsPlayer.maxDist = FLT_MAX;
		footstepsPlayer.refDist = 0.5f;
		footstepsPlayer.rolloffFactor = 1.5f;
		footstepsPlayer.airAbsorption = 1.0f;

		landingPlayer.maxDist = FLT_MAX;
		landingPlayer.refDist = 1.0f;
		landingPlayer.rolloffFactor = 1.5f;
		landingPlayer.airAbsorption = 1.0f;

		collapsePlayer.maxDist = FLT_MAX;
		collapsePlayer.refDist = 1.0f;
		collapsePlayer.rolloffFactor = 1.5f;
		collapsePlayer.airAbsorption = 1.0f;

		footstepsPlayerDuck.maxDist = FLT_MAX;
		footstepsPlayerDuck.refDist = 0.1f;
		footstepsPlayerDuck.rolloffFactor = 1.5f;
		footstepsPlayerDuck.airAbsorption = 1.0f;

		footstepsPlayerSprint.maxDist = FLT_MAX;
		footstepsPlayerSprint.refDist = 1.0f;
		footstepsPlayerSprint.rolloffFactor = 1.5f;
		footstepsPlayerSprint.airAbsorption = 1.0f;

		footstepsPlayerWalk.maxDist = FLT_MAX;
		footstepsPlayerWalk.refDist = 0.3f;
		footstepsPlayerWalk.rolloffFactor = 1.5f;
		footstepsPlayerWalk.airAbsorption = 1.0f;

		footstepsNPC.maxDist = FLT_MAX;
		footstepsNPC.refDist = 0.3f;
		footstepsNPC.rolloffFactor = 2.5f;
		footstepsNPC.airAbsorption = 3.0f;

		landingNPC.maxDist = FLT_MAX;
		landingNPC.refDist = 1.0f;
		landingNPC.rolloffFactor = 2.5f;
		landingNPC.airAbsorption = 3.0f;

		collapseNPC.maxDist = FLT_MAX;
		collapseNPC.refDist = 1.0f;
		collapseNPC.rolloffFactor = 1.5f;
		collapseNPC.airAbsorption = 1.0f;

		footstepsNPCDuck.maxDist = FLT_MAX;
		footstepsNPCDuck.refDist = 0.1f;
		footstepsNPCDuck.rolloffFactor = 2.5f;
		footstepsNPCDuck.airAbsorption = 3.0f;

		footstepsNPCSprint.maxDist = FLT_MAX;
		footstepsNPCSprint.refDist = 0.7f;
		footstepsNPCSprint.rolloffFactor = 2.5f;
		footstepsNPCSprint.airAbsorption = 3.0f;

		footstepsNPCWalk.maxDist = FLT_MAX;
		footstepsNPCWalk.refDist = 0.2f;
		footstepsNPCWalk.rolloffFactor = 2.5f;
		footstepsNPCWalk.airAbsorption = 3.0f;
	}
};

inline AttenuationSet gAttenuationSettings; 
inline std::unordered_map<std::string, AttenuationSet> gWeaponAttenuations;
struct vehInfo
{
	unsigned short model;
	eWeaponType weap;
};
inline std::unordered_map<CPhysical*, vehInfo> gvehInfo;
inline std::unordered_map<CPed*, uint8_t> gentInfo;
struct AudioData {
	std::vector<float> samples;
	unsigned int channels = 0;
	unsigned int sampleRate = 0;
	unsigned int bitsPerSample = 0;
};

// Store audios data
struct SoundInstance
{
	// The source per instance
	ALuint source = 0;
	ALuint buffer = 0;
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
	CVector pos{ 0.0f, 0.0f, 0.0f };
	bool isPossibleGunFire = false;
	ALuint filter = 0;
	ALuint missileSource = 0;
	bool bIsMissile = false;
	// Minigun barrel stuff
	bool spinEndStarted = false;
	bool minigunBarrelSpin = false;
	// Ambience stuff
	bool isAmbience = false;
	EAmbienceTime ambienceTime = EAmbienceTime::Any;
	bool isGunfireAmbience = false;
	bool isManualAmbience = false;
	bool isChainsawSound = false;
	eWeaponType WeaponType;
	CPed* shooter = nullptr;
	float baseGain = 1.0f;
	float pitch = 1.0f;
	bool readPitch = false;
	bool isFire = false;
	FxSystem_c* fireFX = nullptr;
	CFire* firePtr = nullptr;

	SoundInstance() = default;
	~SoundInstance() = default;
};

// RAII wrapper for AL sources
struct ALSourceHandle {
	ALuint id = 0;
	ALSourceHandle() { alGenSources(1, &id); }
	~ALSourceHandle() { if (id && alIsSource(id)) alDeleteSources(1, &id); }
	ALSourceHandle(const ALSourceHandle&) = delete;
	ALSourceHandle& operator=(const ALSourceHandle&) = delete;
	ALSourceHandle(ALSourceHandle&& o) noexcept : id(o.id) { o.id = 0; }
	ALSourceHandle& operator=(ALSourceHandle&& o) noexcept {
		if (&o != this) { if (id && alIsSource(id)) alDeleteSources(1, &id); id = o.id; o.id = 0; }
		return *this;
	}
	explicit operator bool() const { return id && alIsSource(id); }
};

struct SoundInstanceSettings {
	float maxDist = FLT_MAX;
	float gain = 1.0f;
	float airAbsorption = 0.0f;
	float refDist = 1.0f;
	float rollOffFactor = 1.0f;
	float pitch = 1.0f;
	float readPitch = 0.0f;
	bool readPitchFromFile = false;
	CVector pos{ 0.0f, 0.0f, 0.0f };

	bool isFire = false;
	bool isMissile = false;
	bool looping = false;
	bool isAmbience = false;
	bool isGunfire = false;
	bool isChainsawSound = false;

	CFire* firePtr = nullptr;
	int fireEventID = 0;
	CPhysical* entity = nullptr;
	CPed* shooter = nullptr;
	eWeaponType weaponType = WEAPONTYPE_UNARMED;
	bool isPossibleGunFire = false;
	bool isMinigunBarrelSpin = false;
	FxSystem_c* fireFX = nullptr;

	std::optional<fs::path> path;
	std::optional<std::string> name;
};

struct ManualAmbience {
	CVector pos;
	float range;
	bool loop;
	std::vector<ALuint> buffer;
	std::shared_ptr<SoundInstance> loopingInstance;
	EAmbienceTime time;
	uint32_t delay;
	uint32_t nextPlayTime;
	CSphere sphere;
	float maxDist;
	float refDist;
	float rollOff;
	float airAbsorption;
	bool allowOtherAmbiences;
	ALuint source;
	ManualAmbience()
		: pos{ 0.0f,0.0f,0.0f }, range(50.0f), loop(false), allowOtherAmbiences(true), loopingInstance(nullptr), time(EAmbienceTime::Any), delay(3000), nextPlayTime(0),
		maxDist(50.0f), refDist(1.0f), rollOff(1.0f), airAbsorption(1.0f), source(0)
	{
		sphere.Set(range, pos);
	}
};

extern std::vector<ManualAmbience> g_ManualAmbiences;

inline CVector GetRandomAmbiencePosition(const CVector& origin, bool isThunder = false) {
	float angle = (float)(CGeneral::GetRandomNumber() % 628) / 100.0f;
	float distance = (isThunder ? 10.0f : 100.0f) + (CGeneral::GetRandomNumber() % 80);
	return origin + CVector(cosf(angle) * distance, sinf(angle) * distance, isThunder ? 20.0f : 10.0f);
}

inline uint32_t nextZoneAmbienceTime = 0;
inline uint32_t nextFireAmbienceTime = 0;

class CEntryExit;
class CZone;
class CAudioManager
{
private:
	// The device and context
	ALCcontext* pContext;
	ALCdevice* pDevice;
	// FX Slots
	ALuint effectSlot;
	ALuint echoSlot;
	//static ALuint EAXEffectsSlot;
	//static ALuint EAXreverbEffect;
	ALuint reverbEffect;
	ALuint echoEffect;

public:

	static bool efxSupported;
	// WAV buffer storage
	static map<fs::path, ALuint> gBufferMap;
	// Main array to manage ALL currently playing sounds
	static std::vector<std::shared_ptr<SoundInstance>> audiosplaying;
	static const float barrelFadeDuration;

	static ALuint barrelSpinSource;
	static float barrelSpinVolume;
	static std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 5>> m_apChainsawSounds;
	static std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 3>> m_apFlamethrowerSounds;
	static std::unordered_map<CPed*, std::shared_ptr<SoundInstance>> m_apSpraycanSounds;
	static std::unordered_map<CPed*, std::shared_ptr<SoundInstance>> m_apFireextinguisherSounds;
	// array of sounds, 0 - fire, 1 - barrel spin
	static std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 2>> m_apMinigunSound;
	void Initialize();
	void Shutdown();
	// Initialize reverb...
	void InitReverb();
	void InitEcho();
	void AttachReverbToSource(ALuint source, bool detach = false/*, bool EAX = false*/);
	void AttachEchoToSource(ALuint source, bool detach = false/*, bool EAX = false*/);
	bool SetSourceGain(ALuint source, float gain);
	bool SetSourceMaxDist(ALuint source, float maxDist);
	bool SetSourceRefDist(ALuint source, float ref);
	bool SetSourceAirAbsorptionFactor(ALuint source, float air);
	bool SetSourceRolloffFactor(ALuint source, float factor);
	bool SetSourcePitch(ALuint source, float pitch);
	bool PlaySource2D(ALuint buff, bool relative, float volume, float pitch);
	void PauseSource(SoundInstance* inst);
	void ResumeSource(SoundInstance* inst);
	bool StartLoopingAmbience(ManualAmbience& ma);
	void StopLoopingAmbience(ManualAmbience& ma);
	void UnloadManualAmbiences();
	std::shared_ptr<SoundInstance> PlaySource(ALuint buffer, const SoundInstanceSettings& opts);
	AudioData DecodeWAV(const std::string& path);
	AudioData DecodeMP3(const std::string& path);
	AudioData DecodeFLAC(const std::string& path);
	AudioData DecodeOGG(const std::string& path);
	std::string OpenALErrorCodeToString(ALenum error);
	ALuint CreateOpenALBufferFromAudioFile(const fs::path& path);
	void UpdateFireSoundCleanup();
	void AudioPlay(fs::path* audiopath, CPhysical* audioentity);
	// find sound file for specific weapon type or model id
	bool findWeapon(eWeaponType* weapontype, eModelID modelid, std::string filename, CPhysical* audioentity, bool playAudio = true);
	bool PlayAmbienceBuffer(ALuint buffer, const CVector& origin, bool isGunfire = false, bool isThunder = false, bool isManual = false, float manualMaxDist = 250.0f,
		float manualRefDist = 1.0f, float manualRollOff = 1.0f, float manualAirAbsorption = 1.0f, ManualAmbience& ma = ManualAmbience());
	bool PlayOutsideAmbience(
		const CVector& playerPos,
		const CVector& origin,
		eWeaponType weaponType,
		bool isNight,
		bool isRiot,
		bool ambienceStillPlaying,
		CZone* zone);
	bool PlayAmbienceSFX(const CVector& origin, eWeaponType weaponType, bool useOldAmbience);
	void PlayOrStopBarrelSpinSound(CPed* entity, eWeaponType* weapontype, bool spinning, bool playSpinEndSFX = false, bool removeSpinSource = false);
	ALint GetBufferFormat(ALuint buffer);
	ALint GetSourceState(ALuint source);

	ALCcontext* GetContext()
	{
		return pContext;
	}

	ALCdevice* GetDevice()
	{
		return pDevice;
	}
};
extern CAudioManager AudioManager;

// Unmodified
class AudioStream {
public:
	fs::path audiosfolder;

	AudioStream() = default;

	AudioStream(fs::path weaponfolder) {
		audiosfolder = std::move(weaponfolder);
	}

	bool audioPath(std::string filename, fs::path& outPath);

	bool audioPlay(std::string filename, CPhysical* audioEntity);
};

