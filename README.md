# EarShotOpenAL
Fork of the original mod by HzanRsxa2959, which aims to improve &amp; fix the original mod.

                                      
					<img width="3840" height="2160" alt="image" src="https://github.com/user-attachments/assets/51e200e2-9ed0-4072-bc75-49aad0972bcb" />

									
Hi there! I really suggest you to read it all, so you'd fully understand how to add new sounds to your SA build! ( If didn't, not my problem :) )


# About:
This is a revamp of a abandoned mod "EarShot" that allowed you to add NEW weapon sounds WITHOUT replacement of vanilla sounds.
This idea came from my friend, when he experienced problems with this mod and missing things, such as after shot echo (or after shot sound), doppler, reverb, etc.
So he told me if i could do something about it, and that's where things began to escalate...
I began by changing the sound engine from deprecated "irrKlang" to OpenAL, which is used by most games and apps, fixing bugs and improving the mod.
And that's how we are here! :) Have fun adding new sounds to your game! Max detailed instructions are down below.

The .zip also includes four

Most important: double check if your sound is MONO for 3D spatialization (otherwise if it's STEREO, it won't have distance attenuation and will play in 2D space!!!)
But if you neeed it to be stereo, then let it be. Useful for bullet whizzing sounds only i think, since they have no position in 3D space.
All audio files need to be exactly a .wav extension.

The mod has a couple of debug menu entries, such as logging toggle, and reloading all audio folders (may cause unusual behavior, but none was detected during testing).

Credits go to the original EarShot author (HzanRsxa2959) for the original mod.

Special thanks to:
Shimada — testing.
Mentraze — VC weapon pack & testing.
Lividkilla66 — Driv3r footsteps pack & testing.
Dun — Manhunt weapon pack & testing.
Matt1010 & Mentraze - for the logo.
Get the latest OpenAL version here: https://github.com/kcat/openal-soft

*GameFolder is the games root folder

Q & A:

Q: Why are my sounds randomized?
A: Can't believe i'm saying this, but it's because you have a integer at the end of the file, hence the plugin thinks you want more than one file to play.
The solution is to simply either leave "0" or have none numbers in the filename.
Q: Mod doesn't work, no replacement is happening!
A: Double check paths, file names, also peep logs for some info, and more importantly, check if you have OpenAL32.dll in the root game folder
(mod won't even inject if it's missing), and also check if you have "ModelExtras" mod on, because it disables this mod and replaces with it's own implementation.
Nag Grinch to remove that :D.

# Installation:

The .asi and .ini file, as well as "EarShot" folder can be placed anywhere, (scripts, root, modloader), but the OpenAL32.dll should be in the root folder, always.


Possible output:
GameFolder\EarShot.asi, EarShot.ini, EarShot (folder), OpenAL32.dll
GameFolder\scripts\EarShot.asi, EarShot.ini, EarShot (folder)
GameFolder\modloader\EarShotMod\EarShot.asi, EarShot.ini, EarShot (folder)

# 1. Weapon sounds:
So adding new weapon shoot sounds is pretty much the same as you'd do it in previous version of EarShot. You'd create folder inside "EarShot" folder with any name
(for example, AK-47), you'd create a new file with extension ".earshot" and with name of the weapon from weapon.dat (ak47.earshot).
Place "shoot.wav" (weapon shooting sound) inside it.

Tip: To avoid creating too many folders, you can place multiple ".earshot" files inside the same folder as the sound you want to use for multiple weapons.

NOTE!!! If you want alternative sounds to have some variety, simply add a number to the end of the filename. (for example, "shoot0.wav", "shoot1.wav", "filename0.wav", "filename1.wav" etc.).
You can add up to 10 alternatives (300 for ambience), to any file like that.

The pitch for after/shooting sound can be changed directly in the ".earshot" file of the weapon!
Open the file and add "pitch=x" where x is the pitch as a FLOAT value (for example pitch=1.1).
If you don't know what's a floating number: https://en.wikipedia.org/wiki/Floating-point_arithmetic

For a after shot sound effect you paste the sound and name it "after.wav".
It can be a echo or something else.

For a reloading sound add a file named "reload.wav".
Some weapons can have TWO separate reloading sounds, so if that's the case, name first part of reloading "reload_one.wav", and the second "reload_two.wav".

TIP: i recommend checking the "EarShotOpenAL.log" logfile for more useful info.

For a distant shot add a file named "distant.wav", and it'll play it when you're far away enough from the shooter (50 units).

# 1.1. Minigun special:
If you're replacing minigun sounds, the barrel spinning sounds for it is "spin.wav" (main spinning loop), and "spin_end.wav" (when done spinning).
If you're replacing spinning sounds, make sure to replace all of them, not just one.
Possible output: GameFolder\EarShot\Minigun\spin.wav (for barrel spin loop).
Possible output: GameFolder\EarShot\Minigun\spin_end.wav (for barrel spin end).

# 1.2. Missiles:
If you wish to have custom sounds for missiles, simply create a "Missiles" folder inside "EarShot" folder and place a "missile_flyloop.wav" file there.
Possible output: GameFolder\EarShot\Missiles\missile_flyloop.wav (LS gunfire for pistol).

# 1.3. Rhino cannon:
If you wish to have Rhino have a cannon shooting sound, you can create a folder with "Tank Cannon" (strictly with this name) name in "EarShot" folder, and put the "cannon_fire.wav" file inside!

# 1.4. Bullet whizzing:

If you want to replace default bullet whizzing sounds, do this:

Place them at GameFolder\EarShot\generic\bullet_whizz
"left_rear.wav"
"left_front.wav"
"right_rear.wav"
"right_front.wav"

Left sounds is when the bullet is on the left side of the camera, second (right) is when on the right side of the camera.
Rear is when the bullet comes from behind you, and vice versa.
Sounds gotta be stereo, because it plays on front-end, and has no position in 3D space.

# 1.5. Dry-fire & low ammo cues:

If you want dry-fire or low-ammo cues (similar to CS:GO), place a dryfire.wav file for dry-fire and a low_ammo.wav file for low-ammo cues.
Low-ammo cues trigger when ammo falls below 33% of total capacity.
Dry-fire triggers when the clip reaches 0.

# 2. Explosion sounds

Here's my personal favourite, we all love some explosion, right? And we want proper sounds for it without vanilla replacement. So follow these steps for max immersion!
They are replaced in GameFolder\EarShot\generic\explosions folder.

Main explosion sounds are named "explosion.wav", and you can add alternatives for it too (like i said earlier, you can add alternatives for every sound).
For distant explosion sounds add a "distant.wav" audio file, for debris do "debris.wav".
Distant explosion sounds will play when the explosions happens 100 units away from the camera.

Explosion types: https://wiki.multitheftauto.com/wiki/Explosion_types

Possible output for generic explosions: GameFolder\EarShot\generic\explosions\explosion.wav
Possible output for explosion types: GameFolder\EarShot\generic\explosions\explosionTypes\4\explosion.wav, debris.wav, etc. (4 is the car explosion type).

# 3. Footsteps

If you feel like your footstep sounds are boring, you can replace them here too!
They can be replaced at GameFolder\EarShot\generic\footsteps.
That main folder can have subfolders named for each surface you step on ("default", "grass", "metal", "wood", "sand", "water", "dirt", "pavement", "carpet", "flesh", "tile").
Create a folder for that surface, and add footstep sounds inside named (step.wav).

You can also have special sounds for different shoes!
Place them at GameFolder\EarShot\generic\footsteps\shoename, where "shoename" is the shoe model name, for example "sneaker" or "feet".
Then create surface folders inside and place the "step.wav" files inside!
Possible output: GameFolder\EarShot\generic\footsteps\pavement\step.wav (generic sound for pavement).
For shoe types: GameFolder\EarShot\generic\footsteps\sneaker\pavement\step.wav (A different sound type for sneakers on the pavement).


# 4. Ricochet sounds

You can also replace gun ricochect (or impact sounds) here.
You can find them at GameFolder\EarShot\generic\ricochet.
Each subfolder can have the surface name that the bullet will hit ("default", "metal", "wood", "water", "dirt", "glass", "stone", "sand", "flesh")
Same as footsteps, create a folder and add a audiofile named "ricochet.wav".

Possible output: GameFolder\EarShot\generic\ricochet\wood\step.wav (generic sound for wooden surface).

# 5. Fire sounds

Replacing fire sounds is easy, they can be found at GameFolder\EarShot\generic\fire.
Game has multiple fire types so we name each file by the fire type we want to replace.

"fire_mediumloop.wav" for medium fire, "fire_largeloop.wav" for large fires, "fire_smallloop.wav" for small fires, "fire_flameloop.wav" for flame (?),
"fire_bikeloop.wav" for bike fire, "fire_carloop.wav" for car fire, and "fire_molotovloop.wav" for molotov tip fire.

Possible output: GameFolder\EarShot\generic\fire\fire_carloop.wav (sound for car engine flame).

# 6. Melee sounds

You can also replace fists/melee related sounds here as well, just like in previous EarShot.
Fists supported as well.

"hit.wav" is for generic hitting sound against flesh surface (ped or other fleshy surface)
"hitmetal.wav" is when you hit against metal surface.
"martial_kick.wav"/"martial_punch.wav" is for martials.
"hitwood.wav" is when you hit against wood surface.
"stomp.wav" is when you stomp the ped on the ground.
"swing.wav" is when you swing the weapon.

Pistol whipping sound replacement is quite simple too. You place the "hit.wav" sound inside your weapons folder and that's it!

Possible output: GameFolder\EarShot\Pistol\hit.wav (Whipping sfx).

# 6.1. Jacking sounds:
This subsection is for jacking sounds related to car jacking.

Before proceeding please make sure that your sounds have correct hitting time according to the anim, and body-drop sound!
For example, the anim hit the other ped approximatily at 1.3 secs, then in Audacity or other audio editing software, set the hitting sound exactly the same. Also edit in body-drop sound as well.

Specify the sounds at GameFolder\EarShot\generic\jacked.

"jack_car.wav" is when you jack anyone in a car.
"jack_carheadbang.wav" is when you jack someone in a car and bash their head against the dashboard.
"jack_carkick.wav" is when you jack someone in a low car.
"jack_bike.wav" is when you jack someone in a bike.
"jack_bulldozer.wav" is when you jack someone in a bulldozer.

Possible output: GameFolder\EarShot\generic\jacked\jack_car.wav (generic sound for carjacking).

# 7. Ambience sounds.

Here comes the most interesting part, for me personally, is the ambience sounds around the map!
Their main folder is at GameFolder\EarShot\generic\ambience.

"ambience_riot.wav" is for ambience sound during riots.
"ambience_night.wav" is for ambience during night time.
"ambience.wav" is general ambience during the day time.
"thunder.wav" is thunder sounds to replace (without touching vanilla explosion sounds ofc).

In the "zones" subfolder you create folders with zone names, in case you want airport to have airplane sounds or forest to have wolf howling sounds.

Same as before, just add the zone name between the underscore, like so:

Possible output: GameFolder\EarShot\generic\ambience\zones\creek\ambience.wav (day)

"creek" in our case is the zone name from GXT key. View full default zones list here: https://docs.sannybuilder.com/scm-documentation/sa/zones

These work by priority, so if for example riots are happening, only those sounds will play, if you're in a zone, those will play.
Next ambience sound will begin after some time after previous one ended.
They choose a random far away position relative to the camera, and play.

The "gunfire" subfolder specifies the LS gunshots ambience FX. It can have another folder in it with the name of the gun that it uses the sounds of to play.

Default folder can be "ak47" and "pistol". Place "shoot.wav" inside as a shooting sound.

The "interiors" subfolder can have the interiors ambience for specific interior. Inside the "interiors" folder, should be a folder with the interior ID (for example, 1).
And inside that ID folder should be a GXT entry of the interior, for example "CARTER".
Place the "ambience.wav" sounds inside.
Info about the interiors can be found here: https://docs.sannybuilder.com/scm-documentation/sa/interiors or look into the log's for current interior info.

The "country", "LS", "LV" and "SF" folder that can be created in the "zones" folder are the global zones ambiences.
It means if you want some ambiences to play on the entire zone and not "piece-by-piece" then add ambiences to those global zones.

Possible output: GameFolder\EarShot\generic\ambience\ambience.wav (generic ambience during daytime).
Night time: GameFolder\EarShot\generic\ambience\ambience_night.wav (generic ambience during night time).
Riot: GameFolder\EarShot\generic\ambience\ambience_riot.wav (generic ambience during riots).
Interior: GameFolder\EarShot\generic\ambience\interiors\18\X7_11B\ambience.wav (ambience for that interior, in our case it's the 24/7 near Unity-Station in LS).
Gunfire: GameFolder\EarShot\generic\ambience\gunfire\ak47\shoot.wav (LS gunfire for ak47).
Gunfire: GameFolder\EarShot\generic\ambience\gunfire\pistol\shoot.wav (LS gunfire for pistol).
Zone: GameFolder\EarShot\generic\ambience\zones\ambience_creek.wav (daytime ambience for creek zone).
Global zone: GameFolder\EarShot\generic\ambience\zones\ls\ambience.wav (daytime ambience for entire LS).


For developers:

This mod provides a export func named "GetContext", because if two plugins use OpenAL Soft at the same time, it can conflict, so use this func to get the context from this mod.

Have fun!

Join my discord server: https://discord.com/invite/4dxtJCwSx8
