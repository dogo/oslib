# Get the supported image file formats
# Assumes LIB_SOURCES defined and library name used in find_package as the load/write image filename suffix

if(NOT OSL_IMAGE_FORMAT_DIRECTORY)
	set(OSL_IMAGE_FORMAT_DIRECTORY "${PROJECT_SOURCE_DIR}/src/image/format/")
endif()

if(NOT OSL_IMAGE_FORMAT_LOADER_EXTENSION)
	set(OSL_IMAGE_FORMAT_LOADER_EXTENSION .c)
endif()
if(NOT OSL_IMAGE_FORMAT_WRITER_EXTENSION)
	set(OSL_IMAGE_FORMAT_WRITER_EXTENSION .c)
endif()

if(NOT OSL_IMAGE_FORMAT_LOADER_FILENAME)
	set(OSL_IMAGE_FORMAT_LOADER_FILENAME "oslLoadImageFile")
endif()
if(NOT OSL_IMAGE_FORMAT_WRITER_FILENAME)
	set(OSL_IMAGE_FORMAT_WRITER_FILENAME "oslWriteImageFile")
endif()

file(GLOB_RECURSE OSL_IMAGE_FORMAT_LOADERS_SOURCES LIST_DIRECTORIES false RELATIVE ${OSL_IMAGE_FORMAT_DIRECTORY} CONFIGURE_DEPENDS ${OSL_IMAGE_FORMAT_LOADER_FILENAME}*.c)
file(GLOB_RECURSE OSL_IMAGE_FORMAT_WRITERS_SOURCES LIST_DIRECTORIES false RELATIVE ${OSL_IMAGE_FORMAT_DIRECTORY} CONFIGURE_DEPENDS ${OSL_IMAGE_FORMAT_WRITER_FILENAME}*.c)

foreach(FORMAT_SOURCE ${OSL_IMAGE_FORMAT_LOADERS_SOURCES})
	cmake_path(REMOVE_EXTENSION FORMAT_SOURCE)
	cmake_path(GET FORMAT_SOURCE FILENAME FORMAT_SOURCE)
	string(REGEX REPLACE "^${OSL_IMAGE_FORMAT_LOADER_FILENAME}" "" FORMAT_SOURCE ${FORMAT_SOURCE})
	list(APPEND OSL_IMAGE_FORMAT_LOADERS ${FORMAT_SOURCE})
endforeach()
foreach(FORMAT_SOURCE ${OSL_IMAGE_FORMAT_WRITERS_SOURCES})
	cmake_path(REMOVE_EXTENSION FORMAT_SOURCE)
	cmake_path(GET FORMAT_SOURCE FILENAME FORMAT_SOURCE)
	string(REGEX REPLACE "^${OSL_IMAGE_FORMAT_WRITER_FILENAME}" "" FORMAT_SOURCE ${FORMAT_SOURCE})
	list(APPEND OSL_IMAGE_FORMAT_WRITERS ${FORMAT_SOURCE})
endforeach()

list(APPEND OSL_IMAGE_FORMATS_AVAILABLE ${OSL_IMAGE_FORMAT_LOADERS} ${OSL_IMAGE_FORMAT_WRITERS})
list(REMOVE_DUPLICATES OSL_IMAGE_FORMATS_AVAILABLE)

# -------------------------
# Helpers to keep logic simple
# -------------------------
macro(osl_mark_supported FORMAT)
	list(FIND OSL_IMAGE_FORMATS_SUPPORTED ${FORMAT} _already)
	if(_already EQUAL -1)
		list(APPEND OSL_IMAGE_FORMATS_SUPPORTED ${FORMAT})
	endif()
	if(${FORMAT} IN_LIST OSL_IMAGE_FORMAT_LOADERS)
		add_compile_definitions(-DOSL_IMAGE_LOADER_${FORMAT})
	endif()
	if(${FORMAT} IN_LIST OSL_IMAGE_FORMAT_WRITERS)
		add_compile_definitions(-DOSL_IMAGE_WRITER_${FORMAT})
	endif()
endmacro()

macro(osl_use_local FORMAT)
	# Use in-tree implementation
	osl_mark_supported(${FORMAT})
	# Friendly message when GIF is local
	if(${FORMAT} STREQUAL "GIF")
		if(NOT OSL_LOCAL_GIF_MSG_PRINTED)
			message("-- Found Local GIF: ${PROJECT_SOURCE_DIR}/lib/giflib")
			set(OSL_LOCAL_GIF_MSG_PRINTED TRUE)
		endif()
	endif()
endmacro()

macro(osl_try_external FORMAT)
	# Try external dependency via find_package (SDK/toolchain controlled)
	find_package(${FORMAT})
	if(${FORMAT}_FOUND)
		if(${FORMAT} IN_LIST OSL_IMAGE_FORMAT_LOADERS)
			include_directories(${${FORMAT}_INCLUDE_DIRS})
		endif()
		if(${FORMAT} IN_LIST OSL_IMAGE_FORMAT_WRITERS)
			include_directories(${${FORMAT}_INCLUDE_DIRS})
		endif()
		osl_mark_supported(${FORMAT})
		set(_osl_external_found TRUE)
	else()
		set(_osl_external_found FALSE)
	endif()
endmacro()

# Formats that should always use the repository's local implementation
# (skip external find_package). Add names here (e.g. GIF) when the project
# includes a bundled implementation that should take priority.
if(NOT DEFINED FORCE_LOCAL_IMAGE_FORMATS)
	set(FORCE_LOCAL_IMAGE_FORMATS GIF)
endif()

foreach(IMAGE_FORMAT ${OSL_IMAGE_FORMATS_AVAILABLE})
	# If this format is forced to local, use in-tree implementation (no SDK lookup)
	list(FIND FORCE_LOCAL_IMAGE_FORMATS ${IMAGE_FORMAT} _is_forced)
	if(NOT _is_forced EQUAL -1)
		osl_use_local(${IMAGE_FORMAT})
	else()
		# For non-forced formats: require external SDK library (no fallback to local)
		osl_try_external(${IMAGE_FORMAT})
		if(NOT _osl_external_found)
			# External not found → remove format (no local fallback for SDK-only formats)
			list(REMOVE_ITEM OSL_IMAGE_FORMAT_WRITERS ${IMAGE_FORMAT})
			list(REMOVE_ITEM OSL_IMAGE_FORMAT_LOADERS ${IMAGE_FORMAT})
		endif()
	endif()
endforeach()

foreach(IMAGE_FORMAT ${OSL_IMAGE_FORMAT_WRITERS})
	list(APPEND LIB_SOURCES "${PROJECT_SOURCE_DIR}/src/image/format/oslWriteImageFile${IMAGE_FORMAT}.c")
endforeach()
foreach(IMAGE_FORMAT ${OSL_IMAGE_FORMAT_LOADERS})
	list(APPEND LIB_SOURCES "${PROJECT_SOURCE_DIR}/src/image/format/oslLoadImageFile${IMAGE_FORMAT}.c")
endforeach()

message("-- Using ${OSL_IMAGE_FORMATS_SUPPORTED}")
