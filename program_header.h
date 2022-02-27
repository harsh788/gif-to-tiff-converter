

int LetsOpen(FILE *);
int CheckSupportedGifVersion();
void Display_Header_Information();
void set_GHField_wxyz_(char);
void Parse_Gif_Global_Color_Table(FILE *);
void Parse_Gif_Local_Color_Table(FILE *);
void Parse_Graphics_Control_Extension(FILE *);
void Parse_Image_Descriptor(FILE *);
void decode_Image_Packed_Field(unsigned char);
void decode_extension(FILE *fp);


void tag_info(FILE *,unsigned short int,unsigned short int,unsigned int,unsigned int);
void header_info(FILE *);

