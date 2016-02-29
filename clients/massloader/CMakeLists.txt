if (QT4_FOUND)
	set(QT_USE_QTXML ON)
	set(QT_DONT_USE_QTGUI ON)
	include(${QT_USE_FILE})

# 	set(thymioupgrader_SRCS
# 		ThymioUpgrader.cpp)
# 	qt4_wrap_cpp(thymioupgrader_MOCS
# 		ThymioUpgrader.h
# 	)
	
	#include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
	
	add_executable(asebamassloader massloader.cpp)

	target_link_libraries(asebamassloader asebacompiler ${QT_LIBRARIES} ${ASEBA_CORE_LIBRARIES})

	install(TARGETS asebamassloader RUNTIME DESTINATION bin)

endif (QT4_FOUND)
