# - try to find MITK library and include files
#  MITK_INCLUDE_DIR, where to find mitkObject.h, etc.
#  MITK_LIBRARY_DIR, where to find MITK library files
#  MITK_LIBRARIES, the libraries to link against
#  MITK_FOUND, If false, do not try to use MITK.

IF(NOT MITK_DIR)
  SET(MITK_DIR "MITK" CACHE PATH "Path to MITK header and library files") 
ELSE(NOT MITK_DIR)
  SET(MITK_INCLUDE_DIR ${MITK_DIR}/Include)
  SET(MITK_LIBRARY_DIR ${MITK_DIR}/Lib)
  IF(WIN32)
    FIND_LIBRARY(MITK_mitk_LIBRARY NAMES Mitk_dll PATHS ${MITK_LIBRARY_DIR})
  ELSE (WIN32)
    FIND_LIBRARY(MITK_mitk_LIBRARY NAMES mitk Mitk MITK PATHS ${MITK_LIBRARY_DIR})
  ENDIF (WIN32)
ENDIF(NOT MITK_DIR)

SET(MITK_FOUND "NO")
IF(MITK_INCLUDE_DIR AND MITK_LIBRARY_DIR)
  IF(MITK_mitk_LIBRARY)
    SET(MITK_LIBRARIES ${MITK_mitk_LIBRARY})
    SET(MITK_FOUND "YES")
    
    #The following deprecated settings are for backwards compatibility with CMake1.4
    SET(MITK_LIBRARY ${MITK_LIBRARIES})
    SET(MITK_INCLUDE_PATH ${MITK_INCLUDE_DIR})
    
  ENDIF(MITK_mitk_LIBRARY)
ENDIF(MITK_INCLUDE_DIR AND MITK_LIBRARY_DIR)

MARK_AS_ADVANCED(
  MITK_INCLUDE_DIR
  MITK_LIBRARY_DIR
  MITK_LIBRARIES
  )
