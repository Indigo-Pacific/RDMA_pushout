set(mpi_sources)
set(mpi_headers)
set(mpi_libraries)

if(${ENABLE_MPI})
  set(mpi_sources
      model/point-to-point-remote-channel.cc
      model/shared-memory-point-to-point-remote-channel.cc
  )
  set(mpi_headers
      model/point-to-point-remote-channel.h
      shared-memory-point-to-point-remote-channel.h
  )
  set(mpi_libraries
      ${libmpi}
      ${MPI_CXX_LIBRARIES}
  )
endif()

build_lib(
  LIBNAME point-to-point
  SOURCE_FILES
    ${mpi_sources}
    helper/point-to-point-helper.cc
    helper/shared-memory-point-to-point-helper.cc
    model/point-to-point-channel.cc
    model/shared-memory-point-to-point-channel.cc
    model/point-to-point-net-device.cc
    model/shared-memory-point-to-point-net-device.cc
    model/ppp-header.cc
  HEADER_FILES
    ${mpi_headers}
    helper/point-to-point-helper.h
    helper/shared-memory-point-to-point-helper.h
    model/point-to-point-channel.h
    model/shared-memory-point-to-point-channel.h
    model/point-to-point-net-device.h
    model/shared-memory-point-to-point-net-device.h
    model/ppp-header.h
  LIBRARIES_TO_LINK ${libnetwork}
                    ${libinternet}
                    ${mpi_libraries}
  TEST_SOURCES test/point-to-point-test.cc
)
