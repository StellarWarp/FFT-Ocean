cmake_minimum_required(VERSION 3.14)

add_subdirectory("Ocean Simulation")

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Texture)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Texture DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Model)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_subdirectory("Common")
