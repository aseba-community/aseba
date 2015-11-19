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
	ThymioBlockly/ThymioBlockly.cpp
)
qt4_wrap_cpp(qt_plugins_MOCS
	VariablesViewPlugin.h
	StopThymioPlugin.h
	ThymioVPL/ThymioVisualProgramming.h
	ThymioVPL/ResizingView.h
	ThymioVPL/Buttons.h
	ThymioVPL/Block.h
	ThymioVPL/ActionBlocks.h
	ThymioVPL/EventActionsSet.h
	ThymioVPL/Scene.h
	ThymioVPL/UsageLogger.h
	ThymioVPL/LogSignalMapper.h
	ThymioBlockly/ThymioBlockly.h
	ThymioBlockly/AsebaJavascriptInterface.h
)

if (PROTOBUF_FOUND)
	set(qt_plugins_SRCS ${qt_plugins_SRCS} ThymioVPL/UsageProfile.pb.cc ThymioVPL/UsageLogger.cpp)
endif (PROTOBUF_FOUND)


add_library(asebaqtplugins STATIC ${qt_plugins_SRCS} ${qt_plugins_MOCS})

