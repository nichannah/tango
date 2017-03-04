
CC=mpic++
CFLAGS=-std=c++11 -Wall -g -O2 -I../../include
LDFLAGS=-L../../build/ -ltango -lyaml-cpp -lboost_date_time -lnetcdf_c++4

all: ice.exe ice_perf.exe atm_perf.exe

ice.exe: ice.cc
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

ice_perf.exe: ice_perf.cc
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

atm_perf.exe: atm_perf.cc
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf *.o ice.exe
