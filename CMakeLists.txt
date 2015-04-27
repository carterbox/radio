cmake_minimum_required(VERSION 2.8)
project( segment )

set (CMAKE_CXX_FLAGS "-std=c++11")


# Finding Packages
find_package( OpenCV 2.4.9 REQUIRED )
find_package( Boost 1.55.0 REQUIRED COMPONENTS filesystem system date_time thread)

if(Boost_FOUND)
  include_directories( ${Boost_INCLUDE_DIRS} )
  add_executable( segment segment.cpp )
  target_link_libraries( segment ${OpenCV_LIBS} )
  target_link_libraries( segment ${Boost_LIBRARIES} )
endif()

# Including Headers
include_directories("${CMAKE_SOURCE_DIR}/include")
