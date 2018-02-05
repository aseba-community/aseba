include_directories(${SDL2_INCLUDE_DIR})

add_executable(asebajoy
	asebajoy.cpp
)
target_link_libraries(asebajoy asebacommon asebadashelplugins ${SDL2_LIBRARY})
install(TARGETS asebajoy RUNTIME
	DESTINATION bin
)
