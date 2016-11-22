all:
	cl /MD /I. *.lib ezview.c
	ezview.exe

clean:
	rm *.exe *.obj

mine:
	gcc -I. -L. demo.c -ld3dcompiler_47 -lglfw3 -lglfw3dll.lib -llibEGL -llibGLESv2 -o demo
