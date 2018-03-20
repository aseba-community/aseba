set(CMAKE_AUTOMOC ON)

set(challenge_SRCS
	challenge.cpp
	challenge-vm-description.c
	objects/food_base.cpp
	objects/food_charge0.cpp
	objects/food_charge1.cpp
	objects/food_charge2.cpp
	objects/food_charge3.cpp
	objects/food_ring.cpp
)

file(GLOB TRANSLATIONS_SOURCES "asebachallenge*.ts")
set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")
qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )

qt5_add_resources(challenge_RCC_SRCS challenge-textures.qrc)

add_executable(asebachallenge WIN32 ${challenge_SRCS} ${challenge_RCC_SRCS})

target_link_libraries(asebachallenge asebavmbuffer asebavm enkiviewer asebacommon)
install_qt_app(asebachallenge)

codesign(asebachallenge)
