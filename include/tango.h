#if !defined TANGO_H
#define TANGO_H

#define DLLEXPORT extern "C"

DLLEXPORT int tango_init(const char *config, const char *grid_name,
                /* Local  domain */
                unsigned int lis, unsigned int lie,
                unsigned int ljs, unsigned int lje,
                /* Global domain */
                unsigned int gis, unsigned int gie,
                unsigned int gjs, unsigned int gje);

DLLEXPORT void tango_begin_transfer(int time, const char* grid_name);
DLLEXPORT void tango_put(const char* field_name, double array[], int size);
DLLEXPORT void tango_get(const char* field_name, double array[], int size);
DLLEXPORT void tango_end_transfer(void);
DLLEXPORT void tango_finalize(void);

#endif /* TANGO_H */
