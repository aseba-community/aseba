add_executable(asebadump
	dump.cpp
)
target_link_libraries(asebadump ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebadump RUNTIME
	DESTINATION bin
)
