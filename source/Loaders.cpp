#include "Loaders.h"
#include "AudioManager.h"
#include "IniReader.h"

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

void Loaders::LoadMinigunBarrelSpinSound(const fs::path& folder)
{
	auto weapontype = eWeaponType();

	for (auto& directoryentry : fs::recursive_directory_iterator(folder)) {
		const auto& entrypath = directoryentry.path();
		if (!fs::is_directory(entrypath)) {
			std::string filename = entrypath.stem().string();
			if (nameType(&filename, &weapontype)) {
				fs::path foundSpin;
				for (const auto& ext : extensions) {
					fs::path spinPath = entrypath.parent_path() / ("spin" + ext);
					if (fs::exists(spinPath)) {
						foundSpin = spinPath;
						break;
					}
				}
				if (!foundSpin.empty()) {
					barrelSpinBuffer = AudioManager.CreateOpenALBufferFromAudioFile(foundSpin.string().c_str());
					break;
				}
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
			for (const auto& zoneEntry : fs::directory_iterator(zoneDir)) {
				if (!zoneEntry.is_directory()) continue;
				if (isGlobalZoneFolder(zoneEntry.path())) continue;

				std::string zoneName = zoneEntry.path().filename().string();
				std::transform(zoneName.begin(), zoneName.end(), zoneName.begin(), ::tolower);

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
						Log("Skipping invalid ambience filename: %s", filename.c_str());
						continue;
					}

					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(entry.path().string().c_str());
					if (buffer) {
						if (timeSuffix == "_night") g_Buffers.ZoneAmbienceBuffers_Night[zoneName].push_back(buffer);
						else if (timeSuffix == "_riot") g_Buffers.ZoneAmbienceBuffers_Riot[zoneName].push_back(buffer);
						else g_Buffers.ZoneAmbienceBuffers_Day[zoneName].push_back(buffer);

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
				std::transform(zoneName.begin(), zoneName.end(), zoneName.begin(), ::tolower);
				std::string globalKey = region;

				ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(entry.path().string().c_str());
				if (buffer) {
					if (timeSuffix == "_night") g_Buffers.GlobalZoneAmbienceBuffers_Night[globalKey].push_back(buffer);
					else if (timeSuffix == "_riot") g_Buffers.GlobalZoneAmbienceBuffers_Riot[globalKey].push_back(buffer);
					else g_Buffers.GlobalZoneAmbienceBuffers_Day[globalKey].push_back(buffer);

					Log("Loaded ambience for global zone: '%s' (%s) --> %s", globalKey.c_str(), timeSuffix.empty() ? "day" : timeSuffix.c_str(), filename.c_str());
				}
			}
		}

		for (int i = 0; i <= 300; ++i) {
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

	// Interior ambience
	fs::path interiorDir = ambientDir / "interiors";
	if (fs::exists(interiorDir) && fs::is_directory(interiorDir)) {
		for (const auto& interiorEntry : fs::directory_iterator(interiorDir)) {
			if (!interiorEntry.is_directory()) continue;
			std::string interiorIDStr = interiorEntry.path().filename().string();
			int interiorID = 0;
			try { interiorID = std::stoi(interiorIDStr); }
			catch (...) { Log("Invalid interior folder name (not a number): %s", interiorIDStr.c_str()); continue; }

			for (const auto& nameEntry : fs::directory_iterator(interiorEntry.path())) {
				if (!nameEntry.is_directory()) continue;
				std::string gxtKey = nameEntry.path().filename().string();
				std::transform(gxtKey.begin(), gxtKey.end(), gxtKey.begin(), ::tolower);

				for (int i = 0; i <= 300; ++i) {
					fs::path chosenDay, chosenNight;
					fs::path chosenNoIndexDay, chosenNoIndexNight;

					// Look for indexed and non-indexed variants across ALL extensions.
					// Stop early only when we've found every variant we care about.
					for (const auto& ext : extensions) {
						if (chosenDay.empty()) {
							fs::path p = nameEntry.path() / ("ambience" + std::to_string(i) + ext);
							if (fs::exists(p)) chosenDay = p;
						}
						if (chosenNight.empty()) {
							fs::path p = nameEntry.path() / ("ambience_night" + std::to_string(i) + ext);
							if (fs::exists(p)) chosenNight = p;
						}

						if (chosenNoIndexDay.empty()) {
							fs::path p = nameEntry.path() / ("ambience" + ext);
							if (fs::exists(p)) chosenNoIndexDay = p;
						}
						if (chosenNoIndexNight.empty()) {
							fs::path p = nameEntry.path() / ("ambience_night" + ext);
							if (fs::exists(p)) chosenNoIndexNight = p;
						}

						// if we've found everything we could possibly want, break early
						if (!chosenDay.empty() && !chosenNight.empty()
							&& (i != 0 || (!chosenNoIndexDay.empty() && !chosenNoIndexNight.empty())))
						{
							break;
						}
					}

					// Now decide which branch to take
					bool IndexedExists = !chosenDay.empty() || !chosenNight.empty();
					bool NonIndexExists = !chosenNoIndexDay.empty() || !chosenNoIndexNight.empty();

					if (IndexedExists) {
						ALuint bufferDay = chosenDay.empty() ? 0 : AudioManager.CreateOpenALBufferFromAudioFile(chosenDay.string().c_str());
						ALuint bufferNight = chosenNight.empty() ? 0 : AudioManager.CreateOpenALBufferFromAudioFile(chosenNight.string().c_str());

						if (bufferDay || bufferNight) {
							InteriorAmbience sound{ gxtKey, bufferDay, bufferNight };
							g_InteriorAmbience[interiorID].push_back(sound);
							Log("Loaded interior sound for interior ID %d, GXT '%s': day=%s night=%s",
								interiorID, gxtKey.c_str(),
								chosenDay.empty() ? "none" : chosenDay.filename().string().c_str(),
								chosenNight.empty() ? "none" : chosenNight.filename().string().c_str()
							);
						}
					}
					else if (NonIndexExists) {
						ALuint bufferDay = chosenNoIndexDay.empty() ? 0 : AudioManager.CreateOpenALBufferFromAudioFile(chosenNoIndexDay.string().c_str());
						ALuint bufferNight = chosenNoIndexNight.empty() ? 0 : AudioManager.CreateOpenALBufferFromAudioFile(chosenNoIndexNight.string().c_str());

						if (bufferDay || bufferNight) {
							InteriorAmbience sound{ gxtKey, bufferDay, bufferNight };
							g_InteriorAmbience[interiorID].push_back(sound);
							Log("Loaded singular interior sound for interior ID %d, GXT '%s': day=%s night=%s",
								interiorID, gxtKey.c_str(),
								chosenNoIndexDay.empty() ? "none" : chosenNoIndexDay.filename().string().c_str(),
								chosenNoIndexNight.empty() ? "none" : chosenNoIndexNight.filename().string().c_str()
							);
						}
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
		if (!fs::exists(folderPath)) { Log("Weapon gunfire ambience folder '%s' is missing.", weapon.c_str()); continue; }

		fs::path shootFile;
		for (const auto& ext : extensions) {
			fs::path candidate = folderPath / ("shoot" + ext);
			if (fs::exists(candidate)) { shootFile = candidate; break; }
		}
		if (shootFile.empty()) { Log("Weapon gunfire ambience folder '%s' missing shoot file.", weapon.c_str()); continue; }

		auto weapontype = eWeaponType();
		if (nameType(&weapon, &weapontype)) {
			weaponNames.push_back({ weapontype, weapon });
			Log("Found weapon sound folder: %s", weapon.c_str());
		}
	}

	for (auto& vec : weaponNames) {
		auto& folderName = vec.weapName;
		fs::path weaponPath;
		for (const auto& ext : extensions) {
			weaponPath = ambientDir / "gunfire" / folderName / ("shoot" + ext);
			if (fs::exists(weaponPath)) break;
		}
		if (fs::exists(weaponPath) && fs::is_directory(weaponPath.parent_path())) {
			ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(weaponPath.string().c_str());
			if (buffer != 0) {
				g_Buffers.WeaponTypeAmbienceBuffers[vec.weapType] = buffer;
				Log("Loaded weapon ambience for %s: %s", folderName.c_str(), weaponPath.filename().string().c_str());
			}
			else {
				Log("Failed to load weapon ambience: %s", weaponPath.string());
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
				if (files.empty()) {
					break;
				}

				float x = ini.ReadFloat(section, "X", 0.0f);
				float y = ini.ReadFloat(section, "Y", 0.0f);
				float z = ini.ReadFloat(section, "Z", 0.0f);
				float range = ini.ReadFloat(section, "Range", 50.0f);
				float maxDist = ini.ReadFloat(section, "Max distance attenuation", 50.0f);
				float refDist = ini.ReadFloat(section, "Reference distance", 1.0f);
				float rollOff = ini.ReadFloat(section, "Roll off factor", 1.0f);
				bool loop = ini.ReadBoolean(section, "Loop", false);
				bool allow = ini.ReadBoolean(section, "Allow other ambiences", false);
				uint32_t delay = (uint32_t)ini.ReadInteger(section, "Delay", 30000);
				std::string timeStr = ini.ReadString(section, "Time", "any");

				EAmbienceTime timeType = EAmbienceTime::Any;
				std::transform(timeStr.begin(), timeStr.end(), timeStr.begin(), ::tolower);
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
					token.erase(0, token.find_first_not_of(" \t\r\n"));
					token.erase(token.find_last_not_of(" \t\r\n") + 1);

					if (token.empty())
						continue;

					// Resolve file path: allow absolute or relative to ambientDir
					fs::path audioPath = token.front() == '/' || (token.size() > 1 && token[1] == ':')
						? fs::path(token)
						: ambientDir / token;

					if (!fs::exists(audioPath)) {
						Log("Manual ambience file not found: %s (section %s)", audioPath.string().c_str(), section);
						continue;
					}
					ALuint buffer = AudioManager.CreateOpenALBufferFromAudioFile(audioPath.string().c_str());
					if (buffer == 0) {
						Log("Failed to create buffer for manual ambience: %s", audioPath.string().c_str());
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
						ma.allowOtherAmbiences = allow;

						g_ManualAmbiences.push_back(ma);

						Log("Loaded manual ambience (section %s): %s @(%.1f, %.1f, %.1f) R=%.1f Loop=%d Time=%s Buffers=%zu",
							section, files.c_str(), x, y, z, range, loop, timeStr.c_str(), g_ManualAmbiences.back().buffer.size());
					}
				}
			}
		}
		catch (...) {
			Log("Failed to parse manual ambience ini: %s", manualIni.string().c_str());
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
	distanceForDistantGunshot = ini.ReadFloat("MAIN", "Distant gunshot distance", 50.0f);
	distanceForDistantExplosion = ini.ReadFloat("MAIN", "Distant explosion distance", 100.0f);
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

	//Delete all loaded buffers
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
	AudioManager.UnloadManualAmbiences();
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
					Log("Loaded ricochet sound for surface: %s", surface.c_str());
					break;
				}
			}
		}
		else {
			Log("Loaded %d ricochet sound(s) for surface: %s", (int)buffers.size(), surface.c_str());
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

		bool isSurfaceOnly = false;
		for (const auto& ext : extensions) {
			if (fs::exists(entry.path() / ("step0" + ext)) || fs::exists(entry.path() / ("step" + ext))) {
				isSurfaceOnly = true;
				break;
			}
		}
		if (isSurfaceOnly) {
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

			// fallback to single file
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
	g_Buffers.ExplosionTypeUnderwaterBuffers.clear();

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

	// --- load per-explosion-type folders ---
	fs::path explosionTypesDir = folder / "generic/explosions/explosionTypes";
	if (fs::exists(explosionTypesDir) && fs::is_directory(explosionTypesDir)) {
		for (auto& entry : fs::directory_iterator(explosionTypesDir)) {
			if (!fs::is_directory(entry.path())) continue;
			std::string name = entry.path().filename().string();
			int typeID = -1;
			try { typeID = std::stoi(name); }
			catch (...) { continue; }

			auto& mainVec = g_Buffers.ExplosionTypeExplosionBuffers[typeID];
			loadBuffers(entry.path(), mainVec, "explosion");

			auto& debrisVec = g_Buffers.ExplosionTypeDebrisBuffers[typeID];
			loadBuffers(entry.path(), debrisVec, "debris");

			auto& distantVec = g_Buffers.ExplosionTypeDistantBuffers[typeID];
			loadBuffers(entry.path(), distantVec, "distant");

			auto& underwaterVec = g_Buffers.ExplosionTypeUnderwaterBuffers[typeID];
			loadBuffers(entry.path(), underwaterVec, "underwater");

			Log("Loaded explosionType '%d' sounds from %s", typeID, entry.path().string().c_str());
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
		Log("LoadFireSounds: fire folder wasn't found, '%s'", fireDir.string().c_str());
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