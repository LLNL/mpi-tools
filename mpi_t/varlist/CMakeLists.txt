   cmake_minimum_required(VERSION 2.6)
##Add targets
#C
find_package(MPI)
add_executable(varlist varlist.c)

target_link_libraries(varlist ${MPI_C_LIBRARIES})

include_directories(
  ${MPI_C_INCLUDE_PATH}
  ${MPI_Fortran_INCLUDE_PATH})

