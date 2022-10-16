if(WIN32)
	add_library(CrosECDriver INTERFACE)
	target_include_directories(CrosECDriver INTERFACE FrameworkWindowsUtils)
endif()
