INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/opencv)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/mylib)

SET(OpenCV_Libs opencv_videoio opencv_video opencv_core opencv_highgui opencv_imgcodecs)
LINK_LIBRARIES(${OpenCV_Libs})

SET(MyLibs_Libs video rtp msg)
LINK_LIBRARIES(${MyLibs_Libs})

# Declare the executable target built from your sources
ADD_EXECUTABLE( client_test  client_test.cpp client_utils.cpp)
ADD_EXECUTABLE( client  client.cpp client_utils.cpp)
