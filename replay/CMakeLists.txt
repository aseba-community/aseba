add_executable(asebarec
	rec.cpp
	${SOCKETCAN_PLUGIN}
)

target_link_libraries(asebarec asebacore ${EXTRA_LIBS})

install(TARGETS asebarec RUNTIME
	DESTINATION bin
)

add_executable(asebaplay
	play.cpp
	${SOCKETCAN_PLUGIN}
)

target_link_libraries(asebaplay asebacore ${EXTRA_LIBS})

install(TARGETS asebaplay RUNTIME
	DESTINATION bin
)
