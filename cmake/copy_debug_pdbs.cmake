#[[
Copy the three development PDBs only for Debug builds. The build configuration is
known only when a multi-configuration generator executes the flame_dev target.
]]
if(NOT BUILD_CONFIG STREQUAL "Debug")
  return()
endif()

foreach(PDB_MAPPING IN ITEMS
        "${FLAME_PDB};${DESTINATION}/Flame.pdb"
        "${DKII_PDB};${DESTINATION}/DKII.pdb"
        "${PATCH_PDB};${DESTINATION}/PATCH.pdb")
  list(GET PDB_MAPPING 0 SOURCE)
  list(GET PDB_MAPPING 1 TARGET)
  file(COPY_FILE "${SOURCE}" "${TARGET}" ONLY_IF_DIFFERENT)
endforeach()
