cmake_minimum_required(VERSION 3.5)
if( NOT FIPS_IMPORT )
	cmake_minimum_required(VERSION 3.5)

	get_filename_component(FIPS_ROOT_DIR "../../../fips" ABSOLUTE)
	include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

	fips_setup(PROJECT ffmpeg-encode-example)
endif()

#list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# find_package(Eigen3 REQUIRED )
# find_package(Boost COMPONENTS filesystem system program_options REQUIRED )  #n.b. program_options is only required for my version of Stereolabs "Recorder"

## Default OpenCV version
#list(APPEND OPENCV_COMPONENTS core highgui imgproc )
#find_package(OpenCV COMPONENTS ${OPENCV_COMPONENTS} REQUIRED)

## C++ flags applied to all builds
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

## Configure OpenCV ##

## Find Blackmagic SDK
#
# ## Set global include paths
# fips_include_directories(
#   ${CMAKE_SOURCE_DIR}/include
#   ${BLACKMAGIC_INCLUDE_DIR}
#   ${OpenCV_INCLUDE_DIRS}
#  )

fips_begin_app( encode cmdline )

	fips_files( main.cpp VideoEncoder.cpp)
	fips_libs( avutil avcodec swscale avformat )

fips_end_app()



if( NOT FIPS_IMPORT )
  #add_subdirectory("tools/")

	if( FIPS_UNITTESTS )
			# gtest_begin( blackmagic )
			#     fips_src( test/unit/ )
			#     fips_deps( blackmagic )
			# gtest_end()
	endif()

  fips_finish()
endif()
