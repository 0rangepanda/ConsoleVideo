INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/test)

LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/opencv)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/portaudio)
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/mylib)

SET(OpenCV_Libs opencv_videoio opencv_video opencv_core opencv_highgui opencv_imgcodecs)
LINK_LIBRARIES(${OpenCV_Libs})

SET(Portaudio_Libs portaudio)
LINK_LIBRARIES(${Portaudio_Libs})

SET(MyLibs_Libs video rtp)
LINK_LIBRARIES(${MyLibs_Libs})

ADD_EXECUTABLE(video_test test.cpp)
ADD_EXECUTABLE(video0 test0.cpp)
ADD_EXECUTABLE(video_jpeg test_jpeg.cpp)
ADD_EXECUTABLE( videoclient1  client_test1.cpp )
ADD_EXECUTABLE( videoclient2  client_test2.cpp )
ADD_EXECUTABLE( videoclient  client.cpp )

ADD_EXECUTABLE( patest  patest_read_record.c )
ADD_EXECUTABLE( audiotest  audio_test.cpp )
