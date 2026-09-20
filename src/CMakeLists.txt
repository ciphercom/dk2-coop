set(TARGET flame)

set(RESOURCES_FILE ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
set(WELCOME__HEADER ${CMAKE_CURRENT_LIST_DIR}/patches/welcome_window/resources/resources.h)
set(WELCOME__MAIN_BACKGROUND ${CMAKE_CURRENT_LIST_DIR}/patches/welcome_window/resources/main_background.png)
set(VER_PRODUCT_NAME "DK2 Co-op")
set(VER_DESCRIPTION "DK2 Co-op, based on Flame v${FLAME_VERSION}")
configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/resources.rc.in
        ${RESOURCES_FILE}
        @ONLY
)

function(collect_sources OUT PREFIX)
    include(${PREFIX}/sources.cmake)
    list(TRANSFORM SOURCES PREPEND ${PREFIX}/)
    set(${OUT} ${SOURCES} PARENT_SCOPE)
endfunction()

collect_sources(MAIN_LAYOUT_SOURCES dk2/gui/main)
collect_sources(GAME_LAYOUT_SOURCES dk2/gui/game)
collect_sources(GOG_PATCH_SOURCES gog_patch_dll)
collect_sources(WEANETR_SOURCES weanetr_dll)

add_library(${TARGET} SHARED
        main.cpp
        diagnostic/bridge.cpp
        diagnostic/pipe.cpp
        diagnostic/game_bridge.cpp
        diagnostic/graphics_lifecycle.cpp
        patches/test_availability.cpp
        patches/scripted_camera_trigger.cpp
        patches/scripted_camera_hooks.cpp
        patches/network_possession.cpp
        patches/health_flower.cpp
        patches/network_hands.cpp
        patches/network_gem_ending.cpp
        patches/network_gem_ending_ui.cpp
        dkii_exe_functions.cpp
        dk2/MyWindow.cpp
        dk2/MyHeap.cpp
        dk2/MyDxMouse.cpp
        dk2/MyDxKeyboard.cpp
        dk2/CDefaultPlayerInterface.cpp
        dk2/CPCEngineInterface.cpp
        dk2/MyGameSession.cpp
        dk2/PlayerCfg.cpp
        dk2/RegKey.cpp
        dk2/MyComputerPlayer.cpp
        dk2/CRoom.cpp
        dk2/CBridge.cpp
        dk2/CCamera.cpp
        dk2/CWorld.cpp
        dk2/WeaNetR.cpp
        dk2/dk2_memory.cpp
        dk2/dk2_command_line.cpp
        dk2/CNetworkCommunication.cpp
        dk2/MyComputerPlayer_drop_condition.cpp
        dk2/computer_player_flags.cpp
        dk2/MyCreatureCollection.cpp
        dk2/MyComputerPlayer_drop_condition.cpp
        dk2/MyComputerPlayer_drop_condition.h
        dk2/computer_player_flags.cpp
        dk2/computer_player_flags.h

        dk2/components/CFrontEndComponent.cpp
        dk2/components/CGameComponent.cpp
        dk2/components/CEntryComponent.cpp
        dk2/components/CNetworkComponent.cpp
        dk2/components/CComponent.cpp

        dk2/math/int_float.cpp

        dk2/button/button_types.cpp
        dk2/button/CTextInput.cpp
        dk2/button/button_functions.cpp
        dk2/button/CClickButton.cpp

        dk2/entities/entities_type.cpp
        dk2/entities/CCreature.cpp
        dk2/entities/CCreatureExtended.cpp
        dk2/entities/CPlayer.cpp

        dk2/engine/draw_functions.cpp
        dk2/engine/ddraw_functions.cpp
        dk2/engine/window_functions.cpp
        dk2/engine/WinCls_BullfrogLibScreen.cpp
        dk2/engine/CWindowTest.cpp
        dk2/engine/WinEventHandlers.cpp
        dk2/engine/InputCursor.cpp
        dk2/engine/DdModeList.cpp

        dk2/engine/scene/scene.cpp
        dk2/engine/scene/surface.cpp
        dk2/engine/scene/MyCESurfHandle.cpp
        dk2/engine/scene/MyScaledSurface.cpp
        dk2/engine/scene/SurfaceHolder.cpp
        dk2/engine/scene/SurfHashList.cpp
        dk2/engine/scene/SurfHashList2.cpp
        dk2/engine/scene/CEngineDDSurface.cpp
        dk2/engine/scene/MyDirectDraw.cpp

        dk2/engine/primitives/CEngine2DSprite.cpp
        dk2/engine/primitives/CEngine2DRotatableSprite.cpp

        dk2/surface/surface_functions.cpp

        dk2/network/protocol.cpp

        dk2/resources/filesystem.cpp
        dk2/resources/MyResources.cpp
        dk2/resources/dir/MyDir_CFileManager.cpp

        ${MAIN_LAYOUT_SOURCES}
        ${GAME_LAYOUT_SOURCES}

        dk2/gui/main_gui.cpp
        dk2/gui/CDefaultPlayerInterface_game_gui.cpp
        dk2/gui/CGuiManager.cpp
        dk2/gui/button_render.cpp

        tools/StackLimits.cpp
        tools/instance_mutex_name.cpp
        tools/LoadedModules.cpp
        tools/win32_gui_layout.cpp
        tools/ExceptionWindow.cpp
        tools/command_line.cpp
        tools/console.cpp
        tools/last_error.cpp
        tools/flame_config.cpp
        tools/visual_debug.cpp

        tools/bug_hunter.cpp
        tools/bug_hunter/StackWalker.cpp
        tools/bug_hunter/StackWalkerState.cpp
        tools/bug_hunter/StackWalkerIter.cpp
        tools/bug_hunter/WalkerError.cpp
        tools/bug_hunter/StackFrame.cpp
        tools/bug_hunter/MyVersionInfo.cpp

        patches/flame_main.cpp
        patches/replace_mouse_dinput_to_user32.cpp
        patches/use_wheel_to_zoom.cpp
        patches/micro_patches.cpp
        patches/game_version_patch.cpp
        patches/logging.cpp
        patches/inspect_tools.cpp
        patches/original_compatible.cpp
        patches/protocol_dump.cpp
        patches/registry_to_config.cpp
        patches/limit_tps.cpp
        patches/coop_campaign.cpp
        patches/coop_campaign_route.cpp
        patches/coop_campaign_progress.cpp
        patches/coop_campaign_init.cpp
        patches/auto_network.cpp
        patches/scheduler.cpp
        patches/replace_heap.cpp
        patches/remember_window_location_and_size.cpp
        patches/wine_support.cpp
        patches/show_wireframe.cpp
        patches/alternative_resources.cpp

        patches/big_resolution_fix/expand_surf_idx_array.cpp
        patches/big_resolution_fix/big_resolution_fix.cpp
        patches/big_resolution_fix/screen_resolution.cpp

        patches/gui/main/coop_campaign_menu.cpp
        patches/gui/main/main_layout.cpp
        patches/gui/main/single_player/btn_custom_campaign.cpp
        patches/gui/main/single_player/win_custom_campaign.cpp

        patches/gui/game/game_layout.cpp
        patches/gui/game/esc_options/btn_autosave.cpp

        patches/welcome_window/main/imgui_main_dx9.cpp
        patches/welcome_window/main/imgui_texture_dx9.cpp

#        patches/welcome_window/main/imgui_main_opengl3.cpp
#        patches/welcome_window/main/imgui_texture_opengl3.cpp

        patches/welcome_window/welcome_window.cpp
        patches/welcome_window/options_section.cpp
        patches/welcome_window/MyTimer.cpp
        patches/welcome_window/alt_res_section.cpp

        ${GOG_PATCH_SOURCES}
        ${WEANETR_SOURCES}

        ${RESOURCES_FILE}
        )
target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_LIST_DIR} gog_patch_dll)
target_compile_definitions(${TARGET} PRIVATE
        DIRECTINPUT_VERSION=0x0500
        DIRECT3D_VERSION=0x0600
        NOMINMAX=1
        )
set_source_files_properties(
        patches/welcome_window/main/imgui_main_dx9.cpp
        patches/welcome_window/main/imgui_texture_dx9.cpp
        PROPERTIES COMPILE_DEFINITIONS
        DIRECT3D_VERSION=0x0900
)
target_link_libraries(${TARGET} PRIVATE
        dkii_genlib dkii_exe_api
#        gog_patch_dll

        # dk2 specific libs
        qmixer_genlib  # generated in qmixer_dll/genlib
        weanetr_genlib  # generated in weanetr_dll/genlib
        dplay_headers

        # the others libs dk2 depends on
        winmm
        dinput_genlib  # generated in dinput_dll/genlib
        ddraw  # patch - dont link against gog's PATCH.dll because patch addresses are change after recompilation
        dxguid

        # 3rdparty
        cimg  # visual debug
        toml  # flame config
        lodepng  # load/save png
        imgui D3DX9  # welcome window
#        imgui opengl32 stb  # welcome window

        imm32
        advapi32
        # wsock32 is old link target
        ws2_32 # should be better
        dsound
        Version
        Dbghelp
        )

target_compile_options(${TARGET} PRIVATE
        /Gy  # Enable Function-Level Linking
        /Gw  # Optimize Global Data
#        /GL  # Whole program optimization
        /GS-  # Buffer Security Check
        /Gz  # use __stdcall by default
        /wd4624  # Suppress warning destructor was implicitly defined as deleted
        /Zc:threadSafeInit-
#        /Oy  # Frame-Pointer Omission
        /Zi  # Produces a separate PDB file that contains all the symbolic debugging information
)
target_link_options(${TARGET} PRIVATE /debug)  # generate pdb to extract FPO and mapping with release builds
target_link_options(${TARGET} PRIVATE /MAP)
target_link_options(${TARGET} PRIVATE /OPT:NOREF)
target_link_options(${TARGET} PRIVATE /DEF:${CMAKE_HOME_DIRECTORY}/libs/dkii_exe/genlib/dkii_exp.def)
set(OUTPUT_NAME "Flame")
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "${OUTPUT_NAME}")


install(FILES $<TARGET_FILE:${TARGET}> DESTINATION "flame")
target_link_options(${TARGET} PRIVATE
        "$<$<CONFIG:Debug>:/PDBALTPATH:${OUTPUT_NAME}.pdb>"
        "$<$<CONFIG:Debug>:/ILK:${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}.ilk>"  # dont put ilk to bin dir
)
install(FILES $<TARGET_PDB_FILE:${TARGET}> DESTINATION "flame" CONFIGURATIONS Debug)
set_target_properties(${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/flame")

add_executable(instance_mutex_name_tests
        tools/instance_mutex_name.cpp
        tools/instance_mutex_name_tests.cpp
)

add_executable(scripted_camera_timeline_tests patches/scripted_camera_timeline_tests.cpp)
add_executable(network_possession_tests patches/network_possession_tests.cpp)
add_executable(command_line_tests tools/command_line_tests.cpp tools/command_line.cpp)
add_executable(session_discovery_tests weanetr_dll/session_discovery_tests.cpp)
target_link_libraries(session_discovery_tests PRIVATE dkii_exe_api)
target_include_directories(session_discovery_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(session_discovery_tests PRIVATE DIRECTINPUT_VERSION=0x0500 DIRECT3D_VERSION=0x0600)
add_executable(health_flower_tests patches/health_flower_tests.cpp)
add_executable(network_hands_tests patches/network_hands_tests.cpp)
add_executable(network_gem_ending_tests patches/network_gem_ending_tests.cpp)
add_executable(network_gem_ending_ui_tests patches/network_gem_ending_ui_tests.cpp)
add_executable(input_cursor_tests dk2/engine/InputCursor_tests.cpp dk2/engine/InputCursor.cpp)
target_link_libraries(input_cursor_tests PRIVATE dkii_exe_api)
target_compile_definitions(input_cursor_tests PRIVATE DIRECTINPUT_VERSION=0x0500 DIRECT3D_VERSION=0x0600)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../dev")
    include(../dev/dev.cmake)
endif ()


add_custom_target(update_replace_globals
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_HOME_DIRECTORY}/mapping/ida
        ${Python3_EXECUTABLE} ${CMAKE_HOME_DIRECTORY}/tools/genapi/main_update_replace_globals.py
        -sgmap_file ${CMAKE_HOME_DIRECTORY}/mapping/DKII_EXE_v170.sgmap
        -replace_globals ${CMAKE_HOME_DIRECTORY}/src/replace_globals.txt
        DEPENDS
        ${CMAKE_HOME_DIRECTORY}/mapping/DKII_EXE_v170.sgmap
        ${CMAKE_HOME_DIRECTORY}/src/replace_globals.txt
        COMMENT "update replace_globals.txt"
)

add_executable(coop_campaign_tests patches/coop_campaign_tests.cpp patches/coop_campaign_route.cpp)
target_link_libraries(coop_campaign_tests PRIVATE dkii_exe_api)
target_compile_definitions(coop_campaign_tests PRIVATE DIRECTINPUT_VERSION=0x0500 DIRECT3D_VERSION=0x0600)

add_executable(coop_campaign_progress_tests patches/coop_campaign_progress_tests.cpp)

add_executable(coop_campaign_init_tests patches/coop_campaign_init_tests.cpp)

add_executable(shared_dig_overlay_tests patches/shared_dig_overlay_tests.cpp)
target_link_libraries(shared_dig_overlay_tests PRIVATE dkii_exe_api)
target_compile_definitions(shared_dig_overlay_tests PRIVATE DIRECTINPUT_VERSION=0x0500 DIRECT3D_VERSION=0x0600)

# The synthetic host tests the same local transport without starting DK2.
add_executable(diagnostic_bridge_tests diagnostic/bridge_tests.cpp diagnostic/bridge.cpp)
add_executable(diagnostic_bridge_test_host diagnostic/bridge_test_host.cpp diagnostic/bridge.cpp diagnostic/pipe.cpp)
target_link_libraries(diagnostic_bridge_test_host PRIVATE advapi32)
target_compile_definitions(diagnostic_bridge_test_host PRIVATE NOMINMAX=1)
add_executable(test_availability_tests patches/test_availability_tests.cpp patches/test_availability.cpp)
target_link_libraries(test_availability_tests PRIVATE dkii_exe_api)
target_compile_definitions(test_availability_tests PRIVATE DIRECTINPUT_VERSION=0x0500 DIRECT3D_VERSION=0x0600)
