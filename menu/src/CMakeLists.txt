file(GLOB files "*.svg")
foreach(file ${files})
	install(FILES ${file} DESTINATION usr/share/icons/hicolor/scalable/apps)
endforeach()
