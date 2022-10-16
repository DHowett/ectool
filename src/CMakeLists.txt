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
	comm-dev.cc
	comm-host.cc
	comm-i2c.cc
	comm-lpc.cc
	comm-servo-spi.cc
	comm-usb.cc
	ec_flash.cc
	ec_panicinfo.cc
	ectool.cc
	ectool_i2c.cc
	ectool_keyscan.cc
	misc_util.cc
	crc.cc

	lock/file_lock.cc
	lock/gec_lock.cc
)

target_link_libraries(ectool ${libusb_LIBRARIES} ${libftdi1_LIBRARIES})
