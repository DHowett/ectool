# ============================
# Common source files for ectool and libectool
# ============================
set(ECTOOL_COMMON_SOURCES
    ec_flash.cc
    ec_panicinfo.cc
    ectool_i2c.cc
    ectool_keyscan.cc
    misc_util.cc
    crc.cc
    comm-host.cc
    lock/gec_lock.cc
)

if(NOT WIN32)
    list(APPEND ECTOOL_COMMON_SOURCES
        comm-dev.cc
        comm-i2c.cc
        comm-lpc.cc
        comm-servo-spi.cc
        comm-usb.cc
        lock/file_lock.cc
    )
else()
    list(APPEND ECTOOL_COMMON_SOURCES
        comm-win32.cc
        lock/win32_mutex_lock.cc
    )
endif()



# ============================
# Add Executable Target: ectool
# ============================
add_executable(ectool ectool.cc ${ECTOOL_COMMON_SOURCES})

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

if(WIN32)
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
    target_include_directories(ectool PRIVATE
        ../include/win32
    )
endif()

target_link_libraries(ectool ${libusb_LIBRARIES} ${libftdi1_LIBRARIES})


# ============================
# Add Shared Library : libectool.so
# ============================

set(LIBECTOOL_SOURCES ${ECTOOL_COMMON_SOURCES} ectool_lib.cc)

add_library(libectool SHARED ${LIBECTOOL_SOURCES})

target_include_directories(libectool PRIVATE
    ../include
    ${libusb_INCLUDE_DIRS}
)

target_compile_definitions(libectool PRIVATE
    CHROMIUM_EC
    EXTERNAL_ECTOOL_BUILD
)

target_compile_options(libectool PRIVATE
    -Wno-c99-designator
    -Wno-address-of-packed-member
    -Wno-format-security
)

target_link_libraries(libectool PRIVATE
    ${libusb_LIBRARIES}
    ${libftdi1_LIBRARIES}
)

if(WIN32)
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
    target_include_directories(ectool PRIVATE
        ../include/win32
    )
endif()

set_target_properties(libectool PROPERTIES
    OUTPUT_NAME "ectool"  # Generates `libectool.so` or `libectool.dll`
)

