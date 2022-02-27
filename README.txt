Group 22:-
IMT2020553, Abhinav Mahajan
IMT2020006, Harsh Shah
IMT2020524, Anant Pandey
IMT2020100, Teja Janaki Ram
IMT2020127, Dharmin Mehta
IMT2020541, Vamshidhar Reddy

Work Distribution:-
Harsh Shah, Dharmin Mehta, Teja- Worked on TIFF.
Abhinav Mahajan, Vamshidha Reddy, Anant Pandey- Worked on GIF.
Makefile-Anant Pandey, Teja Janaki Ram, Dharmin Mehta
README-Abhinav Mahajan, Harsh Shah

1.)                                   Project execution template:-
gcc comands:-

make
./output (sourcefilename.gif)
make clean

make command, will create the .o files. it will also create the executable, output
./output (sourcefilename.gif) will execute the program, in its process create two intermediate binary files, and one destination file, tiff.tiff
make clean will remove the .o files, intermediate, executable, and destination file(tiff.tiff).



2.)                                  Project flow template:-
Description:- Convert GIF to TIFF.

Step 1.) Parsing GIF input file.
GIF-Header (13 Byte) ->Logical Screen Descriptor -> Global Color table (Optional)
    -> Graphic Control Extension(Optional)
    -> Per image start with 2C Image descriptor (which provide the detail about the image)
    ->Local Color table for this image (Optional) --> Image Data --> Image Terminator (3B)

    It may also contain
    Plain text extension(Optional)
    Application Extension (optional)
    Comment extension (Optional )
    There is a loop structure of these extensions.
    All are parsed until image data is obtained.
    
    When raw image data is encountered if the file has not been corrupted till now, compressed or uncompressed, both, decoder() function is called from GIF_LZW_Decoder.c(open source program).
    decoder function requires a call back, get_byte(). Since the decoder doesnt have file pointer, it gets data, byte by byte from the the GIF file in the GIF_FIle_Parser.c
    once a chunk(sub block) of data is uncompressed, out_line() function prints the Color indices of pixels of a row, in an intermediate file LZW_Intermediate.bin
    This file once created fully after successful decoding, is read by the program.
    An intermediate TIFF_Intermediate.bin is created.
    The first 4 bytes of TIFF_Intermediate are the image width and height. Obtained from parsing GIF.
    From LZW_Intermediate.bin, the color indices are obtained, the corresponding local color table or global color table rgb values are then printed in TIFF_Intermediate
    The last byte of TIFF_Intermediate.bin is 0x3B, same as image trailer of a gif.
    TIFF_Intermediate is a binary file created with a format to suit the programs convenience.
    write_tiff() uses this intermediate file(TIFF_Intermediate), to create the corresponding tiff file for the gif image. 
    
    

Step 2.) Creating the destination TIFF file.
    -> TIFF-Header(8 bytes) -> First IFD -> TIFF tags (12 neccessary tags for RGB implementation) -> stripOffset data -> stripByteCounts data -> RGB data of each pixel

    Header contains 4 bytes of identification which verifies that it is a tiff file. The next 4 bytes contains IFD offset of the first IFD.

    IFD contains 2 bytes of total number of tags value(12for RGB), which is followed by each tag, and 4 bytes of nextIFDOffset value.

    Each tag occupies 12 bytes of memory. First 2 bytes contains the tagID, next 2 bytes contains the datatype of the value which that tag holds, next 4 bytes the number of values depicted by that tag, and last four bytes either contain the value of that tag (if it fits in 4 bytes) or points to the address where that value is stored (if it is larger than 4 bytes).

    Till now 158 bytes of memory is filled.

    stripOffset tag data starts at 159 position in the memory and it contains the address of the starting of each row of the strip. stripByteCounts tag data contains the total number of bytes in each strip of the image.
    Note here that we have taken, height number of rows of the strips. This means each row of the image data is a strip.
    Therefore stripOffset data is calculated as the address of starting of each row of the image. And stripByteCounts data contains a constant value of width*3 bytes as each strip occupies width*3 bytes of memory.
    The image data, i.e. the rgb data of each pixel, recieved from the intermediate binary file, follows the stripbytecounts data.
        


3.)                                Project Scope template:-
Any non corrupted GIF image, 89a or 87a is supported.
Any legal, primitive extension(Graphics control, application, plain text extension and comment extensions) of the GIF file will be parsed
Uncompressed GIFS and Compressed GIFS will be parsed. Intermediate files will have no change in data.
Animated GIFS will be parsed too. Only the first image although. The first image of a multi image GIF file will be converted to TIFF.
Interlacing will be ignored by the program. The output will be displayed primitively, without interlacing.
