#####################################################
# Copy dependencies on Windows
#####################################################

function(install_external_dll DEBUG_FILEPATH RELEASE_FILEPATH DEBUG_FILENAME RELEASE_FILENAME)
  if(EXISTS ${DEBUG_FILEPATH}/${DEBUG_FILENAME}.dll)
    configure_file(${DEBUG_FILEPATH}/${DEBUG_FILENAME}.dll ${PROJECT_BINARY_DIR}/Debug/${DEBUG_FILENAME}.dll COPYONLY)
    message(STATUS "Copy DLL: " ${DEBUG_FILEPATH}/${DEBUG_FILENAME}.dll)
  endif(EXISTS ${DEBUG_FILEPATH}/${DEBUG_FILENAME}.dll)
  if(EXISTS ${RELEASE_FILEPATH}/${RELEASE_FILENAME}.dll)
    configure_file(${RELEASE_FILEPATH}/${RELEASE_FILENAME}.dll ${PROJECT_BINARY_DIR}/Release/${RELEASE_FILENAME}.dll COPYONLY)
    configure_file(${RELEASE_FILEPATH}/${RELEASE_FILENAME}.dll ${PROJECT_BINARY_DIR}/RelWithDebInfo/${RELEASE_FILENAME}.dll COPYONLY)
    configure_file(${RELEASE_FILEPATH}/${RELEASE_FILENAME}.dll ${PROJECT_BINARY_DIR}/MinSizeRel/${RELEASE_FILENAME}.dll COPYONLY)
    message(STATUS "Copy DLL: " ${RELEASE_FILEPATH}/${RELEASE_FILENAME}.dll)
    install(FILES ${RELEASE_FILEPATH}/${RELEASE_FILENAME}.dll DESTINATION . CONFIGURATIONS Release)
  endif(EXISTS ${RELEASE_FILEPATH}/${RELEASE_FILENAME}.dll)
endfunction()

function(install_dependent_dlls DEP_DLL_DBG DEP_DLL_REL)
  get_filename_component(DEP_DLL_PATH_DBG ${DEP_DLL_DBG} DIRECTORY)
  get_filename_component(DEP_DLL_NAME_DBG ${DEP_DLL_DBG} NAME_WLE)
  get_filename_component(DEP_DLL_PATH_REL ${DEP_DLL_REL} DIRECTORY)
  get_filename_component(DEP_DLL_NAME_REL ${DEP_DLL_REL} NAME_WLE)
  install_external_dll(${DEP_DLL_PATH_DBG} ${DEP_DLL_PATH_REL} ${DEP_DLL_NAME_DBG} ${DEP_DLL_NAME_REL})
endfunction()

function(install_dependency_from_target DEP_TARGET)
  get_target_property(DEP_DLL_DBG ${DEP_TARGET} IMPORTED_LOCATION_DEBUG)
  get_target_property(DEP_DLL_REL ${DEP_TARGET} IMPORTED_LOCATION_RELEASE)
  install_dependent_dlls(${DEP_DLL_DBG} ${DEP_DLL_REL})
endfunction()
