cmake_minimum_required(VERSION 3.5)
if( NOT FIPS_IMPORT )
	cmake_minimum_required(VERSION 3.5)

	get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
	include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

	fips_setup(PROJECT libvideoencoder)
endif()

find_package(OpenCV COMPONENTS core REQUIRED)

## C++ flags applied to all builds
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

## This is required on OSX?
IF(APPLE)
		# Fix linking on 10.14+. See https://stackoverflow.com/questions/54068035
		LINK_DIRECTORIES(/usr/local/lib)
		fips_include_directories( /usr/local/include )
ENDIF()

fips_begin_module( videoencoder )

	fips_include_directories(
		${CMAKE_SOURCE_DIR}/include
		${OpenCV_INCLUDE_DIRS}
	)

  fips_src( lib *.cpp )

  fips_libs( ${OpenCV_LIBS} avutil avcodec swscale avformat )

fips_end_module()


if( NOT FIPS_IMPORT )
  add_subdirectory("tools/")

	if( FIPS_UNITTESTS )
	  gtest_begin( videoencoder )
	    fips_src( test/unit/ )

  		fips_libs( avutil avcodec swscale avformat )
			fips_deps( videoencoder )
		gtest_end()
	endif()

  fips_finish()
endif()
