find_package(OpenGL REQUIRED)
set(CMAKE_AUTOMOC ON)

set(enki_SRCS
	AsebaMarxbot.cpp
	EnkiMarxbot.cpp
	AsebaMarxbot-vm-descriptions.c
)

add_executable(asebamarxbot WIN32 ${enki_SRCS})

target_link_libraries(asebamarxbot asebavmbuffer asebavm enkiviewer asebacommon)
codesign(asebamarxbot)
