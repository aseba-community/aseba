add_executable(asebadump
	dump.cpp
)

target_link_libraries(asebadump asebacore ${EXTRA_LIBS})

install(TARGETS asebadump RUNTIME
	DESTINATION bin
)
