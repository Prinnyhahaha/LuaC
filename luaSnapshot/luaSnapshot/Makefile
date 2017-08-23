.PHONY : all win64

win64 :
	g++ -O3 -o ./bin/snapshot.dll -Wall -m64 -shared \
	./src/* \
	-I./luajit/src/ -I./include/ \
	-L./luajit/lib/ \
	-lluajit

exe :
	g++ -O3 -o ./bin/snapshot.exe -Wall -m64 -static \
	./src/* \
	-I./luajit/src/ -I./include/ \
	-L./luajit/lib/ \
	-lluajit
