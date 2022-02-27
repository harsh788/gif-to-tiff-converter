/*
    A GIF file can contain multiple images (as same can be used for animation) and
    GIF file has the following structure

    GIF-Header (13 Byte) ->Logical Screen Descriptor -> Global Color table (Optional)
    Then Graphic Control Extension(Optional)
    then Per image start with 2C Image descriptor (which provide the detail about the image)
    -->Local Color table for this image (Optional) --> Image Data --> Image Terminator (3B)

    It may also contain
    Plain text extension(Optional)
    Application Extension (optional)
    Comment extension (Optional )
    There is a loop structure of these extensions.
    All are parsed until image data is obtained.
    Reference :
    http://giflib.sourceforge.net/whatsinagif/bits_and_bytes.html
    https://www.w3.org/Graphics/GIF/spec-gif89a.txt

    A GIF image data may be encoded/compressed using LZW algorithm. In this project I have used the
    open source code to decode the GIF pixel data
    open source
    https://stuff.mit.edu/afs/athena/project/net_dev/tmp/techinfo.mac/src/GIFdecoder.c
    This library expect the caller to implement
    get_byte() method. Library will keep calling this method to get the next byte to decode and once
    library has decoded one entire line then it will send the decoded pixel data (per line) via method
    out_line()

    Also I want to thanks my TA, Shyam(ph2018) for the support and help.

*/


/*Libraries Included:-*/
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include <string.h>
#include "program_header.h"


/*Structures used:-*/
#pragma pack(1)
struct Header
{
    unsigned char version[6];
    unsigned short width;
    unsigned short height;
    unsigned char fields;
    unsigned char background_color_index;
    unsigned char pixel_aspect_ratio;
}Gif_Header;
#pragma pack(1)
struct rgb
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};
struct rgb *Global_Color_Table = NULL;
struct rgb *Local_Color_Table = NULL;
#pragma pack(1)
struct Graphics_Control_Extension
{
    unsigned char Byte_Size;
    unsigned char Packed_Field;
    unsigned short Delay_Time;
    unsigned char Transparent_Color_Index;
}Graphics_Control_Exten;
#pragma pack(1)
struct Image_Descriptor_Header
{
    unsigned short Image_Left;
    unsigned short Image_Top;
    unsigned short Image_Width;
    unsigned short Image_Height;
    unsigned char Packed_Field;
}Image_Descriptor;


/*Macros:-*/
#define IMAGE_DESCRIPTOR 0x2C
#define TRAILER 0x3B
#define EXTENSION 0x21


/*Global variables:-*/
unsigned char GHFields_w;
unsigned char GHFields_x;
unsigned char GHFields_y;
unsigned char GHFields_z;
int Number_Color_Resolution_Bits = -1;
int Number_Bits_Global_Color_Table = -1;
int local_color_table_flag;
int interlace_flag;
int sort_flag;
int reserved;
int size_local_color_table;
unsigned char LZW_Minimum_Code_Size;

/*variables used for decoding extensions:-*/
int application_extension_loops;
int row_count = 0;
unsigned char Gif_Comments[256];


/*The following function is written in GifLzwDecoder.c
 * Credits given above.*/
short decoder(unsigned short lineWidth, int *bad_code_count);


/*File pointers:-*/
FILE *fp_read, *fp_decoded_file, *fp_for_conversion;


void header_info(FILE *myfile)
{
	unsigned char ch1='I';
	unsigned char ch2='I';
	unsigned short int st=42;
	unsigned int ifd_offset=8;
	fwrite(&ch1,sizeof(unsigned char),1,myfile);
	fwrite(&ch2,sizeof(unsigned char),1,myfile);
	fwrite(&st,sizeof(unsigned short int),1,myfile);
	fwrite(&ifd_offset,sizeof(unsigned int),1,myfile);
	return;
}

void tag_info(FILE *myfile,unsigned short int tagid,unsigned short int datatype,unsigned int datacount,unsigned int dataoffset)
{
	fwrite(&tagid,sizeof(unsigned short int),1,myfile);
	fwrite(&datatype,sizeof(unsigned short int),1,myfile);
	fwrite(&datacount,sizeof(unsigned int),1,myfile);
	fwrite(&dataoffset,sizeof(unsigned int),1,myfile);
	return;
}
struct read_bin{
	        unsigned short int width;
		unsigned short int height;
		unsigned short int end;
};

struct read_rgb{
	        unsigned char r;
		unsigned char g;
		unsigned char b;
};

void write_tiff()
{
    char *fptiff = "tiff.tiff";
	FILE *fo=fopen((char *)fptiff,"w+b");
	FILE *fp=fopen("TIFF_Intermediate.bin","rb");
	struct read_bin bin;
	struct read_rgb rgb;
	
	fread(&bin.width,sizeof(unsigned short int),1,fp);
	fread(&bin.height,sizeof(unsigned short int),1,fp);
	
	if(fo==NULL)
	{
		printf("Error");
	}
	int pointer=0;
	int i,j;
	unsigned short int num_tag=12;
	header_info(fo);
	fwrite(&num_tag,sizeof(unsigned short int),1,fo);
	tag_info(fo,256,4,1,bin.width);
	tag_info(fo,257,4,1,bin.height);
	tag_info(fo,258,3,1,8);
	tag_info(fo,259,3,1,1);
	tag_info(fo,262,3,1,2);
	tag_info(fo,273,4,bin.height,158);

	int nwp=158+(bin.height*4);

	tag_info(fo,277,3,1,3);
	tag_info(fo,278,3,1,1);
	tag_info(fo,279,4,bin.height,nwp);
	tag_info(fo,282,5,1,0);
	tag_info(fo,283,5,1,0);
	tag_info(fo,296,3,1,2);

	int a=0; 
	fwrite(&a,4,1,fo);                 //Extra 4 bytes for nextifdoffset(not needed)

	int fstroffset=158+(bin.height*8);
	int stroffset=fstroffset;
	for(i=0;i<bin.height;i++)               //Writing address
	{
		fwrite(&stroffset,4,1,fo);
		stroffset += bin.width*3;
	}
	for(i=0;i<bin.height;i++)               //Writing the value of stripbytecounts tag
	{
		int m=bin.width*3;
		fwrite(&m,4,1,fo);
	}
        
	/*int arr_red[height][width];
	int arr_blue[height][width];
	int arr_green[height][width];
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			arr_red[i][j]=(int)((i+j)/5);
			arr_green[i][j]=(int)j;
			arr_blue[i][j]=(int)((i+j)/7);
		}
	}*/
	
	for(i=0;i<bin.height*3*bin.width;i++)
	{
		fread(&rgb,sizeof(struct read_rgb),1,fp);
		fwrite(&rgb,sizeof(struct read_rgb),1,fo);
	}


	printf("Tiff Image conversion successfull!\n");
	
	
	return;
}

int main(int argc, char *argv[])
{
    printf("Number of arguments found is:- %d\n", argc);
    if(argc > 2)
    {
        printf("Too many filenames enetered!! or filename entered with space. Please check\n");
        return 1;
    }
    if(argc == 1)
    {
        printf("Please enter source file name as Command line param!!\n");
        return 1;
    }
    char *fpname = argv[1];
    printf("Source(GIF) file name entered is:- %s\n", fpname);
    char *fptiff = "tiff.tiff";
    printf("Destination(TIFF) file name is:- %s\n", fptiff);
	
    fp_read = fopen((char *)fpname, "rb");
    
    if (fp_read == NULL)
    {
        printf("File not found!\n");
    }
    else
    {
        fp_decoded_file = fopen("LZW_Intermediate.bin", "w+b");
        if (fp_decoded_file == NULL)
        {
            fclose(fp_read);
            printf("Unable to create intermediate file. Program will terminate !!!\n");
            return -1;
        }
        fp_for_conversion = fopen("TIFF_Intermediate.bin", "w+b");
        if (fp_decoded_file == NULL)
        {
            fclose(fp_read);
            fclose(fp_decoded_file);
            printf("Unable to create intermediate file. Program will terminate !!!\n");
            return -1;
        }
        if (LetsOpen(fp_read) == 0)
        {
            printf("Decode completed successfully \n");
        }
        else
        {
            printf("Decode failed\n");
        }

        fclose(fp_read);
        fclose(fp_decoded_file);
        fclose(fp_for_conversion);
        if (Global_Color_Table != NULL)
        {
            free(Global_Color_Table);
        }
        if(Local_Color_Table != NULL)
        {
            free(Local_Color_Table);
        }
	write_tiff();
    }
    return 0;
}

/*Program begins:-*/
int LetsOpen(FILE *fp)
{
    /*retVal is the value returned by fread()
     * 0, if decoding is usccessful,
     * 1 if decoding is unsuccessful.*/
    int retVal;
    int byteToRead = sizeof(struct Header);
    int byteRead = fread(&Gif_Header, 1, byteToRead, fp);

    if(byteRead != byteToRead)
    {
        printf("Invalid Gif File. Does not contain correct header information");
        retVal = 1;
        return retVal;
    }

    Display_Header_Information();
    retVal = CheckSupportedGifVersion();

    if(retVal != 0)
    {
        return retVal;
    }

    set_GHField_wxyz_(Gif_Header.fields);

    if (Number_Bits_Global_Color_Table > 0)
    {
        Parse_Gif_Global_Color_Table(fp);
    }

    bool bPreviousWasNull = true;
    bool bContinue = true;
    char ch;
    /*nframes stores number of images.
     * GIF File may contain multiple images.
     *Incase of multiple images only the first picture/frame
     *is decoded and used*/
    int nframes = 0;
    while(bContinue)
    {
        if (fread(&ch, sizeof(ch), 1, fp) != 1)
        {
            printf("GIF File corrupted!");
            break;
        }
        if (bPreviousWasNull || ch == 0)
        {
            switch (ch)
            {
                case EXTENSION: // An extension
                {
                    decode_extension(fp);
                    break;
                }
                case IMAGE_DESCRIPTOR: // image
                {
                    nframes++;
                    if(nframes > 1)
                    {
                        printf("Gif contains multiple images (animated gif file). This version does not convert all images to TIFF. Just converting first image and baling out.\n");
                        retVal = 0;
                        return retVal;
                    }
                    Parse_Image_Descriptor(fp);
                    decode_Image_Packed_Field(Image_Descriptor.Packed_Field);
                    if (local_color_table_flag == 1)
                    {
                        Parse_Gif_Local_Color_Table(fp);
                    }
                    /*badcode stores whether the decoder() function successfully decoded the file or not
                     * badcode = 0 if decoder is successful.
                     */
                    int badcode;

                    decoder(Image_Descriptor.Image_Width, &badcode);
                    /*decoder output has written into an intermediate file, fp_decoded_file
                     *fflush will make sure that content written in memory are flushed to the disk.
                     */
                    fflush(fp_decoded_file);

                    /*To move move file pointer to start of file:-*/
                    fseek(fp_decoded_file, 0, SEEK_SET);

                    if (badcode != 0)
                    {
                        printf("Decoding failed. Wrong GIF/Corrupted file  \n");
                        retVal = 1;
                        return  retVal;
                    }
                    if (Local_Color_Table == NULL && Global_Color_Table == NULL)
                    {
                        printf("Wrong GIF file as file does not have either Global color table or Local color table \n");
                        retVal = 1;
                        return  retVal;
                    }
                    unsigned short width = Image_Descriptor.Image_Width;
                    unsigned short height = Image_Descriptor.Image_Height;
                    fwrite(&width, 2, 1, fp_for_conversion);
                    fwrite(&height, 2, 1, fp_for_conversion);

                    unsigned char buffer;
                    while (fread(&buffer, 1, 1, fp_decoded_file) == 1)
                    {
                        unsigned char r, g, b;
                        if (Local_Color_Table != NULL)
                        {
                            r = Local_Color_Table[buffer].red;
                            g = Local_Color_Table[buffer].green;
                            b = Local_Color_Table[buffer].blue;
                        }
                        else
                        {
                            r = Global_Color_Table[buffer].red;
                            g = Global_Color_Table[buffer].green;
                            b = Global_Color_Table[buffer].blue;
                        }
                        fwrite(&r, 1, 1, fp_for_conversion);
                        fwrite(&g, 1, 1, fp_for_conversion);
                        fwrite(&b, 1, 1, fp_for_conversion);
                    }
                    buffer = TRAILER;
                    fwrite(&buffer, 1, 1, fp_for_conversion);
                    fflush(fp_for_conversion);
                    if(Local_Color_Table != NULL)
                    {
                        free(Local_Color_Table);
                        Local_Color_Table = NULL;
                    }
                    break;
                }
                case TRAILER: //terminator
                    bContinue = false;
                    break;
                default:
                    /*Files may have repeatedly used padding characters.
                     */
                    bPreviousWasNull = (ch == 0);
                    break;
            }
        }
    }
    return retVal;
}
void set_GHField_wxyz_(char f)
{
    GHFields_z = f & 0x07;
    GHFields_y = (f & 0x08) >> 3;
    GHFields_x = (f & 0x70) >> 4;
    GHFields_w = (f & 0x80) >> 7;
    Number_Color_Resolution_Bits = (1 << (GHFields_x + 1));
    if(GHFields_w)

    {
        Number_Bits_Global_Color_Table = (1 << (GHFields_z + 1));
    }
}
int CheckSupportedGifVersion()
{
    int retVal = 0;
    if (strncmp(Gif_Header.version, "GIF89a", 6) != 0 &&
        strncmp(Gif_Header.version, "GIF87a", 6) != 0)
    {
        printf("Unknown/Corrupted GIF version. File supported by program is GIF89a or 87a");
        retVal = 1;
    }
    return retVal;

}

void Display_Header_Information()
{
    printf("GIF version is = ");
    printf("%c", Gif_Header.version[0]);
    printf("%c", Gif_Header.version[1]);
    printf("%c", Gif_Header.version[2]);
    printf("%c", Gif_Header.version[3]);
    printf("%c", Gif_Header.version[4]);
    printf("%c\n", Gif_Header.version[5]);
    printf("Pixel width is = %d\n", Gif_Header.width);
    printf("Pixel height is = %d\n", Gif_Header.height);
    printf("Gif field = %d\n", Gif_Header.fields);
    printf("Gif field w = %d\n", GHFields_w);
    printf("Gif field x = %d\n", GHFields_x);
    printf("Gif field y = %d\n", GHFields_y);
    printf("Gif field z = %d\n", GHFields_z);
    printf("Number of Bits in Global Color Table = %d\n", Number_Bits_Global_Color_Table);
    printf("Number of colour resolution bits, representing how many colours are there = %d\n", Number_Color_Resolution_Bits);
    printf("Gif Background Colour Index = %d\n", Gif_Header.background_color_index);
    printf("Gif Pixel Aspect Ratio = %d\n", Gif_Header.pixel_aspect_ratio);
}
void Parse_Gif_Global_Color_Table(FILE *fp)
{
    Global_Color_Table = (struct rgb *)malloc(3 * Number_Bits_Global_Color_Table);
    fread(Global_Color_Table, 3 * Number_Bits_Global_Color_Table, 1, fp);
    int i;
    for(i = 0;i < Number_Bits_Global_Color_Table;i++)
    {
        printf("\n");
        printf("Serial Number is = %d\n", i);
        printf("Red intensity is = %d\n", Global_Color_Table[i].red);
        printf("Green intensity is = %d\n", Global_Color_Table[i].green);
        printf("Blue intensity is = %d\n", Global_Color_Table[i].blue);
        printf("\n");

    }
}

void Parse_Gif_Local_Color_Table(FILE *fp)
{
    Local_Color_Table = (struct rgb *)malloc(3 * size_local_color_table);
    fread(Local_Color_Table, 3 * size_local_color_table, 1, fp);
    int i;
    for (i = 0; i < size_local_color_table; i++)
    {
        printf("\n");
        printf("Serial Number is = %d\n", i);
        printf("Red intensity is = %d\n", Local_Color_Table[i].red);
        printf("Green intensity is = %d\n", Local_Color_Table[i].green);
        printf("Blue intensity is = %d\n", Local_Color_Table[i].blue);
        printf("\n");
    }
}

void Parse_Graphics_Control_Extension(FILE *fp)
{
    fread(&Graphics_Control_Exten, sizeof(struct Graphics_Control_Extension), 1, fp);
}
void Parse_Image_Descriptor(FILE *fp)
{
    fread(&Image_Descriptor, sizeof(struct Image_Descriptor_Header), 1, fp);
    printf("Image position is (left, top) = (%u, %u)\n", Image_Descriptor.Image_Left, Image_Descriptor.Image_Top);
    printf("Image width and heigth is = %u, %u\n", Image_Descriptor.Image_Width, Image_Descriptor.Image_Height);
    printf("Image field is = %x\n", Image_Descriptor.Packed_Field);
}
void decode_Image_Packed_Field(unsigned char ch)
{
    local_color_table_flag = ((ch & 0x80) >> 7);
    interlace_flag = ((ch & 0x40) >> 6);
    sort_flag = ((ch & 0x20) >> 5);
    reserved = ((ch & 0x18) >> 3);
    size_local_color_table = 0;
    if(local_color_table_flag == 1)
    {
        size_local_color_table = (1 << ((ch & 0x07) + 1));
    }
    printf("Local color table flag = %d\n", local_color_table_flag);
    printf("Interlace flag = %d\n", interlace_flag);
    printf("sort flag = %d\n", sort_flag);
    printf("Reserved bits are = %d\n", reserved);
    printf("Local color table entries = %d\n", size_local_color_table);
}

bool ParseApplicationExtensions(FILE* fp)
{
    unsigned char count;

    bool bContinue = (fread(&count, sizeof(count), 1, fp) == 1);
    bContinue &= (count == 11);
    if (bContinue)
    {
        char AppID[12]; //one more than the count
        bContinue = (fread(AppID, count, 1, fp) == 1);
        AppID[count] = '\0';
        printf("Application ID is = %s\n", (char *)AppID);
        if (bContinue)
        {
            bContinue = (fread(&count, sizeof(count), 1, fp) == 1 );
            if (bContinue)
            {
                unsigned char *temp = (unsigned char*)malloc(count);
                if (temp == NULL)
                {
                    //Memory allocation failed.
                    printf("Oops, Memory allocation failed..\n");
                    return false;
                }

                bContinue = (fread(temp, count, 1, fp) == 1);
                if (count > 2)
                {
                    application_extension_loops = temp[1] + 256 * temp[2];
                    printf("Application extension loop count is:- %d\n", application_extension_loops);
                }

                free(temp);
            }
        }
    }
    return bContinue;
}

void decode_extension(FILE *fp)
{
    bool bContinue;
    unsigned char count;
    unsigned char fc;

    bContinue = (fread(&fc, sizeof(fc), 1, fp) == 1);
    if (bContinue)
    {
        /* AD - for transparency */
        if (fc == 0xF9)
        {
            printf("GIF file has graphic color extension \n ");
            Parse_Graphics_Control_Extension(fp);
        }

        if (fc == 0xFE)
        {
            // Comment extension block
            printf("GIF file has comment extension \n ");

            bContinue = (fread(&count, sizeof(count), 1, fp) == 1);
            if (bContinue)
            {
                bContinue = (1 == fread(Gif_Comments, count, 1, fp));
                Gif_Comments[count] = '\0';
                printf("Gif comment is:- %s\n", (char *)Gif_Comments);
            }
        }

        if (fc == 0xFF)
        {
            //Application Extension block
            printf("GIF file has Application Extension block \n ");

            bContinue = ParseApplicationExtensions(fp);
        }

        while (bContinue && fread(&count, sizeof(count), 1, fp) && count)
        {
            printf("Skipping %d bytes", count);
            fseek(fp, count, SEEK_CUR);
        }
    }

}

/*decoder() function, uncompressing the GIF file,
 *requires the image data. get_byte(), gives the decoder function the data, byte-by-byte
 *out_line() decoder returns the single line decoded output. This is stored in a temporary intermediate
 *file fp_decoded_file. This file is later implemented to write into another file fp_forconversion
 *which stores height, width, and rgb values from the color tables for the respective pixel data.
 *this file is read by the tiff function, to create a corresponding tiff image.
 */
int get_byte()
{
    unsigned char buffer;
    if (fread(&buffer, 1, 1, fp_read) != 1)
    {
        return -1;
    }
    return buffer;
}

int out_line(unsigned char *pixels, int linelen)
{
    if (pixels == NULL)
        return -1;

    printf("Row (Height): %d pixel column  (width): %d \n", ++row_count, linelen);

    fwrite(pixels, 1, linelen, fp_decoded_file);
    return 0;
}
