# libpthread
find_package (Threads REQUIRED)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

# libjemalloc
find_package(JeMalloc)
include_directories(${JEMALLOC_INCLUDE_DIR})
set(LIBS ${LIBS} ${JEMALLOC_LIBRARY})

# boost
add_definitions(-DBOOST_LOG_DYN_LINK=1)
find_package( Boost 1.54 COMPONENTS date_time program_options system regex thread iostreams filesystem log log_setup iostreams REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )
set(LIBS ${LIBS} ${Boost_LIBRARIES})

# jsoncpp
find_package(JsonCpp 0.10.2 REQUIRED)
include_directories(${JSONCPP_INCLUDE_DIR})
set(LIBS ${LIBS} ${JSONCPP_LIBRARIES})
