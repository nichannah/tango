
module tango
interface
    subroutine tango_init(config_dir, grid_name, lis, lie, ljs, lje, gis, gie, gjs, gje) bind(C, NAME='tango_init')
        use iso_c_binding
        character (len=1, kind=C_CHAR), dimension(*), intent(in) :: config_dir
        character (len=1, kind=C_CHAR), dimension(*), intent(in) :: grid_name
        integer (C_INT), value, intent(in) :: lis, lie, ljs, lje
        integer (C_INT), value, intent(in) :: gis, gie, gjs, gje
    end subroutine tango_init

    subroutine tango_begin_transfer(time, grid) bind(C, NAME='tango_begin_transfer')
        use iso_c_binding
        integer (C_INT), value, intent(in) :: time
        character (len=1, kind=C_CHAR), dimension(*), intent(in) :: grid
    end subroutine tango_begin_transfer

    subroutine tango_put(field_name, array, n) bind(C, NAME='tango_put')
        use iso_c_binding
        character (len=1, kind=C_CHAR), dimension(*), intent(in) :: field_name
        real (C_DOUBLE), dimension(n), intent(in) :: array
        integer (C_INT), value, intent(in) :: n
    end subroutine tango_put

    subroutine tango_get(field_name, array, n) bind(C, NAME='tango_get')
        use iso_c_binding
        character (len=1, kind=C_CHAR), dimension(*), intent(in) :: field_name
        real (C_DOUBLE), dimension(n), intent(in) :: array
        integer (C_INT), value, intent(in) :: n
    end subroutine tango_get

    subroutine tango_end_transfer() bind(C, NAME='tango_end_transfer')
    end subroutine tango_end_transfer

    subroutine tango_finalize() bind(C, NAME='tango_finalize')
    end subroutine tango_finalize

end interface
end module tango
