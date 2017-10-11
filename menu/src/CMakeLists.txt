file(GLOB files "*.svg")
foreach(file ${files})
	install(FILES ${file} DESTINATION share/icons/hicolor/scalable/apps)
endforeach()
