---------------------------------------------------------
# README - ezview.c (for project 5)
# Mitchell Ralston - rmr5
#
# ImageViewer
# -----------
# Uses OpenGL (starter kit and demos provided by Dr. Palmer)
# to render a texture map image
#
---------------------------------------------------------

---------------------------------------------------------
# Application
---------------------------------------------------------

     usage: ezview <image.ppm>
     
     example: ezview my_image_p3.ppm

     Keybindings:
     r = rotate clockwise 90 degrees
     c = rotate counter-clockwise 90 degrees
     o = scale down by 1/2 (zoom out)
     i = scale up by 2 (zoom in)
     s = shear top of image to the right
     d = shear top of image to the left
     a = shear right upward
     f = shear right downward
     e = exit
     escape = exit

---------------------------------------------------------
# Makefile targets of interest for grading:
---------------------------------------------------------
       make all               (compile the program and load small p3_test.ppm)
       make clean             (clean *exe and *obj)
       make my_image_p3       (run ezview.exe my_image_p3.ppm)
       make my_image_p6       (run ezview.exe my_image_p6.ppm)
