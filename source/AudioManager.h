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
using namespace std;
namespace fs = std::filesystem;
inline std::unordered_map<std::string, std::optional<float>> s_pitchCache;

inline const std::vector<std::string> extensions = { ".wav", ".mp3", ".flac", ".ogg" }; // all supported audio extensions

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
	bool isManualAmbience = false;
	eWeaponType WeaponType;
	CPed* shooter = nullptr;
	ALenum spinningstatus;
	float baseGain = 1.0f;
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
	CVector pos{ 0.0f, 0.0f, 0.0f };

	bool isFire = false;
	bool isMissile = false;
	bool looping = false;
	bool isAmbience = false;
	bool isGunfire = false;

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
	uint32_t delayMs = 0;
};


enum class EAmbienceTime { Any, Day, Night, Riot };

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
inline std::unordered_map<CPed*, uint32_t> playStartTime;


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
	//static ALuint EAXEffectsSlot;
	//static ALuint EAXreverbEffect;
	ALuint reverbEffect;

public:

	static bool efxSupported;
	static map<string, ALuint> gBufferMap;
	// Main array to manage ALL currently playing sounds
	static std::vector<std::shared_ptr<SoundInstance>> audiosplaying;
	// WAV buffer storage

	void Initialize();
	void Shutdown();
	// Initialize reverb...
	void InitReverb();
	void AttachReverbToSource(ALuint source, bool detach = false/*, bool EAX = false*/);
	bool SetSourceGain(ALuint source, float gain);
	bool SetSourceMaxDist(ALuint source, float maxDist);
	bool SetSourceRefDist(ALuint source, float ref);
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
	void PlayOrStopBarrelSpinSound(CPed* entity, eWeaponType* weapontype, bool spinning, bool playSpinEndSFX = false);
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


inline const float barrelFadeDuration = 0.5f;

inline ALuint barrelSpinBuffer = 0;
inline ALuint barrelSpinSource = 0;
inline float barrelSpinVolume = 0.0f;

