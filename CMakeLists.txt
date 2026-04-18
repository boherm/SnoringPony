cmake_minimum_required(VERSION 3.24)
project(SnoringPony VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    add_compile_options(/W0)
else()
    add_compile_options(-w)
endif()


add_subdirectory(modules/juce)

juce_add_gui_app("${PROJECT_NAME}"
    PRODUCT_NAME ${PROJECT_NAME}
    VERSION 1.0.0b4
    BUNDLE_ID dev.boherm.SnoringPony
    DOCUMENT_EXTENSIONS indy
    ICON_SMALL resources/icon.png
    ICON_BIG resources/icon.png
    COMPANY_NAME boherm
    COMPANY_COPYRIGHT Boris Hermans
    COMPANY_WEBSITE snoringpony.boherm.dev
    COMPANY_EMAIL boris@boherm.dev
    NEEDS_WEB_BROWSER TRUE
    NEEDS_CURL FALSE
    PLIST_TO_MERGE              [[<plist>
                                    <dict>
                                      <key>NSAppTransportSecurity</key>
	                                  <dict>
		                                <key>NSAllowsArbitraryLoads</key>
		                                <true/>
	                                  </dict>
                                    </dict>
                                  </plist>]])

target_compile_features("${PROJECT_NAME}"  PRIVATE cxx_std_17)


file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
target_sources("${PROJECT_NAME}" PRIVATE ${SourceFiles})

target_compile_definitions("${PROJECT_NAME}"  PRIVATE
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:${PROJECT_NAME},JUCE_VERSION>"
    JUCE_ALLOW_SPECIAL_CHARS_IN_ADDRESS=1
    JUCE_ENABLE_BROADCAST_BY_DEFAULT=1
    JUCE_EXCLUSIVE_BINDING_BY_DEFAULT=1
    JUCE_IP_AND_PORT_DETECTION=1
    ORGANICUI_USE_WEBSERVER=1
    ORGANICUI_USE_SERVUS=1
)

file(GLOB_RECURSE ResourcesFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.png" "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ponylayout")
juce_add_binary_data(BinaryData SOURCES ${ResourcesFiles})

juce_add_module(modules/juce_organicui)
juce_add_module(modules/juce_simpleweb)

juce_generate_juce_header("${PROJECT_NAME}")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/servus/include/)

if (WIN32)
    set(OPENSSL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_simpleweb/libs/VisualStudio2022/x64/MD/")
    set(WAPI_LIB "${CMAKE_CURRENT_SOURCE_DIR}/external/wapi/win/libwapi.a")
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(SERVUS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/servus/lib/win/x64/debug/")
    else()
        set(SERVUS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/servus/lib/win/x64/release/")
    endif()
elseif (APPLE)
    set(WAPI_LIB "${CMAKE_CURRENT_SOURCE_DIR}/external/wapi/osx/libwapi.a")
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(CMAKE_OSX_ARCHITECTURES "arm64")
        set(OPENSSL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_simpleweb/libs/MacOSX/arm64/")
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(SERVUS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/servus/lib/silicon/debug/")
        else()
            set(SERVUS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/servus/lib/silicon/release/")
        endif()
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
        set(OPENSSL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_simpleweb/libs/MacOSX/x86_64/")
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(SERVUS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/servus/lib/osx/debug/")
        else()
            set(SERVUS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/servus/lib/osx/release/")
        endif()
    endif()
elseif (UNIX)
    set(OPENSSL_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/juce_simpleweb/libs/Linux/x86_64/")
    set(SERVUS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/servus/lib/linux/")
    set(WAPI_LIB "${CMAKE_CURRENT_SOURCE_DIR}/external/wapi/linux/libwapi.a")
endif()

target_link_directories(${PROJECT_NAME} PRIVATE
    ${OPENSSL_LIB_DIR}
    ${SERVUS_LIB_DIR}
)

target_link_libraries("${PROJECT_NAME}"
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_cryptography
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
        juce::juce_osc
        BinaryData
        juce_organicui
        juce_simpleweb
        Servus
        ${WAPI_LIB}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
