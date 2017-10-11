file(GLOB files "*.png")
foreach(file ${files})
	install(FILES ${file} DESTINATION usr/share/icons/hicolor/48x48/apps)
endforeach()
