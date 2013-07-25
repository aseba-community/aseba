add_executable(asebabotspeak
	botspeak.cpp
)

target_link_libraries(asebabotspeak asebacompiler ${ASEBA_CORE_LIBRARIES})

install(TARGETS asebabotspeak RUNTIME
	DESTINATION bin
)
