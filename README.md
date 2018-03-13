# BMP_HiddenFileFinder
This program searches .bmp for files hidden using LSB subsitution.
The program returns files which share the name of the file in which they were found, as well as the channels 
scanned by the program to find the file.

It only searches for .pdf, .jpg, .bmp, .docx, and .mp3 files; allowing the program to search for additional files is trivial,
the program simply needs to know what the file headers are. 

### Note: the files you pass to the program must be in the same directory as the program

Sample input files are provided. Simply pass the name of a .bmp file (which is in the same directory) to the program.
Output files consist of:
 - lena0-bg.jpg
 - lena1-b.jpg
 - lena1-g.jpg
 - lena1-r.jpg
 - lena2-bg.jpg
 - lena2-br.jpg
 - lena3-b.docx
 - lena3-gr.pdf
 - lena4-bgr.mp3
 - lena5-g.bmp
