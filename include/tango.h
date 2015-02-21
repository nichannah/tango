#if !defined TANGO_H
#define TANGO_H

void tango_init(const char *grid_name,
                /* Local  domain */
                unsigned int lis, unsigned int lie,
                unsigned int ljs, unsigned int lje,
                /* Global domain */
                unsigned int gis, unsigned int gie,
                unsigned int gjs, unsigned int gje);

void tango_begin_transfer(int time, const char* grid_name);
void tango_put(const char* field_name, double array[], int size);
void tango_get(const char* field_name, double array[], int size);
void tango_end_transfer(void);
void tango_finalize(void);

#endif /* TANGO_H */
