add_executable(asebaswitch
	switch.cpp
	${SOCKETCAN_PLUGIN}
)

target_link_libraries(asebaswitch asebacore ${EXTRA_LIBS})

install(TARGETS asebaswitch RUNTIME
	DESTINATION bin
	LIBRARY DESTINATION lib
)
