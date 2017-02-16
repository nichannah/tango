
! Fortran tests:
!   - do single field send/receive between two grids/models. This is a
!   translation of the test in tango_ctest.cc

program tango_test

    use mpi
    use tango, only: tango_init, tango_finalize
    implicit none

    integer, parameter :: g_rows = 4, g_cols = 4, l_rows = 4, l_cols = 4
    integer :: ierror, rank

    call MPI_Init(ierror)
    call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierror)

    if (rank == 0) then
        call tango_init('./test_input-1_mappings-2_grids-4x4_to_4x4/', 'ocean', &
                        0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols)
    else
        call tango_init('./test_input-1_mappings-2_grids-4x4_to_4x4/', 'ice', &
                        0, l_rows, 0, l_cols, 0, g_rows, 0, g_cols)
    endif

    call tango_finalize()
    call MPI_Finalize(ierror)

end program tango_test
