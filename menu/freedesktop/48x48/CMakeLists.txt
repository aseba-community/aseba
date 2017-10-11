file(GLOB files "*.png")
foreach(file ${files})
	install(FILES ${file} DESTINATION share/icons/hicolor/48x48/apps)
endforeach()
