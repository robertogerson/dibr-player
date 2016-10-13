-- premake5.lua
workspace "dibr-player"
   configurations { "debug", "release" }

project "dibr-player"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"

   files { "**.h", "**.cc" }
   links { "GL", "GLU", "SDL", "SDL_image", "vlc" }

   filter "configurations:debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:release"
      defines { "NDEBUG" }
      optimize "On"
