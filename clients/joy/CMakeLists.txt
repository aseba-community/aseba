find_package(SDL2)
if (SDL2_FOUND)
	message(STATUS "SDL2 found, enabling joystick/joypad support")
	include_directories(${SDL2_INCLUDE_DIR})

	add_executable(asebajoy
		asebajoy.cpp
	)
	target_link_libraries(asebajoy ${ASEBA_CORE_LIBRARIES} ${SDL2_LIBRARY})
	install(TARGETS asebajoy RUNTIME
		DESTINATION bin
	)
elseif (SDL2_FOUND)
	message(STATUS "SDL2 *not* found, disabling joystick/joypad support")
endif (SDL2_FOUND)

