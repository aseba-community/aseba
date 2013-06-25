# libxml2
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

add_executable(asebashell
	shell.cpp
)
target_link_libraries(asebashell asebacompiler ${ASEBA_CORE_LIBRARIES} ${LIBXML2_LIBRARIES})
install(TARGETS asebashell RUNTIME
	DESTINATION bin
)
