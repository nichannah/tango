
CC=mpic++
CFLAGS=-std=c++11 -Wall -g -O0 -I../../include
LDFLAGS=-L../../build/ -ltango -lyaml-cpp -lboost_date_time -lnetcdf_c++4

all: ice.exe

ice.exe: ice.cc
	$(CC) $< -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf *.o ice.exe
