cmake_minimum_required (VERSION 2.4)

set (PACKAGE libmpio)
project (${PACKAGE} C)

find_library (LIBUSB NAMES usb)

if (LIBUSB)
	message (STATUS "found libusb at ${LIBUSB}")
else (LIBUSB)
	message (FATAL_ERROR "libusb is required")
endif (LIBUSB)

add_subdirectory (src)
