add_executable(asebaswitch
	switch.cpp
)

target_link_libraries(asebaswitch asebacore ${EXTRA_LIBS})

install(TARGETS asebaswitch RUNTIME
	DESTINATION bin
)
