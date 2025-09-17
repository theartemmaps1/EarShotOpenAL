#include "plugin.h"
#include "AudioManager.h"
#include "logging.h"
#include "ourCommon.h"
#include "IniReader.h"
#include "Loaders.h"
#include <CGame.h>
#include <CClock.h>
#include <CGameLogic.h>
#include <CMenuManager.h>
#include <CEntryExitManager.h>
#include <CTheZones.h>
#include <CCutsceneMgr.h>

CAudioManager AudioManager;

// Is FX supported?
bool CAudioManager::efxSupported = false;

// Main array to manage ALL currently playing sounds
std::vector<std::shared_ptr<SoundInstance>> CAudioManager::audiosplaying;
map<string, ALuint> CAudioManager::gBufferMap;

using namespace plugin;
void CAudioManager::Initialize()
{
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
	pDevice = nullptr;
	pContext = nullptr;

	pDevice = alcOpenDevice(nullptr);
	if (!pDevice) {
		Log("Could not open OpenAL device!");
		modMessage("Could not open OpenAL device!");
		return;
	}

	if (pDevice) {
		auto deviceStr = alcGetString(pDevice, ALC_DEVICE_SPECIFIER);
		Log("Opened playback device: '%s'", deviceStr);
		Log("Device and context created successfully...");
		//ALCint attrs[] = { ALC_MONO_SOURCES, 1, ALC_STEREO_SOURCES, 64, 0 };
		pContext = alcCreateContext(pDevice, nullptr);
		alcMakeContextCurrent(pContext);
	}

	alListenerf(AL_GAIN, 1.0f);
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	//alDopplerFactor(1.5f);

	// Check if we support sound FX, if not, skill issue, BUY A NEW SOUND CARD!!!!!!!!!!!!!!
	efxSupported = alcIsExtensionPresent(pDevice, (ALCchar*)ALC_EXT_EFX_NAME);
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
	Loaders::LoadExplosionRelatedSounds(foldermod);
	Loaders::LoadJackingRelatedSounds(foldermod);
	Loaders::LoadFireSounds(foldermod);
	Loaders::LoadAmbienceSounds(foldermod);
	Loaders::LoadRicochetSounds(foldermod);
	Loaders::LoadFootstepSounds(foldermod);
	Loaders::LoadTankCannonSounds(foldermod);
	Loaders::LoadMissileSounds(foldermod);
	Loaders::LoadBulletWhizzSounds(foldermod);
	Loaders::LoadMinigunBarrelSpinSound(foldermod);
}

void CAudioManager::Shutdown()
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
				PauseSource(&*inst);
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
	alcCloseDevice(pDevice);
	pDevice = nullptr;
	alcSuspendContext(pContext);
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(pContext);
	pContext = nullptr;
	Log("OpenAL device and context closed.");
	Log("Shut down complete. See ya next time! :)");
	initializationstatus = -2;
}

// We can use this to read WAV files, no external libraries needed
WAVData CAudioManager::LoadWAV(const char* filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		Error("Failed to open WAV file: %s", filename);
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
		Error("Only mono or stereo WAV supported: %s", filename);
	}

	WAVData wav = { formatEnum, static_cast<ALsizei>(sampleRate), std::move(data) };
	file.close(); // close it when we're done with it
	return wav;
}

ALuint CAudioManager::CreateOpenALBufferFromWAV(const char* filename) {
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
	WAVData data = LoadWAV(fn.c_str());

	ALuint buff = 0;
	alGenBuffers(1, &buff);
	alBufferData(buff, data.format, data.data.data(), (ALsizei)data.data.size(), data.freq);
	gBufferMap.emplace(fn, buff);
	return buff;
}

// To avoid constant OpenAL blocks, we use this func for everything.
// Used to play 3D sounds in a 3D space.
// For 2D sounds we use the other func called "playBuffer2D".
// Note that this plays the sound only ONCE and looping is only done for missiles/fire/minigun barrel here and managed separately.
// @returns true on success, false otherwise.
bool CAudioManager::PlaySource(ALuint buff,
	float maxDist,
	float gain,
	float airAbsorption,
	float refDist,
	float rollOffFactor, float pitch,
	CVector pos, bool isFire,
	CFire* firePtr,
	int fireEventID, std::shared_ptr<SoundInstance>* outInst,
	CPhysical* ent, bool isPossibleGunFire,
	fs::path* path, std::string nameBuff,
	bool isMinigunBarrelSpin, CPed* shooter, eWeaponType weapType, bool isGunfire,
	bool isInteriorAmbience, bool isMissile, bool looping, uint32_t delay, bool isAmbience, FxSystem_c* fireFX)
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
	ALboolean useLooping = AL_FALSE;
	if (isFire || isMissile || looping)
	{
		useLooping = AL_TRUE;
	}
	alSourcei(inst->source, AL_BUFFER, buff);
	alSource3f(inst->source, AL_POSITION, pos.x, pos.y, pos.z);
	alSourcef(inst->source, AL_GAIN, gain);
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
	inst->baseGain = gain;

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
		inst->isFire = isFire;
		inst->fireFX = fireFX;
		inst->entity = nullptr;
		inst->shooter = nullptr;
		if (firePtr) {
			inst->firePtr = firePtr;
			g_Buffers.fireSounds[firePtr] = inst;
		}
	}
	else if (fireEventID != 0) {
		inst->isFire = false;
		inst->firePtr = nullptr;
		inst->entity = nullptr;
		inst->shooter = nullptr;
		g_Buffers.nonFireSounds[fireEventID] = inst;
	}
	if (inst)
	{
		audiosplaying.push_back(std::move(inst));
		return true;
	}
	return false;
}

void CAudioManager::AudioPlay(fs::path* audiopath, CPhysical* audioentity) {
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

	float dist = DistanceBetweenPoints(cameraposition, pos);

	ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(audiopath->string().c_str());
	if (buffer == 0) {
		modMessage("Could not play " + outputPath(audiopath));
		return;
	}

	CPed* ped = (CPed*)audioentity;
	auto inst = std::make_shared<SoundInstance>();

	// for vehicle guns we make the sound a bit louder by changing it's distance attenuation
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;

	float gain = gameVol * fader;
	AudioManager.PlaySource(buffer, veh ? 125.0f : 100.0f,
		gain, veh ? 1.5f : 2.5f,
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

	// Special cases
	if (Reloads) {
		AudioManager.SetSourceMaxDist(inst->source, 500.0f);
		AudioManager.SetSourceRefDist(inst->source, 2.0f);
		AudioManager.SetSourceRolloffFactor(inst->source, 3.0f);
	}
	else if (NeedsToBeQuieter) {
		AudioManager.SetSourceMaxDist(inst->source, 500.0f);
		AudioManager.SetSourceRefDist(inst->source, 2.0f);
	}
	else if (Distant) {
		AudioManager.SetSourceMaxDist(inst->source, 300.0f);
		AudioManager.SetSourceRefDist(inst->source, 3.0f);
	}

	// With each lower ammo in the clip, the sound get's louder
	if (isLowAmmo && ped && ped->GetWeapon()) {
		int ammoInClip = ped->GetWeapon()->m_nAmmoInClip;
		unsigned short ammoClip = CWeaponInfo::GetWeaponInfo(ped->GetWeapon()->m_eWeaponType, WEAPSKILL_STD)->m_nAmmoClip;
		int left = (ammoClip / 3);
		ammoInClip = std::max(1, std::min(ammoInClip, left));

		float gainMultiplier = 0.1f + (float(left) - ammoInClip) * 0.1f;

		float finalGain = AEAudioHardware.m_fEffectMasterScalingFactor * gainMultiplier;
		AudioManager.SetSourceGain(inst->source, finalGain);
		//alSourcef(inst->source, AL_GAIN, finalGain);
	}
}

bool CAudioManager::findWeapon(eWeaponType* weapontype, eModelID modelid, std::string filename, CPhysical* audioentity, bool playAudio)
{
	auto key = std::make_pair(*weapontype, modelid);
	auto it = registeredweapons.find(key);
	fs::path path;
	if (it == registeredweapons.end())
	{
		Log("registeredweapons == end");
		return false;
	}
	// if we don't want to play any audio, just check for file's existence instead
	if (!playAudio) {
		//	Log("playAudio = false");
		return it->second.audioPath(filename, path);
	}
	//Log("audioPlay");
	return it->second.audioPlay(filename, audioentity);
}

bool CAudioManager::PlayAmbienceBuffer(ALuint buffer, const CVector& origin, bool isGunfire, bool isThunder, bool isInteriorAmbience)
{
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
	if (AudioManager.PlaySource(buffer, 250.0f, VolumeToUse, 1.0f, isThunder ? 6.0f : (isInteriorAmbience ? 15.0f : 5.5f), 1.0f, pitch, pos,
		false, nullptr, 0, nullptr, nullptr, false, nullptr, "", false, nullptr, eWeaponType(0), isGunfire, isInteriorAmbience, false, false, 0, true))
	{
		Log("PlayAmbienceBuffer returned true");
		return true;
	}
	Log("PlayAmbienceBuffer returned false");
	return false;
}

bool CAudioManager::PlayAmbienceSFX(const CVector& origin, eWeaponType weaponType, bool useOldAmbience) {
	static int prevCurrArea = CGame::currArea; // track area transitions
	static uint32_t lastPrintedInteriorID = 0;
	bool isNight = CClock::ms_nGameClockHours >= 20 || CClock::ms_nGameClockHours < 6;
	bool isRiot = CGameLogic::LaRiotsActiveHere();
	// --- 0) Detect interior->interior switch (warp while staying inside) ---
	if (prevCurrArea > 0 && CGame::currArea > 0 && prevCurrArea != CGame::currArea) {
		// We changed interiors while still "inside" -> stop old interior ambience(s) and allow new to play
		nextInteriorAmbienceTime = CTimer::m_snTimeInMilliseconds; // allow immediate play

		for (auto& sfx : AudioManager.audiosplaying) {
			if (!sfx) continue;
			if (sfx->isInteriorAmbience) {
				// stop & delete old source to avoid blocking new ambience
				if (sfx->source != 0) {
					ALint st = AL_STOPPED;
					alGetSourcei(sfx->source, AL_SOURCE_STATE, &st);
					if (st == AL_PLAYING || st == AL_PAUSED) {
						AudioManager.PauseSource(&*sfx);
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

		for (auto& sfx : AudioManager.audiosplaying) {
			if (!sfx) continue;
			if (sfx->isInteriorAmbience) {
				if (sfx->source != 0) {
					ALint state = 0;
					alGetSourcei(sfx->source, AL_SOURCE_STATE, &state);
					if (state == AL_PLAYING || state == AL_PAUSED) {
						AudioManager.PauseSource(&*sfx);
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
	for (auto& sfx : AudioManager.audiosplaying) {
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
				AudioManager.ResumeSource(&*sfx);
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
				for (auto& interiorAmbiences : AudioManager.audiosplaying)
				{
					if (!interiorAmbiences) continue;
					ALint state = AL_STOPPED;
					if (interiorAmbiences->source != 0) {
						alGetSourcei(interiorAmbiences->source, AL_SOURCE_STATE, &state);
					}
					if (interiorAmbiences->isInteriorAmbience && (state == AL_PLAYING || state == AL_PAUSED))
					{
						AudioManager.PauseSource(&*interiorAmbiences);
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
				bool fireAmbiencePlaying = std::any_of(AudioManager.audiosplaying.begin(), AudioManager.audiosplaying.end(), [&](const std::shared_ptr<SoundInstance>& sfx) {
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


void CAudioManager::PlayOrStopBarrelSpinSound(CPed* entity, eWeaponType* weapontype, bool spinning, bool playSpinEndSFX)
{
	//if (!shooter || !ent) return;

//CPed* ped = reinterpret_cast<CPed*>(shooter);
	CPlayerPed* playa = reinterpret_cast<CPlayerPed*>(entity);
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float deltaTime = CTimer::ms_fTimeStep;
	//	CPed* entity = ped;
	auto weaponType = entity->m_aWeapons[entity->m_nSelectedWepSlot].m_eWeaponType;
	auto instance = std::make_shared<SoundInstance>();
	if (spinning) {
		// Start playing if not already
		if (barrelSpinSource == 0) {
			if (AudioManager.PlaySource(barrelSpinBuffer, FLT_MAX, AEAudioHardware.m_fEffectMasterScalingFactor, 1.0f, 1.0f, 1.0f, pitch, entity->GetPosition(), false, nullptr, 0, &instance, entity,
				false, nullptr, "", true, entity, weaponType, false, false, false, true))
			{
				barrelSpinSource = instance->source;
				Log("Playing spin = true");
			}
			Log("Playing spin = false");
			barrelSpinVolume = 0.0f;
		}

		// Fade in
		if (barrelSpinVolume < 1.0f) {
			barrelSpinVolume += deltaTime / barrelFadeDuration;
			if (barrelSpinVolume > 1.0f) barrelSpinVolume = 1.0f;
			AudioManager.SetSourceGain(barrelSpinSource, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
			//alSourcef(barrelSpinSource, AL_GAIN, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
		}
	}
	else {
		// Fade out
		if (barrelSpinVolume > 0.0f) {
			barrelSpinVolume -= deltaTime / barrelFadeDuration;
			if (barrelSpinVolume < 0.0f) barrelSpinVolume = 0.0f;
			AudioManager.SetSourceGain(barrelSpinSource, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
			//alSourcef(barrelSpinSource, AL_GAIN, AEAudioHardware.m_fEffectMasterScalingFactor * barrelSpinVolume);
		}

		// Once volume is faded out, stop source and play SPINEND
		if (barrelSpinVolume <= 0.1f && barrelSpinSource != 0) {
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

// Initialize reverb...
void CAudioManager::InitReverb() {
	if (!pDevice || !pContext || !efxSupported) return;

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

void CAudioManager::PauseSource(SoundInstance* audio)
{
	if (!audio->paused) {
		alGetSourcef(audio->source, AL_SEC_OFFSET, &audio->pauseOffset);
		alSourcePause(audio->source);
		audio->paused = true;
	}
}

void CAudioManager::ResumeSource(SoundInstance* audio)
{
	if (audio->paused) {
		alSourcef(audio->source, AL_SEC_OFFSET, audio->pauseOffset);
		alSourcePlay(audio->source);
		audio->paused = false;
	}
}

void CAudioManager::AttachReverbToSource(ALuint source, bool detach/*, bool EAX*/) {
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

bool CAudioManager::PlaySource2D(ALuint buff, bool relative, float volume, float pitch)
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
}

bool CAudioManager::SetSourceGain(ALuint source, float gain)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_GAIN, gain);
	return true;
}

bool CAudioManager::SetSourceMaxDist(ALuint source, float maxDist)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_MAX_DISTANCE, maxDist);
	return true;
}

bool CAudioManager::SetSourceRefDist(ALuint source, float ref)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_REFERENCE_DISTANCE, ref);
	return true;
}

bool CAudioManager::SetSourceRolloffFactor(ALuint source, float factor)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_ROLLOFF_FACTOR, factor);
	return true;
}

bool AudioStream::audioPath(std::string filename, fs::path& outPath)
{
	fs::path candidate = audiosfolder / fs::path(filename).replace_extension(".wav");
	if (fs::exists(candidate)) {
		auto inst = std::make_shared<SoundInstance>();
		inst->path = candidate;
		outPath = std::move(candidate);
		AudioManager.audiosplaying.push_back(std::move(inst));
		return true;
	}
	return false;
}

bool AudioStream::audioPlay(std::string filename, CPhysical* audioEntity)
{
	std::vector<fs::path> alternatives;
	const std::string& baseName = filename;
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
			AudioManager.audiosplaying.push_back(std::move(inst));
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
	AudioManager.AudioPlay(&selected, audioEntity);

	return true;
}