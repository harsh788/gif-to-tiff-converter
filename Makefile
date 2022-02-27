
CC = gcc
CFLAGS = -I.

output : GIF_LZW_Decoder.o GIF_File_Parser.o 
	
	$(CC) -o output GIF_LZW_Decoder.o GIF_File_Parser.o 

clean :
	rm -rf tiff.tiff GIF_File_Parser.o GIF_LZW_Decoder.o LZW_Intermediate.bin TIFF_Intermediate.bin output 


