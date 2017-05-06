all:
	gcc -c -o lib/dashgl.o lib/dashgl.c -lGL -lGLEW -lpng
	gcc `pkg-config --cflags gtk+-3.0` main.c lib/dashgl.o `pkg-config --libs gtk+-3.0` -lGLEW -lGL -lm -lpng
