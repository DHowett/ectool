add_executable(ectool)

target_include_directories(ectool PRIVATE
	../include
	${libusb_INCLUDE_DIRS}
)

target_compile_definitions(ectool PRIVATE
	CHROMIUM_EC
	EXTERNAL_ECTOOL_BUILD
)

target_compile_options(ectool PRIVATE
	-Wno-c99-designator
	-Wno-address-of-packed-member
	-Wno-format-security
)

target_sources(ectool PRIVATE
	ec_flash.cc
	ec_panicinfo.cc
	ectool.cc
	ectool_i2c.cc
	ectool_keyscan.cc
	misc_util.cc
	crc.cc
	comm-host.cc

	lock/gec_lock.cc
)

if(NOT WIN32)
	target_sources(ectool PRIVATE
		comm-dev.cc
		comm-i2c.cc
		comm-lpc.cc
		comm-servo-spi.cc
		comm-usb.cc

		lock/file_lock.cc
	)
else()
	target_compile_definitions(ectool PRIVATE
		_CRT_SECURE_NO_WARNINGS
	)

	target_link_libraries(ectool PRIVATE
		getopt
		CrosECDriver
		onecoreuap_apiset.lib
	)

	if(MSVC)
		target_compile_options(ectool PRIVATE
			/FI"..\\include\\win32_shim.h"
		)
	else()
		target_compile_options(ectool PRIVATE
			-include "..\\include\\win32_shim.h"
		)
	endif()

	target_sources(ectool PRIVATE
		comm-win32.cc
		lock/win32_mutex_lock.cc
	)

	target_include_directories(ectool PRIVATE
		../include/win32
	)
endif()

target_link_libraries(ectool ${libusb_LIBRARIES} ${libftdi1_LIBRARIES})
