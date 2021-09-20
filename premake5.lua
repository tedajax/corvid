VULKAN_SDK = os.getenv("VULKAN_SDK")

workspace "corvid"
    configurations { "Debug", "Release" }
    platforms { "Linux64", "Win64", "Win32" }
    includedirs { "external/include" }
    --libdirs { "external/lib/%{cfg.platform}/%{cfg.buildcfg}" }
    location "bin"

project "corvid"
    kind "ConsoleApp"
    language "C"
    location "bin/corvid"
    files "src/corvid/**.c"    
    cppdialect "C++latest"
    postbuildcommands { "powershell.exe -File ../../asset_pipeline.ps1 -target %{prj.name} -platform %{cfg.platform} -configuration %{cfg.buildcfg}" }

    filter "platforms:Win64"
        system "Windows"
        defines { "_CRT_SECURE_NO_WARNINGS" }
        architecture "x86_64"
        buildoptions { "/W3" }

    filter "platforms:Win32"
        system "Windows"
        defines { "_CRT_SECURE_NO_WARNINGS" }
        architecture "x86"

    filter "configurations:Debug"
        links {"SDL2maind"}
        optimize "Off"
        
    filter "configurations:Release"
        links { "SDL2main" }
        optimize "Full"

    filter "platforms:Linux64"
        system "Linux"
        architecture "x86_64"

    filter "configurations:Debug"
        defines { "DEBUG", "_DEBUG" }
        symbols "On"

    filter "configurations:Release"
        kind "WindowedApp"
        defines { "NDEBUG", "_NDEBUG" }
        optimize "On"

