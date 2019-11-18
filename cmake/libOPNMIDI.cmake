option(USE_MIDI_OPNMIDI    "Build with libOPNMIDI OPN2 Emulator based MIDI sequencer support" ON)
if(USE_MIDI_OPNMIDI)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(OPNMIDI REQUIRED)
        message("OPNMIDI: [${OPNMIDI_FOUND}] ${OPNMIDI_INCLUDE_DIRS} ${OPNMIDI_LIBRARIES}")
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(OPNMIDI_LIBRARIES OPNMIDI)
        else()
            find_library(OPNMIDI_LIBRARIES NAMES OPNMIDI HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        if(OPNMIDI_LIBRARIES)
            set(OPNMIDI_FOUND 1)
        endif()
        set(OPNMIDI_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libOPNMIDI/include")
    endif()

    if(OPNMIDI_FOUND)
        message("== using OPNMIDI ==")
        add_definitions(-DMUSIC_MID_OPNMIDI)
        set(LIBMATH_NEEDED 1)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${OPNMIDI_INCLUDE_DIRS})
        list(APPEND SDLMixerX_LINK_LIBS ${OPNMIDI_LIBRARIES})
    else()
        message("== skipping OPNMIDI ==")
    endif()
endif()
