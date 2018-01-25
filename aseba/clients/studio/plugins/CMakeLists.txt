set(qt_plugins_SRCS
	VariablesViewPlugin.cpp
	StopThymioPlugin.cpp
	ThymioVPL/ThymioVisualProgramming.cpp
	ThymioVPL/ResizingView.cpp
	ThymioVPL/Buttons.cpp
	ThymioVPL/Block.cpp
	ThymioVPL/EventBlocks.cpp
	ThymioVPL/StateBlocks.cpp
	ThymioVPL/ActionBlocks.cpp
	ThymioVPL/EventActionsSet.cpp
	ThymioVPL/Scene.cpp
	ThymioVPL/Compiler.cpp
	ThymioVPL/CompilerVisitors.cpp
	ThymioVPL/Style.cpp
	ThymioVPL/Utils.cpp
	ThymioVPL/LogSignalMapper.cpp
)

qt5_wrap_cpp(qt_plugins_MOCS
	VariablesViewPlugin.h
	StopThymioPlugin.h
	ThymioVPL/ThymioVisualProgramming.h
	ThymioVPL/ResizingView.h
	ThymioVPL/Buttons.h
	ThymioVPL/Block.h
	ThymioVPL/EventBlocks.h
	ThymioVPL/ActionBlocks.h
	ThymioVPL/EventActionsSet.h
	ThymioVPL/Scene.h
	ThymioVPL/UsageLogger.h
	ThymioVPL/LogSignalMapper.h
)

if (PROTOBUF_FOUND AND PROTOBUF_PROTOC_EXECUTABLE)
	execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/ThymioVPL -I${CMAKE_CURRENT_SOURCE_DIR}/ThymioVPL ${CMAKE_CURRENT_SOURCE_DIR}/ThymioVPL/UsageProfile.proto)
	include_directories(${CMAKE_CURRENT_BINARY_DIR}/ThymioVPL)
	set(qt_plugins_SRCS ${qt_plugins_SRCS} ThymioVPL/UsageProfile.pb.cc ThymioVPL/UsageLogger.cpp)
endif (PROTOBUF_FOUND AND PROTOBUF_PROTOC_EXECUTABLE)

add_library(asebaqtplugins STATIC ${qt_plugins_SRCS} ${qt_plugins_MOCS})
target_link_libraries(asebaqtplugins aseba_conf)

qt5_use_modules(asebaqtplugins Widgets Xml Svg)
if (Qt5X11Extras_FOUND)
	qt5_use_modules(asebaqtplugins X11Extras)
endif ()
