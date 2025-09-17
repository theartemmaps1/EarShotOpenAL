#include "Loaders.h"
#include "AudioManager.h"
#include "IniReader.h"

// Because the tank cannon doesn't have any weapon types on it, we hardcode it to this folder :shrug:
void Loaders::LoadTankCannonSounds(const fs::path& folder) {
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		fs::path firePath = folder / ("Tank Cannon/cannon_fire" + std::to_string(index) + ".wav");
		if (fs::exists(firePath)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(firePath.string().c_str());
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
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallbackPath.string().c_str());
			if (buffer != 0) {
				g_Buffers.tankCannonFireBuffers.push_back(buffer);
			}
		}
	}
}

void Loaders::LoadBulletWhizzSounds(const fs::path& folder) {
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

			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(path.string().c_str());
			if (buffer != 0) {
				entry.buffer->push_back(buffer);
			}

			++index;
		}

		// Fallback: single sound without index
		if (entry.buffer->empty()) {
			fs::path fallback = folder / ("generic/bullet_whizz/" + entry.name + ".wav");
			if (fs::exists(fallback)) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
				if (buffer != 0) {
					entry.buffer->push_back(buffer);
				}
			}
		}
	}
}

void Loaders::LoadAmbienceSounds(const std::filesystem::path& path, bool loadOldAmbience)
{
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

					ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(entry.path().string().c_str());
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

				// Prefix region to zoneName so LS/downtown and SF/downtown dont collide
				std::string globalKey = region/* + "_" + zoneName*/;

				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(entry.path().string().c_str());
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
				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(dayPath.string().c_str());
				if (buffer) g_Buffers.AmbienceBuffs.push_back(buffer);
			}

			if (fs::exists(nightPath)) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(nightPath.string().c_str());
				if (buffer) g_Buffers.NightAmbienceBuffs.push_back(buffer);
			}

			if (fs::exists(riotPath)) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(riotPath.string().c_str());
				if (buffer) g_Buffers.RiotAmbienceBuffs.push_back(buffer);
			}
			if (fs::exists(thunderPath))
			{
				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(thunderPath.string().c_str());
				if (buffer) g_Buffers.ThunderBuffs.push_back(buffer);
			}
		}
	}
	// singular
	if (g_Buffers.AmbienceBuffs.empty()) {
		fs::path fallback = ambientDir / "ambience.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer)g_Buffers.AmbienceBuffs.push_back(buffer);
		}
	}

	if (g_Buffers.NightAmbienceBuffs.empty()) {
		fs::path fallback = ambientDir / "ambience_night.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer) g_Buffers.NightAmbienceBuffs.push_back(buffer);
		}
	}

	if (g_Buffers.RiotAmbienceBuffs.empty()) {
		fs::path fallback = ambientDir / "ambience_riot.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer) g_Buffers.RiotAmbienceBuffs.push_back(buffer);
		}
	}

	if (g_Buffers.ThunderBuffs.empty()) {
		fs::path fallback = ambientDir / "thunder.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
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
						ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(wavEntry.string().c_str());
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
						ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(wavEntryNoIndex.string().c_str());
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
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(weaponPath.string().c_str());
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

void Loaders::LoadMainWeaponsFolder()
{
	Log("File(s):");
	for (auto& directoryentry : fs::recursive_directory_iterator(foldermod)) {
		auto entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			auto filename = entrypath.stem().string();
			auto fileextension = caseLower(entrypath.extension().string());
			if (fileextension == modextension) registerWeapon(entrypath);
		}
	}
	auto registeredtotal = registeredweapons.size();
	if (registeredtotal == 0) {
		initializationstatus = -1;
		Log("No file(s) found. Stopping work for this session.");
		return;
	}
	Log("Total registered weapons: %d", registeredtotal);
}

void Loaders::LoadMinigunBarrelSpinSound(const fs::path& folder)
{
	auto weapontype = eWeaponType();

	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			if (nameType(&filename, &weapontype)) {
				fs::path spinPath = entrypath.parent_path() / "spin.wav";
				if (fs::exists(spinPath)) {
					barrelSpinBuffer = AudioManager.CreateOpenALBufferFromWAV(spinPath.string().c_str());
					break;
				}
			}
		}
	}
}

void Loaders::ReloadAudioFolders()
{
	// Reload .ini as well
	CIniReader ini(PLUGIN_PATH("EarShot.ini"));
	Logging = ini.ReadBoolean("MAIN", "Logging", false);
	maxBytesInLog = (uint64_t)ini.ReadInteger("MAIN", "Max bytes in log", 9000000);

	interiorIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Interior interval min", 5000);
	interiorIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Interior interval max", 10000);
	fireIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Ambience interval min", 5000);
	fireIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Ambience interval max", 10000);
	zoneIntervalMin = (uint32_t)ini.ReadInteger("MAIN", "Zone ambience interval min", 5000);
	zoneIntervalMax = (uint32_t)ini.ReadInteger("MAIN", "Zone ambience interval max", 10000);
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
	AudioManager.audiosplaying.clear();  // Remove all sound instances

	//Delete all loaded WAV buffers
	for (auto& buf : AudioManager.gBufferMap) {
		if (buf.second != 0) {
			Log("Freeing buffer for reload %u, name: %s", buf.second, buf.first.c_str());
			alDeleteBuffers(1, &buf.second);
			buf.second = 0;
		}
	}
	AudioManager.gBufferMap.clear();

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
	LoadMinigunBarrelSpinSound(foldermod);
	LoadMainWeaponsFolder();
}

void Loaders::LoadMissileSounds(const fs::path& folder) {
	int index = 0;

	while (true) {
		bool loadedSomething = false;

		fs::path missilePath = folder / ("Missiles/missile_flyloop" + std::to_string(index) + ".wav");
		if (fs::exists(missilePath)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(missilePath.string().c_str());
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
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallbackPath.string().c_str());
			if (buffer != 0) {
				g_Buffers.missileSoundBuffers.push_back(buffer);
			}
		}
	}
}

void Loaders::LoadRicochetSounds(const fs::path& folder) {
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

			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(file.string().c_str());
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
				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fileNoIndex.string().c_str());

				if (buffer != 0)
					buffers.push_back(buffer);

				Log("Loaded ricochet sound for surface: %s", surface.c_str());
			}
		}
	}
}

void Loaders::LoadFootstepSounds(const fs::path& baseFolder) {
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

				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(stepFile.string().c_str());
				if (buffer != 0) buffers.push_back(buffer);
				++index;
			}

			// fallback to a single file
			fs::path fallback = entry.path() / "step.wav";
			if (buffers.empty() && fs::exists(fallback)) {
				ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
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

					ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(stepFile.string().c_str());
					if (buffer != 0) buffers.push_back(buffer);
					++index;
				}

				// fallback to a single file
				fs::path fallback = surfaceEntry.path() / "step.wav";
				if (buffers.empty() && fs::exists(fallback)) {
					ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
					if (buffer != 0) buffers.push_back(buffer);
				}

				if (!buffers.empty()) {
					Log("Loaded %d footstep sounds for shoe '%s' on surface '%s'", (int)buffers.size(), shoeType.c_str(), surfaceType.c_str());
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
		Log("LoadExplosionRelatedSounds: explosions folder wasn't found, '%s'", expDir.string().c_str());
		return;
	}

	g_Buffers.ExplosionTypeExplosionBuffers.clear();
	g_Buffers.ExplosionTypeDebrisBuffers.clear();
	g_Buffers.ExplosionTypeDistantBuffers.clear();

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
				ALuint buf = AudioManager.CreateOpenALBufferFromWAV(file.string().c_str());
				if (buf != 0) vec.push_back(buf);
				++idx;
			}
		}
		if (!indexedLoaded)
		{
			fs::path file = path / (prefix + ".wav");
			if (fs::exists(file))
			{
				ALuint buf = AudioManager.CreateOpenALBufferFromWAV(file.string().c_str());
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

			Log("Loaded explosionType '%d' sounds from %s", typeID, entry.path().string().c_str());
		}
	}

	// Load generic explosions as fallback
	int idx = 0;
	bool anyIndexedLoaded = false;
	while (true) {
		bool loadedSomething = false;

		fs::path explosionFile = folder / ("generic/explosions/explosion" + std::to_string(idx) + ".wav");
		if (fs::exists(explosionFile)) {
			g_Buffers.explosionBuffers.push_back(AudioManager.CreateOpenALBufferFromWAV(explosionFile.string().c_str()));
			loadedSomething = true;
			anyIndexedLoaded = true;
		}

		fs::path debrisFile = folder / ("generic/explosions/debris" + std::to_string(idx) + ".wav");
		if (fs::exists(debrisFile)) {
			g_Buffers.explosionsDebrisBuffers.push_back(AudioManager.CreateOpenALBufferFromWAV(debrisFile.string().c_str()));
			loadedSomething = true;
			anyIndexedLoaded = true;
		}

		fs::path distantFile = folder / ("generic/explosions/distant" + std::to_string(idx) + ".wav");
		if (fs::exists(distantFile)) {
			g_Buffers.explosionDistantBuffers.push_back(AudioManager.CreateOpenALBufferFromWAV(distantFile.string().c_str()));
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
			g_Buffers.explosionBuffers.push_back(AudioManager.CreateOpenALBufferFromWAV(explosionFile.string().c_str()));

		fs::path debrisFile = folder / "generic/explosions/debris.wav";
		if (fs::exists(debrisFile))
			g_Buffers.explosionsDebrisBuffers.push_back(AudioManager.CreateOpenALBufferFromWAV(debrisFile.string().c_str()));

		fs::path distantFile = folder / "generic/explosions/distant.wav";
		if (fs::exists(distantFile))
			g_Buffers.explosionDistantBuffers.push_back(AudioManager.CreateOpenALBufferFromWAV(distantFile.string().c_str()));
	}

}


void Loaders::LoadFireSounds(const fs::path& folder) {
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
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathSmall.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersSmall.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathMedium = folder / ("generic/fire/fire_mediumloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathMedium)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathMedium.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersMedium.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathLarge = folder / ("generic/fire/fire_largeloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathLarge)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathLarge.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersLarge.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathCar = folder / ("generic/fire/fire_carloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathCar)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathCar.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersCar.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathBike = folder / ("generic/fire/fire_bikeloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathBike)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathBike.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersBike.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathFlame = folder / ("generic/fire/fire_flameloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathFlame)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathFlame.string().c_str());
			if (buffer != 0) {
				g_Buffers.fireLoopBuffersFlame.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathMolotov = folder / ("generic/fire/fire_molotovloop" + std::to_string(index) + ".wav");
		if (fs::exists(pathMolotov)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathMolotov.string().c_str());
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
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersSmall.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersMedium.empty()) {
		fallback = folder / "generic/fire/fire_mediumloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersMedium.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersLarge.empty()) {
		fallback = folder / "generic/fire/fire_largeloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersLarge.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersCar.empty()) {
		fallback = folder / "generic/fire/fire_carloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersCar.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersBike.empty()) {
		fallback = folder / "generic/fire/fire_bikeloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersBike.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersFlame.empty()) {
		fallback = folder / "generic/fire/fire_flameloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersFlame.push_back(buffer);
		}
	}

	if (g_Buffers.fireLoopBuffersMolotov.empty()) {
		fallback = folder / "generic/fire/fire_molotovloop.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.fireLoopBuffersMolotov.push_back(buffer);
		}
	}
}

// Copy paste of the previous func
void Loaders::LoadJackingRelatedSounds(const fs::path& folder) {
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
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathCar.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathHead = folder / ("generic/jacked/jack_carheadbang" + std::to_string(index) + ".wav");
		if (fs::exists(pathHead)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathHead.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackHeadBangBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathKick = folder / ("generic/jacked/jack_carkick" + std::to_string(index) + ".wav");
		if (fs::exists(pathKick)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathKick.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackKickBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathBike = folder / ("generic/jacked/jack_bike" + std::to_string(index) + ".wav");
		if (fs::exists(pathBike)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathBike.string().c_str());
			if (buffer != 0) {
				g_Buffers.carJackBikeBuff.push_back(buffer);
				loadedSomething = true;
			}
		}

		fs::path pathDozer = folder / ("generic/jacked/jack_bulldozer" + std::to_string(index) + ".wav");
		if (fs::exists(pathDozer)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(pathDozer.string().c_str());
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
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackHeadBangBuff.empty()) {
		fallback = folder / "generic/jacked/jack_carheadbang.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackHeadBangBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackKickBuff.empty()) {
		fallback = folder / "generic/jacked/jack_carkick.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackKickBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackBikeBuff.empty()) {
		fallback = folder / "generic/jacked/jack_bike.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackBikeBuff.push_back(buffer);
		}
	}

	if (g_Buffers.carJackBulldozerBuff.empty()) {
		fallback = folder / "generic/jacked/jack_bulldozer.wav";
		if (fs::exists(fallback)) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromWAV(fallback.string().c_str());
			if (buffer != 0) g_Buffers.carJackBulldozerBuff.push_back(buffer);
		}
	}
}