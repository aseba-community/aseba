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

qt5_add_resources(challenge_RCC_SRCS challenge-textures.qrc)

add_executable(asebachallenge WIN32 ${challenge_SRCS} ${challenge_RCC_SRCS})

target_link_libraries(asebachallenge asebavmbuffer asebavm enkiviewer asebacommon)
install_qt_app(asebachallenge)

codesign(asebachallenge)
