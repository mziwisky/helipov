CC = g++

CFLAGS = -O3 -pipe -fomit-frame-pointer -funroll-all-loops -s

heli: EasyBMP.o heliBMP.o
	g++ $(CFLAGS) EasyBMP.o heliBMP.o -o ../bin/heli

EasyBMP.o: EasyBMP/EasyBMP.cpp EasyBMP/EasyBMP*.h
	cp EasyBMP/EasyBMP*.h .
	cp EasyBMP/EasyBMP.cpp .
	g++ $(CFLAGS) -c EasyBMP.cpp

heliBMP.o: heliBMP.cpp
	g++ -c heliBMP.cpp

clean: 
	rm EasyBMP*.h
	rm EasyBMP.cpp
	rm -f *.o
	rm ../bin/heli
