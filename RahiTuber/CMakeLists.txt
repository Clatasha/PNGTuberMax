# Modified by Clatasha for PNGTuber Max (2026-10-11).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(RahiTuber)
set_target_properties(RahiTuber PROPERTIES OUTPUT_NAME "PNGTuberMax")
if(WIN32)
	target_link_options(RahiTuber PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
endif()


target_include_directories(RahiTuber PRIVATE
    ./
    ./imgui-sfml
    ${CMAKE_SOURCE_DIR}/Libraries/freetype/include
    ${CMAKE_SOURCE_DIR}/Libraries/imgui
    ${CMAKE_SOURCE_DIR}/Libraries/mongoose
    ${CMAKE_SOURCE_DIR}/Libraries/portaudio/include
    ${CMAKE_SOURCE_DIR}/Libraries/SFML/include
    ${CMAKE_SOURCE_DIR}/Libraries/tinyxml2
    ${CMAKE_SOURCE_DIR}/Libraries/Simple-FFT/include
)

if(WIN32)
    target_include_directories(RahiTuber PRIVATE ${CMAKE_SOURCE_DIR}/Libraries/Spout2/SPOUTSDK)
endif()




target_sources(RahiTuber PRIVATE
    imgui-sfml/GL/glext.h
    imgui-sfml/KHR/khrplatform.h
    imgui-sfml/imconfig-SFML.h
    imgui-sfml/imgui-SFML.cpp
    imgui-sfml/imgui-SFML.h
    imgui-sfml/imgui-SFML_export.h
    Config.h
    defines.h
    file_browser_modal.cpp
    file_browser_modal.h
    LayerManager.cpp
    LayerManager.h
    EffectManager.cpp
    EffectManager.h
    main.cpp
    SpriteSheet.cpp
    SpriteSheet.h
    TextureManager.cpp
    TextureManager.h
    xmlConfig.cpp
    xmlConfig.h
    websocket.h
    MainEngine.h
    MaxUi.h
    MaxCamera.h
    CameraFrame.h
    GamePad.h
    GamePad.cpp
    ffwdClock.h
)

if(WIN32)
    target_sources(RahiTuber PRIVATE
        resource.h
        RahiTuber.rc
    )
endif()

if(NOT MSVC)
    target_compile_options(RahiTuber PRIVATE -Wl,-rpath./lib/)
endif()

if(MSVC)
    target_compile_definitions(RahiTuber PRIVATE
        -DUNICODE -D_UNICODE
    )
	
	set_target_properties( RahiTuber PROPERTIES OUTPUT_NAME "PNGTuberMax")

    target_compile_definitions(RahiTuber PRIVATE
        IMGUI_DISABLE_WIN32_FUNCTIONS
        SFML_STATIC
        NOMINMAX
        _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
        __USE_SQUARE_BRACKETS_FOR_ELEMENT_ACCESS_OPERATOR
    )
else()
    target_compile_definitions(RahiTuber PRIVATE
        __USE_SQUARE_BRACKETS_FOR_ELEMENT_ACCESS_OPERATOR
    )
endif()

target_link_libraries(RahiTuber PRIVATE ${OPENGL_LIBRARY}
    freetype
    mongoose
    imgui
    portaudio_static
    tinyxml2
    sfml-graphics
    sfml-window
    sfml-system
)

if(WIN32)
    target_link_libraries(RahiTuber PRIVATE Spout_static)
    target_link_libraries(RahiTuber PRIVATE Shcore Xinput hid)
	

endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(X11_FOUND)
        target_link_libraries(RahiTuber PRIVATE ${X11_LIBRARIES})
    endif()

     if(ALSA_FOUND)
        target_link_libraries(RahiTuber PRIVATE ${ALSA_LIBRARIES})
     endif()
 endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    cmake_policy(SET CMP0095 OLD)
    set_target_properties(RahiTuber
        PROPERTIES
            INSTALL_RPATH "\\\$ORIGIN/lib"
    )
endif()


# Stage resources alongside the executable; no symlinks or elevated build step.
add_custom_command(TARGET RahiTuber POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:RahiTuber>/res"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/config.xml" "$<TARGET_FILE_DIR:RahiTuber>/config.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/buildnumber.txt" "$<TARGET_FILE_DIR:RahiTuber>/buildnumber.txt"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/LICENSE.txt" "$<TARGET_FILE_DIR:RahiTuber>/LICENSE.txt"
)
