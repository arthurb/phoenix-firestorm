# -*- cmake -*-

include(FindPkgConfig)
pkg_check_modules(NGHTTP2 REQUIRED libnghttp2)

if (FALSE)
FIND_PATH(NGHTTP2_INCLUDE_DIR nghttp2/nghttp2.h)


FIND_LIBRARY(NGHTTP2_LIBRARIES
  NAMES libnghttp2
  PATHS /usr/lib /usr/local/lib
  )

IF (NGHTTP2_LIBRARY AND NGHTTP2_INCLUDE_DIR)
    SET(NGHTTP2_LIBRARIES ${NGHTTP2_LIBRARY})
    SET(NGHTTP2_FOUND "YES")
ELSE (NGHTTP2_LIBRARY AND NGHTTP2_INCLUDE_DIR)
  SET(NGHTTP2_FOUND "NO")
ENDIF (NGHTTP2_LIBRARY AND NGHTTP2_INCLUDE_DIR)


IF (NGHTTP2_FOUND)
   IF (NOT NGHTTP2_FIND_QUIETLY)
      MESSAGE(STATUS "Found JSONCpp: ${NGHTTP2_LIBRARIES}")
   ENDIF (NOT NGHTTP2_FIND_QUIETLY)
ELSE (NGHTTP2_FOUND)
   IF (NGHTTP2_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find JSONCpp library")
   ENDIF (NGHTTP2_FIND_REQUIRED)
ENDIF (NGHTTP2_FOUND)

# Deprecated declarations.
SET (NATIVE_NGHTTP2_INCLUDE_PATH ${NGHTTP2_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_NGHTTP2_LIB_PATH ${NGHTTP2_LIBRARY} PATH)

MARK_AS_ADVANCED(
  NGHTTP2_LIBRARY
  NGHTTP2_INCLUDE_DIR
  )
endif()