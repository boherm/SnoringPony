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
    VERSION 1.0.0b12
    BUNDLE_ID dev.boherm.SnoringPony
    DOCUMENT_EXTENSIONS indy
    ICON_SMALL resources/icon.png
    ICON_BIG resources/icon.png
    COMPANY_NAME boherm
    COMPANY_COPYRIGHT Boris Hermans
    COMPANY_WEBSITE snoringpony.boherm.dev
    COMPANY_EMAIL boris@boherm.dev
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "SnoringPony uses the microphone for the Metering panel (level meter and spectrogram)."
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

# ---- RTL-SDR (optional, vendored under external/rtlsdr) -----------------------
# If the prebuilt librtlsdr (+ libusb) is present for the current platform, link it
# and enable hardware scanning (SP_HAS_RTLSDR=1). Otherwise the app still builds and
# runs with the simulated source only. See external/rtlsdr/README.md for the layout.
set(RTLSDR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/rtlsdr")
set(RTLSDR_FOUND FALSE)

if (WIN32)
    set(RTLSDR_LIBDIR "${RTLSDR_ROOT}/lib/win/x64")
    # MSVC links against the import library (.lib). With only a DLL, generate the
    # import lib from it (see external/rtlsdr/README.md). libusb is usually bundled
    # into rtlsdr.dll on the osmocom Windows builds; add libusb-1.0 here if separate.
    set(RTLSDR_LIBS rtlsdr)
    if (EXISTS "${RTLSDR_LIBDIR}/rtlsdr.lib")
        set(RTLSDR_FOUND TRUE)
    endif()
elseif (APPLE)
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(RTLSDR_LIBDIR "${RTLSDR_ROOT}/lib/osx/arm64")
    else()
        set(RTLSDR_LIBDIR "${RTLSDR_ROOT}/lib/osx/x86_64")
    endif()
    # Link rtlsdr only; it pulls in libusb transitively (libusb just needs to be
    # loadable at runtime, via its recorded dependency path / rpath).
    set(RTLSDR_LIBS rtlsdr)
    if (EXISTS "${RTLSDR_LIBDIR}/librtlsdr.dylib" OR EXISTS "${RTLSDR_LIBDIR}/librtlsdr.a")
        set(RTLSDR_FOUND TRUE)
    endif()
elseif (UNIX)
    set(RTLSDR_LIBDIR "${RTLSDR_ROOT}/lib/linux/x86_64")
    # A static librtlsdr.a does not contain libusb, so link the system libusb plus
    # pthread/m. (Harmless too when a shared librtlsdr.so is used instead.)
    set(RTLSDR_LIBS rtlsdr usb-1.0 pthread m)
    if (EXISTS "${RTLSDR_LIBDIR}/librtlsdr.so" OR EXISTS "${RTLSDR_LIBDIR}/librtlsdr.a")
        set(RTLSDR_FOUND TRUE)
    endif()
endif()

if (RTLSDR_FOUND)
    message(STATUS "RTL-SDR: found in ${RTLSDR_LIBDIR} -> hardware scanning enabled")
    target_include_directories("${PROJECT_NAME}" PRIVATE "${RTLSDR_ROOT}/include")
    target_link_directories("${PROJECT_NAME}" PRIVATE "${RTLSDR_LIBDIR}")
    target_link_libraries("${PROJECT_NAME}" PRIVATE ${RTLSDR_LIBS})
    target_compile_definitions("${PROJECT_NAME}" PRIVATE SP_HAS_RTLSDR=1)

    # Let the built app find the vendored dylibs at runtime (@rpath install names
    # resolve to the vendored folder). For distribution, bundle them into the .app
    # and add @loader_path-based rpaths instead.
    if (APPLE)
        set_target_properties("${PROJECT_NAME}" PROPERTIES BUILD_RPATH "${RTLSDR_LIBDIR}")
    elseif (UNIX)
        set_target_properties("${PROJECT_NAME}" PROPERTIES BUILD_RPATH "${RTLSDR_LIBDIR}")
    elseif (WIN32)
        # The DLL must sit next to the .exe at runtime; copy any DLLs we vendored.
        file(GLOB RTLSDR_DLLS "${RTLSDR_LIBDIR}/*.dll")
        foreach (dll ${RTLSDR_DLLS})
            add_custom_command(TARGET "${PROJECT_NAME}" POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${dll}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endforeach()
    endif()
else()
    message(STATUS "RTL-SDR: not vendored -> simulated source only (SP_HAS_RTLSDR=0)")
    target_compile_definitions("${PROJECT_NAME}" PRIVATE SP_HAS_RTLSDR=0)
endif()
