.PHONY : all win64

win64 :
	gcc -O3 -o ./bin/snapshot.dll -Wall -m64 -shared \
	./src/* \
	-I./luajit/src/ -I./include/ \
	-L./luajit/lib/ \
	-lluajit

exe :
	gcc -O3 -o ./bin/snapshot.exe -Wall -m64 \
	./src/* \
	-I./luajit/src/ -I./include/ \
	-L./luajit/lib/ \
	-lluajit
