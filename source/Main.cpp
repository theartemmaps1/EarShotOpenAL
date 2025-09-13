#include "ourCommon.h"
#include "plugin.h"
#include <filesystem>
#include <map>
#include <unordered_map>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <AL/alext.h>

#include <CAudioEngine.h>
#include <eSurfaceType.h>

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

// useles ans not neded
//#include <libsndfile/include/sndfile.h>
using namespace plugin;
using namespace std;
DebugMenuAPI gDebugMenuAPI;
std::unordered_map<CEntity*, int> g_lastExplosionType;
namespace fs = std::filesystem;
Buffers g_Buffers;

//std::string version = "1.0b, testing build";
//std::string NewVersion = "1.0r, release build";

// Global vars for reverb
static ALuint effectSlot = 0;
//static ALuint EAXEffectsSlot = 0;
//static ALuint EAXreverbEffect = 0;
static ALuint reverbEffect = 0;

// Is FX supported?
static bool efxSupported = false;

//bool EAXOrNot = false;

// The device and context
static ALCdevice* device = nullptr;
static ALCcontext* context = nullptr;

// Game camera pos
CVector cameraposition;

// WAV buffer storage
static map<string, ALuint> gBufferMap;

auto CalculateVolume = [&](const CVector& pos, float min = 150.0f, float powyx = 6.0f) -> float {
	//float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	float audiovolume = AEAudioHardware.m_fEffectMasterScalingFactor /* fader*/; // 2.5f

	auto audiodistance = CVector(
		cameraposition.x - pos.x,
		cameraposition.y - pos.y,
		cameraposition.z - pos.z
	).Magnitude();
	audiodistance = clamp<float>(audiodistance, 0.0f, min);
	auto audiofactor = (min - audiodistance) / min;
	audiofactor = pow(audiofactor, powyx);
	audiovolume *= audiofactor;

	return audiovolume;
	};

// Initialize reverb...
void InitReverb() {
	if (!device || !context || !efxSupported) return;

	alGenAuxiliaryEffectSlots(1, &effectSlot);

	alGenEffects(1, &reverbEffect);
	alEffecti(reverbEffect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

	alEffectf(reverbEffect, AL_REVERB_DENSITY, 0.2f);                // Lower density
	alEffectf(reverbEffect, AL_REVERB_DECAY_TIME, 0.6f);             // Short decay
	alEffectf(reverbEffect, AL_REVERB_GAIN, 0.3f);                   // Overall reverb level
	alEffectf(reverbEffect, AL_REVERB_GAINHF, 0.3f);                 // High-frequency damping
	alEffectf(reverbEffect, AL_REVERB_DECAY_HFRATIO, 0.3f);          // More HF rolloff
	alEffectf(reverbEffect, AL_REVERB_REFLECTIONS_GAIN, 0.4f);       // Initial reflection strength
	alEffectf(reverbEffect, AL_REVERB_LATE_REVERB_GAIN, 0.3f);       // Late reverb strength
	alEffectf(reverbEffect, AL_REVERB_LATE_REVERB_DELAY, 0.005f);    // Shorter late reverb delay
	alEffectf(reverbEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, 0.3f);    // How quickly it fades with distance

	alAuxiliaryEffectSloti(effectSlot, AL_EFFECTSLOT_EFFECT, reverbEffect);
}

// Seems to not be supported at all?
/*void InitEAXReverb() {
	if (!device || !context || !efxSupported) return;

	alGenAuxiliaryEffectSlots(1, &EAXEffectsSlot);
	alGenEffects(1, &EAXreverbEffect);

	ALboolean eaxReverbSupported;
	alGetBooleanv(AL_EFFECT_EAXREVERB, &eaxReverbSupported);
	if (!eaxReverbSupported) {
		Error("EAX Reverb not supported. Falling back or skipping.\n");
		return;
	}

	alEffecti(EAXreverbEffect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

	alEffectf(EAXreverbEffect, AL_EAXREVERB_DENSITY, 1.2f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_DECAY_TIME, 1.6f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_GAIN, 1.3f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_GAINHF, 1.3f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_DECAY_HFRATIO, 0.3f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_REFLECTIONS_GAIN, 2.4f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_LATE_REVERB_GAIN, 0.3f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_LATE_REVERB_DELAY, 0.005f);
	alEffectf(EAXreverbEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, 0.3f);

	alAuxiliaryEffectSloti(EAXEffectsSlot, AL_EFFECTSLOT_EFFECT, EAXreverbEffect);
}*/


static void AttachReverbToSource(ALuint source, bool detach = false/*, bool EAX = false*/) {
	if (!efxSupported) return;

	if (!detach) {
		// Attach the reverb
		alSource3i(source, AL_AUXILIARY_SEND_FILTER, /*EAX ? EAXEffectsSlot : */effectSlot, 0, AL_FILTER_NULL);
	}
	else {
		// Detach the reverb by setting the effect slot to 0
		alSource3i(source, AL_AUXILIARY_SEND_FILTER, 0, 0, AL_FILTER_NULL);
	}
}


// Not sure where to even use the echo, but the reverb is used in general
static ALuint echoEffect = 0;
static ALuint echoSlot = 0;

void InitEcho()
{
	if (!efxSupported) return;

	alGenAuxiliaryEffectSlots(1, &echoSlot);
	alGenEffects(1, &echoEffect);
	alEffecti(echoEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);

	alEffectf(echoEffect, AL_ECHO_DELAY, 0.3f);
	alEffectf(echoEffect, AL_ECHO_LRDELAY, 0.2f);
	alEffectf(echoEffect, AL_ECHO_DAMPING, 0.5f);
	alEffectf(echoEffect, AL_ECHO_FEEDBACK, 0.5f);
	alEffectf(echoEffect, AL_ECHO_SPREAD, -1.0f);

	alAuxiliaryEffectSloti(echoSlot, AL_EFFECTSLOT_EFFECT, echoEffect);
}

static void AttachEchoToSource(ALuint source) {
	if (!efxSupported) return;
	alSource3i(source, AL_AUXILIARY_SEND_FILTER, echoSlot, 0, AL_FILTER_NULL);
}
auto modname = string("EarShot");
auto modMessage = [&](const string& messagetext, UINT messageflags = MB_OK) {
	//MessageBox(GetActiveWindow(), messagetext.c_str(), modname.c_str(), messageflags);
	if (!Error("%s: %s", modname.c_str(), messagetext.c_str()))
	{
		_exit(0);
	}
	return 0;
	};
auto modextension = string(".earshot");

// Paths
auto folderroot = fs::path(GAME_PATH(""));
auto foldermod = fs::path(PLUGIN_PATH("")) / fs::path(modname);
auto folderdata = folderroot / fs::path("data") / fs::path(modname);
auto rootlength = folderroot.string().length();
auto outputPath = [&](fs::path* filepath)
	{
		string s = filepath->string();
		return s.erase(0, rootlength);
	};

// String cases converting
auto caseLower = [&](string casedstring)
	{
		string caseless;
		for (char ch : casedstring) {
			if (ch >= 'A' && ch <= 'Z') ch = tolower(ch);
			caseless += ch;
		}
		return caseless;
	};
auto caseUpper = [&](string casedstring)
	{
		string caseless;
		for (char ch : casedstring) {
			if (ch >= 'a' && ch <= 'z') ch = toupper(ch);
			caseless += ch;
		}
		return caseless;
	};

struct WAVData
{
	ALenum format;
	ALsizei freq;
	std::vector<char> data;
};
// We can use this to read WAV files, no external libraries needed
WAVData loadWAV(const char* filename) {

	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		if (!Error("Failed to open WAV file: %s", filename)) {
			exit(0);
		}
	}

	char chunkId[4];
	file.read(chunkId, 4);
	if (std::string(chunkId, 4) != "RIFF") {
		Error("Not a valid WAV file: %s", filename);
	}

	file.seekg(4, std::ios::cur); // skip chunk size

	char format[4];
	file.read(format, 4);
	if (std::string(format, 4) != "WAVE") {
		Error("Not a valid WAVE file: %s", filename);
	}

	char subchunk1Id[4];
	file.read(subchunk1Id, 4);
	if (std::string(subchunk1Id, 4) != "fmt ") {
		Error("Invalid fmt chunk in WAV: %s", filename);
	}

	uint32_t subchunk1Size = 0;
	file.read(reinterpret_cast<char*>(&subchunk1Size), 4);

	uint16_t audioFormat = 0;
	uint16_t numChannels = 0;
	uint32_t sampleRate = 0;
	uint32_t byteRate = 0;
	uint16_t blockAlign = 0;
	uint16_t bitsPerSample = 0;

	file.read(reinterpret_cast<char*>(&audioFormat), 2);
	file.read(reinterpret_cast<char*>(&numChannels), 2);
	file.read(reinterpret_cast<char*>(&sampleRate), 4);
	file.read(reinterpret_cast<char*>(&byteRate), 4);
	file.read(reinterpret_cast<char*>(&blockAlign), 2);
	file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

	/*if (audioFormat != 1) {
		Error("Only PCM WAV files supported: %s", filename);
	}*/

	if (subchunk1Size > 16) {
		file.seekg(subchunk1Size - 16, std::ios::cur);
	}

	char subchunk2Id[4];
	uint32_t subchunk2Size = 0;
	while (true) {
		file.read(subchunk2Id, 4);
		file.read(reinterpret_cast<char*>(&subchunk2Size), 4);
		if (std::string(subchunk2Id, 4) == "data") break;
		file.seekg(subchunk2Size, std::ios::cur);
		if (file.eof()) {
			Error("Data chunk not found in file: %s", filename);
		}
	}

	std::vector<char> data(subchunk2Size);
	file.read(data.data(), subchunk2Size);

	ALenum formatEnum = 0;
	if (numChannels == 1) {
		formatEnum = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
		Log("Loaded MONO %d-bit WAV: %s", bitsPerSample, filename);
	}
	else if (numChannels == 2) {
		formatEnum = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
		Log("Loaded STEREO %d-bit WAV (NO 3D SPATIALIZATION!): %s", bitsPerSample, filename);
	}
	else {
		if (!Error("Only mono or stereo WAV supported: %s", filename)) {
			exit(0);
		}
	}

	WAVData wav = { formatEnum, static_cast<ALsizei>(sampleRate), std::move(data) };
	file.close(); // close it when we're done with it
	return wav;
}

ALuint createOpenALBufferFromWAV(const char* filename) {
	std::string fn{ filename };

	// If we already loaded this buffer, return it to prevent overflow
	auto it = gBufferMap.find(fn);
	if (it != gBufferMap.end()) {
		if (alIsBuffer(it->second)) {
			Log("Buffer already loaded for '%s', returning cached buffer.", filename);
			return it->second;
		}
		else {
			Log("Found invalid buffer for '%s', erasing cache entry.", filename);
			gBufferMap.erase(it);
		}
	}
	WAVData data = loadWAV(fn.c_str());

	ALuint buff = 0;
	alGenBuffers(1, &buff);
	alBufferData(buff, data.format, data.data.data(), (ALsizei)data.data.size(), data.freq);
	gBufferMap.emplace(fn, buff);
	return buff;
}
auto initializationstatus = int(-2);
struct WeapInfos
{
	eWeaponType weapType;
	std::string weapName;
};

std::vector<WeapInfos> weaponNames;
static void AudioPlay(fs::path* audiopath, CPhysical* audioentity, bool looped = false, int framesToPlay = -1)
{
	//if (!audioentity || !fs::exists(*audiopath)) return;
	float fallBackPitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float pitch = fallBackPitch;
	std::string stem = audiopath->stem().string();
	bool isShoot = (stem == "shoot" || NameStartsWithIndexedSuffix(stem.c_str(), "shoot"));
	bool isAfter = (stem == "after" || NameStartsWithIndexedSuffix(stem.c_str(), "after"));
	bool isDistant = (stem == "distant" || NameStartsWithIndexedSuffix(stem.c_str(), "distant"));
	bool isLowAmmo = (stem == "low_ammo" || NameStartsWithIndexedSuffix(stem.c_str(), "low_ammo"));
	if (isShoot || isAfter || isDistant) {
		bool foundPitch = false;

		for (auto& info : weaponNames)
		{
			fs::path earshotPath = audiopath->parent_path() / info.weapName;
			earshotPath.replace_extension(modextension);

			//Log("Attempting to read earshot file: %s", earshotPath.string().c_str());

			if (fs::exists(earshotPath)) {
				std::ifstream pitchFile(earshotPath);
				if (pitchFile.is_open())
				{
					std::string line;
					while (std::getline(pitchFile, line)) {
						Log("Read line: '%s'", line.c_str());
						if (line.rfind("pitch=", 0) == 0) {
							try {
								if (fallBackPitch >= 1.0f) {
									pitch = std::stof(line.substr(6));
									Log("Pitch parsed: %.2f", pitch);
									foundPitch = true;
								}
							}
							catch (...) {
								pitch = fallBackPitch;
								Log("Exception parsing pitch, using fallback");
							}
							break;
						}
					}
				}
			}

			if (foundPitch) break;
		}
	}


	CVector pos = audioentity->GetPosition();
	bool veh = false;
	if (audioentity && audioentity->m_nType == ENTITY_TYPE_VEHICLE) {
		Log("AudioPlay: Vehicle");
		veh = true;
	}

	float dx = cameraposition.x - pos.x;
	float dy = cameraposition.y - pos.y;
	float dz = cameraposition.z - pos.z;
	float dist = sqrt(dx * dx + dy * dy + dz * dz);


	ALuint buffer = createOpenALBufferFromWAV(audiopath->string().c_str());
	if (buffer == 0) {
		modMessage("Could not play " + outputPath(audiopath));
		return;
	}

	CPed* ped = (CPed*)audioentity;
	std::shared_ptr<SoundInstance> inst;
	// Sound occlusion attempt
	/*if (inst->isPossibleGunFire) {
		static ALuint muffledFilter = 0;
		static ALuint clearFilter = 0;

		// Lazy initialization of shared filters
		if (muffledFilter == 0) {
			alGenFilters(1, &muffledFilter);
			alFilteri(muffledFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
			alFilterf(muffledFilter, AL_LOWPASS_GAIN, 0.85f);     // reduced gain
			alFilterf(muffledFilter, AL_LOWPASS_GAINHF, 0.05f);   // strong HF cutoff
		}

		if (clearFilter == 0) {
			alGenFilters(1, &clearFilter);
			alFilteri(clearFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
			alFilterf(clearFilter, AL_LOWPASS_GAIN, 1.0f);
			alFilterf(clearFilter, AL_LOWPASS_GAINHF, 1.0f);
		}

		// Check if the sound is occluded (LOS blocked)
		bool occluded = !CWorld::GetIsLineOfSightClear(cameraposition, inst->pos, true, false, false, false, false, true, false);

		float distance = (cameraposition - inst->pos).Magnitude();
		float attenuation = 1.0f;

		if (occluded) {
			float clamped = Clamp(distance, 1.0f, 50.0f);
			float gainHF = 0.05f;
			float gain = 1.0f - (1.0f / clamped);         // e.g. 0.0–0.9 attenuation

			ALuint dynamicFilter;
			alGenFilters(1, &dynamicFilter);
			alFilteri(dynamicFilter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
			alFilterf(dynamicFilter, AL_LOWPASS_GAIN, gain);
			alFilterf(dynamicFilter, AL_LOWPASS_GAINHF, gainHF);
			alSourcei(inst->source, AL_DIRECT_FILTER, dynamicFilter);

			inst->filter = dynamicFilter;
		}
		else {
			// No occlusion
			alSourcei(inst->source, AL_DIRECT_FILTER, clearFilter);
			inst->filter = clearFilter;
		}
	}*/
	// for vehicle guns we make the sound a bit louder by changing it's distance attenuation
	playBuffer(buffer, veh ? 125.0f : 100.0f,
		AEAudioHardware.m_fEffectMasterScalingFactor, veh ? 1.5f : 2.5f,
		veh ? 1.5f : 3.0f, veh ? 0.5f : 1.0f,
		pitch, pos, false, nullptr, 0, &inst, audioentity,
		true, audiopath, audiopath->stem().string(), false,
		nullptr, WEAPONTYPE_UNARMED, false, false, false, false, 40);
	bool NeedsToBeQuieter = false;
	bool Reloads = false;
	bool Distant = false;
	bool After = false;

	// Some sounds need to be quieter or louder
	Reloads = IsMatchingName(inst->nameBuffer.c_str(), { "reload", "reload_one", "reload_two" });
	Distant = _strcmpi(inst->nameBuffer.c_str(), "distant") == 0 || NameStartsWithIndexedSuffix(inst->name, "distant");
	After = _strcmpi(inst->name, "after") == 0 || NameStartsWithIndexedSuffix(inst->name, "after");
	NeedsToBeQuieter = IsMatchingName(inst->name, {
		"hit", "swing", "stomp", "martial_kick", "martial_punch",
		"reload", "reload_one", "reload_two"
		});

	// Check indexed variants for quieter sounds
	if (!NeedsToBeQuieter) {
		static const std::string prefixes[] = {
			"swing", "hit", "stomp", "martial_kick", "martial_punch"
		};
		for (const auto& prefix : prefixes) {
			if (NameStartsWithIndexedSuffix(inst->name, prefix)) {
				NeedsToBeQuieter = true;
				break;
			}
		}
	}

	// Volume & distance handling
	if (Reloads) {
		setSourceGain(inst->source, CalculateVolume(pos, 75.0f, 5.0f));
		//	alSourcef(inst->source, AL_GAIN, CalculateVolume(pos, 75.0f, 5.0f));
		Log("AudioPlay: applied reload distance settings");
	}
	else if (NeedsToBeQuieter) {
		setSourceGain(inst->source, CalculateVolume(pos));
		//	alSourcef(inst->source, AL_GAIN, CalculateVolume(pos));
		Log("AudioPlay: detected sounds that need to be quieter");
	}
	else if (Distant) {
		setSourceGain(inst->source, CalculateVolume(pos, 350.0f, 3.0f));
		//	alSourcef(inst->source, AL_GAIN, CalculateVolume(pos, 350.0f, 3.0f));
		Log("AudioPlay: enhanced distant sound settings");
	}

	if (After) {
		setSourceGain(inst->source, CalculateVolume(pos, 150.0f, 3.5f));
		//	alSourcef(inst->source, AL_GAIN, CalculateVolume(pos, 150.0f, 3.5f));
		Log("AudioPlay: applied 'after' distance setting");
	}

	// With each lower ammo in the clip, the sound get's louder
	if (isLowAmmo && ped && ped->GetWeapon()) {
		int ammoInClip = ped->GetWeapon()->m_nAmmoInClip;
		unsigned short ammoClip = CWeaponInfo::GetWeaponInfo(ped->GetWeapon()->m_eWeaponType, WEAPSKILL_STD)->m_nAmmoClip;
		int left = (ammoClip / 3);
		ammoInClip = std::max(1, std::min(ammoInClip, left));

		float gainMultiplier = 0.1f + (float(left) - ammoInClip) * 0.1f;

		float finalGain = AEAudioHardware.m_fEffectMasterScalingFactor * gainMultiplier;
		setSourceGain(inst->source, finalGain);
		//alSourcef(inst->source, AL_GAIN, finalGain);
	}
}

// Unmodified
#define MODELUNDEFINED eModelID(-1)
class AudioStream {
public:
	fs::path audiosfolder;

	AudioStream() = default;

	AudioStream(fs::path weaponfolder) {
		audiosfolder = std::move(weaponfolder);
	}

	bool audioPath(const std::string& filename, fs::path& outPath) const {
		fs::path candidate = audiosfolder / fs::path(filename).replace_extension(".wav");
		if (fs::exists(candidate)) {
			auto inst = std::make_shared<SoundInstance>();
			inst->path = candidate;
			outPath = std::move(candidate);
			audiosplaying.push_back(std::move(inst));
			return true;
		}
		return false;
	}

	bool audioPlay(std::string* filename, CPhysical* audioEntity) {
		std::vector<fs::path> alternatives;
		const std::string& baseName = *filename;
		CPed* ped = (CPed*)audioEntity;
		// Alternatives: sound0.wav, sound1.wav, ...
		for (int i = 0; i < 10; ++i) {
			std::string altName = baseName + std::to_string(i);
			fs::path altPath;
			if (audioPath(altName, altPath) /*&& fs::exists(altPath)*/) {
				alternatives.push_back(std::move(altPath));
				Log("Alternative audio found: %s", alternatives.back().string().c_str());
			}
			else {
				break; // nothing found, exit the loop
			}
		}

		// Fallback to the base file if no alternatives were found
		if (alternatives.empty()) {
			fs::path fallbackPath;
			if (audioPath(baseName, fallbackPath)/* && fs::exists(fallbackPath)*/) {
				auto inst = std::make_shared<SoundInstance>();
				inst->path = fallbackPath;
				alternatives.push_back(std::move(fallbackPath));
				audiosplaying.push_back(std::move(inst));
				Log("Fallback audio found: %s", alternatives.back().string().c_str());
			}
		}

		// Nothing found
		if (alternatives.empty()) {
			Log("Audiofile '%s' wasn't found at path %s", baseName.c_str(), audiosfolder.string().c_str());
			if (audioEntity && audioEntity->m_nType == ENTITY_TYPE_PED) {
				eWeaponType type = ped->m_aWeapons[ped->m_nSelectedWepSlot].m_eWeaponType;
				CWeaponInfo* weapInfo = CWeaponInfo::GetWeaponInfo(type, WEAPSKILL_STD);
				if (weapInfo) {
					unsigned int anim = weapInfo->m_nAnimToPlay;
					if ((type == WEAPONTYPE_MINIGUN || (anim == ANIM_GROUP_FLAME && weapInfo->m_nWeaponFire == WEAPON_FIRE_INSTANT_HIT)) && _strcmpi(baseName.c_str(), "spin_end") == 0)
					{
						Log("Fallback spin_end");
						if (audioEntity && audioEntity->m_nType == ENTITY_TYPE_PED) {
							CPed* ped = reinterpret_cast<CPed*>(audioEntity);
							ped->m_weaponAudio.PlayMiniGunStopSound(ped);
						}
					}
				}
			}
			return false;
		}

		// Play one random alternative sound
		RandomUnique rnd(alternatives.size());

		int index = rnd.next();
		fs::path selected = alternatives[index];
		Log("Playing non-loop audio: %s", selected.string().c_str());
		AudioPlay(&selected, audioEntity, false);

		return true;
	}
};

void LoadRicochetSounds(const fs::path& folder) {
	const fs::path ricoDir = folder / "generic/ricochet";
	if (!fs::exists(ricoDir))
	{
		Log("LoadRicochetSounds: ricochet folder wasn't found, '%s'", ricoDir.string().c_str());
		return;
	}
	const std::vector<std::string> surfaces = {
		"default", "metal", "wood", "water", "dirt", "glass", "stone", "sand", "flesh"
	};

	for (const auto& surface : surfaces) {
		std::vector<ALuint>& buffers = g_Buffers.ricochetBuffersPerMaterial[surface];

		int index = 0;
		while (true) {
			fs::path file = folder / ("generic/ricochet/" + surface + "/ricochet" + std::to_string(index) + ".wav");
			if (!fs::exists(file))
			{
				Log("LoadRicochetSounds: %s wasn't found at %s", file.filename().string().c_str(), file.parent_path().string().c_str());
				break;
			}

			ALuint buffer = createOpenALBufferFromWAV(file.string().c_str());
			if (buffer != 0)
				buffers.push_back(buffer);

			++index;
		}

		fs::path fileNoIndex = folder / ("generic/ricochet/" + surface + "/ricochet" + ".wav");
		if (!buffers.empty()) {
			Log("Loaded %d ricochet sound(s) for surface: %s", (int)buffers.size(), surface.c_str());
		}
		else
		{
			if (fs::exists(fileNoIndex)) {
				ALuint buffer = createOpenALBufferFromWAV(fileNoIndex.string().c_str());

				if (buffer != 0)
					buffers.push_back(buffer);

				Log("Loaded ricochet sound for surface: %s", surface.c_str());
			}
		}
	}
}

void LoadFootstepSounds(const fs::path& baseFolder) {
	const fs::path footstepsDir = baseFolder / "generic/footsteps";
	if (!fs::exists(footstepsDir))
	{
		Log("LoadFootstepSounds: footsteps folder wasn't found, '%s'", footstepsDir.string().c_str());
		return;
	}
	// Detect shoe folders (subfolders of footsteps/)
	for (const auto& entry : fs::directory_iterator(footstepsDir)) {
		if (!entry.is_directory()) continue;

		const std::string shoeType = entry.path().filename().string();

		// Check if this is a surface-only folder
		if (fs::exists(entry.path() / "step0.wav") || fs::exists(entry.path() / "step.wav")) {
			// This is a surface-only folder
			std::vector<ALuint>& buffers = g_Buffers.footstepSurfaceBuffers[shoeType];
			int index = 0;
			while (true) {
				fs::path stepFile = entry.path() / ("step" + std::to_string(index) + ".wav");
				if (!fs::exists(stepFile))
				{
					Log("LoadFootstepSounds: %s wasn't found at %s", stepFile.filename().string().c_str(), stepFile.parent_path().string().c_str());
					break;
				}

				ALuint buffer = createOpenALBufferFromWAV(stepFile.string().c_str());
				if (buffer != 0) buffers.push_back(buffer);
				++index;
			}

			// fallback to a single file
			fs::path fallback = entry.path() / "step.wav";
			if (buffers.empty() && fs::exists(fallback)) {
				ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
				if (buffer != 0) buffers.push_back(buffer);
			}

			if (!buffers.empty()) {
				Log("Loaded %d general footstep sounds for surface '%s'", (int)buffers.size(), shoeType.c_str());
			}
		}
		else {
			// It is a shoe folder
			for (const auto& surfaceEntry : fs::directory_iterator(entry.path())) {
				if (!surfaceEntry.is_directory()) continue;

				const std::string surfaceType = surfaceEntry.path().filename().string();
				std::vector<ALuint>& buffers = g_Buffers.footstepShoeBuffers[shoeType][surfaceType];

				int index = 0;
				while (true) {
					fs::path stepFile = surfaceEntry.path() / ("step" + std::to_string(index) + ".wav");
					if (!fs::exists(stepFile)) break;

					ALuint buffer = createOpenALBufferFromWAV(stepFile.string().c_str());
					if (buffer != 0) buffers.push_back(buffer);
					++index;
				}

				// fallback to a single file
				fs::path fallback = surfaceEntry.path() / "step.wav";
				if (buffers.empty() && fs::exists(fallback)) {
					ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
					if (buffer != 0) buffers.push_back(buffer);
				}

				if (!buffers.empty()) {
					Log("Loaded %d footstep sounds for shoe '%s' on surface '%s'", (int)buffers.size(), shoeType.c_str(), surfaceType.c_str());
				}
			}
		}
	}
}

void LoadExplosionRelatedSounds(const fs::path& folder) {
	// No point in continuing, the main folder doesn't even exist
	const fs::path expDir = folder / "generic/explosions";
	if (!fs::exists(expDir))
	{
		Log("LoadExplosionRelatedSounds: explosions folder wasn't found, '%s'", expDir.string().c_str());
		return;
	}
	// Clear previous per-type buffers
	//g_Buffers.WeaponTypeExplosionBuffers.clear();
	//g_Buffers.WeaponTypeDebrisBuffers.clear();
	//g_Buffers.WeaponTypeDistantBuffers.clear();
	//g_Buffers.WeaponTypeMolotovBuffers.clear();

	g_Buffers.ExplosionTypeExplosionBuffers.clear();
	g_Buffers.ExplosionTypeDebrisBuffers.clear();
	g_Buffers.ExplosionTypeDistantBuffers.clear();
	//g_Buffers.ExplosionTypeMolotovBuffers.clear();

	auto loadBuffers = [](const fs::path& path, std::vector<ALuint>& vec, const std::string& prefix) {
		int idx = 0;
		bool indexedLoaded = false;
		while (true) {
			fs::path file = path / (prefix + std::to_string(idx) + ".wav");
			if (!fs::exists(file)) {
				break;
			}
			else {
				indexedLoaded = true;
				ALuint buf = createOpenALBufferFromWAV(file.string().c_str());
				if (buf != 0) vec.push_back(buf);
				++idx;
			}
		}
		if (!indexedLoaded)
		{
			fs::path file = path / (prefix + ".wav");
			if (fs::exists(file))
			{
				ALuint buf = createOpenALBufferFromWAV(file.string().c_str());
				if (buf != 0) vec.push_back(buf);
			}
		}
		};

	// --- load per-explosion-type folders ---
	fs::path explosionTypesDir = folder / "generic/explosions/explosionTypes";
	if (fs::exists(explosionTypesDir) && fs::is_directory(explosionTypesDir)) {
		for (auto& entry : fs::directory_iterator(explosionTypesDir)) {
			if (!fs::is_directory(entry.path())) continue;
			std::string name = entry.path().filename().string();
			// try parse integer folder name -> typeID
			int typeID = -1;
			try { typeID = std::stoi(name); }
			catch (...) { continue; }
			// load explosions (explosion*.wav or explosion.wav)
			auto& mainVec = g_Buffers.ExplosionTypeExplosionBuffers[typeID];
			loadBuffers(entry.path(), mainVec, "explosion");

			auto& debrisVec = g_Buffers.ExplosionTypeDebrisBuffers[typeID];
			loadBuffers(entry.path(), debrisVec, "debris");

			auto& distantVec = g_Buffers.ExplosionTypeDistantBuffers[typeID];
			loadBuffers(entry.path(), distantVec, "distant");

			//auto& molotovVec = g_Buffers.ExplosionTypeMolotovBuffers[typeID];
			//loadBuffers(entry.path(), molotovVec, "molotov");

			Log("Loaded explosionType '%d' sounds from %s", typeID, entry.path().string().c_str());
		}
	}

	// --- load per-weapon-type folders ---
	/*fs::path weaponTypesDir = folder / "generic/explosions/weaponTypes";
	if (fs::exists(weaponTypesDir) && fs::is_directory(weaponTypesDir)) {
		for (auto& entry : fs::directory_iterator(weaponTypesDir)) {
			if (!fs::is_directory(entry.path())) continue;
			std::string name = entry.path().filename().string();
			int weaponID = -1;
			try { weaponID = std::stoi(name); }
			catch (...) { continue; }
			auto& mainVec = g_Buffers.WeaponTypeExplosionBuffers[weaponID];
			loadBuffers(entry.path(), mainVec, "explosion");

			auto& debrisVec = g_Buffers.WeaponTypeDebrisBuffers[weaponID];
			loadBuffers(entry.path(), debrisVec, "debris");

			auto& distantVec = g_Buffers.WeaponTypeDistantBuffers[weaponID];
			loadBuffers(entry.path(), distantVec, "distant");

			//auto& molotovVec = g_Buffers.WeaponTypeMolotovBuffers[weaponID];
			//loadBuffers(entry.path(), molotovVec, "molotov");

			Log("Loaded weaponType '%d' explosion sounds from %s", weaponID, entry.path().string().c_str());
		}
	}*/


	// Load generic explosions as fallback
	int idx = 0;
	bool anyIndexedLoaded = false;
	while (true) {
		bool loadedSomething = false;

		fs::path explosionFile = folder / ("generic/explosions/explosion" + std::to_string(idx) + ".wav");
		if (fs::exists(explosionFile)) {
			g_Buffers.explosionBuffers.push_back(createOpenALBufferFromWAV(explosionFile.string().c_str()));
			loadedSomething = true;
			anyIndexedLoaded = true;
		}

		fs::path debrisFile = folder / ("generic/explosions/debris" + std::to_string(idx) + ".wav");
		if (fs::exists(debrisFile)) {
			g_Buffers.explosionsDebrisBuffers.push_back(createOpenALBufferFromWAV(debrisFile.string().c_str()));
			loadedSomething = true;
			anyIndexedLoaded = true;
		}

		fs::path distantFile = folder / ("generic/explosions/distant" + std::to_string(idx) + ".wav");
		if (fs::exists(distantFile)) {
			g_Buffers.explosionDistantBuffers.push_back(createOpenALBufferFromWAV(distantFile.string().c_str()));
			loadedSomething = true;
			anyIndexedLoaded = true;
		}

		if (!loadedSomething) break;
		++idx;
	}

	// fallback: try non-indexed if nothing was loaded with indexes
	if (!anyIndexedLoaded) {
		fs::path explosionFile = folder / "generic/explosions/explosion.wav";
		if (fs::exists(explosionFile))
			g_Buffers.explosionBuffers.push_back(createOpenALBufferFromWAV(explosionFile.string().c_str()));

		fs::path debrisFile = folder / "generic/explosions/debris.wav";
		if (fs::exists(debrisFile))
			g_Buffers.explosionsDebrisBuffers.push_back(createOpenALBufferFromWAV(debrisFile.string().c_str()));

		fs::path distantFile = folder / "generic/explosions/distant.wav";
		if (fs::exists(distantFile))
			g_Buffers.explosionDistantBuffers.push_back(createOpenALBufferFromWAV(distantFile.string().c_str()));
	}

}


void LoadFireSounds(const fs::path& folder) {
	const fs::path fireDir = folder / "generic/fire";
	if (!fs::exists(fireDir))
	{
		Log("LoadFireSounds: fire folder wasn't found, '%s'", fireDir.string().c_str());
		return;
	}
	int index = 0;
	while (true) {
		bool loadedSomething = false;

		fs::path pathSmall = folder / ("generic/fire/fire_smallloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathSmall)) {
			ALuint buffer = createOpenALBufferFromWAV(pathSmall.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersSmall.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathMedium = folder / ("generic/fire/fire_mediumloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathMedium)) {
			ALuint buffer = createOpenALBufferFromWAV(pathMedium.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersMedium.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathLarge = folder / ("generic/fire/fire_largeloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathLarge)) {
			ALuint buffer = createOpenALBufferFromWAV(pathLarge.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersLarge.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathCar = folder / ("generic/fire/fire_carloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathCar)) {
			ALuint buffer = createOpenALBufferFromWAV(pathCar.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersCar.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathBike = folder / ("generic/fire/fire_bikeloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathBike)) {
			ALuint buffer = createOpenALBufferFromWAV(pathBike.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersBike.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathFlame = folder / ("generic/fire/fire_flameloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathFlame)) {
			ALuint buffer = createOpenALBufferFromWAV(pathFlame.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersFlame.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathMolotov = folder / ("generic/fire/fire_molotovloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathMolotov)) {
			ALuint buffer = createOpenALBufferFromWAV(pathMolotov.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersMolotov.push_back(buffer);
				loadedSomething = true;
			}
		}

		if (!loadedSomething)
			break;

		++index;
	}

	// Fallbacks
	fs::path fallback;

	if (g_Buffers.fireLoopBuffersSmall.empty()) {
		fallback = folder / "generic/fire/fire_smallloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersSmall.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersMedium.empty()) {
		fallback = folder / "generic/fire/fire_mediumloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersMedium.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersLarge.empty()) {
		fallback = folder / "generic/fire/fire_largeloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersLarge.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersCar.empty()) {
		fallback = folder / "generic/fire/fire_carloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersCar.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersBike.empty()) {
		fallback = folder / "generic/fire/fire_bikeloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersBike.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersFlame.empty()) {
		fallback = folder / "generic/fire/fire_flameloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersFlame.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersMolotov.empty()) {
		fallback = folder / "generic/fire/fire_molotovloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersMolotov.push_back(buffer);
		}
	}
}

// Copy paste of the previous func
void LoadJackingRelatedSounds(const fs::path& folder) {
	const fs::path jackDir = folder / "generic/jacked";
	if (!fs::exists(jackDir))
	{
		Log("LoadJackingRelatedSounds: jacking folder wasn't found, '%s'", jackDir.string().c_str());
		return;
	}
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		fs::path pathCar = folder / ("generic/jacked/jack_car" + std::to_string(index) + ".wav");
		if (fs::exists(pathCar)) {
			ALuint buffer = createOpenALBufferFromWAV(pathCar.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathHead = folder / ("generic/jacked/jack_carheadbang" + std::to_string(index) + ".wav");
		if (fs::exists(pathHead)) {
			ALuint buffer = createOpenALBufferFromWAV(pathHead.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackHeadBangBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathKick = folder / ("generic/jacked/jack_carkick" + std::to_string(index) + ".wav");
		if (fs::exists(pathKick)) {
			ALuint buffer = createOpenALBufferFromWAV(pathKick.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackKickBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathBike = folder / ("generic/jacked/jack_bike" + std::to_string(index) + ".wav");
		if (fs::exists(pathBike)) {
			ALuint buffer = createOpenALBufferFromWAV(pathBike.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackBikeBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathDozer = folder / ("generic/jacked/jack_bulldozer" + std::to_string(index) + ".wav");
		if (fs::exists(pathDozer)) {
			ALuint buffer = createOpenALBufferFromWAV(pathDozer.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackBulldozerBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		if (!loadedSomething)
			break;

		++index;
	}

	// Fallback
	fs::path fallback;

	if (g_Buffers.carJackBuff.empty()) {
		fallback = folder / "generic/jacked/jack_car.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackHeadBangBuff.empty()) {
		fallback = folder / "generic/jacked/jack_carheadbang.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackHeadBangBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackKickBuff.empty()) {
		fallback = folder / "generic/jacked/jack_carkick.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackKickBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackBikeBuff.empty()) {
		fallback = folder / "generic/jacked/jack_bike.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackBikeBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackBulldozerBuff.empty()) {
		fallback = folder / "generic/jacked/jack_bulldozer.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackBulldozerBuff.push_back(buffer);
		}
	}
}


auto nameType = [&](string* weaponname, eWeaponType* weapontype)
	{
		char weaponchar[255]; sprintf(weaponchar, "%s", caseUpper(*weaponname).c_str());
		*weapontype = CWeaponInfo::FindWeaponType(weaponchar);
		return (*weapontype >= eWeaponType::WEAPONTYPE_UNARMED);
	};

auto registeredweapons = map<pair<eWeaponType, eModelID>, AudioStream>();
auto registerWeapon = [&](std::filesystem::path& filepath) {
	std::string weaponname = filepath.stem().string();
	if (weaponname.empty()) {
		Log("Failed to register weapon sound for file %s", filepath.string().c_str());
		return;
	}

	eModelID modelid = MODELUNDEFINED;

	// this code below responds for the vehicle guns
	size_t sep = weaponname.find(' ');
	if (sep != std::string::npos) {
		std::string modelname = weaponname.substr(sep + 1);
		// Query model info — use int intermediate to avoid casting enum pointer
		int modelfound = -1;
		CBaseModelInfo* info = CModelInfo::GetModelInfo(modelname.c_str(), &modelfound);
		if (info && modelfound > static_cast<int>(MODELUNDEFINED)) {
			modelid = static_cast<eModelID>(modelfound);
		}
		else {
			Log("Skipped weapon '%s': model '%s' was not found", weaponname.c_str(), modelname.c_str());
			return; // do not register if model missing
		}
		// keep only weapon part
		weaponname = weaponname.substr(0, sep);
	}

	// Resolve weapon type from name
	eWeaponType weapontype = WEAPONTYPE_UNARMED; // default
	if (!nameType(&weaponname, &weapontype)) {
		Log("Skipped weapon '%s': unknown weapon name", weaponname.c_str());
		return;
	}

	// avoid accidental overwrite
	auto key = std::make_pair(weapontype, modelid);
	if (registeredweapons.find(key) != registeredweapons.end()) {
		Log("Weapon already registered type=%d model=%d -> skipping", (int)weapontype, (int)modelid);
		return;
	}

	// register: construct AudioStream from parent path
	registeredweapons.emplace(key, AudioStream(filepath.parent_path()));

	// store safe copy of name
	weaponNames.emplace_back(weapontype, weaponname);

	Log("Registered weapon type=%d model=%d path=%s",
		(int)weapontype, (int)modelid, outputPath(&filepath).c_str());
	};

auto findWeapon(eWeaponType* weapontype, eModelID modelid, const std::string& filename, CPhysical* audioentity, bool playAudio = true)
{
	auto key = std::make_pair(*weapontype, modelid);
	auto it = registeredweapons.find(key);
	fs::path path;
	if (it == registeredweapons.end())
		return false;
	// if we don't want to play any audio, just check for file's existence instead
	if (!playAudio)
		return it->second.audioPath(filename, path);

	return it->second.audioPlay((std::string*)&filename, audioentity);
}

/*auto registerExplosionFolder = [&](fs::path* folderpath)
{
	AudioStream stream(*folderpath);
	g_Buffers.registeredExplosions.push_back(stream);
	g_Buffers.registeredExplosionsDebris.push_back(stream);

	Log("Explosion sound registered %s", outputPath(folderpath).c_str());
};*/
static void LoadMainWeaponsFolder() {
	Log("File(s):");
	for (auto& directoryentry : fs::recursive_directory_iterator(foldermod)) {
		auto entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			auto filename = entrypath.stem().string();
			//if (caseLower(filename).starts_with("explosion")) {
			//	auto s = entrypath.parent_path();
			//	registerExplosionFolder(&s);
			//}
			auto fileextension = caseLower(entrypath.extension().string());
			if (fileextension == modextension) registerWeapon(entrypath);
		}
	}
	// This is kinda bullshit, it doesn't need this dll at all, it works fine in modloader :lmao:
	/*auto autoid3000mlmodule = GetModuleHandle("AutoID3000ML.dll");
	if (autoid3000mlmodule) {
		auto getfiles = GetProcAddress(autoid3000mlmodule, "getFiles");
		if (getfiles) {
			Log("File(s) (Mod Loader):");
			typedef const char** (_cdecl* autoid3000mlfiles)(const char*, int*);
			auto AutoID3000MLFiles = (autoid3000mlfiles)getfiles;
			auto mlamount = int();
			auto mlfiles = AutoID3000MLFiles(modextension.c_str(), &mlamount);
			--mlamount;
			for (auto mlindex = int(0); mlindex < mlamount; ++mlindex) {
				auto s = (folderroot / fs::path(mlfiles[mlindex]));
				registerWeapon(&s);
			}
		}
	}
	else {
		Log("Mod Loader support requires AutoID3000.");
	}*/
	auto registeredtotal = registeredweapons.size();
	if (registeredtotal == 0) {
		initializationstatus = -1;
		Log("No file(s) found. Stopping work for this session.");
		return;
	}
	Log("Total registered weapons: %d", registeredtotal);
}

typedef void(__thiscall* originalCAEWeaponAudioEntity__WeaponFire)(eWeaponType weaponType, CPhysical* entity, int audioEventId);
typedef void(__thiscall* originalCAEWeaponAudioEntity__WeaponReload)(eWeaponType weaponType, CPhysical* entity, int audioEventId);
typedef void(__thiscall* originalCAEPedAudioEntity__HandlePedHit)(int a2, CPhysical* a3, unsigned __int8 a4, float a5, unsigned int a6);
typedef char(__thiscall* originalCAEPedAudioEntity__HandlePedSwing)(int a2, int a3, int a4);
typedef void(__thiscall* originalCAEExplosionAudioEntity__AddAudioEvent)(int event, CVector* pos, float volume);
typedef char(__thiscall* originalCAEPedAudioEntity__HandlePedJacked)(int AudioEvent);
typedef void(__thiscall* originalCAEFireAudioEntity__AddAudioEvent)(int AudioEvent, CVector* posn);
typedef int(__fastcall* originalCAudioEngine__ReportBulletHit)(CEntity* entity, eSurfaceType surface, const CVector& posn, float angleWithColPointNorm);
typedef void(__thiscall* originalCAEPedAudioEntity__AddAudioEvent)(eAudioEvents event, float volume, float speed, CPhysical* ped, uint8_t surfaceId, int32_t a7, uint32_t maxVol);
typedef bool(__cdecl* originalCExplosion__AddExplosion)(CEntity* victim, CEntity* creator, eExplosionType type, CVector pos, uint32_t lifetime, uint8_t usesSound, float cameraShake, uint8_t bInvisible);
typedef void(__thiscall* originalCAudioEngine__ReportFrontEndAudioEvent)(eAudioEvents eventId, float volumeChange, float speed);
auto subhookCAEWeaponAudioEntity__WeaponFire = subhook_t();
auto subhookCAEWeaponAudioEntity__WeaponReload = subhook_t();
auto subhookCAEPedAudioEntity__HandlePedHit = subhook_t();
auto subhookCAEPedAudioEntity__HandlePedSwing = subhook_t();
auto subhookCAEExplosionAudioEntity__AddAudioEvent = subhook_t();
auto subhookCAEPedAudioEntity__HandlePedJacked = subhook_t();
auto subhookCAEFireAudioEntity__AddAudioEvent = subhook_t();
auto subhookCAudioEngine__ReportBulletHit = subhook_t();
auto subhookCAEPedAudioEntity__AddAudioEvent = subhook_t();
auto subhookCExplosion__AddExplosion = subhook_t();
auto subhookCAudioEngine__ReportFrontEndAudioEvent = subhook_t();
#define AUDIOPLAY2(MODELID, FILESTEM, RETURNVALUE) if (findWeapon(&weaponType, eModelID(MODELID), FILESTEM, entity, true)) RETURNVALUE;
#define AUDIOPLAY(MODELID, FILESTEM) findWeapon(&weaponType, eModelID(MODELID), FILESTEM, entity, true)
#define AUDIOSHOOT(MODELID) AUDIOPLAY(MODELID, "shoot")
#define AUDIOAFTER(MODELID) AUDIOPLAY(MODELID, "after")
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
#define AUDIOCALL(AUDIOMACRO) \
    ((entity->m_nType == eEntityType::ENTITY_TYPE_PED && AUDIOMACRO(MODELUNDEFINED)) || AUDIOMACRO(entity->m_nModelIndex))
const float barrelFadeDuration = 0.5f;

static bool barrelSpinLoaded = false;
static ALuint barrelSpinBuffer = 0;
static ALuint barrelSpinSource = 0;
static float barrelSpinVolume = 0.0f;

void PlayOrStopBarrelSpinSound(CPed* entity, eWeaponType* weapontype, bool spinning, bool playSpinEndSFX = false) {
	//if (!shooter || !ent) return;

	//CPed* ped = reinterpret_cast<CPed*>(shooter);
	CPlayerPed* playa = reinterpret_cast<CPlayerPed*>(entity);
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	if (!barrelSpinLoaded) {
		auto weapontype = eWeaponType();

		for (auto& directoryentry : fs::recursive_directory_iterator(foldermod)) {
			const auto& entrypath = directoryentry.path();
			if (!fs::is_directory(entrypath)) {
				std::string filename = entrypath.stem().string();
				if (nameType(&filename, &weapontype)) {
					fs::path spinPath = entrypath.parent_path() / "spin.wav";
					if (fs::exists(spinPath)) {
						barrelSpinBuffer = createOpenALBufferFromWAV(spinPath.string().c_str());
						barrelSpinLoaded = (barrelSpinBuffer != 0);
						break;
					}
				}
			}
		}
		if (!barrelSpinLoaded) return;
	}


	float deltaTime = CTimer::ms_fTimeStep;
	//	CPed* entity = ped;
	auto weaponType = entity->m_aWeapons[entity->m_nSelectedWepSlot].m_eWeaponType;
	std::shared_ptr<SoundInstance> instance;
	if (spinning) {
		// Start playing if not already
		if (barrelSpinSource == 0) {
			if (playBuffer(barrelSpinBuffer, FLT_MAX, AEAudioHardware.m_fEffectMasterScalingFactor, 1.0f, 1.0f, 1.0f, pitch, entity->GetPosition(), false, nullptr, 0, &instance, entity,
				false, nullptr, "", true, entity, weaponType, false, false, false, true))
			{
				barrelSpinSource = instance->source;
			}
			barrelSpinVolume = 0.0f;
		}

		// Fade in
		if (barrelSpinVolume < 1.0f) {
			barrelSpinVolume += deltaTime / barrelFadeDuration;
			if (barrelSpinVolume > 1.0f) barrelSpinVolume = 1.0f;
			setSourceGain(barrelSpinSource, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
			//alSourcef(barrelSpinSource, AL_GAIN, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
		}
	}
	else {
		// Fade out
		if (barrelSpinVolume > 0.0f) {
			barrelSpinVolume -= deltaTime / barrelFadeDuration;
			if (barrelSpinVolume < 0.0f) barrelSpinVolume = 0.0f;
			setSourceGain(barrelSpinSource, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
			//alSourcef(barrelSpinSource, AL_GAIN, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
		}

		// Once volume is faded out, stop source and play SPINEND
		if (barrelSpinVolume < 0.1f && barrelSpinSource != 0) {
			if (playSpinEndSFX) {
				if (AUDIOCALL(AUDIOSPINEND)) {
					alSourceStop(barrelSpinSource);
					alDeleteSources(1, &barrelSpinSource);
					barrelSpinSource = 0;
				}
			}
			else {
				alSourceStop(barrelSpinSource);
				alDeleteSources(1, &barrelSpinSource);
				barrelSpinSource = 0;
			}
		}
	}
}
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
	/*if (thispointer->m_pPed) {
		weap = thispointer->m_pPed->GetWeapon();
		info = CWeaponInfo::GetWeaponInfo(weap->m_eWeaponType, WEAPSKILL_STD);
		if (thispointer->m_pPed && weap && info && (weap->IsTypeProjectile() || info->m_nWeaponFire == WEAPON_FIRE_PROJECTILE))
		{
			g_lastWeaponType[thispointer->m_pPed] = (int)weaponType;
		}
	}*/
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
		if (findWeapon(&weaponType, eModelID(-1), "spin", thispointer->m_pPed, false)) {
			PlayOrStopBarrelSpinSound(thispointer->m_pPed, &weaponType, true);
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

	float dist = (cameraposition - entity->GetPosition()).Magnitude();
	bool emptyPlayed = false, dryFirePlayed = false;
	bool alternatePlayed = false;

	// Play low ammo and dryfire sounds
	if (entity && IsEntityPointerValid(entity) && entity->m_nType == ENTITY_TYPE_PED && weap && info) {
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

	CPed* entity = thispointer->m_pPed;
	eWeaponType weaponType = entity->m_aWeapons[entity->m_nSelectedWepSlot].m_eWeaponType;
	//auto task = entity->m_pIntelligence->GetTaskFighting();
	Log("HookedCAEPedAudioEntity__HandlePedHit: AudioEvent: %d, Surface: %d, volume: %.f, maxVolume: %d, weaponType: %d, victim model ID: %d",
		AudioEvent, Surface, volume, maxVolume, weaponType, victim ? victim->m_nModelIndex : -1);
	// door surface(?): 43
	// special door model: 1532 (has default surface by default, why?)
	// Sometimes the function can return the surface higher than 179 (max surfaces in the game), so check if it's within the valid range
	bool metalSurface = Surface <= TOTAL_NUM_SURFACE_TYPES ? IsAudioMetal(eSurfaceType(Surface)) || Surface == SURFACE_GLASS : false/*(Surface == 53 || Surface == 63)*/;
	bool woodSurface = Surface <= TOTAL_NUM_SURFACE_TYPES ? IsAudioWood(eSurfaceType(Surface)) || (victim && (victim->m_nModelIndex == 1532 || victim->m_nModelIndex == 1500 || victim->m_nModelIndex == 1478 || victim->m_nModelIndex == 1432) || IsWood(Surface)) : false;
	bool handled = false;
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
			if (IsAudioMetal(surface)) {
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
			RandomUnique rnd(buffers.size());

			int index = rnd.next();
			//int index = CGeneral::GetRandomNumber() % buffers.size();
			ALuint ricochetBuf = buffers[index];
			playBuffer(ricochetBuf, 50.0f, AEAudioHardware.m_fEffectMasterScalingFactor, 3.0f, 3.5f, 5.0f, pitch, posn);

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

	float dx = cameraposition.x - posn->x;
	float dy = cameraposition.y - posn->y;
	float dz = cameraposition.z - posn->z;
	float dist = sqrtf(dx * dx + dy * dy + dz * dz);
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	const float distantThreshold = 100.0f;
	// We only process those that aren't really far away (except for distant sounds)
	bool farAway = dist > distantThreshold;
	bool handled = false;

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

	if (!farAway) {
		// Main explosion
		if (auto* explosionBuffers =
			chooseBuffers(g_Buffers.ExplosionTypeExplosionBuffers,
				/*g_Buffers.WeaponTypeExplosionBuffers,*/
				&g_Buffers.explosionBuffers, "main explosion"))
		{
			RandomUnique rnd(explosionBuffers->size());

			int id = rnd.next();
			ALuint buff = (*explosionBuffers)[id];
			handled = playBuffer(buff, distantThreshold, AEAudioHardware.m_fEffectMasterScalingFactor /** 5.0f*/, 1.0f, 7.0f, 1.0f, pitch, *posn);
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
			RandomUnique rnd(debrisBuffers->size());

			int id = rnd.next();
			ALuint buff = (*debrisBuffers)[id];
			handled = playBuffer(buff, distantThreshold, AEAudioHardware.m_fEffectMasterScalingFactor /** 5.0f*/, 0.8f, 7.0f, 1.0f, pitch, *posn);
		}
	}
	else {
		// Distant explosion
		if (auto* distantBuffers =
			chooseBuffers(g_Buffers.ExplosionTypeDistantBuffers,
				/*g_Buffers.WeaponTypeDistantBuffers,*/
				&g_Buffers.explosionDistantBuffers, "distant explosion"))
		{
			RandomUnique rnd(distantBuffers->size());

			int id = rnd.next();
			ALuint buff = (*distantBuffers)[id];
			handled = playBuffer(buff,
				200.0f,
				AEAudioHardware.m_fEffectMasterScalingFactor /** 5.0f*/,
				0.6f,
				20.0f,
				0.5f, pitch, *posn);
		}
	}


	if (!handled)
	{
		// fallback to vanilla
		OG();
	}

}

// Executes every frame internally
auto __fastcall HookedCAEFireAudioEntity__AddAudioEvent(CAEFireAudioEntity* ts, void*, int eventId, CVector* posn) {
	// Keep track of CAEFireAudioEntity to check FX existence later
	if (std::find(g_Buffers.ent.begin(), g_Buffers.ent.end(), ts) == g_Buffers.ent.end()) {
		g_Buffers.ent.push_back(ts);
	}
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);

	auto findExistingAudioForFire = [&](CFire* fire) -> std::shared_ptr<SoundInstance> {
		for (auto& a : audiosplaying) {
			if (!a) continue;
			if (a->firePtr == fire) {
				return a;
			}
		}
		return nullptr;
		};

	auto PlayPositionalSound = [&](int eventId, const std::vector<ALuint>& bufferList, const CVector& position) -> bool {
		if (bufferList.empty() || CTimer::ms_fTimeScale <= 0.0f) return false;

		auto it = g_Buffers.nonFireSounds.find(eventId);
		if (it != g_Buffers.nonFireSounds.end()) {
			auto inst = it->second.get();
			if (!inst || inst->source == 0) {
				// fallthrough to create new
			}
			else {
				alSource3f(inst->source, AL_POSITION, position.x, position.y, position.z);
				ALint state;
				alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
				if (state == AL_PAUSED) {
					if (inst->paused) {
						ResumeAudio(inst);
					}
					else {
						// mismatch: source reports paused but our flag not set -> play to be safe
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

		// Pick random buffer and play (fallback)
		RandomUnique rnd(bufferList.size());
		int idx = rnd.next();
		ALuint buffer = bufferList[idx];
		if (buffer == 0) return false;

		// Create a new non-fire sound (playBuffer will insert into nonFireSounds)
		if (playBuffer(buffer, 200.0f, AEAudioHardware.m_fEffectMasterScalingFactor, 4.0f,
			1.0f, 1.5f, pitch, position, false, nullptr, eventId, nullptr, nullptr,
			false, nullptr, "", false, nullptr, WEAPONTYPE_UNARMED,
			false, false, false, true))
		{
			return true;
		}
		return false;
		};

	auto PlayFireLoopSound = [&](CFire* fire, const std::vector<ALuint>& bufferList) -> bool {
		if (!fire->m_nFlags.bActive || CTimer::ms_fTimeScale <= 0.0f || bufferList.empty()) return false;

		auto it = g_Buffers.fireSounds.find(fire);
		CVector pos = fire->m_vecPosition;

		// If there is a mapping, use it, but treat AL_PAUSED specially
		if (it != g_Buffers.fireSounds.end()) {
			auto inst = it->second.get();
			if (inst && inst->source != 0) {
				alSource3f(inst->source, AL_POSITION, pos.x, pos.y, pos.z);
				ALint state;
				alGetSourcei(inst->source, AL_SOURCE_STATE, &state);

				// Update gain bookkeeping
				alSourcef(inst->source, AL_GAIN, AEAudioHardware.m_fEffectMasterScalingFactor * fire->m_fStrength);
				inst->baseGain = AEAudioHardware.m_fEffectMasterScalingFactor;

				if (state == AL_PAUSED) {
					if (inst->paused) {
						// resume via proper handler so pauseOffset is respected
						ResumeAudio(inst);
					}
					else {
						// if our paused flag is false but AL reports PAUSED, try to play (defensive)
						alSourcePlay(inst->source);
						inst->paused = false;
					}
				}
				else if (state == AL_STOPPED) {
					// only actually start if it's stopped
					alSourcePlay(inst->source);
					inst->paused = false;
				}
				// if state == AL_PLAYING -> nothing to do
				return true;
			}
			// if mapped instance is null or invalid, fall through and attempt to reuse from audiosplaying
		}

		// Defensive: maybe the map was cleared but an instance still exists in audiosplaying.
		// Find and re-use it to avoid creating duplicate sources for the same fire.
		{
			auto existing = findExistingAudioForFire(fire);
			if (existing) {
				// re-insert into map and update params, then behave like mapped case above
				g_Buffers.fireSounds[fire] = existing;
				if (existing->source != 0) {
					alSource3f(existing->source, AL_POSITION, pos.x, pos.y, pos.z);
					alSourcef(existing->source, AL_GAIN, AEAudioHardware.m_fEffectMasterScalingFactor);
					existing->baseGain = AEAudioHardware.m_fEffectMasterScalingFactor;
					ALint state;
					alGetSourcei(existing->source, AL_SOURCE_STATE, &state);
					if (state == AL_PAUSED) {
						if (existing->paused) ResumeAudio(&*existing);
						else { alSourcePlay(existing->source); existing->paused = false; }
					}
					else if (state == AL_STOPPED) {
						alSourcePlay(existing->source);
						existing->paused = false;
					}
					return true;
				}
			}
		}

		// create a new looping fire sound
		RandomUnique rnd(bufferList.size());
		int idx = rnd.next();
		ALuint buffer = bufferList[idx];
		if (buffer == 0) return false;

		// Note: playBuffer will create the SoundInstance and populate g_Buffers.fireSounds[fire]
		if (playBuffer(buffer, 200.0f, AEAudioHardware.m_fEffectMasterScalingFactor /* fire->m_fStrength*/, 4.0f,
			1.0f, 1.5f, pitch, pos, true, fire))
		{
			return true;
		}
		return false;
		};

	bool handled = false;
	// Handle special non-CFire (particle-only) events early
	if (posn) {
		switch (eventId) {
		case AE_FIRE_CAR:  handled |= PlayPositionalSound(AE_FIRE_CAR, g_Buffers.fireLoopBuffersCar, *posn); break;
		case AE_FIRE_BIKE: handled |= PlayPositionalSound(AE_FIRE_BIKE, g_Buffers.fireLoopBuffersBike, *posn); break;
		case AE_FIRE_FLAME: handled |= PlayPositionalSound(AE_FIRE_FLAME, g_Buffers.fireLoopBuffersFlame, *posn); break;
		case AE_FIRE_MOLOTOV_FLAME: handled |= PlayPositionalSound(AE_FIRE_MOLOTOV_FLAME, g_Buffers.fireLoopBuffersMolotov, *posn); break;
		}
	}

	for (int i = 0; i < MAX_NUM_FIRES; i++) {
		CFire* fire = &gFireManager.m_aFires[i];
		if (!fire->m_nFlags.bActive || CTimer::ms_fTimeScale <= 0.0f) continue;

		switch (eventId) {
		case AE_FIRE:
			handled |= PlayFireLoopSound(fire, g_Buffers.fireLoopBuffersSmall);
			break;
		case AE_FIRE_MEDIUM:
			handled |= PlayFireLoopSound(fire, g_Buffers.fireLoopBuffersMedium);
			break;
		case AE_FIRE_LARGE:
			handled |= PlayFireLoopSound(fire, g_Buffers.fireLoopBuffersLarge);
			break;
		default: break;
		}
	}

	if (!handled) {
		subhook_remove(subhookCAEFireAudioEntity__AddAudioEvent);
		plugin::CallMethod<0x4DD3C0, CAEFireAudioEntity*, int, CVector*>(ts, eventId, posn);
		subhook_install(subhookCAEFireAudioEntity__AddAudioEvent);
	}
}

void UpdateFireSoundCleanup() {

	// Clean up non-fire sounds tied to inactive CAEFireAudioEntity
	for (auto it = g_Buffers.ent.begin(); it != g_Buffers.ent.end();) {
		CAEFireAudioEntity* entity = *it;
		if (entity && entity->field_84) {
			++it;
			continue;
		}

		for (auto nsIt = g_Buffers.nonFireSounds.begin(); nsIt != g_Buffers.nonFireSounds.end();) {
			auto inst = nsIt->second.get();
			if (inst) {
				alSourceStop(inst->source);
				ALint state;
				alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
				if (state == AL_STOPPED && !inst->paused) {
					alDeleteSources(1, &inst->source);
				}
			}
			nsIt = g_Buffers.nonFireSounds.erase(nsIt);
		}

		it = g_Buffers.ent.erase(it);
	}

	// Update and clean up fire loop sounds
	for (auto it = g_Buffers.fireSounds.begin(); it != g_Buffers.fireSounds.end();) {
		CFire* fire = it->first;
		auto sound = it->second.get();
		if (!fire->m_nFlags.bActive) {
			if (sound) {
				alSourceStop(sound->source);
				ALint state;
				alGetSourcei(sound->source, AL_SOURCE_STATE, &state);
				if (state == AL_STOPPED && !sound->paused) {
					alDeleteSources(1, &sound->source);
				}
			}
			it = g_Buffers.fireSounds.erase(it);
			continue;
		}

		if (sound) {
			CVector pos = fire->m_vecPosition;
			alSource3f(sound->source, AL_POSITION, pos.x, pos.y, pos.z);
			alSourcef(sound->source, AL_GAIN, sound->baseGain * fire->m_fStrength /*CalculateVolume(pos) */);
			//sound->baseGain = AEAudioHardware.m_fEffectMasterScalingFactor * fire->m_fStrength;
			ALint state;
			alGetSourcei(sound->source, AL_SOURCE_STATE, &state);
			if (state == AL_STOPPED && !sound->paused) {
				alSourcePlay(sound->source);

				auto sfx = std::make_shared<SoundInstance>();
				sfx->source = sound->source;
				sfx->pos = pos;
				sfx->baseGain = AEAudioHardware.m_fEffectMasterScalingFactor * fire->m_fStrength;
				sfx->entity = nullptr;
				bool already = false;
				for (auto& a : audiosplaying) if (a->source == sound->source) { already = true; break; }
				if (!already) audiosplaying.push_back(std::move(sfx));
				break;
			}
		}

		++it;
	}

	// Play and update volume of non-fire sounds
	for (auto& [eventId, sound] : g_Buffers.nonFireSounds) {
		if (!sound) continue;

		ALint state;
		alGetSourcei(sound->source, AL_SOURCE_STATE, &state);
		if (state == AL_STOPPED && !sound->paused) {
			alSourcePlay(sound->source);
			alSourcef(sound->source, AL_GAIN, sound->baseGain /*CalculateVolume(sound->pos, 10.0f, 6.0f)*/);

			auto sfx = std::make_shared<SoundInstance>();
			sfx->source = sound->source;
			bool already = false;
			for (auto& a : audiosplaying) if (a->source == sound->source) { already = true; break; }
			if (!already) audiosplaying.push_back(std::move(sfx));
			break;
		}
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

		if (findWeapon(&weapon, eModelID(-1), "spin_end", ts->m_pPed)) {
			PlayOrStopBarrelSpinSound(ts->m_pPed, &weapon, false, false);
			//	ts->PlayMiniGunStopSound(ped);
			Log("PlayMinigunBarrelStopSound: Custom spin_end.wav found, skipping vanilla");
			return;
		}
	}
	Log("PlayMinigunBarrelStopSound fallback: no custom spin_end.wav, using vanilla");
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

	float dx = cameraposition.x - PedPos.x;
	float dy = cameraposition.y - PedPos.y;
	float dz = cameraposition.z - PedPos.z;
	float dist = sqrtf(dx * dx + dy * dy + dz * dz);
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);

	auto PlayJackingSound = [&](const std::vector<ALuint>& bufferList) {
		if (bufferList.empty()) return false;
		RandomUnique rnd(bufferList.size());

		int idx = rnd.next();
		//int idx = CGeneral::GetRandomNumber() % bufferList.size();
		ALuint buf = bufferList[idx];

		if (playBuffer(buf, FLT_MAX, CalculateVolume(PedPos), 0.8f, 5.0f, 1.5f, pitch, PedPos))
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
				RandomUnique rnd(selected->size());

				int index = rnd.next();
				ALuint buffer = (*selected)[index];
				CVector position = pedPtr->GetPosition();
				//position.z += -15.0f;
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

				playBuffer(buffer, pedPtr->IsPlayer() ? 140.0f : 150.0f,
					AEAudioHardware.m_fEffectMasterScalingFactor, pedPtr->IsPlayer() ? 1.5f : 3.0f,
					referenceDistance, pedPtr->IsPlayer() ? 1.5f : 2.5f, pitch, position, false, nullptr, 0, nullptr, pedPtr);
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


struct InteriorAmbience {
	std::string gxtKey;
	ALuint buffer;
};

std::unordered_map<int, std::vector<InteriorAmbience>> g_InteriorAmbience;


void LoadAmbienceSounds(const std::filesystem::path& path, bool loadOldAmbience = true) {
	fs::path ambientDir = path / "generic/ambience";
	if (!fs::exists(ambientDir) || !fs::is_directory(ambientDir)) {
		Log("Ambient sound folder not found: %s", ambientDir.string().c_str());
		return;
	}

	std::vector<std::string> globalRegions = { "country", "LS", "LV", "SF" };
	// Check if a given folder is a global zone folder
	auto isGlobalZoneFolder = [&](const fs::path& folderPath) -> bool {
		std::string folderName = folderPath.filename().string();
		std::transform(folderName.begin(), folderName.end(), folderName.begin(), ::tolower);

		for (const auto& region : globalRegions) {
			std::string regionLower = region;
			std::transform(regionLower.begin(), regionLower.end(), regionLower.begin(), ::tolower);
			if (folderName == regionLower)
				return true;
		}
		return false;
	};
	if (loadOldAmbience) {
		fs::path zoneDir = ambientDir / "zones";
		if (fs::exists(zoneDir) && fs::is_directory(zoneDir)) {
			// iterate only in the local zones folder
			for (const auto& zoneEntry : fs::directory_iterator(zoneDir)) {
				if (!zoneEntry.is_directory())
					continue;

				if (isGlobalZoneFolder(zoneEntry.path())) {
					continue; // skip global zones
				}
				std::string zoneName = zoneEntry.path().filename().string();
				std::transform(zoneName.begin(), zoneName.end(), zoneName.begin(), ::tolower);

				// iterate files inside that zone folder
				for (const auto& entry : fs::directory_iterator(zoneEntry.path())) {
					if (!entry.is_regular_file())
						continue;

					std::string filename = entry.path().filename().string();
					const std::string prefix = "ambience";
					const std::string suffix = ".wav";

					if (filename.rfind(prefix, 0) != 0 || filename.length() <= prefix.length() + suffix.length())
						continue;

					// strip prefix + suffix
					std::string name = filename.substr(prefix.length(), filename.length() - prefix.length() - suffix.length());

					std::string timeSuffix;
					std::string digits;

					if (name.rfind("_night") == 0) {
						timeSuffix = "_night";
						digits = name.substr(6); // after "_night"
					}
					else if (name.rfind("_riot") == 0) {
						timeSuffix = "_riot";
						digits = name.substr(5); // after "_riot"
					}
					else {
						digits = name;
					}

					if (!digits.empty() && !std::all_of(digits.begin(), digits.end(), ::isdigit)) {
						Log("Skipping invalid ambience filename: %s", filename.c_str());
						continue;
					}

					ALuint buffer = createOpenALBufferFromWAV(entry.path().string().c_str());
					if (buffer) {
						if (timeSuffix == "_night") {
							g_Buffers.ZoneAmbienceBuffers_Night[zoneName].push_back(buffer);
						}
						else if (timeSuffix == "_riot") {
							g_Buffers.ZoneAmbienceBuffers_Riot[zoneName].push_back(buffer);
						}
						else {
							g_Buffers.ZoneAmbienceBuffers_Day[zoneName].push_back(buffer);
						}

						Log("Loaded ambience for zone: '%s' (%s) [index=%s] --> %s",
							zoneName.c_str(),
							timeSuffix.empty() ? "day" : timeSuffix.c_str(),
							digits.empty() ? "-" : digits.c_str(),
							filename.c_str());
					}
				}
			}
		}


		for (const auto& region : globalRegions) {
			fs::path regionDir = ambientDir / "zones" / region;
			Log("global zones dir: %s", regionDir.string().c_str());
			if (!fs::exists(regionDir) || !fs::is_directory(regionDir)) {
				Log("Global zones ambience: folder '%s' wasn't found, ignoring...", regionDir.string().c_str());
				continue;
			}

			for (const auto& entry : fs::directory_iterator(regionDir)) {
				if (!entry.is_regular_file()) {
					Log("Nah");
					continue;
				}
				Log("global zones entry: %s", entry.path().string().c_str());
				std::string filename = entry.path().filename().string();
				const std::string prefix = "ambience";
				const std::string suffix = ".wav";

				if (filename.rfind(prefix, 0) != 0 || filename.length() <= prefix.length() + 1)
					continue;

				std::string name = filename.substr(prefix.length(), filename.length() - prefix.length() - suffix.length());

				// Extract zone name
				// Remove trailing digits first
				std::string nameWithoutDigits = name;
				while (!nameWithoutDigits.empty() && isdigit(nameWithoutDigits.back()))
					nameWithoutDigits.pop_back();

				// Then check for _night / _riot at the end
				std::string timeSuffix;
				if (nameWithoutDigits.size() >= 6 && nameWithoutDigits.compare(nameWithoutDigits.size() - 6, 6, "_night") == 0) {
					timeSuffix = "_night";
					nameWithoutDigits.erase(nameWithoutDigits.size() - 6);
				}
				else if (nameWithoutDigits.size() >= 5 && nameWithoutDigits.compare(nameWithoutDigits.size() - 5, 5, "_riot") == 0) {
					timeSuffix = "_riot";
					nameWithoutDigits.erase(nameWithoutDigits.size() - 5);
				}

				std::string zoneName = nameWithoutDigits;

				while (!zoneName.empty() && isdigit(zoneName.back())) {
					zoneName.pop_back();
				}

				std::transform(zoneName.begin(), zoneName.end(), zoneName.begin(), ::tolower);

				// Prefix region to zoneName so LS/downtown and SF/downtown don’t collide
				std::string globalKey = region/* + "_" + zoneName*/;

				ALuint buffer = createOpenALBufferFromWAV(entry.path().string().c_str());
				if (buffer) {
					if (timeSuffix == "_night") {
						g_Buffers.GlobalZoneAmbienceBuffers_Night[globalKey].push_back(buffer);
					}
					else if (timeSuffix == "_riot") {
						g_Buffers.GlobalZoneAmbienceBuffers_Riot[globalKey].push_back(buffer);
					}
					else {
						g_Buffers.GlobalZoneAmbienceBuffers_Day[globalKey].push_back(buffer);
					}

					Log("Loaded ambience for global zone: '%s' (%s) --> %s", globalKey.c_str(), timeSuffix.empty() ? "day" : timeSuffix.c_str(), filename.c_str());
				}
			}
		}

		for (int i = 0; i <= 300; ++i) {
			// Existing
			std::string dayFile = "ambience" + std::to_string(i) + ".wav";
			std::string nightFile = "ambience_night" + std::to_string(i) + ".wav";
			std::string riotFile = "ambience_riot" + std::to_string(i) + ".wav";
			std::string thunderFile = "thunder" + std::to_string(i) + ".wav";

			fs::path dayPath = ambientDir / dayFile;
			fs::path nightPath = ambientDir / nightFile;
			fs::path riotPath = ambientDir / riotFile;
			fs::path thunderPath = ambientDir / thunderFile;

			if (fs::exists(dayPath)) {
				ALuint buffer = createOpenALBufferFromWAV(dayPath.string().c_str());
				if (buffer) g_Buffers.AmbienceBuffs.push_back(buffer);
			}

			if (fs::exists(nightPath)) {
				ALuint buffer = createOpenALBufferFromWAV(nightPath.string().c_str());
				if (buffer) g_Buffers.NightAmbienceBuffs.push_back(buffer);
			}

			if (fs::exists(riotPath)) {
				ALuint buffer = createOpenALBufferFromWAV(riotPath.string().c_str());
				if (buffer) g_Buffers.RiotAmbienceBuffs.push_back(buffer);
			}
			if (fs::exists(thunderPath))
			{
				ALuint buffer = createOpenALBufferFromWAV(thunderPath.string().c_str());
				if (buffer) g_Buffers.ThunderBuffs.push_back(buffer);
			}
		}
	}
	// singular
	if (g_Buffers.AmbienceBuffs.empty()) {
		fs::path fallback = ambientDir / "ambience.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer)g_Buffers.AmbienceBuffs.push_back(buffer);
		}
	}

	if (g_Buffers.NightAmbienceBuffs.empty()) {
		fs::path fallback = ambientDir / "ambience_night.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer) g_Buffers.NightAmbienceBuffs.push_back(buffer);
		}
	}

	if (g_Buffers.RiotAmbienceBuffs.empty()) {
		fs::path fallback = ambientDir / "ambience_riot.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer) g_Buffers.RiotAmbienceBuffs.push_back(buffer);
		}
	}

	if (g_Buffers.ThunderBuffs.empty()) {
		fs::path fallback = ambientDir / "thunder.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer) g_Buffers.ThunderBuffs.push_back(buffer);
		}
	}
	// Load interior ambience sounds
	fs::path interiorDir = ambientDir / "interiors";
	if (fs::exists(interiorDir) && fs::is_directory(interiorDir)) {
		for (const auto& interiorEntry : fs::directory_iterator(interiorDir)) {
			if (!interiorEntry.is_directory())
				continue;

			// try to find interior ID folder
			std::string interiorIDStr = interiorEntry.path().filename().string();
			int interiorID = 0;
			try {
				interiorID = std::stoi(interiorIDStr);
			}
			catch (...) {
				Log("Invalid interior folder name (not a number): %s", interiorIDStr.c_str());
				continue;
			}

			// Iterate over each GXT folder inside the interior folder
			for (const auto& nameEntry : fs::directory_iterator(interiorEntry.path())) {
				if (!nameEntry.is_directory())
					continue;

				std::string gxtKey = nameEntry.path().filename().string();
				std::transform(gxtKey.begin(), gxtKey.end(), gxtKey.begin(), ::tolower);

				// Try numbered ambience files inside the GXT folder
				for (int i = 0; i <= 300; ++i) {
					auto wavEntry = nameEntry.path() / ("ambience" + std::to_string(i) + ".wav");
					auto wavEntryNoIndex = nameEntry.path() / "ambience.wav";

					if (fs::exists(wavEntry)) {
						ALuint buffer = createOpenALBufferFromWAV(wavEntry.string().c_str());
						if (!buffer) {
							Log("Failed to load WAV: %s", wavEntry.string().c_str());
							continue;
						}

						InteriorAmbience sound;
						sound.gxtKey = gxtKey;
						sound.buffer = buffer;

						g_InteriorAmbience[interiorID].push_back(sound);

						Log("Loaded interior sound for interior ID %d, GXT entry '%s': %s",
							interiorID, gxtKey.c_str(), wavEntry.filename().string().c_str());
					}
					else if (i == 0 && fs::exists(wavEntryNoIndex)) { // only check this once per folder
						ALuint buffer = createOpenALBufferFromWAV(wavEntryNoIndex.string().c_str());
						if (!buffer) {
							Log("Failed to load WAV: %s", wavEntryNoIndex.string().c_str());
							continue;
						}

						InteriorAmbience sound;
						sound.gxtKey = gxtKey;
						sound.buffer = buffer;

						g_InteriorAmbience[interiorID].push_back(sound);

						Log("Loaded singular interior sound for interior ID %d, GXT entry '%s': %s",
							interiorID, gxtKey.c_str(), wavEntryNoIndex.filename().string().c_str());
						break;
					}
				}
			}
		}
	}
	else {
		Log("Interior ambience directory not found: %s", interiorDir.string().c_str());
	}

	// those two below is for LS gun ambience
	fs::path pathToGunfireAmbience = foldermod / "generic" / "ambience" / "gunfire";
	std::vector<std::string> expectedWeapons = { "ak47", "pistol" };

	for (auto& weapon : expectedWeapons) {
		fs::path folderPath = pathToGunfireAmbience / weapon;
		if (fs::exists(folderPath)) {
			fs::path wavPath = folderPath / "shoot.wav";
			if (!fs::exists(wavPath)) {
				Log("Weapon gunfire ambience folder '%s' missing shoot.wav.", weapon.c_str());
			}
			else {
				auto weapontype = eWeaponType();
				if (nameType(&weapon, &weapontype)) {
					weaponNames.push_back({ weapontype, weapon });
					Log("Found weapon sound folder: %s", weapon.c_str());
				}
			}
		}
		else {
			Log("Weapon gunfire ambience folder '%s' is missing.", weapon.c_str());
		}
	}


	for (auto& vec : weaponNames) {
		auto& folderName = vec.weapName;
		fs::path weaponPath = ambientDir / "gunfire" / folderName / "shoot.wav";
		if (fs::exists(weaponPath) && fs::is_directory(weaponPath.parent_path())) {
			ALuint buffer = createOpenALBufferFromWAV(weaponPath.string().c_str());
			if (buffer != 0) {
				g_Buffers.WeaponTypeAmbienceBuffers[vec.weapType] = buffer;
				Log("Loaded weapon ambience for %s: %s", folderName.c_str(), weaponPath.filename().string().c_str());
			}
			else {
				modMessage("Failed to load weapon ambience WAV: " + weaponPath.string());
			}
		}
	}
}

CVector GetRandomAmbiencePosition(const CVector& origin, bool isThunder = false, bool isInteriorAmbience = false) {
	float angle = (float)(CGeneral::GetRandomNumber() % 628) / 100.0f;
	float distance = (isThunder ? 10.0f : (isInteriorAmbience ? 15.0f : 100.0f)) + (CGeneral::GetRandomNumber() % 80);
	return origin + CVector(cosf(angle) * distance, sinf(angle) * distance, 2.0f);
}


bool PlayAmbienceBuffer(ALuint buffer, const CVector& origin, bool isGunfire = false, bool isThunder = false, bool isInteriorAmbience = false) {

	if (buffer == 0 || CCutsceneMgr::ms_running || FrontEndMenuManager.m_bMenuActive /*|| (isGunfire && CWeather::WeatherRegion != WEATHER_REGION_LA)*/) // Don't play during a cutscene or when paused
		return false;
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	//Log("Ambience pitch: %.2f", pitch);
	CVector pos = GetRandomAmbiencePosition(origin, isThunder, isInteriorAmbience);

	float VolumeToUse = AEAudioHardware.m_fEffectMasterScalingFactor * 1.3f;
	if (isGunfire) {
		VolumeToUse = AEAudioHardware.m_fEffectMasterScalingFactor * 1.0f;
	}
	else if (isThunder) {
		VolumeToUse = AEAudioHardware.m_fEffectMasterScalingFactor * 1.5f;
	}
	if (playBuffer(buffer, 250.0f, VolumeToUse, 1.0f, isThunder ? 6.0f : (isInteriorAmbience ? 15.0f : 5.5f), 1.0f, pitch, pos,
		false, nullptr, 0, nullptr, nullptr, false, nullptr, "", false, nullptr, eWeaponType(0), isGunfire, isInteriorAmbience, false, false, 0, true))
	{
		Log("PlayAmbienceBuffer returned true");
		return true;
	}
	Log("PlayAmbienceBuffer returned false");
	return false;
}
static uint32_t nextInteriorAmbienceTime = 0;
static uint32_t nextZoneAmbienceTime = 0;
static uint32_t nextFireAmbienceTime = 0;
bool PlayAmbienceSFX(const CVector& origin, eWeaponType weaponType, bool useOldAmbience) {
	static int prevCurrArea = CGame::currArea; // track area transitions
	static uint32_t lastPrintedInteriorID = 0;
	bool isNight = CClock::ms_nGameClockHours >= 20 || CClock::ms_nGameClockHours < 6;
	bool isRiot = CGameLogic::LaRiotsActiveHere();
	// --- 0) Detect interior->interior switch (warp while staying inside) ---
	if (prevCurrArea > 0 && CGame::currArea > 0 && prevCurrArea != CGame::currArea) {
		// We changed interiors while still "inside" -> stop old interior ambience(s) and allow new to play
		nextInteriorAmbienceTime = CTimer::m_snTimeInMilliseconds; // allow immediate play

		for (auto& sfx : audiosplaying) {
			if (!sfx) continue;
			if (sfx->isInteriorAmbience) {
				// stop & delete old source to avoid blocking new ambience
				if (sfx->source != 0) {
					ALint st = AL_STOPPED;
					alGetSourcei(sfx->source, AL_SOURCE_STATE, &st);
					if (st == AL_PLAYING || st == AL_PAUSED) {
						PauseAudio(&*sfx);
						//alSourceStop(sfx->source);
					}
					alDeleteSources(1, &sfx->source);
					sfx->source = 0;
				}
				sfx->isInteriorAmbience = false;
			}
		}
		// reset last printed so new interior logs immediately
		lastPrintedInteriorID = 0;
	}

	// --- 1) Detect leaving/entering interiors and reset timers / cleanup ---
	if (prevCurrArea > 0 && CGame::currArea <= 0) {
		// We just left an interior -> clear interior timers and stop any interior ambiences
		nextInteriorAmbienceTime = 0;

		for (auto& sfx : audiosplaying) {
			if (!sfx) continue;
			if (sfx->isInteriorAmbience) {
				if (sfx->source != 0) {
					ALint state = 0;
					alGetSourcei(sfx->source, AL_SOURCE_STATE, &state);
					if (state == AL_PLAYING || state == AL_PAUSED) {
						PauseAudio(&*sfx);
						//alSourceStop(sfx->source);
					}
					alDeleteSources(1, &sfx->source);
					sfx->source = 0;
				}
				sfx->isInteriorAmbience = false;
			}
		}
	}
	else if (prevCurrArea <= 0 && CGame::currArea > 0) {
		// We just entered an interior -> allow immediate attempt to play ambience
		nextInteriorAmbienceTime = CTimer::m_snTimeInMilliseconds;
	}
	prevCurrArea = CGame::currArea;

	// --- 2) Check if an ambience is still playing (treat PAUSED as "still playing") ---
	bool ambienceStillPlaying = false;
	for (auto& sfx : audiosplaying) {
		if (!sfx) continue;
		if (!sfx->isAmbience)
			continue;

		ALint state = AL_STOPPED;
		if (sfx->source != 0) {
			alGetSourcei(sfx->source, AL_SOURCE_STATE, &state);
		}

		if (state == AL_PLAYING) {
			ambienceStillPlaying = true;
			break;
		}

		// If paused, resume when menu inactive instead of spawning a new ambience.
		if (state == AL_PAUSED) {
			if (!FrontEndMenuManager.m_bMenuActive && sfx->source != 0) {
				ResumeAudio(&*sfx);
				//alSourcePlay(sfx->source);
			}
			ambienceStillPlaying = true;
			break;
		}
	}

	if (useOldAmbience) {

		// INSIDE INTERIOR
		if (CGame::currArea != 0) {  // inside some interior
			int currInterior = CGame::currArea;
			CEntryExit* currentEntryExit = nullptr;
			auto* playa = FindPlayerPed();
			CVector playerPos = FindPlayerCoors();

			float closestDistSq = FLT_MAX;

			for (int i = 0; i < CEntryExitManager::mp_poolEntryExits->m_nSize; i++) {
				if (!CEntryExitManager::mp_poolEntryExits->m_byteMap[i].bEmpty) {
					auto object = CEntryExitManager::mp_poolEntryExits->GetAt(i);
					if (object && playa->m_nAreaCode == object->m_nArea) {
						float distSq = DistanceBetweenPoints(playerPos, object->m_vecExitPos);
						if (distSq < closestDistSq) {
							closestDistSq = distSq;
							currentEntryExit = object;
						}
					}
				}
			}

			if (currentEntryExit && !CEntryExitManager::mp_Active) {
				if (lastPrintedInteriorID != currentEntryExit->m_nArea) {
					Log("Current interior ID '%d', interior name '%s'",
						currentEntryExit->m_nArea, currentEntryExit->m_szName);
					lastPrintedInteriorID = currentEntryExit->m_nArea;
				}

				static bool wasPaused = false;
				if (FrontEndMenuManager.m_bMenuActive) {
					wasPaused = true;
					return false;
				}

				if (wasPaused) {
					// Reset timers to prevent instant spam after coming back from pause
					nextInteriorAmbienceTime = CTimer::m_snTimeInMilliseconds + 300;
					wasPaused = false;
				}

				if (!ambienceStillPlaying && CTimer::m_snTimeInMilliseconds >= nextInteriorAmbienceTime) {
					auto it = g_InteriorAmbience.find(currInterior);
					if (it != g_InteriorAmbience.end() && !it->second.empty()) {
						// Prepare key
						std::string zoneKey(currentEntryExit->m_szName);
						std::transform(zoneKey.begin(), zoneKey.end(), zoneKey.begin(), ::tolower);
						zoneKey = zoneKey.substr(0, zoneKey.find_first_of(" \t\n\r")); // trim

						// Match sounds for this zoneKey
						std::vector<const InteriorAmbience*> matchingSounds;
						for (const auto& sound : it->second) {
							std::string soundKey = sound.gxtKey;
							std::transform(soundKey.begin(), soundKey.end(), soundKey.begin(), ::tolower);
							if (zoneKey == soundKey) {
								matchingSounds.push_back(&sound);
							}
						}

						if (!matchingSounds.empty()) {
							RandomUnique rnd(matchingSounds.size());

							int index = rnd.next();
							const InteriorAmbience* chosenSound =
								matchingSounds[index];

							if (PlayAmbienceBuffer(chosenSound->buffer, origin, false, false, true)) {
								nextInteriorAmbienceTime = CTimer::m_snTimeInMilliseconds +
									(CGeneral::GetRandomNumber() % (interiorIntervalMax - interiorIntervalMin)) + interiorIntervalMin;
								return true; // Stop after playing
							}
						}
					}
				}
			}
			else if (CEntryExitManager::mp_Active)
			{
				// During entry/exit transitions, ensure interior ambiences are stopped.
				for (auto& interiorAmbiences : audiosplaying)
				{
					if (!interiorAmbiences) continue;
					ALint state = AL_STOPPED;
					if (interiorAmbiences->source != 0) {
						alGetSourcei(interiorAmbiences->source, AL_SOURCE_STATE, &state);
					}
					if (interiorAmbiences->isInteriorAmbience && (state == AL_PLAYING || state == AL_PAUSED))
					{
						PauseAudio(&*interiorAmbiences);
						//alSourceStop(interiorAmbiences->source);
						alDeleteSources(1, &interiorAmbiences->source);
						interiorAmbiences->source = 0;
						interiorAmbiences->isInteriorAmbience = false;
					}
				}
			}
		}
		// --- OUTSIDE (zones / fallback) ---
		else if (CGame::currArea <= 0) {
			CZone* zone{};
			CTheZones::GetZoneInfo(&cameraposition, &zone);

			bool playedSomething = false; // track if any ambience actually played
			bool zoneHasCustomAmbience = false;
			if (zone) {
				std::string zoneKey(zone->m_szTextKey);
				std::transform(zoneKey.begin(), zoneKey.end(), zoneKey.begin(), ::tolower);

				const auto& buffersMap =
					isRiot ? g_Buffers.ZoneAmbienceBuffers_Riot :
					isNight ? g_Buffers.ZoneAmbienceBuffers_Night :
					g_Buffers.ZoneAmbienceBuffers_Day;

				if (!zoneKey.empty()) {
					for (const auto& [key, vec] : buffersMap) {
						if (!vec.empty() && zoneKey.starts_with(key)) {
							zoneHasCustomAmbience = true;
							break;
						}
					}
				}

				//std::string boolean = ambienceStillPlaying ? "true" : "false";
				//Log("ambienceStillPlaying: %s", boolean.c_str());
				// First try local zone ambience
				if (!zoneKey.empty() && !ambienceStillPlaying && CTimer::m_snTimeInMilliseconds >= nextZoneAmbienceTime) {
					for (const auto& [key, vec] : buffersMap) {
						if (zoneKey.starts_with(key) && !vec.empty()) {
							RandomUnique rnd(vec.size());

							int index = rnd.next();
							ALuint buffer = vec[index];
							if (PlayAmbienceBuffer(buffer, origin)) {
								nextZoneAmbienceTime = CTimer::m_snTimeInMilliseconds +
									(CGeneral::GetRandomNumber() % (zoneIntervalMax - zoneIntervalMin)) + zoneIntervalMin;
								playedSomething = true;
								//return true;
								break;
							}
						}
					}
				}
			}

			// --- Global zone ambience ---
			const auto& globalBuffersMap =
				isRiot ? g_Buffers.GlobalZoneAmbienceBuffers_Riot :
				isNight ? g_Buffers.GlobalZoneAmbienceBuffers_Night :
				g_Buffers.GlobalZoneAmbienceBuffers_Day;

			std::string globalKey;
			//Log("CTheZones::m_CurrLevel: %d", CTheZones::m_CurrLevel);
			switch (CTheZones::m_CurrLevel) {
			case LEVEL_NAME_COUNTRY_SIDE: globalKey = "country"; break;
			case LEVEL_NAME_LOS_SANTOS:   globalKey = "LS"; break;
			case LEVEL_NAME_SAN_FIERRO:   globalKey = "SF"; break;
			case LEVEL_NAME_LAS_VENTURAS: globalKey = "LV"; break;
			default: break;
			}

			if (!globalKey.empty() && !ambienceStillPlaying && !playedSomething) {
				if (auto it = globalBuffersMap.find(globalKey); it != globalBuffersMap.end() && !it->second.empty()) {
					RandomUnique rnd(it->second.size());

					int index = rnd.next();
					ALuint buffer = it->second[index];
					if (PlayAmbienceBuffer(buffer, origin)) {
						nextZoneAmbienceTime = CTimer::m_snTimeInMilliseconds +
							(CGeneral::GetRandomNumber() % (zoneIntervalMax - zoneIntervalMin)) + zoneIntervalMin;
						playedSomething = true;  // global played
						return true;
					}
				}
			}

			// --- Fallback riot/day/night/fire ambiences ---
			if (!ambienceStillPlaying && !zoneHasCustomAmbience && !playedSomething) {
				//	Log("Fallback playedSomething is false");
				bool fireAmbiencePlaying = std::any_of(audiosplaying.begin(), audiosplaying.end(), [&](const std::shared_ptr<SoundInstance>& sfx) {
					if (!sfx->isAmbience || sfx->isGunfireAmbience) return false;
					ALint state = AL_STOPPED;
					if (sfx->source != 0) alGetSourcei(sfx->source, AL_SOURCE_STATE, &state);
					return state == AL_PLAYING || state == AL_PAUSED;
					});

				if (!fireAmbiencePlaying && CTimer::m_snTimeInMilliseconds >= nextFireAmbienceTime) {
					std::vector<ALuint>* selectedBuffs =
						isRiot && !g_Buffers.RiotAmbienceBuffs.empty() ? &g_Buffers.RiotAmbienceBuffs :
						isNight && !g_Buffers.NightAmbienceBuffs.empty() ? &g_Buffers.NightAmbienceBuffs :
						!g_Buffers.AmbienceBuffs.empty() ? &g_Buffers.AmbienceBuffs : nullptr;

					if (selectedBuffs) {
						RandomUnique rnd(selectedBuffs->size());

						int index = rnd.next();
						ALuint buffer = (*selectedBuffs)[index];
						if (PlayAmbienceBuffer(buffer, origin)) {
							nextFireAmbienceTime = CTimer::m_snTimeInMilliseconds +
								(CGeneral::GetRandomNumber() % (fireIntervalMax - fireIntervalMin)) + fireIntervalMin;
							return true;
						}
					}
				}
			}
		}
	}
	else {
		// LS Gunfire ambience
		if (/*CWeather::WeatherRegion == WEATHER_REGION_LA && !CEntryExitManager::mp_Active*/ CGame::currArea <= 0) {
			auto it = g_Buffers.WeaponTypeAmbienceBuffers.find(weaponType);
			if (it == g_Buffers.WeaponTypeAmbienceBuffers.end() || g_Buffers.WeaponTypeAmbienceBuffers.empty() || it->second == 0)
				return false;

			return PlayAmbienceBuffer(it->second, origin, true);
		}
	}

	return false;
}

void __fastcall HookedCAudioEngine__ReportWeaponEvent(CAudioEngine* engine, void*,
	int32_t audioEvent, eWeaponType weaponType, CPhysical* physical)
{
	Log("HookedCAudioEngine__ReportWeaponEvent: event: %d, weapon type: %d physical model ID %d", audioEvent, weaponType, physical ? physical->m_nModelIndex : -1);
	if (engine && physical) {
		if (audioEvent == 145) // gun ambience in LS
		{
			if (PlayAmbienceSFX(physical->GetPosition(), weaponType, false))
			{
				Log("HookedCAudioEngine__ReportWeaponEvent: playing ambient sound");
				return;
			}
		}
	}
	Log("HookedCAudioEngine__ReportWeaponEvent: fallback");
	CallMethod<0x506F40, CAudioEngine*, int32_t, eWeaponType, CPhysical*>(engine, audioEvent, weaponType, physical);
}

void LoadMissileSounds(const fs::path& folder) {
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		fs::path missilePath = folder / ("Missiles/missile_flyloop" + std::to_string(index) + ".wav");
		if (fs::exists(missilePath)) {
			ALuint buffer = createOpenALBufferFromWAV(missilePath.string().c_str());
			if (buffer != 0) {
				g_Buffers.missileSoundBuffers.push_back(buffer);
				loadedSomething = true;
			}
		}

		if (!loadedSomething)
			break;

		++index;
	}

	// Fallback to non-indexed file if nothing was loaded
	if (g_Buffers.missileSoundBuffers.empty()) {
		fs::path fallbackPath = folder / "Missiles/missile_flyloop.wav";
		if (fs::exists(fallbackPath)) {
			ALuint buffer = createOpenALBufferFromWAV(fallbackPath.string().c_str());
			if (buffer != 0) {
				g_Buffers.missileSoundBuffers.push_back(buffer);
			}
		}
	}
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

			if (playaVehicle && findWeapon(&weaponType, eModelID(playaVehicle->m_nModelIndex), "projfire", playaVehicle))
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
						RandomUnique rnd(g_Buffers.missileSoundBuffers.size());

						int index = rnd.next();
						ALuint buff = g_Buffers.missileSoundBuffers[index];
						playBuffer(buff, 250.0f, AEAudioHardware.m_fEffectMasterScalingFactor, 1.0f, 6.0f, 1.0f, pitch, physical->GetPosition(), false, nullptr, 0, nullptr, physical, false,
							nullptr, "", false, nullptr, weaponType, false, false, true);
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
			RandomUnique rnd(g_Buffers.ThunderBuffs.size());

			int index = rnd.next();
			ALuint buffer = g_Buffers.ThunderBuffs[index];

			if (PlayAmbienceBuffer(buffer, cameraposition, false, true))
			{
				Log("HookedCAudioEngine__ReportWeaponEvent: playing thunder sound");
				return;
			}
		}
	}
	Log("HookedCAEWeatherAudioEntity__AddAudioEvent: fallback");
	CallMethod<0x506800, CAEWeatherAudioEntity*, int>(ts, AudioEvent);
}
// Custom sirens attempt, maybe finish someday later
/*struct SirenInfo {
	std::vector<ALuint> mainBuffers;
	std::vector<ALuint> mainLoopBuffers;
};

std::unordered_map<int, SirenInfo> LoadedSirensByModel;
std::unordered_map<CVehicle*, ALuint> VehicleSirenSources;
std::unordered_map<CVehicle*, ALuint> VehicleMainSirenSources;

class CAESoundManager;
CAESound* __fastcall CAESoundManager__RequestNewSoundPoliceSiren(CAESoundManager* mgr, void*, CAESound* sound) {
	auto vehiclePool = CPools::ms_pVehiclePool;

	for (int i = 0; i < vehiclePool->m_nSize; ++i) {
		CVehicle* vehicle = vehiclePool->GetAt(i);
		if (!vehicle || !vehicle->m_pDriver) continue;

		// Only proceed if police siren should be active
		//if (!vehicle->m_vehicleAudio.m_bSirenOrAlarmPlaying || !vehicle->m_vehicleAudio.m_pPoliceSirenSound)
		//	continue;

		int modelId = vehicle->m_nModelIndex;
		auto it = LoadedSirensByModel.find(modelId);
		if (it == LoadedSirensByModel.end() || it->second.mainBuffers.empty()) continue;

		ALuint& source = VehicleMainSirenSources[vehicle];

		ALint state = 0;
		if (source != 0) {
			alGetSourcei(source, AL_SOURCE_STATE, &state);
		}

		if (source == 0 || state != AL_PLAYING) {
			if (source != 0) {
				alSourceStop(source);
				alDeleteSources(1, &source);
			}

			const auto& buffers = it->second.mainBuffers;
			ALuint buffer = buffers[CGeneral::GetRandomNumber() % buffers.size()];
			alGenSources(1, &source);
			alSourcei(source, AL_BUFFER, buffer);

			CVector pos = vehicle->GetPosition();
			alSource3f(source, AL_POSITION, pos.x, pos.y, pos.z);
			alSourcef(source, AL_GAIN, CalculateVolume(pos, 250.0f, 6.0f));
			alSourcei(source, AL_LOOPING, AL_TRUE);
			alSourcePlay(source);

			auto sfx = std::make_shared<SoundInstance>();
			sfx->source = source;
			sfx->pos = pos;
			sfx->entity = vehicle;
			audiosplaying.push_back(sfx);

			Log("Started/restarted police siren for vehicle %p", vehicle);
			return sound;
		}
	}

	return CallMethodAndReturn<CAESound*, 0x4EFB10, CAESoundManager*, CAESound*>(mgr, sound);
}



CAESound* __fastcall CAESoundManager__RequestNewSoundSiren(CAESoundManager* mgr, void*, CAESound* sound) {
	auto vehiclePool = CPools::ms_pVehiclePool;

	for (int i = 0; i < vehiclePool->m_nSize; ++i) {
		CVehicle* vehicle = vehiclePool->GetAt(i);
		if (!vehicle || !vehicle->m_pDriver) continue;

		// Only proceed if regular siren is active
		//if (!vehicle->m_vehicleAudio.m_bSirenOrAlarmPlaying || !vehicle->m_vehicleAudio.m_pSirenSound)
		//	continue;

		int modelId = vehicle->m_nModelIndex;
		auto it = LoadedSirensByModel.find(modelId);
		if (it == LoadedSirensByModel.end() || it->second.mainLoopBuffers.empty()) continue;

		ALuint& source = VehicleSirenSources[vehicle];

		ALint state = 0;
		if (source != 0) {
			alGetSourcei(source, AL_SOURCE_STATE, &state);
		}

		if (source == 0 || state != AL_PLAYING) {
			if (source != 0) {
				alSourceStop(source);
				alDeleteSources(1, &source);
			}

			const auto& buffers = it->second.mainLoopBuffers;
			ALuint buffer = buffers[CGeneral::GetRandomNumber() % buffers.size()];
			alGenSources(1, &source);
			alSourcei(source, AL_BUFFER, buffer);

			CVector pos = vehicle->GetPosition();
			alSource3f(source, AL_POSITION, pos.x, pos.y, pos.z);
			alSourcef(source, AL_GAIN, CalculateVolume(pos, 250.0f, 6.0f));
			alSourcei(source, AL_LOOPING, AL_TRUE);
			alSourcePlay(source);

			auto sfx = std::make_shared<SoundInstance>();
			sfx->source = source;
			sfx->pos = pos;
			sfx->entity = vehicle;
			audiosplaying.push_back(sfx);

			Log("Started/restarted main siren for vehicle %p", vehicle);
			return sound;
		}
	}

	return CallMethodAndReturn<CAESound*, 0x4EFB10, CAESoundManager*, CAESound*>(mgr, sound);
}



void __fastcall CAESound__StopSoundAndForgetSiren(CAESound* ts, void*) {
	for (auto it = VehicleSirenSources.begin(); it != VehicleSirenSources.end(); ) {
		alSourceStop(it->second);
		alDeleteSources(1, &it->second);
		Log("Stopped main siren for vehicle %p", it->first);
		it = VehicleSirenSources.erase(it);
	}
	ts->StopSoundAndForget();
}

void __fastcall CAESound__StopSoundAndForgetPolSiren(CAESound* ts, void*) {
	for (auto it = VehicleMainSirenSources.begin(); it != VehicleMainSirenSources.end(); ) {
		alSourceStop(it->second);
		alDeleteSources(1, &it->second);
		Log("Stopped police siren for vehicle %p", it->first);
		it = VehicleMainSirenSources.erase(it);
	}
	ts->StopSoundAndForget();
}



CVector operator/(const CVector& vec, float dividend) {
	return { vec.x / dividend, vec.y / dividend, vec.z / dividend };
}*/

// Because the tank cannon doesn't have any weapon types on it, we hardcode it to this folder :shrug:
void LoadTankCannonSounds(const fs::path& folder) {
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		fs::path firePath = folder / ("Tank Cannon/cannon_fire" + std::to_string(index) + ".wav");
		if (fs::exists(firePath)) {
			ALuint buffer = createOpenALBufferFromWAV(firePath.string().c_str());
			if (buffer != 0) {
				g_Buffers.tankCannonFireBuffers.push_back(buffer);
				loadedSomething = true;
			}
		}

		if (!loadedSomething)
			break;

		++index;
	}

	if (g_Buffers.tankCannonFireBuffers.empty()) {
		fs::path fallbackPath = folder / "Tank Cannon/cannon_fire.wav";
		if (fs::exists(fallbackPath)) {
			ALuint buffer = createOpenALBufferFromWAV(fallbackPath.string().c_str());
			if (buffer != 0) {
				g_Buffers.tankCannonFireBuffers.push_back(buffer);
			}
		}
	}
}

void LoadBulletWhizzSounds(const fs::path& folder) {
	const fs::path whizzDir = folder / "generic/bullet_whizz";
	if (!fs::exists(whizzDir))
	{
		Log("LoadBulletWhizzSounds: bullet whizz folder wasn't found, '%s'", whizzDir.string().c_str());
		return;
	}
	struct WhizzEntry {
		std::vector<ALuint>* buffer;
		std::string name;
	};

	std::vector<WhizzEntry> entries = {
		{ &g_Buffers.bulletWhizzLeftRearBuffers,   "left_rear"   },
		{ &g_Buffers.bulletWhizzLeftFrontBuffers,  "left_front"  },
		{ &g_Buffers.bulletWhizzRightRearBuffers,  "right_rear"  },
		{ &g_Buffers.bulletWhizzRightFrontBuffers, "right_front" }
	};

	for (auto& entry : entries) {
		int index = 0;
		while (true) {
			fs::path path = folder / ("generic/bullet_whizz/" + entry.name + std::to_string(index) + ".wav");
			if (!fs::exists(path)) break;

			ALuint buffer = createOpenALBufferFromWAV(path.string().c_str());
			if (buffer != 0) {
				entry.buffer->push_back(buffer);
			}

			++index;
		}

		// Fallback: single sound without index
		if (entry.buffer->empty()) {
			fs::path fallback = folder / ("generic/bullet_whizz/" + entry.name + ".wav");
			if (fs::exists(fallback)) {
				ALuint buffer = createOpenALBufferFromWAV(fallback.string().c_str());
				if (buffer != 0) {
					entry.buffer->push_back(buffer);
				}
			}
		}
	}
}
void ReloadAudioFolders()
{
	// Stop and delete all currently playing sound sources
	for (auto& inst : audiosplaying)
	{
		if (inst->source != 0) {
			ALint state = AL_STOPPED;
			alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
			alSourcei(inst->source, AL_BUFFER, AL_NONE); // Detach buffer from source
			if (state == AL_PLAYING || state == AL_PAUSED) {
				PauseAudio(&*inst);
			}
			Log("Removing source '%u', missile '%u', minigun spin '%u'", inst->source, inst->missileSource, barrelSpinSource);
			alDeleteSources(1, &inst->source);
			if (inst->missileSource)
			{
				alDeleteSources(1, &inst->missileSource);
			}
			if (barrelSpinSource)
			{
				alDeleteSources(1, &barrelSpinSource);
			}
			if (barrelSpinBuffer)
			{
				alDeleteBuffers(1, &barrelSpinBuffer);
			}
			inst->source = 0;
			barrelSpinSource = 0;
			barrelSpinBuffer = 0;
		}
		inst->isAmbience = false;
		inst->isGunfireAmbience = false;
		inst->isInteriorAmbience = false;
		inst->isPossibleGunFire = false;
	}
	audiosplaying.clear();  // Remove all sound instances

	//Delete all loaded WAV buffers
	for (auto& buf : gBufferMap) {
		if (buf.second != 0) {
			Log("Freeing buffer for reload %u, name: %s", buf.second, buf.first.c_str());
			alDeleteBuffers(1, &buf.second);
			buf.second = 0;
		}
	}
	gBufferMap.clear();

	// Remove every single OpenAL buffer
	DeleteAllBuffers(g_Buffers);
	registeredweapons.clear();
	weaponNames.clear();

	// Finally reload all folders
	LoadExplosionRelatedSounds(foldermod);
	LoadJackingRelatedSounds(foldermod);
	LoadFireSounds(foldermod);
	LoadAmbienceSounds(foldermod);
	LoadRicochetSounds(foldermod);
	LoadFootstepSounds(foldermod);
	LoadTankCannonSounds(foldermod);
	LoadMissileSounds(foldermod);
	LoadBulletWhizzSounds(foldermod);
	LoadMainWeaponsFolder();
}
// tank cannon fire sound (the explosion func hook)
bool __cdecl TriggerTankFireHooked(CEntity* victim, CEntity* creator, eExplosionType type, CVector pos, uint32_t lifetime, uint8_t usesSound, float cameraShake, uint8_t bInvisible)
{
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	// We'll reuse this later for explosion-type specific explosion sounds
	g_lastExplosionType[creator] = (int)type;
	if (CPad::GetPad(0)->CarGunJustDown()) {
		if (!g_Buffers.tankCannonFireBuffers.empty() && creator && creator->m_nType == ENTITY_TYPE_PED && ((CPed*)creator)->bInVehicle && type == EXPLOSION_TANK_FIRE)
		{
			RandomUnique rnd(g_Buffers.tankCannonFireBuffers.size());

			int index = rnd.next();
			//int index = CGeneral::GetRandomNumber() % g_Buffers.tankCannonFireBuffers.size();
			ALuint buffer = g_Buffers.tankCannonFireBuffers[index];
			CVector pos = creator->GetPosition();
			playBuffer(buffer, 125.0f, AEAudioHardware.m_fEffectMasterScalingFactor, 0.3f, 3.5f, 0.3f, pitch, pos, false, nullptr, 0, nullptr, (CPhysical*)creator,
				false, nullptr, "", false);
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
		RandomUnique rnd(bufferList->size());

		int index = rnd.next();
		//int index = CGeneral::GetRandomNumber() % bufferList->size();
		ALuint buffer = (*bufferList)[index];
		playBuffer2D(buffer, true, audiovolume, pitch);
		Log("CAudioEngine__ReportFrontEndAudioHooked: playing audio with event '%d'", eventId);
		//audiosplaying.push_back(std::move(inst));
		return;
	}

	subhook_remove(subhookCAudioEngine__ReportFrontEndAudioEvent);
	eng->ReportFrontendAudioEvent(eventId, volumeChange, speed);
	subhook_install(subhookCAudioEngine__ReportFrontEndAudioEvent);
}
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
			DebugMenuAddCmd("EarShot", "Reload all audio folders", ReloadAudioFolders);
			DebugMenuAddUInt64("EarShot", "Max bytes that can be written into log", (uint64_t*)&maxBytesInLog, nullptr, 10, 0, 9000000, nullptr);
			DebugMenuAddVarBool8("EarShot", "Toggle debug log", (int8_t*)&Logging, nullptr);
			//	DebugMenuAddVarBool8("EarShot", "Toggle reverb type (EAX or EFX)", (int8_t*)&EAXOrNot, nullptr);
		}
		// Init everything
		Events::initRwEvent += [] {
			ClearLogFile();

			CIniReader ini(PLUGIN_PATH("EarShot.ini"));
			Logging = ini.ReadBoolean("MAIN", "Logging", false);
			maxBytesInLog = (uint64_t)ini.ReadInteger("MAIN", "Max bytes in log", 9000000);

			interiorIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Interior interval min", 5000);
			interiorIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Interior interval max", 10000);
			fireIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Ambience interval min", 5000);
			fireIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Ambience interval max", 10000);
			zoneIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Zone ambience interval min", 5000);
			zoneIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Zone ambience interval max", 10000);

			//Log("Version: %s", version.c_str());
			Log("Compiling date and time %s @ %s", __DATE__, __TIME__);
			//if (version != NewVersion) 
			//{
			//	Log("Warning: You have a older version of EarShot, your current one '%s', you need to have '%s'! Please update the plugin!", version.c_str(), NewVersion.c_str());
			//}
			// Init OpenAL
			Log("Initializing OpenAL...");
			device = nullptr;
			context = nullptr;

			device = alcOpenDevice(nullptr);
			if (!device) {
				Log("Could not open OpenAL device!");
				modMessage("Could not open OpenAL device!");
				return;
			}

			if (device) {
				auto deviceStr = alcGetString(device, ALC_DEVICE_SPECIFIER);
				Log("Opened playback device: '%s'", deviceStr);
				Log("Device and context created successfully...");
				//ALCint attrs[] = { ALC_MONO_SOURCES, 1, ALC_STEREO_SOURCES, 64, 0 };
				context = alcCreateContext(device, nullptr);
				alcMakeContextCurrent(context);
			}

			alListenerf(AL_GAIN, 1.0f);
			alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
			//alDopplerFactor(1.5f);

			// Check if we support sound FX, if not, skill issue, BUY A NEW SOUND CARD!!!!!!!!!!!!!!
			efxSupported = alcIsExtensionPresent(device, (ALCchar*)ALC_EXT_EFX_NAME);
			Log("Initializing sounds FX...");
			if (!efxSupported) {
				Log("EFX extension not supported, therefore no fancy sound FX, soz :shrug:\n");
				Error("EFX extension not supported, therefore no fancy sound FX, soz :shrug:\n");
				//return;
			}
			if (efxSupported)
			{
				InitReverb();
				//	InitEAXReverb();
				//	InitEcho();
			}
			Log("Sound FX initialized...");
			LoadExplosionRelatedSounds(foldermod);
			LoadJackingRelatedSounds(foldermod);
			LoadFireSounds(foldermod);
			LoadAmbienceSounds(foldermod);
			LoadRicochetSounds(foldermod);
			LoadFootstepSounds(foldermod);
			LoadTankCannonSounds(foldermod);
			LoadMissileSounds(foldermod);
			LoadBulletWhizzSounds(foldermod);
			};
		patch::RedirectCall(0x504D11, PlayMinigunBarrelStopSound);
		patch::RedirectCall(0x504CD2, PlayMinigunBarrelStopSound);
		patch::RedirectCall(0x4DF81B, HookedCAudioEngine__ReportWeaponEvent); // LS gunfire ambience
		//patch::RedirectCall(0x6AE9A5, HookedCAudioEngine__ReportWeaponEvent); // tank fire sfx
	//	patch::RedirectCall(0x7387A0, HookedCAudioEngine__ReportWeaponEvent); // rocket fire sfx for heli's (?)
		patch::RedirectCall(0x4DFAE6, HookedFireProjectile);
		patch::RedirectCall(0x72BB37, HookedCAEWeatherAudioEntity__AddAudioEvent);
		//	patch::RedirectCall(0x5048D9, ZeroVolumeMinigunSounds);
		//	patch::RedirectCall(0x503EEC, ZeroVolumeMinigunSounds);
		//	patch::RedirectCall(0x6AF0F5, TriggerTankFireFXHook);

			// stop
			//patch::RedirectCall(0x504A6A, ZeroVolumeMinigunSoundsStop);

			// Cuso
			// Custtom sirens attempt, comeback later maybe
			/*Events::vehicleRenderEvent += [](CVehicle* veh) {
				int modelId = veh->m_nModelIndex;
				fs::path sfxDir = foldermod / "generic/sirens";

				if (!LoadedSirensByModel.contains(modelId)) {
					SirenInfo sirens;
					int loadedMain = 0, loadedMainLoop = 0;

					for (int i = 0; i <= 50; ++i) {
						fs::path sirenPath = sfxDir / ("siren" + std::to_string(i) + "_" + std::to_string(modelId) + ".wav");
						if (fs::exists(sirenPath)) {
							ALuint buf = createOpenALBufferFromWAV(sirenPath.string().c_str());
							if (buf) {
								Log("Loaded %s to main buff", sirenPath.string().c_str());
								sirens.mainBuffers.push_back(buf);
								++loadedMain;
							}
						}

						fs::path mainloopPath = sfxDir / ("mainsirenloop" + std::to_string(i) + "_" + std::to_string(modelId) + ".wav");
						if (fs::exists(mainloopPath)) {
							ALuint buf = createOpenALBufferFromWAV(mainloopPath.string().c_str());
							if (buf) {
								sirens.mainLoopBuffers.push_back(buf);
								++loadedMainLoop;
							}
						}
					}

					if (loadedMain || loadedMainLoop)
						Log("Loaded sirens for model %d: %d holding loop, %d main loop", modelId, loadedMain, loadedMainLoop);

					LoadedSirensByModel[modelId] = sirens;
				}
				for (auto it = VehicleSirenSources.begin(); it != VehicleSirenSources.end(); ) {
					ALint state;
					alGetSourcei(it->second, AL_SOURCE_STATE, &state);
					CVehicle* vehicle = it->first;

					if (vehicle && !vehicle->m_nVehicleFlags.bSirenOrAlarm) {
						if (state == AL_PLAYING) {
							alSourceStop(it->second);
							alDeleteSources(1, &it->second);
							Log("Stopped main siren for vehicle %p", vehicle);

							auto sound = vehicle->m_vehicleAudio.m_pSirenSound;
							bool someBool = vehicle->m_vehicleAudio.m_bSirenOrAlarmPlaying && !vehicle->m_vehicleAudio.m_bHornPlaying && sound;

							it = VehicleSirenSources.erase(it);

							if (someBool) {
								sound->StopSoundAndForget();
							}
							continue;
						}
					}
					++it;
				}
			};*/



			/*Events::vehicleDtorEvent += [](CVehicle* veh)
			{
					for (auto it = VehicleSirenSources.begin(); it != VehicleSirenSources.end(); ) {
						CVehicle* veh = it->first;
						ALuint source = it->second;

						if (!veh || !veh->m_vehicleAudio.m_pSirenSound) {
							alSourceStop(source);
							alDeleteSources(1, &source);
							it = VehicleSirenSources.erase(it);
						}
						else {
							++it;
						}
					}

					for (auto it = VehicleMainSirenSources.begin(); it != VehicleMainSirenSources.end(); ) {
						CVehicle* veh = it->first;
						ALuint source = it->second;

						if (!veh || !veh->m_vehicleAudio.m_pPoliceSirenSound) {
							alSourceStop(source);
							alDeleteSources(1, &source);
							it = VehicleMainSirenSources.erase(it);
						}
						else {
							++it;
						}
					}
			};*/
		Events::processScriptsEvent += [] {
			if (initializationstatus != -2) return;
			initializationstatus = 0;

			Events::gameProcessEvent += [] {
				//	Log("CWeather::WeatherRegion: %d", CWeather::WeatherRegion);
				if (initializationstatus == 0) {
					if (!device || !context) {
						modMessage("Could not initialize OpenAL audio engine.");
						initializationstatus = -1;
					}
				}
				// Experiments :D
				/*auto playa = FindPlayerPed();
				if (playa) {
					auto shooting = FindPlayerPed()->m_pIntelligence->GetTaskUseGun();
					if (shooting && shooting->m_nFireGunThisFrame) {
						float recoilMult = 1.0f;
						auto weaponType = playa->m_aWeapons[playa->m_nSelectedWepSlot].m_eWeaponType;

						if (weaponType == WEAPONTYPE_AK47)
							recoilMult += 0.5f;

						if (weaponType == WEAPONTYPE_PISTOL_SILENCED)
							recoilMult += 0.4f;

						if (playa->m_nPedFlags.bIsDucking)
							recoilMult -= 0.5f;

						TheCamera.m_aCams[TheCamera.m_nActiveCam].m_fVerticalAngle += CGeneral::GetRandomNumberInRange(0.001f * recoilMult, 0.005f * recoilMult);
						TheCamera.m_aCams[TheCamera.m_nActiveCam].m_fHorizontalAngle += CGeneral::GetRandomNumberInRange(-0.005f * recoilMult, 0.005f * recoilMult);
					}
				}*/

				// Don't update any fire sound if the game is paused
				if (!FrontEndMenuManager.m_bMenuActive)
				{
					UpdateFireSoundCleanup();
					// Play the ambience when it's not rainy and the screen didn't fade out yet
					if (CWeather::Rain <= 0.0f && TheCamera.GetScreenFadeStatus() == 0)
					{
						PlayAmbienceSFX(cameraposition, eWeaponType(0), true);
					}
				}
				// Install hooks and load weapon sounds
				if (initializationstatus != -1) {
					if (initializationstatus == 0) {

						LoadMainWeaponsFolder();

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
						Events::gameProcessEvent += [] {
							for (auto& inst : audiosplaying)
							{
								// This gave me PTSD, it created one bug to another, and i managed to get it fixed
								// So i prefer to not touch it anymore and leave it as it is
								if (inst->source == barrelSpinSource && inst->shooter && inst->minigunBarrelSpin)
								{
									ALint spinState;
									alGetSourcei(inst->source, AL_SOURCE_STATE, &spinState);
									eWeaponType weaponType = inst->shooter->m_aWeapons[inst->shooter->m_nSelectedWepSlot].m_eWeaponType;
									CTaskSimpleUseGun* shooting = inst->shooter->m_pIntelligence->GetTaskUseGun();
									bool gunTaskFinished = shooting && shooting->m_bIsFinished;
									bool shouldStop = (weaponType != inst->WeaponType || (gunTaskFinished) || inst->shooter->m_ePedState == PEDSTATE_DEAD || inst->shooter->m_fHealth <= 0.0f);
									if (spinState == AL_PLAYING && shouldStop)
									{
										//	inst->shooter->m_weaponAudio.PlayMiniGunStopSound(inst->shooter);
										PlayOrStopBarrelSpinSound(inst->shooter, &weaponType, false, gunTaskFinished ? true : false);
										//	Log("Gargle");
									}
								}
								// We update each source's gain so when the screen fades, the sound can fade smoothly as well
								ALint state;
								alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
								if (state == AL_PLAYING)
								{
									float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
									float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
									float gain = (gameVol <= 0.0f) ? 0.0f : inst->baseGain * fader;
									alSourcef(inst->source, AL_GAIN, gain);
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
									PauseAudio(inst.get());
								}
								else
								{
									ResumeAudio(inst.get());
								}
								AttachReverbToSource(inst.get()->source);
							}
							};
					}
					// The listener is the player, set appropriate values...
					CVector pos = *TheCamera.GetGameCamPosition();
					cameraposition = pos;
					alListener3f(AL_POSITION, cameraposition.x, cameraposition.y, cameraposition.z);
					// For correct sound panning we use camera's heading, ensuring convincing 3D audio
					ALfloat orientation[6] =
					{
					TheCamera.GetMatrix()->GetForward().x, TheCamera.GetMatrix()->GetForward().y, TheCamera.GetMatrix()->GetForward().z, // forward vector
					TheCamera.GetMatrix()->GetUp().x, TheCamera.GetMatrix()->GetUp().y,  TheCamera.GetMatrix()->GetUp().z // up vector
					};
					alListenerfv(AL_ORIENTATION, orientation);
					audiosplaying.erase(remove_if(audiosplaying.begin(), audiosplaying.end(),
						[&](const std::shared_ptr<SoundInstance>& inst) {
							ALint state;
							alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
							/*if (efxSupported)
							{
								if (CWeather::InTunnelness > 0.0f || CCullZones::InRoomForAudio())
								{
									AttachReverbToSource(inst->source, false, EAXOrNot);
								}
								else {
									AttachReverbToSource(inst->source, true, EAXOrNot);
								}
							}*/
							// If attached to some entity, match it's velocity for Doppler effect
							/*if (inst->shooter) {
								alSource3f(inst->source, AL_VELOCITY,
									inst->shooter->m_vecMoveSpeed.x,
									inst->shooter->m_vecMoveSpeed.y,
									inst->shooter->m_vecMoveSpeed.z);
							}
							else if (inst->entity) {
								alSource3f(inst->source, AL_VELOCITY,
									inst->entity->m_vecMoveSpeed.x,
									inst->entity->m_vecMoveSpeed.y,
									inst->entity->m_vecMoveSpeed.z);
							}*/

							// Update missile sound position and velocity
							if (inst->entity && inst->missileSource != 0 && inst->source == inst->missileSource) {
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
							if (!inst->isFire && state == AL_PLAYING) {
								if (inst->entity) {
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
								else if (inst->shooter)
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

							if (inst->shooter && inst->minigunBarrelSpin && inst->source != 0)
							{
								alSource3f(inst->source, AL_POSITION, inst->shooter->GetPosition().x, inst->shooter->GetPosition().y, inst->shooter->GetPosition().z);
							}

							// To prevent overflow, we erase any sources that are no longer used
							if (/*(*/state != AL_PLAYING && state != AL_PAUSED/*) || (!alIsSource(inst->source) || inst->source == 0)*/) {
								//	Log("Removed source '%d'", inst->source);
								alDeleteSources(1, &inst->source);
								inst->source = 0;
								//	alDeleteFilters(1, &inst->filter);
									//delete inst;
								return true;
							}
							return false;
						}), audiosplaying.end());
				}

				if (initializationstatus == 0) initializationstatus = 1;
				};
			};
		// Shut down everything
		shutdownGameEvent += []()
			{
				// Free OpenAL stuff on shutdown
				Log("Shutting down OpenAL...");
				Log("Freeing buffers and sources...");
				// Stop and delete all currently playing sound sources
				for (auto& inst : audiosplaying)
				{
					if (inst->source != 0) {
						ALint state = AL_STOPPED;
						alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
						alSourcei(inst->source, AL_BUFFER, AL_NONE); // Detach buffer from source
						if (state == AL_PLAYING || state == AL_PAUSED) {
							PauseAudio(&*inst);
						}
						Log("Removing source '%u', missile '%u', minigun spin '%u'", inst->source, inst->missileSource, barrelSpinSource);
						alDeleteSources(1, &inst->source);
						if (inst->missileSource)
						{
							alDeleteSources(1, &inst->missileSource);
						}
						if (barrelSpinSource)
						{
							alDeleteSources(1, &barrelSpinSource);
						}
						if (barrelSpinBuffer)
						{
							alDeleteBuffers(1, &barrelSpinBuffer);
						}
						inst->source = 0;
						barrelSpinSource = 0;
						barrelSpinBuffer = 0;
					}
					inst->isAmbience = false;
					inst->isGunfireAmbience = false;
					inst->isInteriorAmbience = false;
					inst->isPossibleGunFire = false;
				}
				audiosplaying.clear();  // Remove all sound instances

				//Delete all loaded WAV buffers
				for (auto& buf : gBufferMap) {
					if (buf.second != 0) {
						Log("Freeing buffer %u, name: %s", buf.second, buf.first.c_str());
						alDeleteBuffers(1, &buf.second);
						buf.second = 0;
					}
				}
				gBufferMap.clear();

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

				// Remove every single OpenAL buffer
				DeleteAllBuffers(g_Buffers);
				registeredweapons.clear();
				weaponNames.clear();
				Log("Freeing buffers and sources complete.");

				Log("Closing OpenAL device and context...");
				alcCloseDevice(device);
				device = nullptr;
				alcSuspendContext(context);
				alcMakeContextCurrent(nullptr);
				alcDestroyContext(context);
				context = nullptr;
				Log("OpenAL device and context closed.");
				Log("Shut down complete. See ya next time! :)");
				initializationstatus = -2;
			};

		ClearForRestartEvent += []()
			{
				//	ReloadAudioFolders();

				// Reset ambience stuff on reload to prevent "never playing" issues
				nextInteriorAmbienceTime = 0;
				nextZoneAmbienceTime = 0;
				nextFireAmbienceTime = 0;
				for (auto& inst : audiosplaying)
				{
					if (inst->isAmbience) {
						if (inst->source != 0) {
							ALint state = AL_STOPPED;
							alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
							if (state == AL_PLAYING || state == AL_PAUSED) {
								PauseAudio(&*inst);
							}
							alDeleteSources(1, &inst->source);
							inst->source = 0;
						}
						inst->isAmbience = false;
						inst->isGunfireAmbience = false;
						inst->isInteriorAmbience = false;
					}
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
				for (auto& inst : audiosplaying)
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
				for (auto& inst : audiosplaying)
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
				for (auto& inst : audiosplaying) {
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
extern "C" __declspec(dllexport) ALCcontext * GetContext()
{
	return context;
}
