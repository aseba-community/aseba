add_executable(asebacmd
	cmd.cpp
)
target_link_libraries(asebacmd ${ASEBA_CORE_LIBRARIES})
install(TARGETS asebacmd RUNTIME
	DESTINATION bin
)
