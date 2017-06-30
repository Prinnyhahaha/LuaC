.PHONY : all win64

win64 :
	gcc -O3 -o ./bin/snapshot.dll -Wall -m64 -shared \
	./src/snapshot.c \
	-I./luajit/src/ \
	-L./luajit/lib/ \
	-lluajit
