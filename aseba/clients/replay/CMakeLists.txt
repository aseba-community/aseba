add_executable(asebarec
	rec.cpp
)
target_link_libraries(asebarec asebacommon)
install(TARGETS asebarec RUNTIME
	DESTINATION bin
)

add_executable(asebaplay
	play.cpp
)
target_link_libraries(asebaplay asebacommon)
install(TARGETS asebaplay RUNTIME
	DESTINATION bin
)
