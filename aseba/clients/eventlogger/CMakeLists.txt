if (Qt5Widgets_FOUND)
	if (QWT_INCLUDE_DIR AND QWT_LIBRARIES)
	
		set(eventlogger_SRCS eventlogger.cpp)
		
		include_directories (${QWT_INCLUDE_DIR})
		
		#add_executable(asebastudio WIN32 ${eventlogger_SRCS} ${resfiles})
		add_executable(asebaeventlogger WIN32 ${eventlogger_SRCS})
		
		# Only include the qtmain lib if we are on the windows platform
		target_link_libraries(asebaeventlogger ${QWT_LIBRARIES} Qt5::Widgets ${ASEBA_CORE_LIBRARIES})
		
	else ()
		message(WARNING "Cannot find Qwt, event logger disabled")
	endif ()
endif ()
