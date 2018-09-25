cmake_minimum_required(VERSION 3.5)
if( NOT FIPS_IMPORT )
	cmake_minimum_required(VERSION 3.5)

	get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
	include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

	fips_setup(PROJECT libvideoencoder)
endif()

#list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

## C++ flags applied to all builds
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

fips_begin_module( videoencoder )

  fips_src( lib *.cpp )

   # fips_libs( ${Boost_LIBRARIES} ${OpenCV_LIBS}
   #            ${ZLIB_LIBRARIES} ${TINYXML2_LIBRARIES} )
   # fips_deps( g3logger )

   fips_libs( avutil avcodec swscale avformat )

   fips_include_directories(
	 	${CMAKE_SOURCE_DIR}/include
	 )

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
