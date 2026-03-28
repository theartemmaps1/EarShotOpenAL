#pragma once
#include "plugin.h"
#include <filesystem>
#include "ourCommon.h"
#include "AudioManager.h"
namespace fs = std::filesystem;
class Loaders {
public:
	static void LoadMinigunSounds(const fs::path& folder);
	static void LoadChainsawSounds(const fs::path& folder);
	static void LoadFlamethrowerSounds(const fs::path& folder);
	static void LoadSpraycanSound(const fs::path& folder);
	static void LoadExtinguisherSound(const fs::path& folder);
	static void LoadRicochetSounds(const fs::path& folder);
	static void LoadFootstepSounds(const fs::path& baseFolder);
	static void LoadExplosionRelatedSounds(const fs::path& folder);
	static void LoadFireSounds(const fs::path& folder);
	static void LoadJackingRelatedSounds(const fs::path& folder);
	static void InstallHooks();
	static void RegisterAllWeapons();
	static void LoadAmbienceSounds(const std::filesystem::path& path, bool loadOldAmbience = true);
	static void LoadMissileSounds(const fs::path& folder);
	static void LoadTankCannonSounds(const fs::path& folder);
	static void LoadBulletWhizzSounds(const fs::path& folder);
	static void LoadCameraAndGoggleSounds(const fs::path& folder);
	static void InitializeIniFile(int stage, bool loadAll = false);
	static void ReloadAudioFolders();
};

struct WeapInfos
{
	eWeaponType weapType;
	std::string weapName;
	std::string vehicleWeapName;
	uint32_t modelId;

	WeapInfos(eWeaponType t = WEAPONTYPE_UNARMED, const std::string& wn = {},
		const std::string& vwn = {}, uint32_t mid = MODELUNDEFINED)
		: weapType(t), weapName(wn), vehicleWeapName(vwn), modelId(mid) {
	}
};

struct SoundFile {
	std::string fileName;
	std::vector<ALuint>& bufferVec;
};

#ifdef QUAKE_KILLSOUNDS_TEST
// TODO: variations?
struct QuakeSound 
{
	ALuint headshot;
};
#endif

inline  std::unordered_map<int, WeapInfos> weaponNamese;
inline  std::vector<WeapInfos> weaponNames;
inline auto registeredweapons = map<pair<eWeaponType, eModelID>, AudioStream>();

inline void registerWeapon(fs::path& filepath) {
	std::string weaponname = filepath.stem().string();
	std::string weaponnameFull = filepath.filename().string();
	int modelfound = -1;
	if (weaponname.empty()) {
		LOG("Failed to register weapon sound for file %s", filepath.string().c_str());
		return;
	}

	eModelID modelid = MODELUNDEFINED;

	// this code below responds for the vehicle guns
	size_t sep = weaponname.find(' ');
	if (sep != std::string::npos) {
		std::string modelname = weaponname.substr(sep + 1);
		CBaseModelInfo* info = CModelInfo::GetModelInfo(modelname.c_str(), &modelfound);
		if (info && modelfound > static_cast<int>(MODELUNDEFINED)) {
			modelid = static_cast<eModelID>(modelfound);
		}
		else {
			LOG("Skipped weapon '%s': model '%s' was not found", weaponname.c_str(), modelname.c_str());
			return; // do not register if model missing
		}
		// keep only weapon part
		weaponname = weaponname.substr(0, sep);
		// add weapon part + model part
		weaponnameFull = weaponname + " " + modelname;
		LOG("full weapon name '%s'", weaponnameFull.c_str());
		LOG("Found vehicle weapon '%s' model '%s' -> id=%d",
			weaponname.c_str(), modelname.c_str(), modelid);
	}

	// Resolve weapon type from name
	eWeaponType weapontype = WEAPONTYPE_UNARMED; // default
	if (!nameType(&weaponname, &weapontype)) {
		LOG("Skipped weapon '%s': unknown weapon name", weaponname.c_str());
		return;
	}

	// avoid accidental overwrite
	auto key = std::make_pair(weapontype, modelid);
	if (registeredweapons.find(key) != registeredweapons.end()) {
		LOG("Weapon already registered type=%d(%s) model=%d -> skipping", weapontype, weaponname.c_str(), modelid);
		return;
	}

	registeredweapons.emplace(key, AudioStream(filepath.parent_path()));
	
	weaponNames.emplace_back(weapontype, weaponname, weaponnameFull, static_cast<uint32_t>(modelid));

	LOG("Registered weapon type=%d(%s) model=%d path=%s",
		weapontype, weaponname.c_str(), modelid, outputPath(&filepath).c_str());
	};

// to get the .earshot file for X entity, so we could read settings from it
inline std::optional<fs::path> findEarshotForEntity(CEntity* audioentity, const fs::path& audiopath)
{
	if (!audioentity) return std::nullopt;

	for (const auto& info : weaponNames) {
		if (static_cast<int>(info.modelId) == audioentity->m_nModelIndex) {
			fs::path earshotPath = audiopath.parent_path() / info.vehicleWeapName;
			earshotPath.replace_extension(modextension);
			if (fs::exists(earshotPath)) return earshotPath;
		}
	}

	for (const auto& info : weaponNames) {
		CPed* ped = nullptr;
		if (audioentity && audioentity->m_nType == ENTITY_TYPE_PED)
		{
			ped = (CPed*)audioentity;
		}
		if (info.weapType == ped->GetWeapon()->m_eWeaponType) {
			fs::path earshotPath = audiopath.parent_path() / info.weapName;
			earshotPath.replace_extension(modextension);
			if (fs::exists(earshotPath)) return earshotPath;
		}
	}

	return std::nullopt;
}