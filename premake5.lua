workspace "EarShotOpenAL"
	configurations { "ReleaseSA", "DebugSA", }
	location "project_files"
   
project "EarShotOpenAL"
	kind "SharedLib"
	language "C++"
	targetdir "$(GTA_SA_DIR)/modloader/EarShotMod/"
	objdir ("output/obj")
	targetextension ".asi"
	characterset ("MBCS")
	targetname "EarShot"
	cppdialect "C++latest"
	linkoptions "/SAFESEH:NO"
	buildoptions { "-std:c++latest", "/permissive"}
	warnings "Off"
	defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_NON_CONFORMING_SWPRINTFS", "_USE_MATH_DEFINES", "RW" }
	
	files {
		"source/**.*",
	}
	removefiles {
    "source/include/subhook-0.8.2/subhook_x86.c",
	"source/include/subhook-0.8.2/subhook_unix.c",
	"source/include/subhook-0.8.2/subhook_windows.c",
    "source/**/tests/**.c",
	"source/**/tests/**.cpp",
	"source/**/tests/**.asm"
}
	includedirs { 
		"source/**",
	}

	defines { "GTASA", "PLUGIN_SGV_10EN", "AL_ALEXT_PROTOTYPES" }
	includedirs {
		"$(PLUGIN_SDK_DIR)/plugin_SA/",
		"$(PLUGIN_SDK_DIR)/plugin_SA/game_SA/",
		"$(PLUGIN_SDK_DIR)/plugin_SA/game_SA/rw",
		"$(PLUGIN_SDK_DIR)/shared/",
		"$(PLUGIN_SDK_DIR)/shared/game/",
		"source/vendor/openal-soft/include"
	--	"source/vendor/libsndfile/include"
	}

	libdirs { 
		"$(PLUGIN_SDK_DIR)/output/lib/",
	    "source/vendor/openal-soft/libs"
	--	"source/vendor/libsndfile/lib"
		}

	filter "configurations:DebugSA"
		defines { "DEBUG", "SA" }
		symbols "on"
		staticruntime "on"
		debugdir "$(GTA_SA_DIR)"
		debugcommand "$(GTA_SA_DIR)/gta_sa.exe"
	--	postbuildcommands {
	--		"copy /y \"$(TargetPath)\" \"$(GTA_SA_DIR)\\modloader\\EarShotMod\\EarShot.asi\""
	--	}

	filter "configurations:ReleaseSA"
		defines { "NDEBUG", "SA" }
		symbols "on"
		optimize "On"
		staticruntime "on"
		flags { "StaticRuntime" }
		debugdir "$(GTA_SA_DIR)"
		debugcommand "$(GTA_SA_DIR)/gta_sa.exe"
	--	postbuildcommands {
	--		"copy /y \"$(TargetPath)\" \"$(GTA_SA_DIR)\\modloader\\EarShotMod\\EarShot.asi\""
	--	}
		
	filter "configurations:ReleaseSA"
		links { "plugin" }	
		links { "OpenAL32" }
		--links { "sndfile" }		
	filter "configurations:DebugSA"
		links { "plugin_d" }
		links { "OpenAL32" }	
	--	links { "sndfile" }	
