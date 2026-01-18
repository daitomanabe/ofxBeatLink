meta:
	ADDON_NAME = ofxBeatLink
	ADDON_DESCRIPTION = openFrameworks addon for Pioneer DJ Link protocol (beat-link-cpp wrapper)
	ADDON_AUTHOR = Daito Manabe
	ADDON_TAGS = "dj" "pioneer" "cdj" "beat" "sync" "pro dj link"
	ADDON_URL = https://github.com/daitomanabe/beat-link-cpp

common:
	# Include paths
	ADDON_INCLUDES = src
	ADDON_INCLUDES += libs/beat-link-cpp/include
	ADDON_INCLUDES += libs/beat-link-cpp/src
	ADDON_INCLUDES += libs/asio-include

	# Exclude examples, tests, and python bindings from beat-link-cpp
	ADDON_SOURCES_EXCLUDE = libs/beat-link-cpp/examples/%
	ADDON_SOURCES_EXCLUDE += libs/beat-link-cpp/tests/%
	ADDON_SOURCES_EXCLUDE += libs/beat-link-cpp/src/python_bindings.cpp

	# Source files
	ADDON_SOURCES = src/ofxBeatLink.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/Beat.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/BeatFinder.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/CdjStatus.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/DeviceAnnouncement.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/DeviceFinder.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/DeviceUpdate.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/MixerStatus.cpp
	ADDON_SOURCES += libs/beat-link-cpp/src/Util.cpp

	# C++ flags (Asio standalone mode, C++17 required)
	ADDON_CPPFLAGS = -DASIO_STANDALONE -std=c++17

osx:
	# macOS specific settings
	ADDON_FRAMEWORKS =

linux64:
	# Linux specific settings
	ADDON_LDFLAGS = -lpthread

linux:
	ADDON_LDFLAGS = -lpthread

linuxarmv6l:
	ADDON_LDFLAGS = -lpthread

linuxarmv7l:
	ADDON_LDFLAGS = -lpthread

msys2:
	# Windows MSYS2 settings
	ADDON_LDFLAGS = -lws2_32

vs:
	# Visual Studio settings
	ADDON_LDFLAGS = ws2_32.lib
