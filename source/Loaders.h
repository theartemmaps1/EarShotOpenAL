#pragma once
#include "plugin.h"
#include <filesystem>
#include "ourCommon.h"
#include "AudioManager.h"
namespace fs = std::filesystem;
class Loaders {
public:
	static void LoadMinigunBarrelSpinSound(const fs::path& folder);
	static void LoadRicochetSounds(const fs::path& folder);
	static void LoadFootstepSounds(const fs::path& baseFolder);
	static void LoadExplosionRelatedSounds(const fs::path& folder);
	static void LoadFireSounds(const fs::path& folder);
	static void LoadJackingRelatedSounds(const fs::path& folder);
	static void LoadMainWeaponsFolder();
	static void LoadAmbienceSounds(const std::filesystem::path& path, bool loadOldAmbience = true);
	static void LoadMissileSounds(const fs::path& folder);
	static void LoadTankCannonSounds(const fs::path& folder);
	static void LoadBulletWhizzSounds(const fs::path& folder);
	static void ReloadAudioFolders();
};

struct InteriorAmbience {
	std::string gxtKey;
	ALuint buffer;
};


struct WeapInfos
{
	eWeaponType weapType;
	std::string weapName;
};

struct SoundFile {
	std::string fileName;
	std::vector<ALuint>& bufferVec;
};

inline std::unordered_map<int, std::vector<InteriorAmbience>> g_InteriorAmbience;
inline std::vector<WeapInfos> weaponNames;
inline auto registeredweapons = map<pair<eWeaponType, eModelID>, AudioStream>();

inline auto registerWeapon = [&](fs::path& filepath) {
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