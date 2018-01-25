find_package(OpenGL REQUIRED)

set(QT_USE_QTMAIN ON)
set(QT_USE_QTOPENGL ON)
include(${QT_USE_FILE})

set(enki_SRCS
	AsebaMarxbot.cpp
	EnkiMarxbot.cpp
	AsebaMarxbot-vm-descriptions.c
)

add_executable(asebamarxbot WIN32 ${enki_SRCS})

target_link_libraries(asebamarxbot asebavmbuffer asebavm enki_viewer ${QT_LIBRARIES} ${OPENGL_LIBRARIES} asebacommon)
