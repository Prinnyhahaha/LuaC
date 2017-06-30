.PHONY : all win64

win64 :
	gcc -O3 -o ./bin/snapshot.dll -Wall -m64 -shared \
	./src/snapshot.c ./src/lua51.c \
	-I./luajit/src/ -I./include/ \
	-L./luajit/lib/ \
	-lluajit

exe :
	gcc -O3 -o ./bin/snapshot.exe -Wall -m64 \
	./src/snapshot.c ./src/lua51.c \
	-I./luajit/src/ -I./include/ \
	-L./luajit/lib/ \
	-lluajit
