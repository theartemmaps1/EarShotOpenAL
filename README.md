<img width="3840" height="2160" alt="image" src="https://github.com/user-attachments/assets/ec4eff43-c193-401a-bdd6-f062d089736d" />

# EarShot — Revamp by wuzimu7171_ (TheArtemMaps)

A revamp of the abandoned **EarShot** mod, originally by HzanRsxa2959. EarShot lets you add **new weapon and world sounds to GTA San Andreas without replacing vanilla sounds.** The sound engine has been rewritten from the deprecated irrKlang library to OpenAL Soft, adding proper 3D spatialization, distance attenuation, doppler, reverb, air absorption, and more.

> **Please read this document in full before asking for support.** Most issues come down to file placement, filename spelling, stereo/mono format, or missing OpenAL files.

---

## Credits

- **HzanRsxa2959** — Original EarShot mod
- **Shimada** — Testing
- **Mentraze** — VC weapon pack & testing
- **Lividkilla66** — Driv3r footsteps pack & testing
- **Dun & CrimsonW** — Manhunt weapon pack & testing
- **Matt1010 & Mentraze** — Logo

OpenAL Soft (latest version): https://github.com/kcat/openal-soft

Join the Discord: https://discord.com/invite/4dxtJCwSx8

---

## Important Notes

- **Audio files must be MONO for correct 3D spatialization.** Stereo files will play in 2D space with no distance attenuation. Stereo is only appropriate for sounds that have no position in the world, such as bullet whizz sounds or goggles/camera.
- Supported audio formats: `.wav`, `.mp3`, `.flac`, `.ogg`
- For **alternative/randomized sounds**, add a number to the end of the filename: `shoot0.wav`, `shoot1.wav`, etc. Up to **10 alternatives** for most sounds, and **300 for ambience** sounds.
- **If your sounds are being randomly selected when you don't want that**, check that your filename does not end in a number (e.g. rename `sound1.wav` to `sound.wav`). The mod treats any trailing integer as an alternative index.
- Enable HRTF in `alsoft.ini` when using headphones. **Disable it when using speakers** — HRTF is designed for headphones only and will sound wrong on speakers.
- Check `EarShotOpenAL.log` in the mod folder for detailed loading and playback info.
- The Debug Menu exposes a **Reload all audio folders** command and toggles for logging.

---

## Installation

Place `EarShot.asi`, `EarShot.ini`, and the `EarShot` folder anywhere the game loads ASI plugins from (root, `scripts\`, or a ModLoader subfolder). `OpenAL32.dll` and `alsoft.ini` **must always be in the game's root folder**.

**Example layouts:**
```
GameFolder\EarShot.asi, EarShot.ini, EarShot\, OpenAL32.dll, alsoft.ini
GameFolder\scripts\EarShot.asi, EarShot.ini, EarShot\
GameFolder\modloader\EarShotMod\EarShot.asi, EarShot.ini, EarShot\
```

> `*GameFolder` refers to the game's root directory.

---

## File Structure Overview

All custom sounds live under the `EarShot\` folder (referred to as `GameFolder\EarShot\` below). The layout is:

```
EarShot\
  <WeaponFolderName>\
    <weaponname>.earshot
    shoot.wav
    after.wav
    reload.wav
    distant.wav
    ...
  Tank Cannon\
    cannon_fire.wav
  Missiles\
    missile_flyloop.wav
  generic\
    explosions\
    footsteps\
    ricochet\
    fire\
    ambience\
    jacked\
    bullet_whizz\
```

---

## 1. Weapon Sounds

Create a folder inside `EarShot\` with any name (e.g. `AK-47`). Inside it, create a file named after the weapon as it appears in `weapon.dat`, with the extension `.earshot` (e.g. `ak47.earshot`). Place your audio files alongside it.

**Tip:** To avoid many folders, you can place multiple `.earshot` files in the same folder and have them all reference the same audio files inside it.

### Sound Files

| Filename | Description |
|---|---|
| `shoot.wav` | Weapon firing sound |
| `after.wav` | Sound played after firing (e.g. echo, tail) |
| `reload.wav` | Single reload sound |
| `reload_one.wav` | First part of a two-stage reload |
| `reload_two.wav` | Second part of a two-stage reload |
| `distant.wav` | Plays when the shooter is beyond the distant-shot threshold (default 50 units, configurable in `.ini`) |
| `dryfire.wav` | Plays once when the clip reaches 0 |
| `low_ammo.wav` | Plays every shot when ammo drops below 33% of magazine capacity |
| `hit.wav` | Pistol-whip / melee impact sound (place inside the weapon's folder) |

**Example path:**
```
GameFolder\EarShot\AK-47\ak47.earshot
GameFolder\EarShot\AK-47\shoot.wav
GameFolder\EarShot\AK-47\after.wav
```

---

## 1.1 Looped Weapon Sounds

The following weapons use looping sounds. Place files alongside their `.earshot` file.

### Flamethrower

| Filename | Looped | Description |
|---|---|---|
| `flamethrower_idlegasloop.wav` | Yes | Idle gas hiss when holding the weapon |
| `flamethrower_start.wav` | No | Plays once when firing begins |
| `flamethrower_fire.wav` | Yes | Main firing loop |

### Fire Extinguisher

| Filename | Looped | Description |
|---|---|---|
| `extinguisher_loop.wav` | Yes | Spray loop |

### Minigun

| Filename | Looped | Description |
|---|---|---|
| `minigun_fireloop.wav` | Yes | Main firing loop |
| `minigun_barrelspinloop.wav` | Yes | Barrel spin loop |
| `minigun_barrelspinend.wav` | No | Plays once when the barrel stops |

### Spray Can

| Filename | Looped | Description |
|---|---|---|
| `spraycan_sprayloop.wav` | Yes | Spray loop |

### Chainsaw

| Filename | Looped | Description |
|---|---|---|
| `chainsaw_idle.wav` | Yes | Engine idle loop while holding |
| `chainsaw_active.wav` | Yes | Loop while holding the attack button (before contact) |
| `chainsaw_cuttingflesh.wav` | Yes | Loop while cutting into a ped |
| `chainsaw_stop.wav` | No | Plays once when the attack button is released |

---

## 1.2 Goggles & Camera

These sounds play in 2D space (no world position).

| Filename | Description |
|---|---|
| `goggles_on.wav` | Night-vision goggles equipped |
| `goggles_off.wav` | Night-vision goggles removed |
| `camera_shutter.wav` | Camera shutter when taking a photo |

Place these files alongside a `.earshot` file for the appropriate weapon (e.g. `nightvision.earshot` / `camera.earshot`).

---

## 1.3 Missiles

Create a `Missiles` folder and place `missile_flyloop.wav` inside it.

```
GameFolder\EarShot\Missiles\missile_flyloop.wav
```

Attenuation can be configured in `EarShot.ini` under `[MISSILE]`:
```ini
[MISSILE]
maxDist = 250.0
refDist = 6.0
rolloffFactor = 1.0
airAbsorption = 1.0
```

---

## 1.4 Rhino Tank Cannon

Create a folder named exactly `Tank Cannon` and place `cannon_fire.wav` inside it.

```
GameFolder\EarShot\Tank Cannon\cannon_fire.wav
```

Attenuation can be configured in `EarShot.ini` under `[TANKCANNON]`:
```ini
[TANKCANNON]
maxDist = 125.0
refDist = 3.5
rolloffFactor = 0.3
airAbsorption = 0.3
```

---

## 1.5 Vehicle-Mounted Projectile Fire

To add a sound for a vehicle firing its projectile weapon (e.g. a helicopter firing rockets), place a file named `projfire.wav` inside the weapon's folder. The mod will play it when that vehicle type fires a projectile.

---

## 1.6 Bullet Whizzing

Place files at `GameFolder\EarShot\generic\bullet_whizz\`. These sounds **must be stereo** as they play in 2D frontend space.

| Filename | Description |
|---|---|
| `left_rear.wav` | Bullet passing from the left rear |
| `left_front.wav` | Bullet passing from the left front |
| `right_rear.wav` | Bullet passing from the right rear |
| `right_front.wav` | Bullet passing from the right front |

---

## 1.7 Weapon Attenuation & Pitch Settings

Attenuation and pitch for weapon sounds are configured inside the `.earshot` file itself (not in `EarShot.ini`). Open the `.earshot` file in a text editor and add entries using the following format:

```
<soundType>.<property> = <value>
```

### Sound type prefixes

| Prefix | Applies to |
|---|---|
| `shoot.*` | Gunshot sounds |
| `reload.*` | Reload sounds |
| `after.*` | After-shot sounds |
| `distant.*` | Distant gunshot sounds |
| `low_ammo.*` | Low ammo cue sounds |
| `vehicle.*` | Vehicle-mounted gun sounds |
| `melee.*` | Melee weapon sounds |
| `fireExtinguisher.sprayLoop.*` | Fire extinguisher |
| `sprayCan.sprayLoop.*` | Spray can |
| `chainsaw.idleLoop.*` | Chainsaw idle |
| `chainsaw.activeLoop.*` | Chainsaw active (pre-contact) |
| `chainsaw.cuttingLoop.*` | Chainsaw cutting flesh |
| `chainsaw.stop.*` | Chainsaw stop |
| `minigun.spinLoop.*` | Minigun barrel spin |
| `minigun.spinEnd.*` | Minigun barrel stop |
| `minigun.fireLoop.*` | Minigun fire loop |

### Properties

| Property | Description |
|---|---|
| `maxDist` | Distance beyond which the sound stops attenuating |
| `refDist` | Distance at which the sound plays at full volume |
| `rolloffFactor` | How quickly volume drops with distance |
| `airAbsorption` | High-frequency loss over distance (higher = more muffled at range) |
| `pitch` | Pitch multiplier for the sound |

### Per-weapon section in EarShot.ini

You can also define per-weapon attenuation in `EarShot.ini` using section headers:

```ini
; By weapon type only (weapon type 31 = M4)
[WEAPONSOUND_31]
shoot.maxDist = 120.0
shoot.refDist = 90.0
shoot.rolloffFactor = 1.0
shoot.airAbsorption = 2.0
shoot.pitch = 1.0

; By vehicle model ID + weapon type (model 425 = Hunter)
[WEAPONSOUND_425_31]
vehicle.maxDist = 120.0
vehicle.refDist = 90.0
vehicle.rolloffFactor = 1.0
vehicle.airAbsorption = 2.0
```

Settings in the `.earshot` file take priority and are the recommended approach for per-weapon tuning.

---

## 2. Explosion Sounds

Place files at `GameFolder\EarShot\generic\explosions\`.

| Filename | Description |
|---|---|
| `explosion.wav` | Main explosion sound |
| `distant.wav` | Plays when the explosion is beyond the distant threshold (default 100 units) |
| `debris.wav` | Debris/shrapnel sounds |
| `underwater.wav` | Underwater explosion sound |

To override sounds for a specific explosion type, create a subfolder with the numeric explosion type ID:
```
GameFolder\EarShot\generic\explosions\explosionTypes\4\explosion.wav
```
See https://wiki.multitheftauto.com/wiki/Explosion_types for the full list of IDs.

### Attenuation in EarShot.ini

```ini
[EXPLOSIONS]
explosion.main.maxDist = 100.0
explosion.main.refDist = 10.0
explosion.main.rolloffFactor = 0.7
explosion.main.airAbsorption = 0.6

explosion.distant.maxDist = 4000.0
explosion.distant.refDist = 10.0
explosion.distant.rolloffFactor = 0.5
explosion.distant.airAbsorption = 1.0

explosion.debris.maxDist = 150.0
explosion.debris.refDist = 1.0
explosion.debris.rolloffFactor = 1.5
explosion.debris.airAbsorption = 1.0

explosion.underwater.maxDist = 4000.0
explosion.underwater.refDist = 15.0
explosion.underwater.rolloffFactor = 0.2
explosion.underwater.airAbsorption = 3.0
```

Per explosion type (falls back to `[EXPLOSIONS]` if the section doesn't exist):
```ini
[EXPLOSION_TYPE_0]
explosion.main.maxDist = 80.0
...
```

---

## 3. Footsteps

Place files at `GameFolder\EarShot\generic\footsteps\`.

The system resolves sounds in this priority order:
1. Shoe texture name → surface type subfolder
2. Shoe model name → surface type subfolder
3. Generic surface type subfolder
4. `default` surface subfolder

### Generic surface sounds

Create a subfolder named after a surface type and place `step.wav` inside it.

Supported surface names: `default`, `grass`, `metal`, `wood`, `sand`, `water`, `dirt`, `pavement`, `carpet`, `flesh`, `tile`

```
GameFolder\EarShot\generic\footsteps\pavement\step.wav
```

### Shoe-specific sounds (by model name)

Create a subfolder with the shoe model name, then surface subfolders inside that:
```
GameFolder\EarShot\generic\footsteps\sneaker\pavement\step.wav
```

### Shoe-specific sounds (by texture name)

Same structure, but using the shoe texture name instead:
```
GameFolder\EarShot\generic\footsteps\sneakerprored\pavement\step.wav
```

See https://wiki.multitheftauto.com/wiki/CJ_Clothes%5CShoes_(3) for vanilla shoe model and texture names.

### Attenuation in EarShot.ini

```ini
[FOOTSTEPS]
footsteps.player.maxDist = 9999999999.9
footsteps.player.refDist = 0.5
footsteps.player.rolloffFactor = 1.5
footsteps.player.airAbsorption = 1.0
footsteps.player.pitch = 1.0

footsteps.player.duck.maxDist = 9999999999.9
footsteps.player.duck.refDist = 0.1
footsteps.player.duck.rolloffFactor = 1.5
footsteps.player.duck.airAbsorption = 1.0

footsteps.player.sprint.maxDist = 9999999999.9
footsteps.player.sprint.refDist = 1.0
footsteps.player.sprint.rolloffFactor = 1.5
footsteps.player.sprint.airAbsorption = 1.0

footsteps.player.walk.maxDist = 9999999999.9
footsteps.player.walk.refDist = 0.3
footsteps.player.walk.rolloffFactor = 1.5
footsteps.player.walk.airAbsorption = 1.0

footsteps.npc.maxDist = 9999999999.9
footsteps.npc.refDist = 0.3
footsteps.npc.rolloffFactor = 2.5
footsteps.npc.airAbsorption = 3.0

footsteps.npc.duck.refDist = 0.1
footsteps.npc.sprint.refDist = 0.7
footsteps.npc.walk.refDist = 0.2
```

---

## 3.1 Landing Sounds

Landing sounds play when a ped lands after a jump or fall. They use the **same folder structure as footsteps** (surface subfolders, shoe model subfolders, shoe texture subfolders) and support the same alternatives system.

| Filename | Description |
|---|---|
| `landing.wav` | Landing sound (or `landing0.wav`, `landing1.wav`, … for alternatives) |

**Example paths:**
```
GameFolder\EarShot\generic\footsteps\pavement\landing.wav
GameFolder\EarShot\generic\footsteps\sneaker\pavement\landing.wav
```

Attenuation keys in `[FOOTSTEPS]`:
```ini
landing.player.maxDist = 9999999999.9
landing.player.refDist = 0.3
landing.player.rolloffFactor = 2.5
landing.player.airAbsorption = 3.0
landing.player.pitch = 1.0

landing.npc.maxDist = 9999999999.9
landing.npc.refDist = 0.3
landing.npc.rolloffFactor = 2.5
landing.npc.airAbsorption = 3.0
```

---

## 3.2 Collapse Sounds

Collapse sounds play when a ped falls from a significant height. Same folder structure and alternatives system as footsteps and landing sounds.

| Filename | Description |
|---|---|
| `collapse.wav` | Collapse/hard-fall sound |

**Example paths:**
```
GameFolder\EarShot\generic\footsteps\pavement\collapse.wav
GameFolder\EarShot\generic\footsteps\sneaker\pavement\collapse.wav
```

Attenuation keys in `[FOOTSTEPS]`:
```ini
collapse.player.maxDist = 9999999999.9
collapse.player.refDist = 0.3
collapse.player.rolloffFactor = 2.5
collapse.player.airAbsorption = 3.0

collapse.npc.maxDist = 9999999999.9
collapse.npc.refDist = 0.3
collapse.npc.rolloffFactor = 2.5
collapse.npc.airAbsorption = 3.0
```

---

## 4. Ricochet / Bullet Impact Sounds

Place files at `GameFolder\EarShot\generic\ricochet\<surface>\ricochet.wav`.

Supported surface names: `default`, `metal`, `wood`, `water`, `dirt`, `glass`, `stone`, `sand`, `flesh`

```
GameFolder\EarShot\generic\ricochet\metal\ricochet.wav
```

### Attenuation in EarShot.ini

Global fallback:
```ini
[RICOCHET]
maxDist = 50.0
refDist = 3.5
rolloffFactor = 5.0
airAbsorption = 3.0
```

Per surface type (falls back to `[RICOCHET]` if missing):
```ini
[RICOCHET_SURFACE_TYPE_4]
maxDist = 50.0
refDist = 3.5
rolloffFactor = 5.0
airAbsorption = 3.0
```

---

## 5. Fire Sounds

Place files at `GameFolder\EarShot\generic\fire\`.

| Filename | Description |
|---|---|
| `fire_smallloop.wav` | Small fire loop |
| `fire_mediumloop.wav` | Medium fire loop |
| `fire_largeloop.wav` | Large fire loop |
| `fire_flameloop.wav` | Flame particle loop |
| `fire_bikeloop.wav` | Bike on fire |
| `fire_carloop.wav` | Car on fire |
| `fire_molotovloop.wav` | Molotov tip flame |

### Attenuation in EarShot.ini

```ini
[FIRE]
maxDist = 200.0
refDist = 1.0
rolloffFactor = 1.5
airAbsorption = 4.0

; Non-harming fire particles
nonfire.maxDist = 200.0
nonfire.refDist = 1.0
nonfire.rolloffFactor = 1.5
nonfire.airAbsorption = 4.0
```

---

## 6. Melee Sounds

Place melee sounds alongside the weapon's `.earshot` file.

| Filename | Description |
|---|---|
| `hit.wav` | Generic hit against flesh |
| `hitmetal.wav` | Hit against a metal surface |
| `hitwood.wav` | Hit against a wood surface |
| `stomp.wav` | Stomping a downed ped |
| `swing.wav` | Weapon swing (no contact) |
| `martial_punch.wav` | Martial arts punch |
| `martial_kick.wav` | Martial arts kick |

For pistol whipping, place `hit.wav` in the pistol's weapon folder:
```
GameFolder\EarShot\Pistol\hit.wav
```

See Section 1.7 for attenuation settings using the `melee.*` prefix.

---

## 6.1 Car Jacking Sounds

Place files at `GameFolder\EarShot\generic\jacked\`.

| Filename | Description |
|---|---|
| `jack_car.wav` | Generic car jack (punch + body drop) |
| `jack_carheadbang.wav` | Head bashed against dashboard |
| `jack_carkick.wav` | Kick out of a low car |
| `jack_bike.wav` | Bike jacking |
| `jack_bulldozer.wav` | Bulldozer jacking |

> **Note:** Align the hit timing in your audio file to match the animation. The punch/kick connects at approximately 1.3 seconds into the jack animation. Include the body-drop sound in the same file.

### Attenuation in EarShot.ini

```ini
[JACKED]
maxDist = 9999999999.9
refDist = 3.0
rolloffFactor = 1.5
airAbsorption = 0.8
```

---

## 7. Ambience Sounds

Place files at `GameFolder\EarShot\generic\ambience\`.

| Filename | Description |
|---|---|
| `ambience.wav` | General daytime ambience |
| `ambience_night.wav` | Nighttime ambience |
| `ambience_riot.wav` | Riot ambience |
| `thunder.wav` | Thunder sound (replaces vanilla without touching explosion sounds) |

Ambience sounds choose a random far-away position relative to the camera each time they play. The next sound plays after a random interval (configurable in `EarShot.ini`).

Priority order: manual ambiences → zone ambiences → global zone ambiences → fallback day/night/riot.

### Zone-specific ambience

Create a subfolder inside `zones\` named after the zone's GXT key:
```
GameFolder\EarShot\generic\ambience\zones\creek\ambience.wav
```
See https://docs.sannybuilder.com/scm-documentation/sa/zones for the full zone list.

Night and riot variants follow the same naming convention (`ambience_night.wav`, `ambience_riot.wav`) inside the zone folder.

### Global zone ambience

The folders `country`, `LS`, `LV`, and `SF` inside `zones\` apply to the entire corresponding area:
```
GameFolder\EarShot\generic\ambience\zones\LS\ambience.wav
```

### LS Gunfire ambience

```
GameFolder\EarShot\generic\ambience\gunfire\ak47\shoot.wav
GameFolder\EarShot\generic\ambience\gunfire\pistol\shoot.wav
```

Only `ak47` and `pistol` are used by the game internally. Other weapon names will not work.

---

## 7.1 Manual Map Ambiences

For precise placement of ambient sounds on the map, create a `map_ambience.ini` file at:
```
GameFolder\EarShot\generic\ambience\map_ambience.ini
```

Each ambience is defined as a numbered section:

```ini
[Ambience0]
File = myambience.wav          ; filename relative to the ambience folder, or absolute path
                               ; multiple files separated by commas for random selection
X = 1234.5                     ; world X position
Y = -567.8                     ; world Y position
Z = 10.0                       ; world Z position
Range = 50.0                   ; radius within which the player must be to trigger playback
Max distance attenuation = 50.0
Reference distance = 1.0
Roll off factor = 1.0
Air absorption = 1.0
Loop = false                   ; true = looping while player is in range
Allow other ambiences = false  ; true = other ambiences can still play alongside this one
Delay = 30000                  ; milliseconds between non-looping plays
Time = any                     ; any / day / night / riot
```

---

## 8. EarShot.ini — Main Settings

```ini
[MAIN]
Logging = false
Max bytes in log = 9000000
Ambience interval min = 5000          ; ms between general ambience sounds
Ambience interval max = 10000
Zone ambience interval min = 5000
Zone ambience interval max = 10000
Distant gunshot distance = 50.0       ; units — beyond this, distant.wav plays instead
Distant explosion distance = 100.0    ; units — beyond this, distant explosion sounds play
Stereo ambience volume = 0.3          ; volume multiplier for stereo ambience files
```

---

## 9. Experimental

Experimental features are playable but may change between builds. Use them for testing packs, and check `EarShotOpenAL.log` if a vehicle sound does not load or stop as expected.

### 9.1 Vehicle SFX

EarShot can replace or extend a small set of vehicle-related sounds by vehicle model ID. These are configured through `GameFolder\EarShot\generic\sirens\sirens.ini`, with the referenced audio files stored in the same `generic\sirens\` folder or a subfolder inside it.

Supported experimental vehicle sound slots:

| INI key | Description | Playback |
|---|---|---|
| `siren` | Main emergency siren loop | Loops while the main siren is active |
| `sirenidle` | Alternate/idle siren loop | Loops while the idle siren state is active |
| `reverse_beep` | Reverse warning beep | Loops while the vehicle is reversing with the engine on |
| `air_brakes` | Air-brake release or stop sound | One-shot sound when the vehicle slows or changes movement sharply |

**Example layout:**
```
GameFolder\EarShot\generic\sirens\sirens.ini
GameFolder\EarShot\generic\sirens\police_siren.wav
GameFolder\EarShot\generic\sirens\police_idle.wav
GameFolder\EarShot\generic\sirens\truck_reverse.wav
GameFolder\EarShot\generic\sirens\truck_airbrake.wav
```

**Example `sirens.ini`:**
```ini
; Vehicle model IDs use GTA SA model IDs.
; 596 = Police LS, 403 = Linerunner.

[VEHICLE_596]
siren = police_siren.wav
sirenidle = police_idle.wav
siren.maxDist = 300.0
siren.refDist = 8.0
siren.rolloffFactor = 0.6
siren.airAbsorption = 1.0
siren.pitch = 1.0
sirenidle.pitch = 1.0

[VEHICLE_403]
reverse_beep = truck_reverse.wav
air_brakes = truck_airbrake.wav
reverse_beep.maxDist = 80.0
reverse_beep.refDist = 4.0
reverse_beep.rolloffFactor = 1.0
reverse_beep.airAbsorption = 1.5
air_brakes.maxDist = 100.0
air_brakes.refDist = 5.0
air_brakes.rolloffFactor = 1.0
air_brakes.airAbsorption = 1.5
```

Vehicle SFX supports the same audio extensions as the rest of EarShot. You may include the extension in `sirens.ini` or omit it and let EarShot search for a supported format. For positional playback, keep these files mono unless you specifically want a non-positional stereo effect.

---

## 9. Developer Notes

If another plugin also uses OpenAL Soft, a context conflict may occur. EarShot exports two functions for sharing its context:

```cpp
extern "C" ALCcontext* GetContext();
extern "C" ALCdevice*  GetDevice();
```

Call these from your plugin to obtain EarShot's context/device and avoid conflicts.

---

## Q & A

**Q: My sounds play randomly even though I only have one file.**
A: Your filename ends in a number (e.g. `sound1.wav`). The mod treats trailing integers as alternative indices. Rename it to `sound.wav` or `sound0.wav`.

**Q: The mod doesn't work at all.**
A: Check that `OpenAL32.dll` is in the game's root folder — the plugin will not inject without it. Then check paths and filenames, and review `EarShotOpenAL.log` for errors.

**Q: Everything sounds like it's coming through a cheap speaker.**
A: Disable HRTF in `alsoft.ini`. HRTF is intended for headphones only and degrades quality on speakers.

**Q: Some sounds are not affected by distance and always play at full volume.**
A: The audio file is stereo. Stereo files play in 2D space with no attenuation. Convert to mono for 3D sounds.
