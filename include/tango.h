#if !defined TANGO_H
#define TANGO_H

void tango_init(void);
void tango_begin_transfer(int time);
void tango_put(double array[], int size);
void tango_get(double array[], int size);
void tango_end_transfer(void);
void tango_finalize(void);

#endif /* TANGO_H */
