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

# Formats that should always use the repository's local implementation
# (skip external find_package). Add names here (e.g. GIF) when the project
# includes a bundled implementation that should take priority.
if(NOT DEFINED FORCE_LOCAL_IMAGE_FORMATS)
	set(FORCE_LOCAL_IMAGE_FORMATS GIF)
endif()

foreach(IMAGE_FORMAT ${OSL_IMAGE_FORMATS_AVAILABLE})
	# If this format is forced to use the local implementation, skip find_package
	list(FIND FORCE_LOCAL_IMAGE_FORMATS ${IMAGE_FORMAT} _is_forced)
	if(NOT _is_forced EQUAL -1)
		# Use local implementation (mark supported if loader/writer exists)
		if(${IMAGE_FORMAT} IN_LIST OSL_IMAGE_FORMAT_LOADERS)
			list(APPEND OSL_IMAGE_FORMATS_SUPPORTED ${IMAGE_FORMAT})
			add_compile_definitions(-DOSL_IMAGE_LOADER_${IMAGE_FORMAT})
		endif()
		if(${IMAGE_FORMAT} IN_LIST OSL_IMAGE_FORMAT_WRITERS)
			list(APPEND OSL_IMAGE_FORMATS_SUPPORTED ${IMAGE_FORMAT})
			add_compile_definitions(-DOSL_IMAGE_WRITER_${IMAGE_FORMAT})
		endif()
	else()
		find_package(${IMAGE_FORMAT})
		if(${IMAGE_FORMAT}_FOUND)
		# External package found (SDK or system) -> use it
		list(APPEND OSL_IMAGE_FORMATS_SUPPORTED ${IMAGE_FORMAT})
		if(${IMAGE_FORMAT} IN_LIST OSL_IMAGE_FORMAT_LOADERS)
			add_compile_definitions(-DOSL_IMAGE_LOADER_${IMAGE_FORMAT})
			include_directories(${${IMAGE_FORMAT}_INCLUDE_DIRS})
		endif()
		if(${IMAGE_FORMAT} IN_LIST OSL_IMAGE_FORMAT_WRITERS)
			add_compile_definitions(-DOSL_IMAGE_WRITER_${IMAGE_FORMAT})
			include_directories(${${IMAGE_FORMAT}_INCLUDE_DIRS})
		endif()
		else()
			# No external package found. If we have a local implementation (source
			# files under src/image/format), keep the loader/writer and mark the
			# format as supported using the local code. Otherwise remove it.
			set(_has_local_impl FALSE)
			if(${IMAGE_FORMAT} IN_LIST OSL_IMAGE_FORMAT_LOADERS)
				list(APPEND OSL_IMAGE_FORMATS_SUPPORTED ${IMAGE_FORMAT})
				add_compile_definitions(-DOSL_IMAGE_LOADER_${IMAGE_FORMAT})
				set(_has_local_impl TRUE)
			endif()
			if(${IMAGE_FORMAT} IN_LIST OSL_IMAGE_FORMAT_WRITERS)
				list(APPEND OSL_IMAGE_FORMATS_SUPPORTED ${IMAGE_FORMAT})
				add_compile_definitions(-DOSL_IMAGE_WRITER_${IMAGE_FORMAT})
				set(_has_local_impl TRUE)
			endif()
			if(NOT _has_local_impl)
				list(REMOVE_ITEM OSL_IMAGE_FORMAT_WRITERS ${IMAGE_FORMAT})
				list(REMOVE_ITEM OSL_IMAGE_FORMAT_LOADERS ${IMAGE_FORMAT})
			endif()
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
