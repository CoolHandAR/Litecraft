-- premake5.lua
workspace "LiteCraft"
   architecture "x64"
   configurations { "Debug", "Release" }
   location "build/Litecraft"	

project "Litecraft"
   language "C"
   cdialect "C11"
   compileas "C"
   targetdir "bin/%{cfg.buildcfg}"
   location "build/Litecraft"		
	includedirs { "thirdparty" }
	includedirs { "src" }
	links{ "glfw3"}
	libdirs { "lib" }

   files { "**.h", "**.c", "**.vert", "**.frag", "**.comp", "**.geo", "**.py", "**.incl"}

   filter "configurations:Debug"
      kind "ConsoleApp"
      defines { "DEBUG" }
      symbols "On"
      postbuildcommands 
      { 
      	 	"{COPYDIR} %[shaders] %[bin/Debug/shaders]",
		"{COPYDIR} %[assets] %[bin/Debug/assets]",
		"{COPYFILE} %[LC_Readme.txt] %[bin/Debug/LC_Readme.txt]",
      }

   filter "configurations:Release"
      kind "WindowedApp"
      defines { "NDEBUG" }
      optimize "On"
      postbuildcommands 
      { 
		"{COPYDIR} %[shaders] %[bin/Release/shaders]",
      	 	"{COPYDIR} %[assets] %[bin/Release/assets]",
		"{COPYFILE} %[LC_Readme.txt] %[bin/Release/LC_Readme.txt]",
      }