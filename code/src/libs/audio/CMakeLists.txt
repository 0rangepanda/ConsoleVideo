#INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
#LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib/portaudio)

#SET(Portaudio_Libs portaudio)
#LINK_LIBRARIES(${Portaudio_Libs})


#SET(CMAKE_MACOSX_RPATH 1)
#SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/mylib)

# Dynamic lib
#ADD_LIBRARY(audio_so SHARED audio.cpp)
#SET_TARGET_PROPERTIES(audio_so PROPERTIES OUTPUT_NAME "audio")

# Static Lib
#ADD_LIBRARY(audio_a STATIC audio.cpp)
#SET_TARGET_PROPERTIES(audio_a PROPERTIES OUTPUT_NAME "audio")
