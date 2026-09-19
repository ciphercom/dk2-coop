cmake_minimum_required(VERSION 3.25)
project(dk2_coop)

set(CMAKE_CXX_STANDARD 23)

# Keep programmer invariants and assertion-based tests active in optimized builds.
# Remove the default definition instead of overriding it (which produces MSVC warnings).
foreach(language C CXX)
  foreach(configuration RELEASE RELWITHDEBINFO MINSIZEREL)
    string(REPLACE "/DNDEBUG" "" CMAKE_${language}_FLAGS_${configuration} "${CMAKE_${language}_FLAGS_${configuration}}")
  endforeach()
endforeach()

# Parallelize translation units inside large MSVC targets; build.cmd also parallelizes targets.
add_compile_options(
        "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/MP>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/MP>"
)

option(NO_VCRUNTIME "Will remove dependency on VCRUNTIME*.dll")
if(NO_VCRUNTIME)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(USE_CONFIGURE_DATE "Add configure date marker in output build")
option(DK2_DIR "Dungeon Keeper 2 directory (for comfortable development)")

message(STATUS "CMAKE_SYSTEM_VERSION: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_RC_COMPILER: ${CMAKE_RC_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

# force using python through venv
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/.venv/Scripts/python.exe)
  set(Python3_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/.venv/Scripts/python.exe")
  # test for capstone package installed
  execute_process(COMMAND ${Python3_EXECUTABLE} -c "import capstone" RESULT_VARIABLE EXIT_CODE OUTPUT_QUIET)
else ()
  # create venv
  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  execute_process(COMMAND_ECHO STDOUT COMMAND ${Python3_EXECUTABLE} -m venv ${CMAKE_CURRENT_LIST_DIR}/.venv COMMAND_ERROR_IS_FATAL ANY)
  set(Python3_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/.venv/Scripts/python.exe")
  set(EXIT_CODE 1)
endif ()
if(NOT EXIT_CODE EQUAL 0)  # not installed
  execute_process(COMMAND_ECHO STDOUT COMMAND ${Python3_EXECUTABLE} -m pip install -U pip wheel COMMAND_ERROR_IS_FATAL ANY)
  execute_process(COMMAND_ECHO STDOUT COMMAND ${Python3_EXECUTABLE} -m pip install -r ${CMAKE_CURRENT_LIST_DIR}/requirements.txt COMMAND_ERROR_IS_FATAL ANY)
endif ()
message(STATUS "Python3 found: ${Python3_EXECUTABLE}")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

find_package(Git REQUIRED)
message(STATUS "Git found: ${GIT_EXECUTABLE}")

# Visual Studio generators resolve MSVC tools without adding them to the configure process PATH.
if(MSVC)
  if(NOT EXISTS "${CMAKE_AR}")
    message(FATAL_ERROR "The MSVC library manager was not resolved by CMake: ${CMAKE_AR}")
  endif()
  set(LIB_PROGRAM "${CMAKE_AR}")
  get_filename_component(MSVC_TOOL_DIRECTORY "${CMAKE_LINKER}" DIRECTORY)
  find_program(DUMPBIN_PROGRAM NAMES dumpbin HINTS "${MSVC_TOOL_DIRECTORY}" REQUIRED)
endif()

# the commit's SHA1, and whether the building workspace was dirty or not
execute_process(COMMAND
        "${GIT_EXECUTABLE}" describe --match=NeVeRmAtCh --always --abbrev=40 --dirty
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_SHA1
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND
        "${GIT_EXECUTABLE}" describe --match=NeVeRmAtCh --always --abbrev=7 --dirty
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_SHORT_SHA1
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

# the date of the commit
execute_process(COMMAND
        "${GIT_EXECUTABLE}" log -1 --format=%ad --date=local
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_DATE
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

# the subject of the commit
execute_process(COMMAND
        "${GIT_EXECUTABLE}" log -1 --format=%s
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_SUBJECT
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "GIT_SHA1: ${GIT_SHA1}")
message(STATUS "GIT_DATE: ${GIT_DATE}")
message(STATUS "GIT_COMMIT_SUBJECT: ${GIT_COMMIT_SUBJECT}")

if(USE_CONFIGURE_DATE)
  string(TIMESTAMP CONFIGURE_DATE "%y%m%d")
endif ()

include(resources/CMakeLists.txt)

set(OUTPUT_VERSION "${COOP_VERSION}")
if(CONFIGURE_DATE)
  string(APPEND OUTPUT_VERSION "-${CONFIGURE_DATE}")
endif ()

add_subdirectory(mapping)
add_subdirectory(tools)
add_subdirectory(libs)
add_subdirectory(src)

if(DK2_DIR)
  message(STATUS "Dungeon Keeper 2 directory: ${DK2_DIR}")
  if(NOT EXISTS "${DK2_DIR}/DKII-DX.EXE")
    message(FATAL_ERROR "\"${DK2_DIR}/DKII-DX.EXE\" is not found")
  endif()
  add_custom_target(flame_dev
          DEPENDS flame dkii_dll_stub flame_loader
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:flame> "${DK2_DIR}/flame/Flame.dll"
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:dkii_dll_stub> "${DK2_DIR}/flame/DKII.dll"
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:flame_loader> "${DK2_DIR}/PATCH.dll"
          # The helper makes PDB copies conditional at build time for multi-config generators.
          COMMAND ${CMAKE_COMMAND}
          "-DBUILD_CONFIG=$<CONFIG>"
          "-DFLAME_PDB=$<TARGET_PDB_FILE:flame>"
          "-DDKII_PDB=$<TARGET_PDB_FILE:dkii_dll_stub>"
          "-DPATCH_PDB=$<TARGET_PDB_FILE:flame_loader>"
          "-DDESTINATION=${DK2_DIR}/flame"
          -P "${CMAKE_CURRENT_LIST_DIR}/cmake/copy_debug_pdbs.cmake"
          VERBATIM
  )
endif()

# Package from configured metadata so source edits cannot relabel an existing build.
set(COOP_ARTIFACT_NAME "DK2-Coop-${OUTPUT_VERSION}$<$<CONFIG:Debug>:-debug>")
file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build_metadata-$<CONFIG>.json"
        CONTENT "{\n  \"artifactName\": \"${COOP_ARTIFACT_NAME}\",\n  \"flameVersion\": \"${FLAME_VERSION}\",\n  \"coopVersion\": \"${COOP_VERSION}\"\n}\n"
)

# Generate a configuration-specific value, then expose the historical artifact_name.txt interface.
file(GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/artifact_name-$<CONFIG>.txt"
        CONTENT "${COOP_ARTIFACT_NAME}\n"
)
add_custom_target(get_artifact_name
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/artifact_name-$<CONFIG>.txt"
        "${CMAKE_CURRENT_BINARY_DIR}/artifact_name.txt"
        VERBATIM
)

set(Python_EXECUTABLE "")  # disable warning: Manually-specified variables were not used by the project
