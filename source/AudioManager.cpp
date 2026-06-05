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
#include "vendor/dr_libs/dr_wav.h"
#include "vendor/dr_libs/dr_flac.h"
#include "vendor/dr_libs/dr_mp3.h"
#include <stb_vorbis.c>
#include "CSphere.h"
#include <CFireManager.h>
CAudioManager AudioManager;

// Is FX supported?
bool CAudioManager::efxSupported = false;

// Main array to manage ALL currently playing sounds
std::vector<std::shared_ptr<SoundInstance>> CAudioManager::audiosplaying;
map<fs::path, ALuint> CAudioManager::gBufferMap;
const float CAudioManager::barrelFadeDuration = 0.5f;
// With the other static definitions at the top
std::vector<DelayedSoundEntry> CAudioManager::scheduledSounds;

ALuint CAudioManager::barrelSpinSource = 0;
float CAudioManager::barrelSpinVolume = 0.0f;
 
std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 5>> CAudioManager::m_apChainsawSounds;
std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 3>> CAudioManager::m_apFlamethrowerSounds;
std::unordered_map<CPed*, std::shared_ptr<SoundInstance>> CAudioManager::m_apSpraycanSounds;
std::unordered_map<CPed*, std::shared_ptr<SoundInstance>> CAudioManager::m_apFireextinguisherSounds;
// array of sounds, 0 - fire, 1 - barrel spin
std::unordered_map<CPed*, std::array<std::shared_ptr<SoundInstance>, 2>> CAudioManager::m_apMinigunSound;
std::unordered_map<CVehicle*, std::array<std::shared_ptr<SoundInstance>, 3>> CAudioManager::m_apSirens;


std::vector<ManualAmbience> g_ManualAmbiences;

using namespace plugin;
namespace fs = std::filesystem;
void CAudioManager::Initialize()
{
	ClearLogFile();

	LOG("Compiling date and time %s @ %s", __DATE__, __TIME__);
	// Init OpenAL
	LOG("Initializing OpenAL...");
	pDevice = nullptr;
	pContext = nullptr;

	pDevice = alcOpenDevice(nullptr);
	if (pDevice) {
		const ALCchar* deviceStr = alcGetString(pDevice, ALC_DEVICE_SPECIFIER);
		LOG("Opened playback device: '%s'", deviceStr);
		LOG("Device and context created successfully...");
		//ALCint attrs[] = { ALC_MONO_SOURCES, 1, ALC_STEREO_SOURCES, 64, 0 };
		pContext = alcCreateContext(pDevice, nullptr);
		alcMakeContextCurrent(pContext);
		ALenum error = alGetError();
		if (!pContext) {
			LOG("Could not create OpenAL context! Error: %s", OpenALErrorCodeToString(error).c_str());
			modMessage("Could not create OpenAL context! Error:" + OpenALErrorCodeToString(error));
			return;
		}
	}
	else {
		ALenum error = alGetError();
		if (error != AL_NO_ERROR) {
			LOG("Could not open OpenAL device! Error: %s", OpenALErrorCodeToString(error).c_str());
			modMessage("Could not open OpenAL device! Error:" + OpenALErrorCodeToString(error));
			return;
		}
	}

	alListenerf(AL_GAIN, 1.0f);
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	//alDopplerFactor(1.5f);

	// Check if we support sound FX, if not, skill issue, BUY A NEW SOUND CARD!!!!!!!!!!!!!!
	efxSupported = alcIsExtensionPresent(pDevice, (ALCchar*)ALC_EXT_EFX_NAME);
	LOG("Initializing sounds FX...");
	if (!efxSupported) {
		LOG("EFX extension not supported, therefore no fancy sound FX, soz :shrug:\n");
		modMessage("EFX extension not supported, therefore no fancy sound FX, soz :shrug:\n");
		//return;
	}
	if (efxSupported)
	{
		InitReverb();
		//InitEcho();
		LOG("Sound FX initialized...");
	}
	else {
		LOG("Sound FX initialization failed...");
	}

	// Load everything
	Loaders::LoadExplosionRelatedSounds(foldermod);
	Loaders::LoadJackingRelatedSounds(foldermod);
	Loaders::LoadFireSounds(foldermod);
	Loaders::LoadAmbienceSounds(foldermod);
	Loaders::LoadRicochetSounds(foldermod);
	Loaders::LoadGunshellSounds(foldermod);
	Loaders::LoadFootstepSounds(foldermod);
	Loaders::LoadTankCannonSounds(foldermod);
	Loaders::LoadMissileSounds(foldermod);
	Loaders::LoadBulletWhizzSounds(foldermod);
	Loaders::LoadMinigunSounds(foldermod);
	Loaders::LoadChainsawSounds(foldermod);
	Loaders::LoadFlamethrowerSounds(foldermod);
	Loaders::LoadSpraycanSound(foldermod);
	Loaders::LoadExtinguisherSound(foldermod);
	Loaders::LoadCameraAndGoggleSounds(foldermod);
	Loaders::LoadCarSirenSounds(foldermod);
	Loaders::LoadGrenadeBounceSounds(foldermod);
}

void CAudioManager::Shutdown()
{
	// Free OpenAL stuff on shutdown
	LOG("Shutting down OpenAL...");
	LOG("Freeing buffers and sources...");
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
			LOG("Removing source '%u', missile '%u', minigun spin '%u'", inst->source, inst->missileSource, barrelSpinSource);
			alDeleteSources(1, &inst->source);
			if (inst->missileSource)
			{
				alDeleteSources(1, &inst->missileSource);
			}
			if (barrelSpinSource)
			{
				alDeleteSources(1, &barrelSpinSource);
			}
			//if (barrelSpinBuffer)
			//{
			//	alDeleteBuffers(1, &barrelSpinBuffer);
			//}
			inst->source = 0;
			barrelSpinSource = 0;
			//barrelSpinBuffer = 0;
		}
		inst->isAmbience = false;
		inst->isGunfireAmbience = false;
		inst->isPossibleGunFire = false;
	}
	audiosplaying.clear();  // Remove all sound instances

	//Delete all loaded WAV buffers
	for (auto& buf : gBufferMap) {
		if (buf.second != 0) {
			LOG("Freeing buffer %u, path: %s", buf.second, buf.first.string().c_str());
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
	gentInfo.clear();
	gvehInfo.clear();
	UnloadManualAmbiences();
	LOG("Freeing buffers and sources complete.");

	LOG("Closing OpenAL device and context...");
	alcCloseDevice(pDevice);
	pDevice = nullptr;
	alcSuspendContext(pContext);
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(pContext);
	pContext = nullptr;
	LOG("OpenAL device and context closed.");
	LOG("Shut down complete. See ya next time! :)");
}

void CAudioManager::ScheduleDelayedSound(ALuint buffer, const SoundInstanceSettings& opts, float delaySeconds)
{
	if (!alIsBuffer(buffer)) return;
	float clamped = std::min(delaySeconds, 5.0f); // cap at 5 s; beyond that the player won't notice
	uint32_t playAt = CTimer::m_snTimeInMilliseconds + static_cast<uint32_t>(clamped * 1000.0f);
	scheduledSounds.push_back({ buffer, opts, playAt });
	LOG("Scheduled sound in %.0f ms (dist %.1f m)", clamped * 1000.0f, delaySeconds * 343.0f);
}

void CAudioManager::ProcessScheduledSounds()
{
	if (scheduledSounds.empty()) return;
	uint32_t now = CTimer::m_snTimeInMilliseconds;
	scheduledSounds.erase(
		std::remove_if(scheduledSounds.begin(), scheduledSounds.end(),
			[&](const DelayedSoundEntry& e) -> bool {
				if (now >= e.playAtTime) {
					PlaySource(e.buffer, e.opts);
					return true;
				}
				return false;
			}),
		scheduledSounds.end());
}

AudioData CAudioManager::DecodeWAV(const std::string& path)
{
	drwav wav;
	if (!drwav_init_file(&wav, path.c_str(), NULL)) {
		LOG("%s: Failed to init WAV %s", __FUNCTION__, path.c_str());
		return {};
	}

	drwav_uint64 totalFrames = wav.totalPCMFrameCount;
	unsigned int channels = wav.channels;
	unsigned int sampleRate = wav.sampleRate;
	unsigned int bitsPerSample = wav.bitsPerSample;

	if (totalFrames == 0 || channels == 0) {
		drwav_uninit(&wav);
		return {};
	}

	// check overflow: totalFrames * channels must fit in size_t
	uint64_t totalSamples64 = totalFrames * (uint64_t)channels;
	if (totalSamples64 > std::numeric_limits<size_t>::max()) {
		drwav_uninit(&wav);
		return {};
	}

	std::vector<float> buffer;
	buffer.resize(static_cast<size_t>(totalSamples64));

	drwav_uint64 framesRead = drwav_read_pcm_frames_f32(&wav, totalFrames, buffer.data());
	drwav_uninit(&wav);

	if (framesRead == 0) {
		return {};
	}

	buffer.resize(static_cast<size_t>(framesRead) * channels);

	AudioData out;
	out.channels = channels;
	out.sampleRate = sampleRate;
	out.bitsPerSample = bitsPerSample; // source bits per sample
	out.samples = std::move(buffer);

	return out;
}

AudioData CAudioManager::DecodeMP3(const std::string& path)
{
	drmp3_config config{};
	drmp3_uint64 totalFrames = 0;

	float* pData = drmp3_open_file_and_read_pcm_frames_f32(path.c_str(), &config, &totalFrames, NULL);
	if (!pData || totalFrames == 0 || config.channels == 0) {
		if (pData) drmp3_free(pData, NULL);
		return {};
	}

	uint64_t totalSamples64 = totalFrames * (uint64_t)config.channels;
	if (totalSamples64 > std::numeric_limits<size_t>::max()) {
		drmp3_free(pData, NULL);
		return {};
	}

	std::vector<float> buffer;
	buffer.assign(pData, pData + static_cast<size_t>(totalSamples64));
	drmp3_free(pData, NULL);

	AudioData out;
	out.channels = config.channels;
	out.sampleRate = config.sampleRate;
	out.bitsPerSample = 32u; // decoded as float32
	out.samples = std::move(buffer);

	return out;
}

AudioData CAudioManager::DecodeFLAC(const std::string& path)
{
	unsigned int channels = 0, sampleRate = 0;
	drflac_uint64 totalFrames = 0;

	float* pData = drflac_open_file_and_read_pcm_frames_f32(path.c_str(), &channels, &sampleRate, &totalFrames, NULL);
	if (!pData || totalFrames == 0 || channels == 0) {
		if (pData) drflac_free(pData, NULL);
		return {};
	}

	uint64_t totalSamples64 = totalFrames * (uint64_t)channels;
	if (totalSamples64 > std::numeric_limits<size_t>::max()) {
		drflac_free(pData, NULL);
		return {};
	}

	std::vector<float> buffer;
	buffer.assign(pData, pData + static_cast<size_t>(totalSamples64));
	drflac_free(pData, NULL);

	AudioData out;
	out.channels = channels;
	out.sampleRate = sampleRate;
	out.bitsPerSample = 32u;
	out.samples = std::move(buffer);

	return out;
}

AudioData CAudioManager::DecodeOGG(const std::string& path)
{
	short* decoded = nullptr;
	int channels = 0, sampleRate = 0;

	int totalSamples = stb_vorbis_decode_filename(path.c_str(), &channels, &sampleRate, &decoded);
	if (totalSamples <= 0 || !decoded || channels <= 0) {
		if (decoded) free(decoded);
		return {};
	}

	// stb returns total samples across all channels (i.e. frames*channels)
	std::vector<float> buffer;
	buffer.resize(static_cast<size_t>(totalSamples));

	// Use clamp and convert. Dividing by 32768 maps [-32768,32767] to [-1, ~0.99997]
	for (int i = 0; i < totalSamples; ++i) {
		float v = static_cast<float>(decoded[i]) / 32768.0f;
		buffer[static_cast<size_t>(i)] = std::clamp(v, -1.0f, 1.0f);
	}

	free(decoded);

	AudioData out;
	out.channels = static_cast<unsigned int>(channels);
	out.sampleRate = static_cast<unsigned int>(sampleRate);
	out.bitsPerSample = 16u;
	out.samples = std::move(buffer);

	return out;
}

std::string CAudioManager::OpenALErrorCodeToString(ALenum error)
{
	switch (error) {
	case AL_INVALID_NAME:
		return "Invalid name (ID) passed to an AL call.";
		break;

	case AL_INVALID_ENUM:
		return "Invalid enumeration passed to AL call.";
		break;

	case AL_INVALID_VALUE:
		return "Invalid value passed to AL call.";
		break;

	case AL_INVALID_OPERATION:
		return "Illegal AL call.";
		break;

	case AL_OUT_OF_MEMORY:
		return "Not enough memory to execute the AL call.";
		break;

	default:
		return "Unknown error.";
		break;
	}
}

ALuint CAudioManager::CreateOpenALBufferFromAudioFile(const fs::path& path) {
	std::string fn{ path.string() };

	// If we already loaded this buffer, return it to prevent overflow
	auto it = gBufferMap.find(path);
	if (it != gBufferMap.end()) {
		if (alIsBuffer(it->second)) {
			LOG("Buffer already loaded for '%s', returning cached buffer.", path.string().c_str());
			return it->second;
		}
		else {
			LOG("Found invalid buffer for '%s', erasing cache entry.", path.string().c_str());
			gBufferMap.erase(it);
		}
	}
	std::string fileExtension = path.extension().string();
	AudioData data;

	fs::path p = path;

	// Decode possible audio extensions
	for (const auto& ext : extensions) {
		p.replace_extension(ext);
		if (fs::exists(p)) {
			std::string actualExtension = p.extension().string();
			if (actualExtension == ".wav") data = DecodeWAV(p.string());
			else if (actualExtension == ".mp3") data = DecodeMP3(p.string());
			else if (actualExtension == ".flac") data = DecodeFLAC(p.string());
			else if (actualExtension == ".ogg") data = DecodeOGG(p.string());
			break;
		}
	}

	ALenum format = 0;

	switch (data.channels) {
	case 1:
		format = AL_FORMAT_MONO_FLOAT32;
		break;
	case 2:
		format = AL_FORMAT_STEREO_FLOAT32;
		break;
	case 4:
		format = AL_FORMAT_QUAD32;      // AL_EXT_MCFORMATS
		break;
	case 6:
		format = AL_FORMAT_51CHN32;     // 5.1
		break;
	case 7:
		format = AL_FORMAT_61CHN32;     // 6.1
		break;
	case 8:
		format = AL_FORMAT_71CHN32;     // 7.1
		break;

	default:
		LOG("Unsupported number of channels (%d) in audio file %s", data.channels, fn.c_str());
		break;
	}

	switch (format) {
	case AL_FORMAT_MONO_FLOAT32:
		LOG("Loaded MONO %d-bit audio file %s", data.bitsPerSample, fn.c_str());
		break;

	case AL_FORMAT_STEREO_FLOAT32:
		LOG("Loaded STEREO (!!!NO 3D SPATIALIZATION!!!) %d-bit audio file %s", data.bitsPerSample, fn.c_str());
		break;

	case AL_FORMAT_QUAD32:
		LOG("Loaded QUAD 4-channel %d-bit audio file %s", data.bitsPerSample, fn.c_str());
		break;

	case AL_FORMAT_51CHN32:
		LOG("Loaded 5.1 surround %d-bit audio file %s", data.bitsPerSample, fn.c_str());
		break;

	case AL_FORMAT_61CHN32:
		LOG("Loaded 6.1 surround %d-bit audio file %s", data.bitsPerSample, fn.c_str());
		break;

	case AL_FORMAT_71CHN32:
		LOG("Loaded 7.1 surround %d-bit audio file %s", data.bitsPerSample, fn.c_str());
		break;

	default:
		LOG("Loaded UNKNOWN audio format (%d channels, %d-bit) %s",
			data.channels, data.bitsPerSample, fn.c_str());
		break;
	}


	ALuint buff = 0;
	alGenBuffers(1, &buff);
	alBufferData(buff, format, data.samples.data(), static_cast<ALsizei>(data.samples.size() * sizeof(float)), data.sampleRate);
	gBufferMap.emplace(path, buff);
	return buff;
}

// Get the buffer format based on number of channels
ALint CAudioManager::GetBufferFormat(ALuint buffer)
{
	ALint channels = 0;
	alGetBufferi(buffer, AL_CHANNELS, &channels);
	switch (channels) {
	case 1:
		return AL_FORMAT_MONO_FLOAT32;
		break;
	case 2:
		return AL_FORMAT_STEREO_FLOAT32;
		break;
	case 4:
		return AL_FORMAT_QUAD32;      // AL_EXT_MCFORMATS
		break;
	case 6:
		return AL_FORMAT_51CHN32;     // 5.1
		break;
	case 7:
		return AL_FORMAT_61CHN32;     // 6.1
		break;
	case 8:
		return AL_FORMAT_71CHN32;     // 7.1
		break;
	}
}

// Get the sources current state
ALint CAudioManager::GetSourceState(ALuint source)
{
	if (!alIsSource(source))
		return -1;

	ALint state = 0;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	//LOG("%s: Source state: %d", __FUNCTION__, state);
	return state;
}

// To avoid constant OpenAL blocks, we use this func for everything.
// Used to play 3D sounds in a 3D space.
// For 2D sounds we use the other func called "PlaySource2D".
// Note that this plays the sound only ONCE and looping is only done for missiles/fire/minigun barrel here and managed separately.
// @returns shared_ptr of the SoundInstance on success, nullptr otherwise.
std::shared_ptr<SoundInstance> CAudioManager::PlaySource(ALuint buffer, const SoundInstanceSettings& opts)
{
	// No point in continuing, there's no valid buffers!
	if (!alIsBuffer(buffer)) return nullptr;

	ALSourceHandle srcHandle;
	if (!srcHandle) return nullptr; // failed to create OpenAL source

	auto inst = std::make_shared<SoundInstance>();
	inst->source = srcHandle.id;
	inst->buffer = buffer;
	srcHandle.id = 0;

	ALboolean useLooping = (opts.isFire || opts.isMissile || opts.looping) ? AL_TRUE : AL_FALSE;
	alSourcei(inst->source, AL_BUFFER, buffer);
	alSource3f(inst->source, AL_POSITION, opts.pos.x, opts.pos.y, opts.pos.z);
	SetSourceGain(inst->source, opts.gain);
	SetSourceAirAbsorptionFactor(inst->source, opts.airAbsorption);
	if (opts.readPitchFromFile) 
	{
		LOG("Read pitch: %.2f", opts.readPitch);
		SetSourcePitch(inst->source, opts.readPitch);
	}
	else {
		LOG("Default pitch: %.2f", opts.pitch);
		SetSourcePitch(inst->source, opts.pitch);
	}
	alSourcei(inst->source, AL_LOOPING, useLooping);
	SetSourceRefDist(inst->source, opts.refDist);
	SetSourceMaxDist(inst->source, opts.maxDist);
	SetSourceRolloffFactor(inst->source, opts.rollOffFactor);
	alSourcePlay(inst->source);

	// fill SoundInstance
	if (opts.path) inst->path = *opts.path;
	if (opts.name) inst->nameBuffer = *opts.name;
	if (!inst->nameBuffer.empty()) inst->name = inst->nameBuffer.c_str();

	if (!inst->entity)
	{
		inst->entity = opts.entity;
	}

	if (!inst->shooter) 
	{
		inst->shooter = opts.shooter;
	}

	inst->pos = opts.pos;
	inst->isPossibleGunFire = opts.isPossibleGunFire;
	inst->minigunBarrelSpin = opts.isMinigunBarrelSpin;
	inst->WeaponType = opts.weaponType;
	inst->isAmbience = opts.isAmbience;
	inst->isGunfireAmbience = opts.isGunfire;
	inst->bIsMissile = opts.isMissile;
	inst->baseGain = opts.gain;
	inst->isChainsawSound = opts.isChainsawSound;
	inst->readPitch = opts.readPitchFromFile;
	inst->pitch = opts.pitch;
	if (inst->bIsMissile) inst->missileSource = inst->source;

	if (opts.isFire) {
		inst->isFire = true;
		inst->fireFX = opts.fireFX;
		if (opts.firePtr) {
			inst->firePtr = opts.firePtr;
			g_Buffers.fireSounds[opts.firePtr] = inst;
		}
	}
	else if (opts.fireEventID != 0) {
		inst->fireFX = opts.fireFX;
		inst->firePtr = nullptr;
		g_Buffers.nonFireSounds[opts.fireEventID] = inst;
	}

	audiosplaying.push_back(inst);
	return inst;
}

// types: 0 - normal, 1 - shotgun
// exported to use in VCParticles
void CAudioManager::PlayGunshellSound(int type, eSurfaceType surface, const CVector& pos) {
	std::string surfaceType = "default";
	switch (surface) {
	case SURFACE_PED:
	case SURFACE_GORE: surfaceType = "flesh"; break;
	case SURFACE_GLASS:
	case SURFACE_GLASS_WINDOWS_LARGE: surfaceType = "glass"; break;
	default:
		if (IsAudioGrass(surface))                                              surfaceType = "grass";
		else if (IsAudioWood(surface))                                          surfaceType = "wood";
		else if (IsAudioMetal(surface))                                         surfaceType = "metal";
		else if (IsAudioSand(surface))                                          surfaceType = "sand";
		else if (IsAudioGravel(surface))                                        surfaceType = "dirt";
		else if (IsAudioConcrete(surface))                                      surfaceType = "pavement";
		else if (IsAudioWater(surface) || IsWater(surface) || IsShallowWater(surface)) surfaceType = "water";
		else if (IsAudioTile(surface))                                          surfaceType = "tile";
		break;
	}

	LOG("Current surface type for gunshell: %s, ID %d", surfaceType.c_str(), surface);

	auto& bufferMap = (type == 1)
		? g_Buffers.shotgunshellBuffersPerSurface
		: g_Buffers.gunshellBuffersPerSurface;

	std::vector<ALuint>* selected = nullptr;
	auto it = bufferMap.find(surfaceType);
	if (it != bufferMap.end() && !it->second.empty()) {
		selected = &it->second;
	}
	else {
		auto defIt = bufferMap.find("default");
		if (defIt != bufferMap.end() && !defIt->second.empty())
			selected = &defIt->second;
	}

	if (!selected || selected->empty()) {
		LOG("No gunshell buffer found for surface: %s, type: %d", surfaceType.c_str(), type);
		return;
	}

	ALuint buffer = selected->at(rand() % selected->size());

	SoundInstanceSettings opts;
	std::optional<float> whatToChoose = std::nullopt;
	float maxDist = 50.0f;
	float refDist = 1.0f;
	float rollOff = 1.0f;
	float airAbsorption = 0.0f;
	if (type == 1) {
		whatToChoose = gPitches.shotgunshell;
		maxDist = gAttenuationSettings.shotgunshell.maxDist;
		refDist = gAttenuationSettings.shotgunshell.refDist;
		rollOff = gAttenuationSettings.shotgunshell.rolloffFactor;
		airAbsorption = gAttenuationSettings.shotgunshell.airAbsorption;
	}
	else {
		whatToChoose = gPitches.gunshell;
		maxDist = gAttenuationSettings.gunshell.maxDist;
		refDist = gAttenuationSettings.gunshell.refDist;
		rollOff = gAttenuationSettings.gunshell.rolloffFactor;
		airAbsorption = gAttenuationSettings.gunshell.airAbsorption;
	}
	opts.pos = pos;
	opts.gain = AEAudioHardware.m_fEffectMasterScalingFactor;
	opts.pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	if (whatToChoose.has_value())
	{
		opts.readPitchFromFile = whatToChoose.has_value();
		opts.readPitch = *whatToChoose;
	}
	opts.refDist = refDist;
	opts.rollOffFactor = rollOff;
	opts.maxDist = maxDist;
	opts.airAbsorption = airAbsorption;

	PlaySource(buffer, opts);
}

// Main weapon sounds handling func
// TODO: refactor someday, but works fine as is right now
void CAudioManager::AudioPlay(fs::path* audiopath, CPhysical* audioentity) {
	// if (!audioentity || !fs::exists(*audiopath)) return;
	float fallBackPitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float pitch = fallBackPitch;
	float readPitch = fallBackPitch;

	// Start with defaults
	AttenuationSet settings = gAttenuationSettings;
	Attenuation finalAtt = settings.base; // will be updated below
	Pitch pset;
	std::string stem = audiopath->stem().string();
	bool isShoot = (stem == "shoot" || NameStartsWithIndexedSuffix(stem.c_str(), "shoot"));
	bool isAfter = (stem == "after" || NameStartsWithIndexedSuffix(stem.c_str(), "after"));
	bool isDistant = (stem == "distant" || NameStartsWithIndexedSuffix(stem.c_str(), "distant"));
	bool isLowAmmo = (stem == "low_ammo" || NameStartsWithIndexedSuffix(stem.c_str(), "low_ammo"));
	bool Reloads = IsMatchingName(stem.c_str(), { "reload", "reload_one", "reload_two" })
		|| NameStartsWithIndexedSuffix(stem.c_str(), "reload");

	bool NeedsToBeQuieter = IsMatchingName(stem.c_str(), { "hit", "swing", "stomp", "martial_kick", "martial_punch" });
	if (!NeedsToBeQuieter) {
		static const std::string prefixes[] = { "swing", "hit", "stomp", "martial_kick", "martial_punch" };
		for (const auto& prefix : prefixes) {
			if (NameStartsWithIndexedSuffix(stem.c_str(), prefix.c_str())) {
				NeedsToBeQuieter = true;
				break;
			}
		}
	}

	bool readPitchFromFile = false;
	bool veh = false;
	bool haveVinfo = false;
	if (audioentity && audioentity->m_nType == ENTITY_TYPE_VEHICLE) {
		LOG("audioentity is a vehicle");
		veh = true;
	}

	CPed* ped = nullptr;
	CWeapon* weapon = nullptr;
	if (audioentity && audioentity->m_nType == ENTITY_TYPE_PED) {
		ped = (CPed*)audioentity;
		weapon = ped->GetWeapon();
	}

	std::string filename;
	auto p = findEarshotForEntity(audioentity, *audiopath);
	if (p) {
		filename = p->stem().string();
		LOG("Earshot file: %s", filename.c_str());
	}

	// Try to find pitch
		auto it = gWeaponPitches.find(filename);
		if (it != gWeaponPitches.end()) {
			pset = it->second;
			LOG("Using pitch settings from mapKey='%s'", filename.c_str());
		}

	// Try to find attenuation
		auto it2 = gWeaponAttenuations.find(filename);
		if (it2 != gWeaponAttenuations.end()) {
			settings = it2->second;
			LOG("Using attenuations settings from mapKey='%s'", filename.c_str());
		}

	if (veh && isDistant) {
		finalAtt = settings.distant;
		LOG("Using distant attenuation for vehicle");
	}
	else if (isDistant) {
		finalAtt = settings.distant;
		LOG("Using distant attenuation");
	}
	else if (veh) {
		finalAtt = settings.vehicle;
		LOG("Using vehicle attenuation");
	}
	else if (Reloads) {
		finalAtt = settings.reload;
		LOG("Using reloading attenuation");
	}
	else if (isAfter) {
		finalAtt = settings.after;
		LOG("Using after attenuation");
	}
	else if (NeedsToBeQuieter) {
		finalAtt = settings.quieter;
		LOG("Using melee attenuation");
	}
	else if (isLowAmmo) {
		finalAtt = settings.low_ammo;
		LOG("Using low_ammo attenuation");
	}
	else {
		finalAtt = settings.base;
		LOG("base settings: maxDist %.2f refDist %.2f rollOff %.2f airAbs %.2f", finalAtt.maxDist, finalAtt.refDist, finalAtt.rolloffFactor, finalAtt.airAbsorption);
		LOG("Using base attenuation");
	}

	bool picked = false;
	if (fallBackPitch >= 1.0f) {
		if (NeedsToBeQuieter && pset.quieter.has_value()) {
			readPitch = *pset.quieter; picked = true;
		}
		else if (isLowAmmo && pset.low_ammo.has_value()) { readPitch = *pset.low_ammo; picked = true; }
		else if ((veh && isDistant) && pset.distant.has_value()) { readPitch = *pset.distant; picked = true; }
		else if (isDistant && pset.distant.has_value()) { readPitch = *pset.distant; picked = true; }
		else if (veh && pset.vehicle.has_value()) { readPitch = *pset.vehicle; picked = true; }
		else if (Reloads && pset.reload.has_value()) { readPitch = *pset.reload; picked = true; }
		else if (isShoot && pset.shoot.has_value()) { readPitch = *pset.shoot; picked = true; }
		else if (isAfter && pset.after.has_value()) { readPitch = *pset.after; picked = true; }
		else if (pset.base.has_value()) { readPitch = *pset.base; picked = true; }
		else { readPitch = fallBackPitch; }
	}


	CVector pos = audioentity->GetPosition();

	ALuint buffer = CreateOpenALBufferFromAudioFile(audiopath->string().c_str());
	if (!alIsBuffer(buffer) || buffer == 0) {
		LOG("Could not play %s", outputPath(audiopath).c_str());
		return;
	}
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	// With each lower ammo in the clip, the sound get's louder
	if (isLowAmmo && audioentity && audioentity->m_nType == ENTITY_TYPE_PED && ped && weapon) {
		int ammoInClip = weapon->m_nAmmoInClip;
		unsigned short ammoClip = CWeaponInfo::GetWeaponInfo(weapon->m_eWeaponType, WEAPSKILL_STD)->m_nAmmoClip;
		int left = (ammoClip / 3);
		ammoInClip = std::max(1, std::min(ammoInClip, left));
		float gainMultiplier = 0.1f + (float(left) - ammoInClip) * 0.1f;
		gameVol *= gainMultiplier;
	}
	gameVol *= fader;

	SoundInstanceSettings opts;
	opts.maxDist = finalAtt.maxDist;
	opts.gain = gameVol;
	opts.airAbsorption = finalAtt.airAbsorption;
	opts.refDist = finalAtt.refDist;
	opts.rollOffFactor = finalAtt.rolloffFactor;
	opts.readPitchFromFile = picked;
	opts.readPitch = readPitch;
	opts.pitch = pitch;
	opts.pos = pos;

	opts.isPossibleGunFire = true;
	opts.entity = audioentity;
	opts.weaponType = weapon ? weapon->m_eWeaponType : WEAPONTYPE_UNARMED;
	opts.path = *audiopath;
	opts.name = audiopath->stem().string();

	PlaySource(buffer, opts);
}


bool CAudioManager::findWeapon(eWeaponType* weapontype, eModelID modelid, std::string filename, CPhysical* audioentity, bool playAudio)
{
	auto key = std::make_pair(*weapontype, modelid);
	auto it = registeredweapons.find(key);
	fs::path path;
	if (it == registeredweapons.end())
	{
	//	LOG("registeredweapons == end");
		return false;
	}
	// if we don't want to play any audio, just check for file's existence instead
	if (!playAudio) {
	//	LOG("playAudio = false");
		return it->second.audioPath(filename, path);
	}
	//LOG("audioPlay = true");
	return it->second.audioPlay(filename, audioentity);
}

bool CAudioManager::PlayAmbienceBuffer(ALuint buffer, const CVector& origin, bool isGunfire, bool isThunder, bool isManual, float manualMaxDist,
	float manualRefDist, float manualRollOff, float manualAirAbsorption, ManualAmbience& ma)
{
	// Don't play when paused or during a cutscene
	if (buffer == 0
		|| CCutsceneMgr::ms_running
		|| FrontEndMenuManager.m_bMenuActive)
		return false;

	if (!isManual && CGame::currArea > 0)
		return false;

	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	//LOG("Ambience pitch: %.2f", pitch);
	CVector pos = GetRandomAmbiencePosition(origin, isThunder);
	float VolumeToUse = AEAudioHardware.m_fEffectMasterScalingFactor * 1.3f;
	float refDist = 1.0f;
	if (isGunfire) {
		VolumeToUse = AEAudioHardware.m_fEffectMasterScalingFactor * 1.0f;
	}
	else if (isThunder) {
		VolumeToUse = AEAudioHardware.m_fEffectMasterScalingFactor * 1.5f;
	}

	if (isThunder) {
		refDist = 15.0f;
	}
	else if (isManual) {
		refDist = manualRefDist;
	}
	else {
		refDist = 6.5f;
	}
	SoundInstanceSettings opts;
	opts.maxDist = isManual ? manualMaxDist : 250.0f;
	opts.gain = VolumeToUse;
	opts.airAbsorption = isManual ? manualAirAbsorption : 1.0f;
	opts.refDist = refDist;
	opts.rollOffFactor = isManual ? manualRollOff : 1.0f;
	opts.pitch = pitch;
	opts.pos = isManual ? origin : pos;

	opts.isGunfire = isGunfire;
	opts.isAmbience = isManual ? false : true;

	for (const auto& kv : gBufferMap) {
		if (kv.second == buffer) {
			opts.path = kv.first;
			opts.name = kv.first.stem().string();
			break;
		}
	}

	auto inst = PlaySource(buffer, opts);

	if (inst)
	{
		inst->isManualAmbience = isManual;
		ma.source = inst->source;
		LOG("PlayAmbienceBuffer returned true");
		return true;
	}
	LOG("PlayAmbienceBuffer returned false");
	return false;
}

bool CAudioManager::PlayOutsideAmbience(
	const CVector& playerPos,
	const CVector& origin,
	eWeaponType weaponType,
	bool isNight,
	bool isRiot,
	bool ambienceStillPlaying,
	CZone* zone)
{
	// BEFORE step 1) Manual ambiences: stop if left sphere
	bool inManualArea = false;
	for (const auto& ma : g_ManualAmbiences) {
		if (ma.buffer.empty()) continue;
		if (!ma.allowOtherAmbiences && IsPointWithinSphere(ma.sphere, playerPos)) {
			inManualArea = true;
			break;
		}
	}

	// 1) Manual ambiences: stop if left sphere
	for (auto& ma : g_ManualAmbiences) {
		bool playerInside = IsPointWithinSphere(ma.sphere, playerPos);

		if (ma.loop && ma.loopingInstance && !playerInside) {
			StopLoopingAmbience(ma);
			LOG("ManualAmbience: stopped looping ambience - player left its range");
		}

		if (!playerInside) {
			for (auto& inst : audiosplaying) {
				if (!inst) continue;
				if (!inst->isManualAmbience) continue;
				if (inst->source == 0) continue;

				if (!IsPointWithinSphere(ma.sphere, playerPos)) continue;

				ALint state = AL_STOPPED;
				if (inst->source != 0) state = GetSourceState(inst->source);

				if (state == AL_PLAYING || state == AL_PAUSED) {
					PauseSource(&*inst);
					alSourceStop(inst->source);
				}

				alDeleteSources(1, &inst->source);
				inst->source = 0;
				ma.nextPlayTime = CTimer::m_snTimeInMilliseconds + ma.delay;
				LOG("ManualAmbience: stopped non-looping manual ambience instance at (%.1f,%.1f,%.1f).",
					inst->pos.x, inst->pos.y, inst->pos.z);
			}
		}
	}


	// 2) Try to play manual ambiences
	for (auto& ma : g_ManualAmbiences) {
		if (ma.buffer.empty()) continue;
		bool shouldStop = (ma.time == EAmbienceTime::Night && !isNight)
			|| (ma.time == EAmbienceTime::Riot && !isRiot);

		bool insideRange = IsPointWithinSphere(ma.sphere, playerPos);

		// pick random buffer
		RandomIntegers id(ma.buffer.size());
		int idx = id.next();
		ALuint buff = ma.buffer[idx];

		// Handle looping ambiences (start if not running)
		if (ma.loop) {
			bool hasInstance = (ma.loopingInstance != nullptr);
			if (insideRange) {
				// not it's time of day anymore or riot's ended, stop it
				if (shouldStop)
				{
					if (hasInstance) {
						StopLoopingAmbience(ma);
						LOG("ManualAmbience: stopped looping ambience (buffer=%u) - not it's time of day anymore or riot's ended.", buff);
					}
				}
				if (!shouldStop && !hasInstance && (!ambienceStillPlaying || ma.allowOtherAmbiences)) {
					if (StartLoopingAmbience(ma)) {
						LOG("ManualAmbience: started looping ambience (buffer=%u).", buff);
						if (!ma.allowOtherAmbiences) {
							return true;
						}
					}
					else {
						LOG("ManualAmbience: StartLoopingAmbience failed for buffer=%u", buff);
					}
				}
				else if (hasInstance) {
					if (!ma.allowOtherAmbiences) {
						return true;
					}
				}
			}
			else {
				if (hasInstance) {
					StopLoopingAmbience(ma);
					LOG("ManualAmbience: stopped looping ambience (buffer=%u) - player left range.", buff);
				}
			}
			continue; // next manual ambience
		}

		// Non-looping manual ambiences
		// If the allowOtherAmbiences is true, then allow others to play as well (non manual ambiences)
		if ((!ambienceStillPlaying || ma.allowOtherAmbiences) && !ma.loop && CTimer::m_snTimeInMilliseconds >= ma.nextPlayTime) {
			if (insideRange) {
				// prevent overlapping instances of the same manual ambience
				// If there's already a manual-ambience instance within this manual ambience's sphere
				// that is PLAYING or PAUSED, don't start another one.
				// looping ambience doesn't need this, since it's looped and not a one-shot :shrug:
				bool instanceAlreadyActive = false;
				for (auto& inst : audiosplaying) {
					if (!inst) continue;
					if (!inst->isManualAmbience) continue;
					if (inst->source == 0) continue;
					if (!IsPointWithinSphere(ma.sphere, playerPos)) continue;
					if (shouldStop) continue;

					ALint state = AL_STOPPED;
					state = GetSourceState(inst->source);

					if (state == AL_PLAYING || state == AL_PAUSED) {
						instanceAlreadyActive = true;
						break;
					}
				}

				if (instanceAlreadyActive) {
					continue;
				}

				// pick random buffer
				RandomIntegers id(ma.buffer.size());
				int idx = id.next();
				ALuint buff = ma.buffer[idx];

				if (PlayAmbienceBuffer(buff, ma.pos, false, false, true, ma.maxDist, ma.refDist, ma.rollOff, ma.airAbsorption, ma)) {
					ma.nextPlayTime = CTimer::m_snTimeInMilliseconds + ma.delay;
					LOG("CurrentTime=%u, NextPlayTime=%u, Delay=%u",
						CTimer::m_snTimeInMilliseconds, ma.nextPlayTime, ma.delay);
					LOG("ManualAmbience: played transient buffer=%u, size=%zu, next time is=%u", buff, ma.buffer.size(), ma.nextPlayTime);
					if (!ma.allowOtherAmbiences) {
						return true;
					}
				}
				else {
					LOG("ManualAmbience: PlayAmbienceBuffer returned false for buffer=%u", buff);
				}
			}
		}

	}

	// 3) local zone ambience (if any)
	bool zoneHasCustomAmbience = false;
	if (zone) {
		std::string zoneKey(zone->m_szTextKey);
		zoneKey = caseLower(zoneKey);

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

		if (!zoneKey.empty() && !ambienceStillPlaying && !inManualArea && CTimer::m_snTimeInMilliseconds >= nextZoneAmbienceTime) {
			for (const auto& [key, vec] : buffersMap) {
				if (zoneKey.starts_with(key) && !vec.empty()) {
					RandomIntegers rnd(vec.size());
					int index = rnd.next();
					ALuint buffer = vec[index];
					if (PlayAmbienceBuffer(buffer, origin)) {
						nextZoneAmbienceTime = CTimer::m_snTimeInMilliseconds +
							CGeneral::GetRandomNumberInRange((int)zoneIntervalMin, (int)zoneIntervalMax);
						return true;
					}
				}
			}
		}
	}

	// 4) Global zone ambience
	const auto& globalBuffersMap =
		isRiot ? g_Buffers.GlobalZoneAmbienceBuffers_Riot :
		isNight ? g_Buffers.GlobalZoneAmbienceBuffers_Night :
		g_Buffers.GlobalZoneAmbienceBuffers_Day;

	std::string globalKey;
	switch (CTheZones::m_CurrLevel) {
	case LEVEL_NAME_COUNTRY_SIDE: globalKey = "country"; break;
	case LEVEL_NAME_LOS_SANTOS:   globalKey = "LS"; break;
	case LEVEL_NAME_SAN_FIERRO:   globalKey = "SF"; break;
	case LEVEL_NAME_LAS_VENTURAS: globalKey = "LV"; break;
	default: break;
	}

	if (!globalKey.empty() && !ambienceStillPlaying && !inManualArea) {
		if (auto it = globalBuffersMap.find(globalKey); it != globalBuffersMap.end() && !it->second.empty()) {
			RandomIntegers rnd(it->second.size());
			int index = rnd.next();
			ALuint buffer = it->second[index];
			if (PlayAmbienceBuffer(buffer, origin)) {
				nextZoneAmbienceTime = CTimer::m_snTimeInMilliseconds +
					CGeneral::GetRandomNumberInRange((int)zoneIntervalMin, (int)zoneIntervalMax);
				return true;
			}
		}
	}

	// 5) fallback riot/day/night/fire ambiences
	if (!ambienceStillPlaying && !zoneHasCustomAmbience && !inManualArea) {
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
				RandomIntegers rnd(selectedBuffs->size());
				int index = rnd.next();
				ALuint buffer = (*selectedBuffs)[index];
				if (PlayAmbienceBuffer(buffer, origin)) {
					nextFireAmbienceTime = CTimer::m_snTimeInMilliseconds +
						CGeneral::GetRandomNumberInRange((int)fireIntervalMin, (int)fireIntervalMax);
					return true;
				}
			}
		}
	}

	return false;
}

bool CAudioManager::PlayAmbienceSFX(const CVector& origin, eWeaponType weaponType, bool useOldAmbience) {

	bool isNight = (CClock::ms_nGameClockHours >= 20 || CClock::ms_nGameClockHours < 6);
	bool isRiot = CGameLogic::LaRiotsActiveHere();
	CVector playerPos = FindPlayerCoors();

	// Check if an ambience is still playing (treat PAUSED as "still playing") ---
	bool ambienceStillPlaying = false;
	for (auto& sfx : audiosplaying) {
		if (!sfx) continue;
		if (!sfx->isAmbience) continue;

		if (sfx->source != 0) {
			ALint state;
			state = GetSourceState(sfx->source);

			float gain = 0.0f;
			alGetSourcef(sfx->source, AL_GAIN, &gain);

			// Only consider as "still playing" if audible
			if ((state == AL_PLAYING || state == AL_PAUSED) && gain > 0.0f) {
				ambienceStillPlaying = true;
				break;
			}
		}
	}

	if (useOldAmbience) {
		// OUTSIDE (zones / fallback)
		CZone* zone = nullptr;
		CTheZones::GetZoneInfo(&cameraposition, &zone);

		if (PlayOutsideAmbience(playerPos, origin, weaponType, isNight, isRiot, ambienceStillPlaying, zone)) {
			return true;
		}
	}
	else {
		// LS Gunfire ambience
		if (CGame::currArea <= 0) {
			auto it = g_Buffers.WeaponTypeAmbienceBuffers.find(weaponType);
			if (it == g_Buffers.WeaponTypeAmbienceBuffers.end() || g_Buffers.WeaponTypeAmbienceBuffers.empty() || it->second == 0)
				return false;

			return PlayAmbienceBuffer(it->second, origin, true);
		}
	}

	return false;
}

void CAudioManager::UnloadManualAmbiences()
{
	for (auto& ma : g_ManualAmbiences) {
		if (ma.loopingInstance) {
			StopLoopingAmbience(ma);
		}

		if (!ma.buffer.empty()) {
			RandomIntegers id(ma.buffer.size());

			int idx = id.next();
			ALuint buff = ma.buffer[idx];
			alDeleteBuffers(1, &buff);
			buff = 0;
		}
		ma.nextPlayTime = 0;
		ma.loopingInstance = nullptr;
	}
	g_ManualAmbiences.clear();
}

// Create and play an OpenAL source for looping ambience.
// Returns true on success and sets ma.loopingInstance to the created instance.
bool CAudioManager::StartLoopingAmbience(ManualAmbience& ma)
{
	if (ma.buffer.empty()) return false;
	if (ma.loopingInstance) return true; // already running

	RandomIntegers id(ma.buffer.size());

	int idx = id.next();
	ALuint buff = ma.buffer[idx];
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	gameVol *= fader;
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);

	SoundInstanceSettings opts;

	opts.maxDist = ma.maxDist;
	opts.gain = gameVol;
	opts.airAbsorption = ma.airAbsorption;
	opts.refDist = ma.refDist;
	opts.rollOffFactor = ma.rollOff;
	opts.pitch = pitch;
	opts.pos = ma.pos;

	opts.looping = true;
	opts.isGunfire = false;
	opts.isAmbience = !ma.allowOtherAmbiences;

	auto inst = PlaySource(buff, opts);
	bool ok = (inst != nullptr);

	if (!ok || !inst) {
		LOG("StartLoopingAmbience: PlaySource failed for buffer %u", ma.buffer);
		return false;
	}

	// store the instance so we can stop it later
	ma.loopingInstance = inst;

	LOG("Started looping ambience via PlaySource (instance) for buffer %u at (%.1f, %.1f, %.1f), R=%.1f",
		ma.buffer, ma.pos.x, ma.pos.y, ma.pos.z, ma.range);
	return true;
}

void CAudioManager::StopLoopingAmbience(ManualAmbience& ma)
{
	if (!ma.loopingInstance) return;

	// Try to pause/stop using AudioManager's PauseSource (existing code used this pattern).
	auto inst = ma.loopingInstance;
	if (inst) {
		PauseSource(&*inst);
		if (inst != nullptr) {
			ALint state = 0;
			alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
			if (state == AL_PLAYING || state == AL_PAUSED) {
				// already paused above; ensure stopped
				alSourceStop(inst->source);
			}
			alDeleteSources(1, &inst->source);
			inst->source = 0;
		}
	}

	ma.loopingInstance = nullptr;
	LOG("Stopped looping ambience instance");
}

void CAudioManager::PlayOrStopBarrelSpinSound(CPed* entity, eWeaponType* weapontype, bool spinning, bool playSpinEndSFX, bool removeSpinSource)
{
	//if (!shooter || !ent) return;

//CPed* ped = reinterpret_cast<CPed*>(shooter);
	float pitch = Clamp(CTimer::ms_fTimeScale, 0.0f, 1.0f);
	float deltaTime = CTimer::ms_fTimeStep;
	float gameVol = AEAudioHardware.m_fEffectMasterScalingFactor;
	float fader = AEAudioHardware.m_fEffectsFaderScalingFactor;
	float gameVolEnd = gameVol * fader;
	gameVol *= fader;
	//	CPed* entity = ped;
	if (spinning) {
		// Start playing if not already
		if (entity && m_apMinigunSound[entity][1] == nullptr) {
			eWeaponType weaponType = *weapontype;
			SoundInstanceSettings opts;
			opts.maxDist = gAttenuationSettings.minigunSpin.maxDist;
			opts.airAbsorption = gAttenuationSettings.minigunSpin.airAbsorption;
			opts.refDist = gAttenuationSettings.minigunSpin.refDist;
			opts.rollOffFactor = gAttenuationSettings.minigunSpin.rolloffFactor;
			opts.gain = gameVol;
			opts.pitch = pitch;
			opts.pos = entity->GetPosition();

			opts.looping = true;
			opts.isMinigunBarrelSpin = true;

			opts.shooter = entity;
			opts.weaponType = weaponType;

			auto& instance = PlaySource(g_Buffers.minigunBuffers[1], opts);
			if (instance)
			{
				barrelSpinSource = instance->source;
				m_apMinigunSound[entity][1] = instance;
				LOG("Playing spin = true");
			}
			LOG("Playing spin = false");
			barrelSpinVolume = 0.0f;
		}

		// Fade in
		if (barrelSpinVolume < 1.0f) {
			barrelSpinVolume += deltaTime / barrelFadeDuration;
			if (barrelSpinVolume > 1.0f) barrelSpinVolume = 1.0f;

			gameVol *= barrelSpinVolume;
			SetSourceGain(barrelSpinSource, gameVol);
		}
	}
	else {
		// Fade out
		if (barrelSpinVolume > 0.0f) {
			barrelSpinVolume -= deltaTime / barrelFadeDuration;
			if (barrelSpinVolume < 0.0f) barrelSpinVolume = 0.0f;
			gameVol *= barrelSpinVolume;
			SetSourceGain(barrelSpinSource, gameVol);
		}

		// Once volume is faded out, stop source and play SPINEND if needed
		if (barrelSpinVolume <= 0.1f) {
			if (playSpinEndSFX) {
				//if (m_apMinigunSound[entity][2] == nullptr) 
				//{
					eWeaponType weaponType = *weapontype;
					SoundInstanceSettings opts;
					opts.maxDist = gAttenuationSettings.minigunSpinEnd.maxDist;
					opts.airAbsorption = gAttenuationSettings.minigunSpinEnd.airAbsorption;
					opts.refDist = gAttenuationSettings.minigunSpinEnd.refDist;
					opts.rollOffFactor = gAttenuationSettings.minigunSpinEnd.rolloffFactor;
					opts.gain = gameVolEnd;
					opts.pitch = pitch;
					opts.pos = entity->GetPosition();

					opts.shooter = entity;
					opts.weaponType = weaponType;

					PlaySource(g_Buffers.minigunBuffers[2], opts);
					if (removeSpinSource && barrelSpinSource != 0)
					{
						alSourceStop(barrelSpinSource);
						alDeleteSources(1, &barrelSpinSource);
						barrelSpinSource = 0;
						LOG("removed spin source after playing spin end SFX");
					}
					//if (instance)
					//{
						//m_apMinigunSound[entity][2] = instance;
						LOG("Played spin end SFX");
					//}
				//}
			}
			else {
				if (barrelSpinSource != 0) 
				{
					alSourceStop(barrelSpinSource);
					alDeleteSources(1, &barrelSpinSource);
					barrelSpinSource = 0;
					LOG("removed spin source without playing spin end SFX");
				}
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

void CAudioManager::InitEcho()
{
	if (!pDevice || !pContext || !efxSupported)
		return;

	alGenAuxiliaryEffectSlots(1, &echoSlot);
	alGenEffects(1, &echoEffect);

	alEffecti(echoEffect, AL_EFFECT_TYPE, AL_EFFECT_ECHO);

	alEffectf(echoEffect, AL_ECHO_DELAY, 0.320f);
	alEffectf(echoEffect, AL_ECHO_LRDELAY, 0.320f);
	alEffectf(echoEffect, AL_ECHO_DAMPING, 0.25f);
	alEffectf(echoEffect, AL_ECHO_FEEDBACK, 0.68f);
	alEffectf(echoEffect, AL_ECHO_SPREAD, 1.0f);

	alAuxiliaryEffectSloti(echoSlot, AL_EFFECTSLOT_EFFECT, echoEffect);
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

void CAudioManager::PauseAllSources()
{
	for (auto& audio : audiosplaying) {
		if (!audio) continue;
		PauseSource(&*audio);
	}
}

void CAudioManager::ResumeAllSources()
{
	for (auto& audio : audiosplaying) {
		if (!audio) continue;
		ResumeSource(&*audio);
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

void CAudioManager::AttachEchoToSource(ALuint source, bool detach) {
	if (!efxSupported) return;

	if (!detach) {
		// Attach the echo
		alSource3i(source, AL_AUXILIARY_SEND_FILTER, echoSlot, 0, AL_FILTER_NULL);
	}
	else {
		// Detach the echo by setting the effect slot to 0
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
	SetSourceGain(inst->source, volume);
	//SetSourcePitch(inst->source, pitch);
	alSourcePlay(inst->source);
	audiosplaying.push_back(inst);
	return true;
}

bool CAudioManager::SetSourceGain(ALuint source, float gain)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_GAIN, gain);
	return true;
}

bool CAudioManager::SetSourcePitch(ALuint source, float pitch)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_PITCH, pitch);
	return true;
}

bool CAudioManager::SetSourceMaxDist(ALuint source, float maxDist)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_MAX_DISTANCE, maxDist);
	return true;
}

bool CAudioManager::SetSourceAirAbsorptionFactor(ALuint source, float factor)
{
	// Is it a valid source?
	if (!alIsSource(source)) return false;
	alSourcef(source, AL_AIR_ABSORPTION_FACTOR, factor);
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
	const fs::path filePath(filename);

	// If provided an absolute path, check it first
	if (filePath.is_absolute()) {
		if (fs::exists(filePath)) {
			outPath = filePath;
			auto inst = std::make_shared<SoundInstance>();
			inst->path = outPath;
			AudioManager.audiosplaying.push_back(inst);
			return true;
		}
		return false;
	}

	// Reuse base path object so we don't rebuild it each loop
	fs::path base = filePath;
	for (const auto& ext : extensions) {
		fs::path candidate = audiosfolder / base;
		candidate.replace_extension(ext);
		if (fs::exists(candidate)) {
			outPath = candidate;
			auto inst = std::make_shared<SoundInstance>();
			inst->path = candidate;
			AudioManager.audiosplaying.push_back(inst);
			return true;
		}
	}
	return false;
}

bool AudioStream::audioPlay(std::string filename, CPhysical* audioEntity)
{
	std::vector<fs::path> alternatives;
	const std::string& baseName = filename;
	// Alternatives: sound0.wav, sound1.ogg, ...
	for (int i = 0; i < MAX_SOUND_ALTERNATIVES; ++i) {
		std::string altName = baseName + std::to_string(i);
		fs::path altPath;
		if (audioPath(altName, altPath) /*&& fs::exists(altPath)*/) {
			alternatives.push_back(altPath);
			LOG("Alternative audio found: %s", alternatives.back().string().c_str());
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
			alternatives.push_back(fallbackPath);
			AudioManager.audiosplaying.push_back(inst);
			LOG("Fallback audio found: %s", alternatives.back().string().c_str());
		}
	}

	if (alternatives.empty()) 
	{
		return false;
	}

	// Play one random alternative sound
	RandomIntegers rnd(alternatives.size());

	int index = rnd.next();
	fs::path selected = alternatives[index];
	LOG("Playing non-loop audio: %s", selected.string().c_str());
	AudioManager.AudioPlay(&selected, audioEntity);

	return true;
}

void CAudioManager::UpdateFireSoundCleanup()
{
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
	// stop sound for X inactive fire
	for (int i = 0; i < MAX_NUM_FIRES; i++) {
		CFire* fire = &gFireManager.m_aFires[i];
		if (fire && (!fire->m_nFlags.bActive || !fire->m_nFlags.bMakesNoise)) {
			auto it = g_Buffers.fireSounds.find(fire);
			if (it != g_Buffers.fireSounds.end()) {
				SafeDeleteInstanceSource(it->second);
				g_Buffers.fireSounds.erase(it);
			}
			continue;
		}
	}
}

