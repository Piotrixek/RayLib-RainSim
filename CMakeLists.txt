
cmake_minimum_required(VERSION 3.10)


project(RaylibRainSim LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



set(RAYLIB_LOCATION "C:/libs/raylib-5.5_win64_msvc16") 


if(NOT EXISTS "${RAYLIB_LOCATION}/include/raylib.h")
    message(FATAL_ERROR "Raylib not found at specified location: ${RAYLIB_LOCATION}. Did u extract it there? Check the path in CMakeLists.txt.")
endif()


set(RAYLIB_INCLUDE_DIR "${RAYLIB_LOCATION}/include")

set(RAYLIB_LIBRARY_DIR "${RAYLIB_LOCATION}/lib") 


set(RAYLIB_LIBRARY "${RAYLIB_LIBRARY_DIR}/raylib.lib") 

set(RAYLIB_DLL "${RAYLIB_LIBRARY_DIR}/raylib.dll") 




add_executable(RaylibRainSim main.cpp)


target_include_directories(RaylibRainSim PRIVATE ${RAYLIB_INCLUDE_DIR})



target_link_libraries(RaylibRainSim PRIVATE 
    ${RAYLIB_LIBRARY} 
    winmm           
    kernel32        
    user32          
    gdi32           
    shell32         
    ole32           
)





if(WIN32)
    add_custom_command(TARGET RaylibRainSim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${RAYLIB_DLL}"
        
        $<TARGET_FILE_DIR:RaylibRainSim> 
        COMMENT "Copying raylib.dll to output directory")
endif()


source_group("Source Files" FILES main.cpp)
