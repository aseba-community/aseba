if (Qt5Core_FOUND AND Qt5Xml_FOUND)

	add_executable(asebamassloader massloader.cpp)

	target_link_libraries(asebamassloader asebacompiler Qt5::Xml Qt5::Core ${ASEBA_CORE_LIBRARIES})

	install(TARGETS asebamassloader RUNTIME DESTINATION bin)

endif ()
