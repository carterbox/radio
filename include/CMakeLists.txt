cmake_minimum_required(VERSION 2.8)
project( frames )

set (CMAKE_CXX_FLAGS "-std=c++11")


# Finding Packages
find_package( OpenCV 2.4.10 REQUIRED )
find_package( Boost 1.55.0 REQUIRED COMPONENTS filesystem system date_time thread)

if(Boost_FOUND)
  include_directories( ${Boost_INCLUDE_DIRS} )
  add_executable( frames harriscorners.cpp )
  target_link_libraries( frames ${OpenCV_LIBS} )
  target_link_libraries( frames ${Boost_LIBRARIES} )
endif()

# Including Headers
include_directories("${CMAKE_SOURCE_DIR}/include")
