add_executable(asebacmd
	HexFile.cpp
	cmd.cpp
)

target_link_libraries(asebacmd asebacore ${EXTRA_LIBS})