if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # Enable additional warnings.
  # add_definitions(-Weffc++) 
  # add_definitions(-Winline.) 
  add_definitions(-Wall -Wextra -Wshadow -Winvalid-pch)
  add_definitions(-Wctor-dtor-privacy -Wold-style-cast -Woverloaded-virtual)

  # Turn off all pragma warnings.
  add_definitions(-Wno-pragmas -Wno-unknown-pragmas)

  add_definitions(-march=native -ffast-math)
  #add_definitions(-std=c++11)

  if(CMAKE_BUILD_TYPE STREQUAL ${Debug})
    # Debugging symbols
    add_definitions(-g3)
    add_definitions(-O0)
    add_definitions(-D _DEBUG)

  elseif(CMAKE_BUILD_TYPE STREQUAL ${Release})
    add_definitions(-O3)
    add_definitions(-flto=2 -fwhole-program)
    set(CMAKE_EXE_LINKER_FLAGS "-flto=2 -fwhole-program -fuse-linker-plugin")

  elseif(CMAKE_BUILD_TYPE STREQUAL ${RelWithDebInfo})
    add_definitions(-g3)
    add_definitions(-O3)

  elseif(CMAKE_BUILD_TYPE STREQUAL ${Profile})
    add_definitions(-pg)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    add_definitions(-g)
    add_definitions(--coverage)
    add_definitions(-O3)
  endif()

  if(PLATFORM_TARGET STREQUAL ${x86})
    add_definitions(-m32)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
    set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -m32")
  endif()

else()

  message( FATAL_ERROR "Unsupported compiler '${CMAKE_CXX_COMPILER_ID}', CMake will exit." )

endif()
