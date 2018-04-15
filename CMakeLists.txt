cmake_minimum_required(VERSION 2.8)
project( simple-pose-estimation )
find_package( OpenCV 3.0 REQUIRED )

include_directories( ${OpenCV_INCLUDE_DIRS} )

#print(${OpenCV_INCLUDE_DIRS})

file(GLOB pose_estimator
    "src/*.h"
    "src/*.cpp"
)

add_executable( pe ${pose_estimator} )
target_link_libraries( pe ${OpenCV_LIBS} )
