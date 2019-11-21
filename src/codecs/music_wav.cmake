option(USE_WAV "Build with WAV codec" ON)
if(USE_WAV)
    add_definitions(-DMUSIC_WAV)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/load_aiff.c
        ${CMAKE_CURRENT_LIST_DIR}/load_voc.c
        ${CMAKE_CURRENT_LIST_DIR}/music_wav.c)
endif()