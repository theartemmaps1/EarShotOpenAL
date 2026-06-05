#include "Loaders.h"
#include "AudioManager.h"
#include "IniReader.h"
using namespace plugin;
namespace fs = std::filesystem;
// Because the tank cannon doesn't have any weapon types on it, we hardcode it to this folder :shrug:
void Loaders::LoadTankCannonSounds(const fs::path& folder) {
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		fs::path foundFile;
		for (const auto& ext : extensions) {
			fs::path firePath = folder / ("Tank Cannon/cannon_fire" + std::to_string(index) + ext);
			if (fs::exists(firePath)) {
				foundFile = firePath;
				break;
			}
		}

		if (!foundFile.empty()) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
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
		for (const auto& ext : extensions) {
			fs::path fallbackPath = folder / ("Tank Cannon/cannon_fire" + ext);
			if (fs::exists(fallbackPath)) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallbackPath.string().c_str());
				if (buffer != 0) g_Buffers.tankCannonFireBuffers.push_back(buffer);
				break;
			}
		}
	}
}

void Loaders::LoadMinigunSounds(const fs::path& folder)
{
	const char* names[3] = { "minigun_fireloop", "minigun_barrelspinloop", "minigun_barrelspinend" };
	auto weapontype = eWeaponType();
	int loaded = 0;
	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		if (loaded >= 3) break;
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			std::string extension = entrypath.extension().string();
			if (extension == modextension && nameType(&filename, &weapontype)) {
				for (int i = 0; i < 3 && loaded < 3; ++i) {
					for (const auto& ext : extensions) {
						fs::path spinPath = entrypath.parent_path() / (names[i] + ext);
						if (fs::exists(spinPath)) {
							g_Buffers.minigunBuffers[i] = AudioManager.CreateOpenALBufferFromAudioFile(spinPath.string().c_str());
							loaded++;
							break;
						}
					}
				}
			}
		}
	}
}

void Loaders::LoadChainsawSounds(const fs::path& folder)
{
	const char* names[4] = { "chainsaw_idle", "chainsaw_active", "chainsaw_cuttingflesh", "chainsaw_stop" };
	auto weapontype = eWeaponType();
	int loaded = 0;
	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		if (loaded >= 4) break;
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			std::string extension = entrypath.extension().string();
			if (extension == modextension && nameType(&filename, &weapontype)) {
				for (int i = 0; i < 4 && loaded < 4; ++i) {
					for (const auto& ext : extensions) {
						fs::path spinPath = entrypath.parent_path() / (names[i] + ext);
						if (fs::exists(spinPath)) {
							g_Buffers.chainsawBuffers[i] = AudioManager.CreateOpenALBufferFromAudioFile(spinPath.string().c_str());
							loaded++;
							break;
						}
					}
				}
			}
		}
	}
}

void Loaders::LoadCarSirenSounds(const fs::path& folder)
{
	const fs::path sirensDir = folder / "generic/sirens";
	const fs::path sirensIni = sirensDir / "sirens.ini";

	if (!fs::exists(sirensDir))
	{
		LOG("Sirens folder wasn't found, '%s'", sirensDir.string().c_str());
		return;
	}

	if (!fs::exists(sirensIni))
	{
		LOG("Sirens ini wasn't found, '%s'", sirensIni.string().c_str());
		return;
	}

	CIniReader ini(sirensIni.string());

	auto readOpt = [&](const std::string& section, const std::string& key) -> std::optional<float>
		{
			constexpr float sentinel = std::numeric_limits<float>::quiet_NaN();
			float v = ini.ReadFloat(section, key, sentinel);
			if (std::isnan(v))
				return std::nullopt;
			return v;
		};

	auto loadAtt = [&](const std::string& section, const char* keyPrefix, Attenuation& att,
		float defMax, float defRef, float defRolloff, float defAir)
		{
			att.maxDist = ini.ReadFloat(section, std::string(keyPrefix) + ".maxDist", defMax);
			att.refDist = ini.ReadFloat(section, std::string(keyPrefix) + ".refDist", defRef);
			att.rolloffFactor = ini.ReadFloat(section, std::string(keyPrefix) + ".rolloffFactor", defRolloff);
			att.airAbsorption = ini.ReadFloat(section, std::string(keyPrefix) + ".airAbsorption", defAir);
		};

	auto loadSoundFile = [&](int modelId, int slot, const fs::path& filePath) -> bool
		{
			for (const auto& ext : extensions)
			{
				fs::path path = filePath;
				if (!path.has_extension())
					path += ext;

				if (fs::exists(path))
				{
					g_Buffers.sirenBuffers[modelId][slot] =
						AudioManager.CreateOpenALBufferFromAudioFile(path.string().c_str());
					g_Buffers.g_VehicleHasSiren[modelId] = true;
					return true;
				}
			}

			LOG("Missing sound file for model %d: '%s'", modelId, filePath.string().c_str());
			return false;
		};

	std::vector<char> sectionNames(65536);
	DWORD copied = GetPrivateProfileSectionNamesA(
		sectionNames.data(),
		static_cast<DWORD>(sectionNames.size()),
		sirensIni.string().c_str()
	);

	if (copied == 0)
	{
		LOG("No INI sections found in '%s'", sirensIni.string().c_str());
		return;
	}

	int totalLoaded = 0;

	for (const char* sec = sectionNames.data(); *sec; sec += std::strlen(sec) + 1)
	{
		int modelId = -1;
		if (std::sscanf(sec, "VEHICLE_%d", &modelId) != 1)
			continue;

		std::string section(sec);

		std::string sirenPath = ini.ReadString(section, "siren", "");
		std::string sirenIdlePath = ini.ReadString(section, "sirenidle", "");
		std::string reversePath = ini.ReadString(section, "reverse_beep", "");
		std::string airBrakePath = ini.ReadString(section, "air_brakes", "");

		int loaded = 0;

		if (!sirenPath.empty())
			loaded += loadSoundFile(modelId, 0, sirensDir / sirenPath) ? 1 : 0;
		if (!sirenIdlePath.empty())
			loaded += loadSoundFile(modelId, 1, sirensDir / sirenIdlePath) ? 1 : 0;
		if (!reversePath.empty())
			loaded += loadSoundFile(modelId, 2, sirensDir / reversePath) ? 1 : 0;
		if (!airBrakePath.empty())
			loaded += loadSoundFile(modelId, 3, sirensDir / airBrakePath) ? 1 : 0;

		if (loaded > 0)
		{
			totalLoaded += loaded;

			loadAtt(section, "siren", gAttenuationSettings.siren[modelId], FLT_MAX, 1.0f, 0.5f, 1.0f);
			loadAtt(section, "sirenidle", gAttenuationSettings.sirenidle[modelId], FLT_MAX, 1.0f, 0.5f, 1.0f);
			loadAtt(section, "reverse_beep", gAttenuationSettings.reverse_beep[modelId], FLT_MAX, 1.0f, 1.0f, 1.5f);
			loadAtt(section, "air_brakes", gAttenuationSettings.air_brake[modelId], FLT_MAX, 1.0f, 1.0f, 1.5f);

			if (auto v = readOpt(section, "siren.pitch"))        gPitches.siren[modelId] = v;
			if (auto v = readOpt(section, "sirenidle.pitch"))    gPitches.sirenidle[modelId] = v;
			if (auto v = readOpt(section, "reverse_beep.pitch")) gPitches.reverse_beep[modelId] = v;
			if (auto v = readOpt(section, "air_brakes.pitch"))   gPitches.air_brake[modelId] = v;

			LOG("Loaded %d siren sounds for model ID %d", loaded, modelId);
		}
		else
		{
			LOG("Section '%s' found, but no valid sounds loaded for model ID %d", section.c_str(), modelId);
		}
	}

	LOG("Total siren sounds loaded: %d", totalLoaded);
}
void Loaders::LoadGrenadeBounceSounds(const fs::path& folder)
{
	const fs::path grenadebounceDir = folder / "generic/grenadebounce";
	if (!fs::exists(grenadebounceDir))
	{
		LOG("Grenade bounce folder wasn't found, '%s'", grenadebounceDir.string().c_str());
		return;
	}

	const std::vector<std::string> surfaces = {
			"default", "metal", "wood", "water", "dirt", "glass", "stone", "sand", "flesh",
			"pavement", "grass", "tile"
	};

	for (const auto& surface : surfaces) {
		std::vector<ALuint>& buffers = g_Buffers.grenadeBounceBufferPerSurface[surface];
		int idx = 0;
		while (true) {
			fs::path foundFile;
			for (const auto& ext : extensions) {
				fs::path candidate = grenadebounceDir / (surface + "/grenadebounce" + std::to_string(idx) + ext);
				if (fs::exists(candidate)) {
					foundFile = candidate;
					break;
				}
			}
			if (foundFile.empty()) break;
			ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
			if (buffer != 0) {
				buffers.push_back(buffer);
				loadedAnyGrenadeBounceSounds = true;
			}
			++idx;
		}

		if (buffers.empty()) {
			for (const auto& ext : extensions) {
				fs::path fileNoIndex = grenadebounceDir / (surface + "/grenadebounce" + ext);
				if (fs::exists(fileNoIndex)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fileNoIndex.string().c_str());
					if (buffer != 0) {
						buffers.push_back(buffer);
						loadedAnyGrenadeBounceSounds = true;
						LOG("Loaded grenade bounce sound for surface '%s' (no index): %s",
							surface.c_str(), fileNoIndex.string().c_str());
					}
					break;
				}
			}
		}
		else {
			LOG("Loaded %d grenade bounce sound(s) for surface: %s",
				buffers.size(), surface.c_str());
		}
	}
}

void Loaders::LoadFlamethrowerSounds(const fs::path& folder)
{
	const char* names[3] = { "flamethrower_idlegasloop", "flamethrower_start", "flamethrower_fire" };
	auto weapontype = eWeaponType();
	int loaded = 0;
	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		if (loaded >= 3) break;
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			std::string extension = entrypath.extension().string();
			if (extension == modextension && nameType(&filename, &weapontype)) {
				for (int i = 0; i < 3 && loaded < 3; ++i) {
					for (const auto& ext : extensions) {
						fs::path spinPath = entrypath.parent_path() / (names[i] + ext);
						if (fs::exists(spinPath)) {
							g_Buffers.flamethrowerBuffers[i] = AudioManager.CreateOpenALBufferFromAudioFile(spinPath.string().c_str());
							loaded++;
							break;
						}
					}
				}
			}
		}
	}
}

void Loaders::LoadSpraycanSound(const fs::path& folder)
{
	auto weapontype = eWeaponType();

	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			std::string extension = entrypath.extension().string();
			if (extension == modextension && nameType(&filename, &weapontype)) {
				for (const auto& ext : extensions) {
					fs::path spinPath = entrypath.parent_path() / ("spraycan_sprayloop" + ext);
					if (fs::exists(spinPath)) {
						g_Buffers.sprayCanLoopBuffer = AudioManager.CreateOpenALBufferFromAudioFile(spinPath.string().c_str());
						break;
					}
				}

			}
		}
	}
}

void Loaders::LoadCameraAndGoggleSounds(const fs::path& folder)
{
	auto weapontype = eWeaponType();
	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			std::string extension = entrypath.extension().string();
			if (extension == modextension && nameType(&filename, &weapontype)) {
				bool shutterLoaded = false;
				bool gogglesOnLoaded = false;
				bool gogglesOffLoaded = false;

				for (const auto& ext : extensions) {
					if (!shutterLoaded) {
						fs::path shutterPath = entrypath.parent_path() / ("camera_shutter" + ext);
						if (fs::exists(shutterPath)) {
							g_Buffers.cameraShutterBuffer =
								AudioManager.CreateOpenALBufferFromAudioFile(shutterPath.string().c_str());
							shutterLoaded = true;
						}
					}

					if (!gogglesOnLoaded) {
						fs::path gogglesOnPath = entrypath.parent_path() / ("goggles_on" + ext);
						if (fs::exists(gogglesOnPath)) {
							g_Buffers.gogglesBuffer[0] =
								AudioManager.CreateOpenALBufferFromAudioFile(gogglesOnPath.string().c_str());
							gogglesOnLoaded = true;
						}
					}

					if (!gogglesOffLoaded) {
						fs::path gogglesOffPath = entrypath.parent_path() / ("goggles_off" + ext);
						if (fs::exists(gogglesOffPath)) {
							g_Buffers.gogglesBuffer[1] =
								AudioManager.CreateOpenALBufferFromAudioFile(gogglesOffPath.string().c_str());
							gogglesOnLoaded = true;
						}
					}

					if (shutterLoaded || gogglesOnLoaded || gogglesOffLoaded)
						break;
				}
			}
		}
	}
}

void Loaders::LoadExtinguisherSound(const fs::path& folder)
{
	auto weapontype = eWeaponType();
	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			std::string extension = entrypath.extension().string();
			if (extension == modextension && nameType(&filename, &weapontype)) {
				for (const auto& ext : extensions) {
					fs::path spinPath = entrypath.parent_path() / ("extinguisher_loop" + ext);
					if (fs::exists(spinPath)) {
						g_Buffers.fireExtinguisherLoopBuffer = AudioManager.CreateOpenALBufferFromAudioFile(spinPath.string().c_str());
						break;
					}
				}
			}
		}
	}
}


void Loaders::LoadBulletWhizzSounds(const fs::path& folder) {
	const fs::path whizzDir = folder / "generic/bullet_whizz";
	if (!fs::exists(whizzDir))
	{
		LOG("bullet whizz folder wasn't found, '%s'", whizzDir.string().c_str());
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
			fs::path foundFile;
			for (const auto& ext : extensions) {
				fs::path candidate = folder / ("generic/bullet_whizz/" + entry.name + std::to_string(index) + ext);
				if (fs::exists(candidate)) {
					foundFile = candidate;
					break;
				}
			}
			if (foundFile.empty()) break;

			ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
			if (buffer != 0) {
				entry.buffer->push_back(buffer);
			}
			++index;
		}

		// Fallback: single sound without index
		if (entry.buffer->empty()) {
			for (const auto& ext : extensions) {
				fs::path fallback = folder / ("generic/bullet_whizz/" + entry.name + ext);
				if (fs::exists(fallback)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
					if (buffer != 0) entry.buffer->push_back(buffer);
					break;
				}
			}
		}
	}
}

void Loaders::InitializeIniFile(int stage, bool loadAll)
{
	CIniReader ini(PLUGIN_PATH("EarShot.ini"));
	if (stage == 1 || loadAll) {
		Logging = ini.ReadBoolean("MAIN", "Logging", false);
		maxBytesInLog = (uint64_t)ini.ReadInteger("MAIN", "Max bytes in log", 9000000);

		fireIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Ambience interval min", 5000);
		fireIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Ambience interval max", 10000);
		zoneIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Zone ambience interval min", 5000);
		zoneIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Zone ambience interval max", 10000);
		distanceForDistantGunshot = ini.ReadFloat("MAIN", "Distant gunshot distance", 50.0f);
		distanceForDistantExplosion = ini.ReadFloat("MAIN", "Distant explosion distance", 100.0f);
		stereoAmbienceVol = ini.ReadFloat("MAIN", "Stereo ambience volume", 0.3f);
	}

	auto readOpt = [&](std::string kname, std::string section) -> std::optional<float> {
		constexpr float sentinel = std::numeric_limits<float>::quiet_NaN();
		float v = ini.ReadFloat(section, kname, sentinel);
		if (std::isnan(v)) return std::nullopt;
		return v;
		};
	if (stage == 2 || loadAll) {
		// lambda to reduce code duplication
		auto LoadAttenuation = [&](const std::string& section, const std::string& keyPrefix, Attenuation& att,
			float defaultMax, float defaultRef, float defaultRolloff, float defaultAirAbs)
			{
				std::string max = !keyPrefix.empty() ? keyPrefix + ".maxDist" : "maxDist";
				std::string ref = !keyPrefix.empty() ? keyPrefix + ".refDist" : "refDist";
				std::string roll = !keyPrefix.empty() ? keyPrefix + ".rolloffFactor" : "rolloffFactor";
				std::string air = !keyPrefix.empty() ? keyPrefix + ".airAbsorption" : "airAbsorption";
				att.maxDist = ini.ReadFloat(section, max, defaultMax);
				att.refDist = ini.ReadFloat(section, ref, defaultRef);
				att.rolloffFactor = ini.ReadFloat(section, roll, defaultRolloff);
				att.airAbsorption = ini.ReadFloat(section, air, defaultAirAbs);
			};
		// Tank cannon
		LoadAttenuation("TANKCANNON", "", gAttenuationSettings.tankcannon, 125.0f, 3.5f, 0.3f, 0.3f);
		if (auto v = readOpt("tankcannon.pitch", "TANKCANNON")) gPitches.tankcannon = v;
		// Missile
		LoadAttenuation("MISSILE", "", gAttenuationSettings.missile, 250.0f, 6.0f, 1.0f, 1.0f);
		if (auto v = readOpt("missile.pitch", "MISSILE"))    gPitches.missile = v;

		// Footsteps player
		LoadAttenuation("FOOTSTEPS", "footsteps.player", gAttenuationSettings.footstepsPlayer, FLT_MAX, 0.5f, 1.5f, 1.0f);
		LoadAttenuation("FOOTSTEPS", "footsteps.player.duck", gAttenuationSettings.footstepsPlayerDuck, FLT_MAX, 0.1f, 1.5f, 1.0f);
		LoadAttenuation("FOOTSTEPS", "footsteps.player.sprint", gAttenuationSettings.footstepsPlayerSprint, FLT_MAX, 1.0f, 1.5f, 1.0f);
		LoadAttenuation("FOOTSTEPS", "footsteps.player.walk", gAttenuationSettings.footstepsPlayerWalk, FLT_MAX, 0.3f, 1.5f, 1.0f);
		LoadAttenuation("FOOTSTEPS", "landing.player", gAttenuationSettings.landingPlayer, FLT_MAX, 0.3f, 2.5f, 3.0f);
		LoadAttenuation("FOOTSTEPS", "collapse.player", gAttenuationSettings.collapsePlayer, FLT_MAX, 0.3f, 2.5f, 3.0f);
		if (auto v = readOpt("footsteps.player.pitch", "FOOTSTEPS"))     gPitches.footstepsPlayer = v;
		if (auto v = readOpt("landing.player.pitch", "FOOTSTEPS"))       gPitches.landingPlayer = v;
		if (auto v = readOpt("collapse.player.pitch", "FOOTSTEPS"))      gPitches.collapsePlayer = v;

		// Footsteps NPC
		LoadAttenuation("FOOTSTEPS", "footsteps.npc", gAttenuationSettings.footstepsNPC, FLT_MAX, 0.3f, 2.5f, 3.0f);
		LoadAttenuation("FOOTSTEPS", "footsteps.npc.duck", gAttenuationSettings.footstepsNPCDuck, FLT_MAX, 0.1f, 2.5f, 3.0f);
		LoadAttenuation("FOOTSTEPS", "footsteps.npc.sprint", gAttenuationSettings.footstepsNPCSprint, FLT_MAX, 0.7f, 2.5f, 3.0f);
		LoadAttenuation("FOOTSTEPS", "footsteps.npc.walk", gAttenuationSettings.footstepsNPCWalk, FLT_MAX, 0.2f, 2.5f, 3.0f);
		LoadAttenuation("FOOTSTEPS", "landing.npc", gAttenuationSettings.landingNPC, FLT_MAX, 0.3f, 2.5f, 3.0f);
		LoadAttenuation("FOOTSTEPS", "collapse.npc", gAttenuationSettings.collapseNPC, FLT_MAX, 0.3f, 2.5f, 3.0f);
		if (auto v = readOpt("landing.npc.pitch", "FOOTSTEPS"))         gPitches.landingNPC = v;
		if (auto v = readOpt("collapse.npc.pitch", "FOOTSTEPS"))        gPitches.collapseNPC = v;
		if (auto v = readOpt("footsteps.npc.pitch", "FOOTSTEPS"))       gPitches.footstepsNPC = v;

		// Miscellaneous
		LoadAttenuation("JACKED", "", gAttenuationSettings.jacked, FLT_MAX, 3.0f, 1.5f, 0.8f);
		LoadAttenuation("FIRE", "", gAttenuationSettings.fire, 200.0f, 1.0f, 1.5f, 4.0f);
		LoadAttenuation("FIRE", "nonfire", gAttenuationSettings.nonfire, 200.0f, 1.0f, 1.5f, 4.0f);
		LoadAttenuation("GUNSHELLS", "general_gunshell", gAttenuationSettings.gunshell, FLT_MAX, 1.0f, 1.5f, 4.0f);
		LoadAttenuation("GUNSHELLS", "shotgun_shell", gAttenuationSettings.shotgunshell, FLT_MAX, 1.0f, 1.5f, 3.0f);
		LoadAttenuation("GRENADE_BOUNCE", "", gAttenuationSettings.grenade_bounce, FLT_MAX, 1.0f, 2.0f, 3.0f);
		if (auto v = readOpt("jacked.pitch", "JACKED")) gPitches.jacked = v;
		if (auto v = readOpt("fire.pitch", "FIRE"))   gPitches.fire = v;
		if (auto v = readOpt("nonfire.pitch", "FIRE"))gPitches.nonfire = v;
		if (auto v = readOpt("gunshells.pitch", "GUNSHELLS"))gPitches.gunshell = v;
		if (auto v = readOpt("shotgunshells.pitch", "GUNSHELLS"))gPitches.shotgunshell = v;
		if (auto v = readOpt("grenadebounce.pitch", "GRENADE_BOUNCE"))gPitches.grenade_bounce = v;

		// Explosions
		for (int i = 0; i < MAX_EXPLOSIONTYPES; i++) {
			std::string section = "EXPLOSION_TYPE_" + std::to_string(i);
			std::string defaultSection = "EXPLOSIONS";

			if (!ini.SectionExists(section)) {
				LOG("Section %s didn't exist: using defaults from 'EXPLOSIONS'", section.c_str());
				section = defaultSection;
			}

			LoadAttenuation(section, "explosion.main", gAttenuationSettings.explosion[i], distanceForDistantExplosion, 10.0f, 0.7f, 0.6f);
			LoadAttenuation(section, "explosion.distant", gAttenuationSettings.distexplosion[i], 4000.0f, 10.0f, 0.5f, 1.0f);
			LoadAttenuation(section, "explosion.debris", gAttenuationSettings.debris[i], 150.0f, 1.0f, 1.5f, 1.0f);
			LoadAttenuation(section, "explosion.underwater", gAttenuationSettings.underwater[i], 4000.0f, 15.0f, 0.2f, 3.0f);

			if (auto v = readOpt("explosion.main.pitch", section)) { gPitches.explosion[i] = v; }

			if (auto v = readOpt("explosion.distant.pitch", section)) gPitches.distexplosion[i] = v;

			if (auto v = readOpt("explosion.debris.pitch", section)) gPitches.debris[i] = v;

			if (auto v = readOpt("explosion.underwater.pitch", section)) gPitches.underwater[i] = v;
		}

		// Ricochet surfaces
		for (int i = 0; i < TOTAL_NUM_SURFACE_TYPES; i++) {
			std::string section = "RICOCHET_SURFACE_TYPE_" + std::to_string(i);
			std::string defaultSection = "RICOCHET";

			if (!ini.SectionExists(section)) {
				LOG("Section %s didn't exist: using values from section 'RICOCHET'", section.c_str());
				section = defaultSection;
			}

			if (auto v = readOpt("pitch", section)) gPitches.ricochet[i] = v;
			LoadAttenuation(section, "", gAttenuationSettings.ricochet[i], 50.0f, 3.5f, 5.0f, 3.0f);
		}
	}
}

void Loaders::LoadAmbienceSounds(const fs::path& path, bool loadOldAmbience)
{
	fs::path ambientDir = path / "generic/ambience";
	if (!fs::exists(ambientDir) || !fs::is_directory(ambientDir)) {
		LOG("Ambient sound folder not found: %s", ambientDir.string().c_str());
		return;
	}

	std::vector<std::string> globalRegions = { "country", "LS", "LV", "SF" };
	// Check if a given folder is a global zone folder
	auto isGlobalZoneFolder = [&](const fs::path& folderPath) -> bool {
		std::string folderName = folderPath.filename().string();
		folderName = caseLower(folderName);

		for (const auto& region : globalRegions) {
			std::string regionLower = region;
			regionLower = caseLower(regionLower);
			if (folderName == regionLower)
				return true;
		}
		return false;
		};

	if (loadOldAmbience) {
		fs::path zoneDir = ambientDir / "zones";
		if (fs::exists(zoneDir) && fs::is_directory(zoneDir)) {
			for (const auto& zoneEntry : fs::directory_iterator(zoneDir)) {
				if (!zoneEntry.is_directory()) continue;
				if (isGlobalZoneFolder(zoneEntry.path())) continue;

				std::string zoneName = zoneEntry.path().filename().string();
				zoneName = caseLower(zoneName);

				// iterate files inside that zone folder
				for (const auto& entry : fs::directory_iterator(zoneEntry.path())) {
					if (!entry.is_regular_file()) continue;

					std::string filename = entry.path().filename().string();
					const std::string prefix = "ambience";

					bool matchesExtension = false;
					std::string suffix;
					for (const auto& ext : extensions) {
						if (filename.rfind(prefix, 0) == 0 && filename.size() >= prefix.size() + ext.size() &&
							filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
							matchesExtension = true;
							suffix = ext;
							break;
						}
					}
					if (!matchesExtension) continue;

					std::string name = filename.substr(prefix.size(), filename.size() - prefix.size() - suffix.size());

					std::string timeSuffix;
					std::string digits;

					if (name.rfind("_night") == 0) {
						timeSuffix = "_night";
						digits = name.substr(6);
					}
					else if (name.rfind("_riot") == 0) {
						timeSuffix = "_riot";
						digits = name.substr(5);
					}
					else {
						digits = name;
					}

					if (!digits.empty() && !std::all_of(digits.begin(), digits.end(), ::isdigit)) {
						LOG("Skipping invalid ambience filename: %s", filename.c_str());
						continue;
					}

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(entry.path().string().c_str());
					if (buffer) {
						if (timeSuffix == "_night") g_Buffers.ZoneAmbienceBuffers_Night[zoneName].push_back(buffer);
						else if (timeSuffix == "_riot") g_Buffers.ZoneAmbienceBuffers_Riot[zoneName].push_back(buffer);
						else g_Buffers.ZoneAmbienceBuffers_Day[zoneName].push_back(buffer);

						LOG("Loaded ambience for zone: '%s' (%s) [index=%s] --> %s",
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
			if (!fs::exists(regionDir) || !fs::is_directory(regionDir)) continue;

			for (const auto& entry : fs::directory_iterator(regionDir)) {
				if (!entry.is_regular_file()) continue;

				std::string filename = entry.path().filename().string();
				const std::string prefix = "ambience";

				bool matchesExtension = false;
				std::string suffix;
				for (const auto& ext : extensions) {
					if (filename.rfind(prefix, 0) == 0 && filename.size() >= prefix.size() + ext.size() &&
						filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
						matchesExtension = true;
						suffix = ext;
						break;
					}
				}
				if (!matchesExtension) continue;

				std::string name = filename.substr(prefix.size(), filename.size() - prefix.size() - suffix.size());

				std::string nameWithoutDigits = name;
				while (!nameWithoutDigits.empty() && isdigit(nameWithoutDigits.back())) nameWithoutDigits.pop_back();

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
				while (!zoneName.empty() && isdigit(zoneName.back())) zoneName.pop_back();
				zoneName = caseLower(zoneName);
				std::string globalKey = region;

				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(entry.path().string().c_str());
				if (buffer) {
					if (timeSuffix == "_night") g_Buffers.GlobalZoneAmbienceBuffers_Night[globalKey].push_back(buffer);
					else if (timeSuffix == "_riot") g_Buffers.GlobalZoneAmbienceBuffers_Riot[globalKey].push_back(buffer);
					else g_Buffers.GlobalZoneAmbienceBuffers_Day[globalKey].push_back(buffer);

					LOG("Loaded ambience for global zone: '%s' (%s) --> %s", globalKey.c_str(), timeSuffix.empty() ? "day" : timeSuffix.c_str(), filename.c_str());
				}
			}
		}

		for (int i = 0; i <= MAX_AMBIENCE_ALTERNATIVES; i++) {
			for (const auto& ext : extensions) {
				fs::path dayPath = ambientDir / ("ambience" + std::to_string(i) + ext);
				fs::path nightPath = ambientDir / ("ambience_night" + std::to_string(i) + ext);
				fs::path riotPath = ambientDir / ("ambience_riot" + std::to_string(i) + ext);
				fs::path thunderPath = ambientDir / ("thunder" + std::to_string(i) + ext);

				if (fs::exists(dayPath)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(dayPath.string().c_str());
					if (buffer) g_Buffers.AmbienceBuffs.push_back(buffer);
				}
				if (fs::exists(nightPath)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(nightPath.string().c_str());
					if (buffer) g_Buffers.NightAmbienceBuffs.push_back(buffer);
				}
				if (fs::exists(riotPath)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(riotPath.string().c_str());
					if (buffer) g_Buffers.RiotAmbienceBuffs.push_back(buffer);
				}
				if (fs::exists(thunderPath)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(thunderPath.string().c_str());
					if (buffer) g_Buffers.ThunderBuffs.push_back(buffer);
				}
			}
		}
	}

	// singular fallbacks
	for (const auto& ext : extensions) {
		if (g_Buffers.AmbienceBuffs.empty()) {
			fs::path fallback = ambientDir / ("ambience" + ext);
			if (fs::exists(fallback)) g_Buffers.AmbienceBuffs.push_back(AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str()));
		}
		if (g_Buffers.NightAmbienceBuffs.empty()) {
			fs::path fallback = ambientDir / ("ambience_night" + ext);
			if (fs::exists(fallback)) g_Buffers.NightAmbienceBuffs.push_back(AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str()));
		}
		if (g_Buffers.RiotAmbienceBuffs.empty()) {
			fs::path fallback = ambientDir / ("ambience_riot" + ext);
			if (fs::exists(fallback)) g_Buffers.RiotAmbienceBuffs.push_back(AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str()));
		}
		if (g_Buffers.ThunderBuffs.empty()) {
			fs::path fallback = ambientDir / ("thunder" + ext);
			if (fs::exists(fallback)) g_Buffers.ThunderBuffs.push_back(AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str()));
		}
	}

	// those two below is for LS gun ambience
	fs::path pathToGunfireAmbience = foldermod / "generic" / "ambience" / "gunfire";
	std::vector<std::string> expectedWeapons = { "ak47", "pistol" };
	for (auto& weapon : expectedWeapons) {
		fs::path folderPath = pathToGunfireAmbience / weapon;
		if (!fs::exists(folderPath)) { LOG("Weapon gunfire ambience folder '%s' is missing.", weapon.c_str()); continue; }

		fs::path shootFile;
		for (int i = 0; i <= MAX_SOUND_ALTERNATIVES; i++) {
			for (const auto& ext : extensions) {
				fs::path candidate = folderPath / ("shoot" + ext);
				fs::path candidate2 = folderPath / ("shoot" + std::to_string(i) + ext);
				if (fs::exists(candidate))
				{
					shootFile = candidate; break;
				}
				else if (fs::exists(candidate2))
				{
					shootFile = candidate2; break;
				}
			}
		}
		if (shootFile.empty()) { LOG("Weapon gunfire ambience folder '%s' missing shoot file.", weapon.c_str()); continue; }

		auto weapontype = eWeaponType();
		if (nameType(&weapon, &weapontype)) {
			weaponNames.push_back({ weapontype, weapon });
			LOG("Found weapon sound folder: %s", weapon.c_str());
		}
	}

	for (auto& vec : weaponNames) {
		auto& folderName = vec.weapName;
		fs::path weaponPath;
		for (int i = 0; i <= MAX_SOUND_ALTERNATIVES; i++) {
			for (const auto& ext : extensions) {
				weaponPath = ambientDir / "gunfire" / folderName / ("shoot" + ext);
				if (!fs::exists(weaponPath))
					weaponPath = ambientDir / "gunfire" / folderName / ("shoot" + std::to_string(i) + ext);
				else
					break;
			}
			if (fs::exists(weaponPath) && fs::is_directory(weaponPath.parent_path())) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(weaponPath.string().c_str());
				bool no = false;
				if (folderName.find("ak47") == std::string::npos || folderName.find("pistol") == std::string::npos)
				{
					LOG("Sorry, but only ak47 and pistol are used by the game internally, other weapons won't work.");
					no = true;
				}
				if (buffer != 0) {
					g_Buffers.WeaponTypeAmbienceBuffers[vec.weapType] = buffer;

					if (!no)
						LOG("Loaded weapon ambience for %s: %s", folderName.c_str(), weaponPath.filename().string().c_str());
				}
				else {
					LOG("Failed to load weapon ambience: %s", weaponPath.string());
				}
				break;
			}
		}
	}

	// Manual ambiences
	fs::path manualIni = ambientDir / "map_ambience.ini";
	if (fs::exists(manualIni) && fs::is_regular_file(manualIni)) {
		try {
			CIniReader ini(manualIni.string().c_str());

			// We don't require a count; iterate numbered sections until none found
			for (int i = 0; ; ++i) {
				char section[64];
				sprintf_s(section, "Ambience%d", i);

				std::string files = ini.ReadString(section, "File", "");
				if (!ini.SectionExists(section)) {
					break;
				}

				float x = ini.ReadFloat(section, "X", 0.0f);
				float y = ini.ReadFloat(section, "Y", 0.0f);
				float z = ini.ReadFloat(section, "Z", 0.0f);
				float range = ini.ReadFloat(section, "Range", 50.0f);
				float maxDist = ini.ReadFloat(section, "Max distance attenuation", 50.0f);
				float refDist = ini.ReadFloat(section, "Reference distance", 1.0f);
				float rollOff = ini.ReadFloat(section, "Roll off factor", 1.0f);
				float airAbsorption = ini.ReadFloat(section, "Air absorption", 1.0f);
				bool loop = ini.ReadBoolean(section, "Loop", false);
				bool allow = ini.ReadBoolean(section, "Allow other ambiences", false);
				uint32_t delay = (uint32_t)ini.ReadInteger(section, "Delay", 30000);
				std::string timeStr = ini.ReadString(section, "Time", "any");

				EAmbienceTime timeType = EAmbienceTime::Any;
				timeStr = caseLower(timeStr);
				if (timeStr == "day") timeType = EAmbienceTime::Day;
				else if (timeStr == "night") timeType = EAmbienceTime::Night;
				else if (timeStr == "riot") timeType = EAmbienceTime::Riot;
				else timeType = EAmbienceTime::Any;

				// Split the File= line by commas
				std::vector<ALuint> buffers;
				std::stringstream ss(files);
				std::string token;
				while (std::getline(ss, token, ',')) {
					// Trim whitespace
					std::string trimmed = trimStr(token);

					if (trimmed.empty())
						continue;

					// Resolve file path: allow absolute or relative to ambientDir
					fs::path audioPath = trimmed.front() == '/' || (trimmed.size() > 1 && trimmed[1] == ':')
						? fs::path(trimmed)
						: ambientDir / trimmed;

					if (!fs::exists(audioPath)) {
						LOG("Manual ambience file not found: %s (section %s)", audioPath.string().c_str(), section);
						continue;
					}
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(audioPath.string().c_str());
					if (buffer == 0) {
						LOG("Failed to create buffer for manual ambience: %s", audioPath.string().c_str());
						continue;
					}
					buffers.push_back(buffer);
				}
				if (!buffers.empty()) {
					auto it = std::find_if(g_ManualAmbiences.begin(), g_ManualAmbiences.end(),
						[&](const ManualAmbience& s) { return s.buffer == buffers; });
					if (it == g_ManualAmbiences.end()) {
						ManualAmbience ma;
						ma.pos = CVector(x, y, z);
						ma.range = range;
						ma.loop = loop;
						ma.buffer = buffers;
						ma.time = timeType;
						ma.delay = delay;
						ma.nextPlayTime = 0;
						ma.sphere.Set(ma.range, ma.pos);
						ma.refDist = refDist;
						ma.rollOff = rollOff;
						ma.maxDist = maxDist;
						ma.airAbsorption = airAbsorption;
						ma.allowOtherAmbiences = allow;

						g_ManualAmbiences.push_back(ma);

						LOG("Loaded manual ambience (section %s): %s @(%.1f, %.1f, %.1f) R=%.1f Loop=%d Time=%s Delay=%d Buffers=%zu",
							section, files.c_str(), x, y, z, range, loop, timeStr.c_str(), delay, g_ManualAmbiences.back().buffer.size());
					}
				}
			}
		}
		catch (...) {
			LOG("Failed to parse manual ambience ini: %s", manualIni.string().c_str());
		}
	}
}

void Loaders::InstallHooks()
{
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
	subhookCAudioEngine__ReportWeaponEvent = subhook_new((void*)(originalCAudioEngine__ReportWeaponEvent)0x506F40, HookedCAudioEngine__ReportWeaponEvent, subhook_flags_t(0));
	subhookCAEWeaponAudioEntity__PlayFlameThrowerSounds = subhook_new((void*)(originalCAEWeaponAudioEntity__PlayFlameThrowerSounds)0x504470, CAEWeaponAudioEntity__PlayFlameThrowerSounds, subhook_flags_t(0));
	subhookCAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop = subhook_new((void*)(originalCAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop)0x503870, CAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop, subhook_flags_t(0));
	subhookCAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop = subhook_new((void*)(originalCAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop)0x5034E0, CAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop, subhook_flags_t(0));
	subhookCAESound__StopSoundAndForget = subhook_new((void*)(originalStopSoundAndForget)0x4EF850, CAESound__StopSirenSound, subhook_flags_t(0));
	subhookMainWndProc = subhook_new((void*)(originalMainWndProc)0x747EB0, MainWndProcHOOK, subhook_flags_t(0));
	subhookCAudioEngine__ReportCollision = subhook_new((void*)(originalCAudioEngine__ReportCollision)0x506EB0, CAudioEngine__ReportCollision, subhook_flags_t(0));
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
	subhook_install(subhookCAudioEngine__ReportWeaponEvent);
	subhook_install(subhookCAEWeaponAudioEntity__PlayFlameThrowerSounds);
	subhook_install(subhookCAEWeaponAudioEntity__PlayFlameThrowerIdleGasLoop);
	subhook_install(subhookCAEWeaponAudioEntity__StopFlameThrowerIdleGasLoop);
	subhook_install(subhookCAESound__StopSoundAndForget);
	subhook_install(subhookMainWndProc);
	subhook_install(subhookCAudioEngine__ReportCollision);
#if 0
	subhook_install(subhookPlaySoundHook);
#endif

	//patch::RedirectCall({ 0x504D11, 0x504CD2 }, PlayMinigunBarrelStopSound);
	patch::RedirectCall(0x72BB37, HookedCAEWeatherAudioEntity__AddAudioEvent);
	patch::RedirectCall(0x504BD4, StopFlamethrowerFireSound);
	patch::RedirectCall(0x504C4D, StopSpraycanSound);
	patch::RedirectCall(0x504C71, StopFireExtinguisherSound);
	patch::RedirectCall({ 0x505196, 0x5051CB }, CAEWeaponAudioEntity__PlayWeaponLoopSound);
	// for flamethrower, spraycan, extinguisher, chainsaw, and minigun
	patch::RedirectCall({ 0x504539, 0x5045E6, 0x5038E8,
						  0x5046C5, 0x5048D9, 0x504A6A,
						  0x503E58, 0x503EEC, 0x5040DC,
						  0x504153, 0x5041DF, 0x504277,
						  0x5043B5, 0x50443E, 0x503AE0, 0x503A36, 
						  0x504B3E }, CAESound__Dummy);

	patch::RedirectCall({ 0x4F9E2E, 0x4F9D61 }, CAESound__DummyVeh);
	//patch::RedirectCall({ 0x4F9E79, 0x4F9DA4 }, CAESound__StopSirenSound);
	patch::RedirectCall(0x500366, CAEVehicleAudioEntity__PlayHornOrSiren);
	//patch::RedirectCall({ 0x4F9C23, 0x4F9AD0, 0x4F9E2E, 0x4F9D1A, 0x4F9D6 }, CAESound__DummyVeh);
	patch::RedirectCall(0x50493D, CAEWeaponAudioEntity__PlayGunSounds);
	patch::RedirectCall({ 0x504CEC, 0x504D22 }, StopMinigunSounds);
	patch::RedirectCall(0x504D8F, StopChainsawSounds);
	patch::RedirectCall(0x4E6A67, PlayChainsawEvent);
	// to mute original sounds if a replacement for it is present
	patch::RedirectCall(0x4F041A, CAESound__CalculateVolume);

	patch::RedirectCall(0x4E2C5F, CAEPedAudioEntity__HandleLandingEvent);

	patch::RedirectCall(0x50523A, CAEWeaponAudioEntity__PlayGoggleSound);
	patch::RedirectCall(0x5051ED, CAEWeaponAudioEntity__PlayCameraSound);
	patch::RedirectCall(0x5E604E, CPed__RemoveGogglesModel);
	patch::RedirectCall(0x4DCF4B, CAESoundManager__CancelSoundsOwnedByAudioEntity);
	//patch::RedirectCall({ 0x4DBD5A, 0x4DBC68 }, CAECollisionAudioEntity__PlayOneShotCollisionSound);
	//patch::RedirectCall(0x4E6A4C, CWeaponAudio__PlayStealthEvent);
	//patch::RedirectCall({ 0x61F1C9, 0x61F288, 0x61F2C3, 0x61EF0B, 0x61EF49, 0x61F0D8, 0x61F114 }, IKChainManager_c__PointArm);
	//patch::RedirectCall({ 0x61F128, 0x61EF7A, 0x61EF66 }, CPedIK__RotateTorsoForArm);
	//patch::RedirectCall({ 0x61EEDC, 0x61F0AE, 0x61F1A7, 0x61F25F }, IKChainManager_c__LookAt);
#ifdef QUAKE_KILLSOUNDS_TEST
	patch::RedirectCall(0x4B93AA, HookedRegisterKillByPlayer);
#endif
}

void Loaders::RegisterAllWeapons()
{
	auto LoadAttenuation = [&](const std::string& keyPrefix, Attenuation& att,
		float MaxDist, float RefDist, float Rolloff, float AirAbs)
		{
			att.maxDist = MaxDist;
			att.refDist = RefDist;
			att.rolloffFactor = Rolloff;
			att.airAbsorption = AirAbs;
		};

	LOG("File(s):");

	for (auto& directoryentry : fs::recursive_directory_iterator(foldermod)) {
		fs::path entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			std::string fileextension = caseLower(entrypath.extension().string());
			if (fileextension == modextension)
			{
				registerWeapon(entrypath);

				// open the .earshot file and read line-by-line if we want to read settings from it later
				std::ifstream infile(entrypath);
				if (!infile.is_open()) {
					LOG("Failed to open file: %s", entrypath.string().c_str());
					continue;
				}

				AttenuationSet aset = gAttenuationSettings;
				Pitch pset{};

				auto makeKey = [](const std::string& prefix, const std::string& suffix) -> std::string {
					if (prefix.empty()) return suffix;
					return prefix + "." + suffix;
					};

				auto readOptFromLine = [&](const std::string& line, const std::string& kname) -> std::optional<float> {
					auto pos = line.find(kname);
					if (pos == std::string::npos) return std::nullopt;

					// find '=' after the key
					auto eq = line.find('=', pos + kname.size());
					if (eq == std::string::npos) return std::nullopt;
					std::size_t i = eq + 1;

					// skip whitespace
					while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
					if (i >= line.size()) return std::nullopt;

					// parse until whitespace or comment delimiter
					std::size_t j = i;
					while (j < line.size() && !std::isspace(static_cast<unsigned char>(line[j])) && line[j] != ';' && line[j] != '#') ++j;

					float value{};
					auto res = std::from_chars(line.data() + i, line.data() + j, value);
					if (res.ec == std::errc()) return value;
					return std::nullopt;
					};

				std::string line;
				while (std::getline(infile, line)) {
					// list of attenuation prefixes we care about
					const std::vector<std::string> attPrefixes = {
						"shoot","reload", "after", "low_ammo","distant","vehicle","melee",
						"minigun.spinLoop","minigun.spinEnd","minigun.fireLoop",
						"fireExtinguisher.sprayLoop","sprayCan.sprayLoop",
						"chainsaw.idleLoop","chainsaw.activeLoop","chainsaw.cuttingLoop","chainsaw.stop"
					};

					for (const auto& prefix : attPrefixes) {
						// maxDist
						if (auto v = readOptFromLine(line, makeKey(prefix, "maxDist"))) {
							// map prefix to the correct Attenuation reference in aset
							if (prefix == "shoot") aset.base.maxDist = *v;
							else if (prefix == "reload") aset.reload.maxDist = *v;
							else if (prefix == "after") aset.after.maxDist = *v;
							else if (prefix == "low_ammo") aset.low_ammo.maxDist = *v;
							else if (prefix == "distant") aset.distant.maxDist = *v;
							else if (prefix == "vehicle") aset.vehicle.maxDist = *v;
							else if (prefix == "melee") aset.quieter.maxDist = *v;
							else if (prefix == "minigun.spinLoop") gAttenuationSettings.minigunSpin.maxDist = *v;
							else if (prefix == "minigun.spinEnd") gAttenuationSettings.minigunSpinEnd.maxDist = *v;
							else if (prefix == "minigun.fireLoop") gAttenuationSettings.minigunShoot.maxDist = *v;
							else if (prefix == "fireExtinguisher.sprayLoop") gAttenuationSettings.fireExtinguisher.maxDist = *v;
							else if (prefix == "sprayCan.sprayLoop") gAttenuationSettings.sprayCan.maxDist = *v;
							else if (prefix == "chainsaw.idleLoop") gAttenuationSettings.chainsawIdle.maxDist = *v;
							else if (prefix == "chainsaw.activeLoop") gAttenuationSettings.chainsawActive.maxDist = *v;
							else if (prefix == "chainsaw.cuttingLoop") gAttenuationSettings.chainsawCutting.maxDist = *v;
							else if (prefix == "chainsaw.stop") gAttenuationSettings.chainsawStop.maxDist = *v;
						}

						// refDist
						if (auto v = readOptFromLine(line, makeKey(prefix, "refDist"))) {
							if (prefix == "shoot") aset.base.refDist = *v;
							else if (prefix == "reload") aset.reload.refDist = *v;
							else if (prefix == "after") aset.after.refDist = *v;
							else if (prefix == "low_ammo") aset.low_ammo.refDist = *v;
							else if (prefix == "distant") aset.distant.refDist = *v;
							else if (prefix == "vehicle") aset.vehicle.refDist = *v;
							else if (prefix == "melee") aset.quieter.refDist = *v;
							else if (prefix == "minigun.spinLoop") gAttenuationSettings.minigunSpin.refDist = *v;
							else if (prefix == "minigun.spinEnd") gAttenuationSettings.minigunSpinEnd.refDist = *v;
							else if (prefix == "minigun.fireLoop") gAttenuationSettings.minigunShoot.refDist = *v;
							else if (prefix == "fireExtinguisher.sprayLoop") gAttenuationSettings.fireExtinguisher.refDist = *v;
							else if (prefix == "sprayCan.sprayLoop") gAttenuationSettings.sprayCan.refDist = *v;
							else if (prefix == "chainsaw.idleLoop") gAttenuationSettings.chainsawIdle.refDist = *v;
							else if (prefix == "chainsaw.activeLoop") gAttenuationSettings.chainsawActive.refDist = *v;
							else if (prefix == "chainsaw.cuttingLoop") gAttenuationSettings.chainsawCutting.refDist = *v;
							else if (prefix == "chainsaw.stop") gAttenuationSettings.chainsawStop.refDist = *v;
						}

						// rolloffFactor
						if (auto v = readOptFromLine(line, makeKey(prefix, "rolloffFactor"))) {
							if (prefix == "shoot") aset.base.rolloffFactor = *v;
							else if (prefix == "reload") aset.reload.rolloffFactor = *v;
							else if (prefix == "after") aset.after.rolloffFactor = *v;
							else if (prefix == "low_ammo") aset.low_ammo.rolloffFactor = *v;
							else if (prefix == "distant") aset.distant.rolloffFactor = *v;
							else if (prefix == "vehicle") aset.vehicle.rolloffFactor = *v;
							else if (prefix == "melee") aset.quieter.rolloffFactor = *v;
							else if (prefix == "minigun.spinLoop") gAttenuationSettings.minigunSpin.rolloffFactor = *v;
							else if (prefix == "minigun.spinEnd") gAttenuationSettings.minigunSpinEnd.rolloffFactor = *v;
							else if (prefix == "minigun.fireLoop") gAttenuationSettings.minigunShoot.rolloffFactor = *v;
							else if (prefix == "fireExtinguisher.sprayLoop") gAttenuationSettings.fireExtinguisher.rolloffFactor = *v;
							else if (prefix == "sprayCan.sprayLoop") gAttenuationSettings.sprayCan.rolloffFactor = *v;
							else if (prefix == "chainsaw.idleLoop") gAttenuationSettings.chainsawIdle.rolloffFactor = *v;
							else if (prefix == "chainsaw.activeLoop") gAttenuationSettings.chainsawActive.rolloffFactor = *v;
							else if (prefix == "chainsaw.cuttingLoop") gAttenuationSettings.chainsawCutting.rolloffFactor = *v;
							else if (prefix == "chainsaw.stop") gAttenuationSettings.chainsawStop.rolloffFactor = *v;
						}

						// airAbsorption
						if (auto v = readOptFromLine(line, makeKey(prefix, "airAbsorption"))) {
							if (prefix == "shoot") aset.base.airAbsorption = *v;
							else if (prefix == "reload") aset.reload.airAbsorption = *v;
							else if (prefix == "after") aset.after.airAbsorption = *v;
							else if (prefix == "low_ammo") aset.low_ammo.airAbsorption = *v;
							else if (prefix == "distant") aset.distant.airAbsorption = *v;
							else if (prefix == "vehicle") aset.vehicle.airAbsorption = *v;
							else if (prefix == "melee") aset.quieter.airAbsorption = *v;
							else if (prefix == "minigun.spinLoop") gAttenuationSettings.minigunSpin.airAbsorption = *v;
							else if (prefix == "minigun.spinEnd") gAttenuationSettings.minigunSpinEnd.airAbsorption = *v;
							else if (prefix == "minigun.fireLoop") gAttenuationSettings.minigunShoot.airAbsorption = *v;
							else if (prefix == "fireExtinguisher.sprayLoop") gAttenuationSettings.fireExtinguisher.airAbsorption = *v;
							else if (prefix == "sprayCan.sprayLoop") gAttenuationSettings.sprayCan.airAbsorption = *v;
							else if (prefix == "chainsaw.idleLoop") gAttenuationSettings.chainsawIdle.airAbsorption = *v;
							else if (prefix == "chainsaw.activeLoop") gAttenuationSettings.chainsawActive.airAbsorption = *v;
							else if (prefix == "chainsaw.cuttingLoop") gAttenuationSettings.chainsawCutting.airAbsorption = *v;
							else if (prefix == "chainsaw.stop") gAttenuationSettings.chainsawStop.airAbsorption = *v;
						}
					} // end attPrefixes loop

					// pitches
					// weapons
					if (auto v = readOptFromLine(line, "shoot.pitch"))      pset.shoot = *v;
					if (auto v = readOptFromLine(line, "reload.pitch"))     pset.reload = *v;
					if (auto v = readOptFromLine(line, "after.pitch"))      pset.after = *v;
					if (auto v = readOptFromLine(line, "distant.pitch"))    pset.distant = *v;
					if (auto v = readOptFromLine(line, "low_ammo.pitch"))   pset.low_ammo = *v;
					if (auto v = readOptFromLine(line, "vehicle.pitch"))    pset.vehicle = *v;
					if (auto v = readOptFromLine(line, "melee.pitch"))      pset.quieter = *v;

					// minigun specific
					if (auto v = readOptFromLine(line, "minigun.spinLoop.pitch"))  gPitches.minigunSpin = *v;
					if (auto v = readOptFromLine(line, "minigun.spinEnd.pitch"))   gPitches.minigunSpinEnd = *v;
					if (auto v = readOptFromLine(line, "minigun.fireLoop.pitch"))  gPitches.minigunShoot = *v;
					if (auto v = readOptFromLine(line, "minigun.spin.pitch"))      gPitches.minigunSpin = *v;
					if (auto v = readOptFromLine(line, "minigun.spinEnd.pitch"))   gPitches.minigunSpinEnd = *v;
					if (auto v = readOptFromLine(line, "minigun.fire.pitch"))      gPitches.minigunShoot = *v;

					// flamethrower
					if (auto v = readOptFromLine(line, "flamethrower.start.pitch"))     gPitches.flamethrowerStart = *v;
					if (auto v = readOptFromLine(line, "flamethrower.fireLoop.pitch"))  gPitches.flamethrowerFireLoop = *v;
					if (auto v = readOptFromLine(line, "flamethrower.gasLoop.pitch"))   gPitches.flamethrowerGasLoop = *v;
					if (auto v = readOptFromLine(line, "flamethrower.startLoop.pitch")) gPitches.flamethrowerStart = *v; // alt

					// extinguisher / spraycan
					if (auto v = readOptFromLine(line, "fireExtinguisher.sprayLoop.pitch")) gPitches.fireExtinguisher = *v;
					if (auto v = readOptFromLine(line, "fireextinguisher.pitch"))           gPitches.fireExtinguisher = *v; // alt
					if (auto v = readOptFromLine(line, "sprayCan.sprayLoop.pitch"))         gPitches.sprayCan = *v;
					if (auto v = readOptFromLine(line, "spraycan.pitch"))                   gPitches.sprayCan = *v; // alt

					// chainsaw
					if (auto v = readOptFromLine(line, "chainsaw.idleLoop.pitch"))   gPitches.chainsawIdle = *v;
					if (auto v = readOptFromLine(line, "chainsaw.activeLoop.pitch")) gPitches.chainsawActive = *v;
					if (auto v = readOptFromLine(line, "chainsaw.cuttingLoop.pitch"))gPitches.chainsawCutting = *v;
					if (auto v = readOptFromLine(line, "chainsaw.stop.pitch"))       gPitches.chainsawStop = *v;
					if (auto v = readOptFromLine(line, "chainsaw.idle.pitch"))       gPitches.chainsawIdle = *v;    // alternate names
					if (auto v = readOptFromLine(line, "chainsaw.active.pitch"))     gPitches.chainsawActive = *v;
					if (auto v = readOptFromLine(line, "chainsaw.cutting.pitch"))    gPitches.chainsawCutting = *v;
					gWeaponAttenuations[filename] = aset;
					gWeaponPitches[filename] = pset;
				}
			}
		}
	}

	if (registeredweapons.empty()) {
		LOG("No file(s) found. Stopping work for this session.");
		return;
	}
	size_t registeredtotal = registeredweapons.size();
	LOG("Total registered weapons: %zu", registeredtotal);
}


void Loaders::ReloadAudioFolders()
{
	gWeaponAttenuations.clear();
	gWeaponPitches.clear();
	gvehInfo.clear();
	gentInfo.clear();
	// Reload .ini as well
	InitializeIniFile(1, true);

	// Stop and delete all currently playing sound sources
	for (auto& inst : AudioManager.audiosplaying)
	{
		if (inst->source != 0) {
			ALint state = AL_STOPPED;
			alGetSourcei(inst->source, AL_SOURCE_STATE, &state);
			alSourcei(inst->source, AL_BUFFER, AL_NONE); // Detach buffer from source
			if (state == AL_PLAYING || state == AL_PAUSED) {
				AudioManager.PauseSource(&*inst);
			}
			LOG("Removing source '%u', missile '%u', minigun spin '%u'", inst->source, inst->missileSource, AudioManager.barrelSpinSource);
			alDeleteSources(1, &inst->source);
			if (inst->missileSource)
			{
				alDeleteSources(1, &inst->missileSource);
			}
			if (AudioManager.barrelSpinSource)
			{
				alDeleteSources(1, &AudioManager.barrelSpinSource);
			}
			inst->source = 0;
			AudioManager.barrelSpinSource = 0;
		}
		inst->isAmbience = false;
		inst->isGunfireAmbience = false;
		inst->isPossibleGunFire = false;
	}
	AudioManager.audiosplaying.clear();  // Remove all sound instances

	//Delete all loaded buffers
	for (auto& buf : AudioManager.gBufferMap) {
		if (buf.second != 0) {
			LOG("Freeing buffer for reload %u, path: %s", buf.second, buf.first.c_str());
			alDeleteBuffers(1, &buf.second);
			buf.second = 0;
		}
	}
	AudioManager.gBufferMap.clear();

	// Remove every single OpenAL buffer
	DeleteAllBuffers(g_Buffers);
	registeredweapons.clear();
	weaponNames.clear();
	AudioManager.UnloadManualAmbiences();
	// Finally reload all folders
	RegisterAllWeapons();
	LoadExplosionRelatedSounds(foldermod);
	LoadJackingRelatedSounds(foldermod);
	LoadFireSounds(foldermod);
	LoadAmbienceSounds(foldermod);
	LoadRicochetSounds(foldermod);
	LoadFootstepSounds(foldermod);
	LoadGunshellSounds(foldermod);
	LoadTankCannonSounds(foldermod);
	LoadMissileSounds(foldermod);
	LoadBulletWhizzSounds(foldermod);
	LoadMinigunSounds(foldermod);
	LoadChainsawSounds(foldermod);
	LoadFlamethrowerSounds(foldermod);
	LoadSpraycanSound(foldermod);
	LoadExtinguisherSound(foldermod);
	LoadCarSirenSounds(foldermod);
	LoadGrenadeBounceSounds(foldermod);
}

void Loaders::LoadMissileSounds(const fs::path& folder) {
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		fs::path foundFile;
		for (const auto& ext : extensions) {
			fs::path candidate = folder / ("Missiles/missile_flyloop" + std::to_string(index) + ext);
			if (fs::exists(candidate)) {
				foundFile = candidate;
				break;
			}
		}

		if (!foundFile.empty()) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
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
		for (const auto& ext : extensions) {
			fs::path fallbackPath = folder / ("Missiles/missile_flyloop" + ext);
			if (fs::exists(fallbackPath)) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallbackPath.string().c_str());
				if (buffer != 0) g_Buffers.missileSoundBuffers.push_back(buffer);
				break;
			}
		}
	}
}

void Loaders::LoadRicochetSounds(const fs::path& folder) {
	const fs::path ricoDir = folder / "generic/ricochet";
	if (!fs::exists(ricoDir))
	{
		LOG("Ricochet folder wasn't found, '%s'", ricoDir.string().c_str());
		return;
	}
	const std::vector<std::string> surfaces = {
		"default", "metal", "wood", "water", "dirt", "glass", "stone", "sand", "flesh"
	};

	for (const auto& surface : surfaces) {
		std::vector<ALuint>& buffers = g_Buffers.ricochetBuffersPerMaterial[surface];

		int index = 0;
		while (true) {
			fs::path foundFile;
			for (const auto& ext : extensions) {
				fs::path candidate = folder / ("generic/ricochet/" + surface + "/ricochet" + std::to_string(index) + ext);
				if (fs::exists(candidate)) {
					foundFile = candidate;
					break;
				}
			}
			if (foundFile.empty()) break;

			ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
			if (buffer != 0) buffers.push_back(buffer);

			++index;
		}

		// fallback to non-indexed file
		if (buffers.empty()) {
			for (const auto& ext : extensions) {
				fs::path fileNoIndex = folder / ("generic/ricochet/" + surface + "/ricochet" + ext);
				if (fs::exists(fileNoIndex)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fileNoIndex.string().c_str());
					if (buffer != 0) buffers.push_back(buffer);
					LOG("Loaded ricochet sound for surface: %s", surface.c_str());
					break;
				}
			}
		}
		else {
			LOG("Loaded %d ricochet sound(s) for surface: %s", (int)buffers.size(), surface.c_str());
		}
	}
}

void Loaders::LoadGunshellSounds(const fs::path& folder)
{
	fs::path gunshellsDir = folder / "generic/gunshells";
	if (!fs::exists(gunshellsDir))
	{
		LOG("Gunshells folder wasn't found, '%s'", gunshellsDir.string().c_str());
		return;
	}

	const std::vector<std::string> surfaces = {
		"default", "metal", "wood", "water", "dirt", "glass", "stone", "sand"
	};

	auto loadSurface = [&](const fs::path& surfaceDir, std::vector<ALuint>& buffers, const std::string& prefix)
		{
			int index = 0;

			while (true)
			{
				fs::path foundFile;

				for (const auto& ext : extensions)
				{
					fs::path candidate = surfaceDir / (prefix + std::to_string(index) + ext);
					if (fs::exists(candidate))
					{
						foundFile = candidate;
						break;
					}
				}

				if (foundFile.empty())
					break;

				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
				if (buffer != 0)
					buffers.push_back(buffer);

				++index;
			}

			if (buffers.empty())
			{
				for (const auto& ext : extensions)
				{
					fs::path candidate = surfaceDir / (prefix + ext);
					if (fs::exists(candidate))
					{
						ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(candidate.string().c_str());
						if (buffer != 0)
							buffers.push_back(buffer);
						break;
					}
				}
			}

			LOG("Loaded %d %s sound(s) from: %s",
				(int)buffers.size(),
				prefix.c_str(),
				surfaceDir.string().c_str());
		};

	for (const auto& surface : surfaces)
	{
		fs::path surfaceDir = gunshellsDir / surface;

		if (!fs::exists(surfaceDir) || !fs::is_directory(surfaceDir))
		{
			LOG("Surface folder wasn't found, '%s'", surfaceDir.string().c_str());
			continue;
		}

		loadSurface(surfaceDir, g_Buffers.gunshellBuffersPerSurface[surface], "gunshell");
		loadSurface(surfaceDir, g_Buffers.shotgunshellBuffersPerSurface[surface], "shotgunshell");
	}
}

void Loaders::LoadFootstepSounds(const fs::path& baseFolder) {
	const fs::path footstepsDir = baseFolder / "generic/footsteps";
	if (!fs::exists(footstepsDir))
	{
		LOG("Footsteps folder wasn't found, '%s'", footstepsDir.string().c_str());
		return;
	}
	// Detect shoe folders
	for (const auto& entry : fs::directory_iterator(footstepsDir)) {
		if (!entry.is_directory()) continue;

		const std::string shoeType = entry.path().filename().string();

		bool isSurfaceOnly = false;
		for (const auto& ext : extensions) {
			fs::path step0File = entry.path() / ("step0" + ext);
			fs::path stepFile = entry.path() / ("step" + ext);
			fs::path landing0File = entry.path() / ("landing0" + ext);
			fs::path landingFile = entry.path() / ("landing" + ext);
			if (fs::exists(step0File) || fs::exists(stepFile) || fs::exists(landing0File) || fs::exists(landingFile)) {
				isSurfaceOnly = true;
				break;
			}
		}
		if (isSurfaceOnly) {
			// Footsteps
			std::vector<ALuint>& buffers = g_Buffers.footstepSurfaceBuffers[shoeType];
			int index = 0;
			while (true) {
				fs::path foundFile;
				for (const auto& ext : extensions) {
					fs::path candidate = entry.path() / ("step" + std::to_string(index) + ext);
					if (fs::exists(candidate)) {
						foundFile = candidate;
						break;
					}
				}
				if (foundFile.empty()) break;

				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
				if (buffer != 0) buffers.push_back(buffer);
				++index;
			}

			// fallback to single file for steps
			if (buffers.empty()) {
				for (const auto& ext : extensions) {
					fs::path fallback = entry.path() / ("step" + ext);
					if (fs::exists(fallback)) {
						ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
						if (buffer != 0) buffers.push_back(buffer);
						break;
					}
				}
			}

			if (!buffers.empty()) {
				LOG("Loaded %d general footstep sounds for surface '%s'", (int)buffers.size(), shoeType.c_str());
			}

			// Landings
			std::vector<ALuint>& lBuffers = g_Buffers.landingPerSurfaceBuffers[shoeType];
			index = 0;
			while (true) {
				fs::path foundFile;
				for (const auto& ext : extensions) {
					fs::path candidate = entry.path() / ("landing" + std::to_string(index) + ext);
					if (fs::exists(candidate)) {
						foundFile = candidate;
						break;
					}
				}
				if (foundFile.empty()) break;

				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
				if (buffer != 0) lBuffers.push_back(buffer);
				++index;
			}

			// fallback to single file for landings
			if (lBuffers.empty()) {
				for (const auto& ext : extensions) {
					fs::path fallback = entry.path() / ("landing" + ext);
					if (fs::exists(fallback)) {
						ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
						if (buffer != 0) lBuffers.push_back(buffer);
						break;
					}
				}
			}

			if (!lBuffers.empty()) {
				LOG("Loaded %d general landing sounds for surface '%s'", (int)lBuffers.size(), shoeType.c_str());
			}

			// Collapse (surface-only)
			std::vector<ALuint>& cBuffers = g_Buffers.collapsePerSurfaceBuffers[shoeType];
			index = 0;
			while (true) {
				fs::path foundFile;
				for (const auto& ext : extensions) {
					fs::path candidate = entry.path() / ("collapse" + std::to_string(index) + ext);
					if (fs::exists(candidate)) {
						foundFile = candidate;
						break;
					}
				}
				if (foundFile.empty()) break;

				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
				if (buffer != 0) cBuffers.push_back(buffer);
				++index;
			}

			// fallback to single file for collapses
			if (cBuffers.empty()) {
				for (const auto& ext : extensions) {
					fs::path fallback = entry.path() / ("collapse" + ext);
					if (fs::exists(fallback)) {
						ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
						if (buffer != 0) cBuffers.push_back(buffer);
						break;
					}
				}
			}

			if (!cBuffers.empty()) {
				LOG("Loaded %d general collapse sounds for surface '%s'", (int)cBuffers.size(), shoeType.c_str());
			}
		}
		else {
			// It is a shoe folder
			for (const auto& surfaceEntry : fs::directory_iterator(entry.path())) {
				if (!surfaceEntry.is_directory()) continue;

				const std::string surfaceType = surfaceEntry.path().filename().string();
				std::vector<ALuint>& buffers = g_Buffers.footstepShoeBuffers[shoeType][surfaceType];
				std::vector<ALuint>& buffersByTexture = g_Buffers.footstepShoeByTextureBuffers[shoeType][surfaceType];

				int index = 0;
				while (true) {
					fs::path foundFile;
					for (const auto& ext : extensions) {
						fs::path candidate = surfaceEntry.path() / ("step" + std::to_string(index) + ext);
						if (fs::exists(candidate)) {
							foundFile = candidate;
							break;
						}
					}
					if (foundFile.empty()) break;

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
					if (buffer != 0) buffers.push_back(buffer);
					++index;
				}

				// fallback to single file
				if (buffers.empty()) {
					for (const auto& ext : extensions) {
						fs::path fallback = surfaceEntry.path() / ("step" + ext);
						if (fs::exists(fallback)) {
							ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
							if (buffer != 0) buffers.push_back(buffer);
							break;
						}
					}
				}

				// load footsep sounds but by a texture name
				index = 0;
				while (true) {
					fs::path foundFile;
					for (const auto& ext : extensions) {
						fs::path candidate = surfaceEntry.path() / ("step" + std::to_string(index) + ext);
						if (fs::exists(candidate)) {
							foundFile = candidate;
							break;
						}
					}
					if (foundFile.empty()) break;

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
					if (buffer != 0) buffersByTexture.push_back(buffer);
					++index;
				}

				// fallback to single file
				if (buffersByTexture.empty()) {
					for (const auto& ext : extensions) {
						fs::path fallback = surfaceEntry.path() / ("step" + ext);
						if (fs::exists(fallback)) {
							ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
							if (buffer != 0) buffersByTexture.push_back(buffer);
							break;
						}
					}
				}

				// Landings (specific shoe + surface)
				std::vector<ALuint>& lBuffers = g_Buffers.landingBuffers[shoeType][surfaceType];
				std::vector<ALuint>& lBuffersByTexture = g_Buffers.landingByShoeTextureBuffers[shoeType][surfaceType];
				index = 0;
				while (true) {
					fs::path foundFile;
					for (const auto& ext : extensions) {
						fs::path candidate = surfaceEntry.path() / ("landing" + std::to_string(index) + ext);
						if (fs::exists(candidate)) {
							foundFile = candidate;
							break;
						}
					}
					if (foundFile.empty()) break;

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
					if (buffer != 0) lBuffers.push_back(buffer);
					++index;
				}

				// fallback to single file for landings
				if (lBuffers.empty()) {
					for (const auto& ext : extensions) {
						fs::path fallback = surfaceEntry.path() / ("landing" + ext);
						if (fs::exists(fallback)) {
							ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
							if (buffer != 0) lBuffers.push_back(buffer);
							break;
						}
					}
				}

				// --- load texture-specific landing buffers (kept separate) ---
				index = 0;
				while (true) {
					fs::path foundFile;
					for (const auto& ext : extensions) {
						fs::path candidate = surfaceEntry.path() / ("landing" + std::to_string(index) + ext);
						if (fs::exists(candidate)) {
							foundFile = candidate;
							break;
						}
					}
					if (foundFile.empty()) break;

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
					if (buffer != 0) lBuffersByTexture.push_back(buffer);
					++index;
				}

				// fallback to single file for landing-by-texture
				if (lBuffersByTexture.empty()) {
					for (const auto& ext : extensions) {
						fs::path fallback = surfaceEntry.path() / ("landing" + ext);
						if (fs::exists(fallback)) {
							ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
							if (buffer != 0) lBuffersByTexture.push_back(buffer);
							break;
						}
					}
				}

				// Collapse (specific shoe + surface)
				std::vector<ALuint>& cBuffers = g_Buffers.collapseBuffers[shoeType][surfaceType];
				std::vector<ALuint>& cBuffersByTexture = g_Buffers.collapseByShoeTextureBuffers[shoeType][surfaceType];
				index = 0;
				while (true) {
					fs::path foundFile;
					for (const auto& ext : extensions) {
						fs::path candidate = surfaceEntry.path() / ("collapse" + std::to_string(index) + ext);
						if (fs::exists(candidate)) {
							foundFile = candidate;
							break;
						}
					}
					if (foundFile.empty()) break;

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
					if (buffer != 0) cBuffers.push_back(buffer);
					++index;
				}

				// fallback to single file for collapses
				if (cBuffers.empty()) {
					for (const auto& ext : extensions) {
						fs::path fallback = surfaceEntry.path() / ("collapse" + ext);
						if (fs::exists(fallback)) {
							ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
							if (buffer != 0) cBuffers.push_back(buffer);
							break;
						}
					}
				}

				// load collapse sounds by shoe-texture
				index = 0;
				while (true) {
					fs::path foundFile;
					for (const auto& ext : extensions) {
						fs::path candidate = surfaceEntry.path() / ("collapse" + std::to_string(index) + ext);
						if (fs::exists(candidate)) {
							foundFile = candidate;
							break;
						}
					}
					if (foundFile.empty()) break;

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
					if (buffer != 0) cBuffersByTexture.push_back(buffer);
					++index;
				}

				// fallback to single file for collapse-by-texture
				if (cBuffersByTexture.empty()) {
					for (const auto& ext : extensions) {
						fs::path fallback = surfaceEntry.path() / ("collapse" + ext);
						if (fs::exists(fallback)) {
							ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
							if (buffer != 0) cBuffersByTexture.push_back(buffer);
							break;
						}
					}
				}
			}
		}
	}
}


void Loaders::LoadExplosionRelatedSounds(const fs::path& folder) {
	// No point in continuing, the main folder doesn't even exist
	const fs::path expDir = folder / "generic/explosions";
	if (!fs::exists(expDir))
	{
		LOG("Explosions folder wasn't found, '%s'", expDir.string().c_str());
		return;
	}

	g_Buffers.ExplosionTypeExplosionBuffers.clear();
	g_Buffers.ExplosionTypeDebrisBuffers.clear();
	g_Buffers.ExplosionTypeDistantBuffers.clear();
	g_Buffers.ExplosionTypeUnderwaterBuffers.clear();
	g_lastExplosionType.clear();

	auto loadBuffers = [](const fs::path& path, std::vector<ALuint>& vec, const std::string& prefix) {
		int idx = 0;
		bool indexedLoaded = false;
		while (true) {
			fs::path foundFile;
			for (const auto& ext : extensions) {
				fs::path candidate = path / (prefix + std::to_string(idx) + ext);
				if (fs::exists(candidate)) {
					foundFile = candidate;
					break;
				}
			}
			if (foundFile.empty()) break;

			indexedLoaded = true;
			ALuint buf = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
			if (buf != 0) vec.push_back(buf);
			++idx;
		}
		if (!indexedLoaded)
		{
			for (const auto& ext : extensions) {
				fs::path candidate = path / (prefix + ext);
				if (fs::exists(candidate)) {
					ALuint buf = AudioManager.CreateOpenALBufferFromAudioFile(candidate.string().c_str());
					if (buf != 0) vec.push_back(buf);
					break;
				}
			}
		}
		};

	// load per-explosion-type folders
	fs::path explosionTypesDir = folder / "generic/explosions/explosionTypes";
	if (fs::exists(explosionTypesDir) && fs::is_directory(explosionTypesDir)) {
		for (auto& entry : fs::directory_iterator(explosionTypesDir)) {
			if (!fs::is_directory(entry.path())) continue;
			std::string name = entry.path().filename().string();
			int typeID = -1;
			try { typeID = std::stoi(name); }
			catch (...) { continue; }

			{
				std::vector<ALuint> vec;
				loadBuffers(entry.path(), vec, "explosion");
				if (!vec.empty()) g_Buffers.ExplosionTypeExplosionBuffers[typeID] = std::move(vec);
			}
			{
				std::vector<ALuint> vec;
				loadBuffers(entry.path(), vec, "debris");
				if (!vec.empty()) g_Buffers.ExplosionTypeDebrisBuffers[typeID] = std::move(vec);
			}
			{
				std::vector<ALuint> vec;
				loadBuffers(entry.path(), vec, "distant");
				if (!vec.empty()) g_Buffers.ExplosionTypeDistantBuffers[typeID] = std::move(vec);
			}
			{
				std::vector<ALuint> vec;
				loadBuffers(entry.path(), vec, "underwater");
				if (!vec.empty()) g_Buffers.ExplosionTypeUnderwaterBuffers[typeID] = std::move(vec);
			}

			LOG("Loaded explosionType '%d' sounds from %s", typeID, entry.path().string().c_str());
		}
	}

	// Load generic explosions as fallback
	int idx = 0;
	bool anyIndexedLoaded = false;
	std::vector<SoundFile> genericFiles = {
		{ "explosion", g_Buffers.explosionBuffers },
		{ "debris", g_Buffers.explosionsDebrisBuffers },
		{ "distant", g_Buffers.explosionDistantBuffers },
		{ "underwater", g_Buffers.explosionUnderwaterBuffers }
	};

	while (true) {
		bool loadedSomething = false;
		for (auto& file : genericFiles) {
			fs::path foundFile;
			for (const auto& ext : extensions) {
				fs::path candidate = folder / ("generic/explosions/" + file.fileName + std::to_string(idx) + ext);
				if (fs::exists(candidate)) {
					foundFile = candidate;
					break;
				}
			}
			if (!foundFile.empty()) {
				ALuint buf = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
				if (buf != 0) file.bufferVec.push_back(buf);
				loadedSomething = true;
				anyIndexedLoaded = true;
			}
		}
		if (!loadedSomething) break;
		++idx;
	}

	// fallback: try non-indexed if nothing was loaded with indexes
	if (!anyIndexedLoaded) {
		for (auto& file : genericFiles) {
			for (const auto& ext : extensions) {
				fs::path candidate = folder / ("generic/explosions/" + file.fileName + ext);
				if (fs::exists(candidate)) {
					ALuint buf = AudioManager.CreateOpenALBufferFromAudioFile(candidate.string().c_str());
					if (buf != 0) file.bufferVec.push_back(buf);
					break;
				}
			}
		}
	}
}


void Loaders::LoadFireSounds(const fs::path& folder) {
	const fs::path fireDir = folder / "generic/fire";
	if (!fs::exists(fireDir))
	{
		LOG("Fire folder wasn't found, '%s'", fireDir.string().c_str());
		return;
	}
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		std::vector<SoundFile> filesToLoad = {
			{ "fire_smallloop", g_Buffers.fireLoopBuffersSmall },
			{ "fire_mediumloop", g_Buffers.fireLoopBuffersMedium },
			{ "fire_largeloop", g_Buffers.fireLoopBuffersLarge },
			{ "fire_carloop", g_Buffers.fireLoopBuffersCar },
			{ "fire_bikeloop", g_Buffers.fireLoopBuffersBike },
			{ "fire_flameloop", g_Buffers.fireLoopBuffersFlame },
			{ "fire_molotovloop", g_Buffers.fireLoopBuffersMolotov }
		};

		for (auto& file : filesToLoad) {
			fs::path foundFile;
			for (const auto& ext : extensions) {
				fs::path candidate = folder / ("generic/fire/" + file.fileName + std::to_string(index) + ext);
				if (fs::exists(candidate)) {
					foundFile = candidate;
					break;
				}
			}

			if (!foundFile.empty()) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
				if (buffer != 0) {
					file.bufferVec.push_back(buffer);
					loadedSomething = true;
				}
			}
		}

		if (!loadedSomething)
			break;

		++index;
	}

	// Fallbacks
	fs::path fallback;

	std::vector<SoundFile> fallbacks = {
		{ "fire_smallloop", g_Buffers.fireLoopBuffersSmall },
		{ "fire_mediumloop", g_Buffers.fireLoopBuffersMedium },
		{ "fire_largeloop", g_Buffers.fireLoopBuffersLarge },
		{ "fire_carloop", g_Buffers.fireLoopBuffersCar },
		{ "fire_bikeloop", g_Buffers.fireLoopBuffersBike },
		{ "fire_flameloop", g_Buffers.fireLoopBuffersFlame },
		{ "fire_molotovloop", g_Buffers.fireLoopBuffersMolotov }
	};

	for (auto& fb : fallbacks) {
		if (fb.bufferVec.empty()) {
			for (const auto& ext : extensions) {
				fallback = folder / ("generic/fire/" + fb.fileName + ext);
				if (fs::exists(fallback)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
					if (buffer != 0) fb.bufferVec.push_back(buffer);
					break;
				}
			}
		}
	}
}
// Fuck off dirty minded people
void Loaders::LoadJackingRelatedSounds(const fs::path& folder) {
	const fs::path jackDir = folder / "generic/jacked";
	if (!fs::exists(jackDir))
	{
		LOG("Car jacking folder wasn't found, '%s'", jackDir.string().c_str());
		return;
	}
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		std::vector<SoundFile> filesToLoad = {
			{ "jack_car", g_Buffers.carJackBuff },
			{ "jack_carheadbang", g_Buffers.carJackHeadBangBuff },
			{ "jack_carkick", g_Buffers.carJackKickBuff },
			{ "jack_bike", g_Buffers.carJackBikeBuff },
			{ "jack_bulldozer", g_Buffers.carJackBulldozerBuff }
		};

		for (auto& file : filesToLoad) {
			fs::path foundFile;
			for (const auto& ext : extensions) {
				fs::path candidate = folder / ("generic/jacked/" + file.fileName + std::to_string(index) + ext);
				if (fs::exists(candidate)) {
					foundFile = candidate;
					break;
				}
			}

			if (!foundFile.empty()) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(foundFile.string().c_str());
				if (buffer != 0) {
					file.bufferVec.push_back(buffer);
					loadedSomething = true;
				}
			}
		}

		if (!loadedSomething)
			break;

		++index;
	}

	// Fallback
	fs::path fallback;

	std::vector<SoundFile> fallbacks = {
		{ "jack_car", g_Buffers.carJackBuff },
		{ "jack_carheadbang", g_Buffers.carJackHeadBangBuff },
		{ "jack_carkick", g_Buffers.carJackKickBuff },
		{ "jack_bike", g_Buffers.carJackBikeBuff },
		{ "jack_bulldozer", g_Buffers.carJackBulldozerBuff }
	};

	for (auto& fb : fallbacks) {
		if (fb.bufferVec.empty()) {
			for (const auto& ext : extensions) {
				fallback = folder / ("generic/jacked/" + fb.fileName + ext);
				if (fs::exists(fallback)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(fallback.string().c_str());
					if (buffer != 0) fb.bufferVec.push_back(buffer);
					break;
				}
			}
		}
	}
}