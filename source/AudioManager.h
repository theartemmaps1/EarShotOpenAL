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
	bool isInteriorAmbience = false;
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
	bool allowOtherAmbiences;
	ManualAmbience()
		: pos{ 0.0f,0.0f,0.0f }, range(50.0f), loop(false), loopingInstance(nullptr), time(EAmbienceTime::Any), delay(0), nextPlayTime(0),
		maxDist(50.0f), refDist(1.0f), rollOff(1.0f)
	{
		sphere.Set(range, pos);
	}
};

extern std::vector<ManualAmbience> g_ManualAmbiences;

inline CVector GetRandomAmbiencePosition(const CVector& origin, bool isThunder = false, bool isInteriorAmbience = false) {
	float angle = (float)(CGeneral::GetRandomNumber() % 628) / 100.0f;
	float distance = (isThunder ? 10.0f : (isInteriorAmbience ? 15.0f : 100.0f)) + (CGeneral::GetRandomNumber() % 80);
	return origin + CVector(cosf(angle) * distance, sinf(angle) * distance, 2.0f);
}

inline static uint32_t nextInteriorAmbienceTime = 0;
inline static uint32_t nextZoneAmbienceTime = 0;
inline static uint32_t nextFireAmbienceTime = 0;

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
	bool PlaySource2D(ALuint buff, bool relative, float volume, float pitch);
	void PauseSource(SoundInstance* inst);
	void ResumeSource(SoundInstance* inst);
	bool StartLoopingAmbience(ManualAmbience& ma);
	void StopLoopingAmbience(ManualAmbience& ma);
	void UnloadManualAmbiences();
	bool PlaySource(ALuint buff,
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
		bool isInteriorAmbience = false, bool isMissile = false, bool looping = false, uint32_t delay = 0, bool isAmbience = false, FxSystem_c* fireFX = nullptr);
	AudioData DecodeWAV(const std::string& path);
	AudioData DecodeMP3(const std::string& path);
	AudioData DecodeFLAC(const std::string& path);
	AudioData DecodeOGG(const std::string& path);
	std::string OpenALErrorCodeToString(ALenum error);
	ALuint CreateOpenALBufferFromAudioFile(const fs::path& path);
	void UpdateFireSoundCleanup();
	void AudioPlay(fs::path* audiopath, CPhysical* audioentity);
	bool findWeapon(eWeaponType* weapontype, eModelID modelid, std::string filename, CPhysical* audioentity, bool playAudio = true);
	bool PlayAmbienceBuffer(ALuint buffer, const CVector& origin, bool isGunfire = false, bool isThunder = false, bool isInteriorAmbience = false, bool isManual = false, float manualMaxDist = 250.0f,
		float manualRefDist = 1.0f, float manualRollOff = 1.0f, ManualAmbience& ma = ManualAmbience());
	bool PlayAmbienceSFX(const CVector& origin, eWeaponType weaponType, bool useOldAmbience);
	void PlayOrStopBarrelSpinSound(CPed* entity, eWeaponType* weapontype, bool spinning, bool playSpinEndSFX = false);
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

