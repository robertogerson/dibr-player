-- premake5.lua
workspace "DibrPlayer"
   configurations { "Debug", "Release" }

project "DibrPlayer"
   kind "ConsoleApp"
   language "C++"
   targetdir "bin/%{cfg.buildcfg}"

   files { "**.h", "**.cc" }
   links { "GL", "GLU", "SDL", "SDL_image", "vlc" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
