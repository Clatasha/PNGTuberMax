# Modified by Clatasha for PNGTuber Max (2026-10-11).
cmake_minimum_required(VERSION 3.24)
project(PNGTuberMax)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "RahiTuber")

if(${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
   message(FATAL_ERROR "In-tree builds are NOT supported.")
endif()

#
# Option prep
#
add_compile_definitions(SFML_STATIC)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Turn off to disable SFML building shared libs" FORCE)
set(PA_DISABLE_INSTALL ON CACHE BOOL "Disable targets install and uninstall (for embedded builds)" FORCE)
set(tinyxml2_BUILD_TESTING OFF CACHE BOOL "Build tests for tinyxml2" FORCE)
set(SPOUT_BUILD_CMT OFF CACHE BOOL "For Visual Studio - build /MT to link runtime libraries" FORCE)

#
# Add dependencies
#
add_subdirectory(Libraries/freetype)
add_subdirectory(Libraries/portaudio)
add_subdirectory(Libraries/SFML)
if(WIN32)
    add_subdirectory(Libraries/Spout2)
endif()
add_subdirectory(Libraries/tinyxml2)

# Set folder of Spout projects (for IDEs which support them)
if(WIN32)
    set_target_properties("Spout" PROPERTIES FOLDER "Spout")
    set_target_properties("Spout_static" PROPERTIES FOLDER "Spout")
    set_target_properties("SpoutLibrary" PROPERTIES FOLDER "Spout")
endif()

include(imgui) # imgui doesn't ship with a standard CMakeLists.txt
include(mongoose) # mongoose doesn't ship with a standard CMakeLists.txt

target_include_directories(sfml-graphics PRIVATE ${CMAKE_SOURCE_DIR}/Libraries/freetype/include)
target_include_directories(freetype PRIVATE ${PROJECT_BINARY_DIR}/include/freetype/config)
target_include_directories(sfml-graphics PRIVATE ${CMAKE_SOURCE_DIR}/Libraries/Simple-FFT/include)

#
# External dependencies
#
find_package(OpenGL REQUIRED)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11)
endif()


#
# Add main project
#
add_subdirectory(RahiTuber)

#
# Add test project
#
option(PNGMAX_BUILD_LEGACY_TESTS "Build inherited legacy tests" OFF)
if(MSVC AND PNGMAX_BUILD_LEGACY_TESTS)
    add_subdirectory(RahiTuber_Test)
endif()

add_executable(MaxCameraFrameTests tests/CameraFrameTests.cpp)
target_compile_features(MaxCameraFrameTests PRIVATE cxx_std_17)

add_executable(MaxWorkspaceTests tests/WorkspaceTests.cpp)
target_compile_features(MaxWorkspaceTests PRIVATE cxx_std_17)

add_executable(MaxPlushMathTests tests/PlushMathTests.cpp)
target_compile_features(MaxPlushMathTests PRIVATE cxx_std_17)
add_executable(MaxPlushRenderTests tests/PlushRenderTests.cpp)
target_compile_features(MaxPlushRenderTests PRIVATE cxx_std_17)
target_link_libraries(MaxPlushRenderTests PRIVATE sfml-graphics sfml-window sfml-system)

add_executable(MaxEditorTests tests/EditorTests.cpp
 RahiTuber/LayerManager.cpp RahiTuber/SpriteSheet.cpp RahiTuber/TextureManager.cpp
 RahiTuber/EffectManager.cpp RahiTuber/GamePad.cpp RahiTuber/file_browser_modal.cpp
 RahiTuber/imgui-sfml/imgui-SFML.cpp)
target_compile_features(MaxEditorTests PRIVATE cxx_std_17)
get_target_property(MAX_EDITOR_INCLUDES RahiTuber INCLUDE_DIRECTORIES)
get_target_property(MAX_EDITOR_DEFINES RahiTuber COMPILE_DEFINITIONS)
get_target_property(MAX_EDITOR_LIBRARIES RahiTuber LINK_LIBRARIES)
target_include_directories(MaxEditorTests PRIVATE ${MAX_EDITOR_INCLUDES})
target_compile_definitions(MaxEditorTests PRIVATE ${MAX_EDITOR_DEFINES})
target_link_libraries(MaxEditorTests PRIVATE ${MAX_EDITOR_LIBRARIES})

add_executable(MaxPlushRasterTests tests/PlushRasterTests.cpp)
target_compile_features(MaxPlushRasterTests PRIVATE cxx_std_17)

if(WIN32)
 add_executable(MaxCameraNegotiationTests tests/CameraNegotiationTests.cpp)
 target_link_libraries(MaxCameraNegotiationTests PRIVATE ole32 oleaut32 strmiids)
endif()
