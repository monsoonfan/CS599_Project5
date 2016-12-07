all:
	cl /MD /I. *.lib ezview.c
	run ezview.exe p3test.ppm

clean:
	rm *.exe *.obj

my_image_p3:
	run ezview.exe my_image_p3.ppm

my_image_p6:
	run ezview.exe my_image_p6.ppm

mine:
	gcc -I. -L. demo.c -ld3dcompiler_47 -lglfw3 -lglfw3dll.lib -llibEGL -llibGLESv2 -o demo

tut:
	cl /MD /I. *.lib tutorial.c
