add_executable(asebadump
	dump.cpp
)

target_link_libraries(asebadump aseba ${EXTRA_LIBS})