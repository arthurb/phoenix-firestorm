# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()

# ND: To streamline arguments passed, switch from OPENAL to USE_OPENAL
# To not break all old build scripts convert old arguments but warn about it
if(OPENAL)
  message( WARNING "Use of the OPENAL argument is deprecated, please switch to USE_OPENAL")
  set(USE_OPENAL ${OPENAL})
endif()

if (USE_OPENAL)
  create_target( ll::openal )
  set_target_include_dirs( ll::openal "${LIBS_PREBUILT_DIR}/include/AL")
  target_compile_definitions( ll::openal INTERFACE LL_OPENAL=1)
  use_prebuilt_binary(openal)

  if(WINDOWS)
    set_target_libraries( ll::openal
            OpenAL32
            alut
            )
  elseif(LINUX)
    set_target_libraries( ll::openal
            openal
            alut
            )
  else()
    message(FATAL_ERROR "OpenAL is not available for this platform")
  endif()
endif ()
