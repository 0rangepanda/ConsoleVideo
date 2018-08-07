INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/opencv)

SET(OpenCV_Libs opencv_videoio opencv_video opencv_core opencv_highgui opencv_imgcodecs)
LINK_LIBRARIES(${OpenCV_Libs})


SET(CMAKE_MACOSX_RPATH 1)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/mylib)

# Dynamic lib
ADD_LIBRARY(video_so SHARED video.cpp)
SET_TARGET_PROPERTIES(video_so PROPERTIES OUTPUT_NAME "video")

# Static Lib
ADD_LIBRARY(video_a STATIC video.cpp)
SET_TARGET_PROPERTIES(video_a PROPERTIES OUTPUT_NAME "video")
