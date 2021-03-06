option(USE_GME             "Build with Game Music Emulators library" ON)
if(USE_GME)
    option(USE_GME_DYNAMIC "Use dynamical loading of Game Music Emulators library" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(GME QUIET)
        message("GME: [${GME_FOUND}] ${GME_INCLUDE_DIRS} ${GME_LIBRARIES}")
        if(USE_GME_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DGME_DYNAMIC=\"${GME_DYNAMIC_LIBRARY}\")
            message("Dynamic GME: ${GME_DYNAMIC_LIBRARY}")
        endif()

        cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/gme.c
            ""
            ${GME_INCLUDE_DIRS}
            "${GME_LIBRARIES};${M_LIBRARY}"
            STDCPP_NEEDED
        )

    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBGME_LIB gme)
            set(LIBZLIB_LIB zlib)
        else()
            find_library(LIBGME_LIB NAMES gme
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBZLIB_LIB NAMES zlib
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        mark_as_advanced(LIBGME_LIB LIBZLIB_LIB)
        set(GME_LIBRARIES ${LIBGME_LIB} ${LIBZLIB_LIB})
        set(GME_FOUND 1)
        set(STDCPP_NEEDED 1) # Statically linking GME which is C++ library
        set(GME_INCLUDE_DIRS
            ${AUDIO_CODECS_INSTALL_PATH}/include/gme
            ${AUDIO_CODECS_INSTALL_PATH}/include
            ${AUDIO_CODECS_PATH}/libgme/include
            ${AUDIO_CODECS_PATH}/zlib/include
        )
    endif()

    if(GME_FOUND)
        message("== using GME ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_GME)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${GME_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_GME_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${GME_LIBRARIES})
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_gme.c)
    else()
        message("-- skipping GME --")
    endif()
endif()
