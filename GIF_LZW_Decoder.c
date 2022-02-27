/* DECODE.C - An LZW decoder for GIF
 * Copyright (C) 1987, by Steven A. Bennett
 *
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * In accordance with the above, I want to credit Steve Wilhite who wrote
 * the code which this is heavily inspired by...
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 * Release Notes: This file contains a decoder routine for GIF images
 * which is similar, structurally, to the original routine by Steve Wilhite.
 * It is, however, somewhat noticably faster in most cases.
 *
 * Revised: early 1992 - mec
 * Added Macintosh specific code inside of #ifdef MACINTOSH constructs
 *		this included redefinitions of malloc and free to make them use
 *		Macintosh standard NewPtr and DisposePtr and also making the large
 *		decoding buffers dynamically allocated instead of global arrays.
 *		(The Mac chokes if you have too many data structures this large.)
 *		this also meant adding code to dispose of the arrays at all of the
 *		routine's exit points.
 * Added debugging code inside of #ifdef DEBUGSTR constructs.  DEBUGSTR is
 *		automatically defined as SysBreakStr if it is not defined at the
 *		command line.
 *
 * Revised: 4/28/92 - mec
 * Removed the default of SysBreakStr for DEBUGSTR.  This makes the code more
 *		compatible with GIFPictMgr.c and will hopefully stop the crashes during
 *		aborts.
 *
 * Revised: 6/9/92 - mec
 * UNDEF'd DEBUGSTR - this stuff seems stable and I'm tired of accidental crashes
 *		caused by random SysBreakStrs and SysBreakFuncs
 */
 

#include<stdlib.h>
#include<string.h>


/////////////////// TYPE DEFENITIONS //////////////////////////////////////////


/* Various error codes used by decoder */
#define OUT_OF_MEMORY -10
#define BAD_CODE_SIZE -20
#define BAD_LINE_WIDTH -5
#define MAX_CODES   4095


static const unsigned int code_mask[] = { 0x0000, 
0x0001, 0x0003, 0x0007, 0x000F,
0x001F, 0x003F, 0x007F, 0x00FF,
0x01FF, 0x03FF, 0x07FF, 0x0FFF,
0x1FFF, 0x3FFF, 0x7FFF,0xFFFF };





signed short navail_bytes;              /* # bytes left in block */
signed short nbits_left;                /* # bits left in current unsigned char */
unsigned char b1;                           /* Current unsigned char */
unsigned char byte_buff[257];               /* Current block */
unsigned char *pbytes;                      /* Pointer to next unsigned char in block */
                                      //unsigned char * buf;
unsigned char stack[MAX_CODES + 1];            /* Stack for storing pixels */
unsigned char suffix[MAX_CODES + 1];           /* Suffix table */
unsigned short prefix[MAX_CODES + 1];           /* Prefix linked list */


signed short curr_size;                     /* The current code size */
signed short clear;                         /* Value for a clear code */
signed short ending;                        /* Value for a ending code */
signed short newcodes;                      /* First available code */
signed short top_slot;                      /* Highest code for current size */
signed short slot;                          /* Last read code */




////////////////// FUNCTION DECLARATIONS //////////////////////////////////////

short decoder(unsigned short linewidth, int *bad_code_count);
int get_byte();
short init_exp(short size);
short get_next_code();
int out_line(unsigned char *pixels, int linelen);

///////////////////////////////////////////////////////////////////////////////



// Open source code


short decoder(unsigned short linewidth, int *bad_code_count)
{
    register unsigned char *sp, *bufptr;
    unsigned char *buf;
    register short code, fc, oc, bufcnt;
    short c, size, ret;

    if (linewidth <= 0)
        return BAD_LINE_WIDTH;

    /* Initialize for decoding a new image... */
    *bad_code_count = 0;
    if ((size = (short)get_byte()) < 0)	return(size);
    if (size < 2 || 9 < size)				return(BAD_CODE_SIZE);


    init_exp(size);
    //printf("L %d %x\n",linewidth,size);

    /* Initialize in case they forgot to put in a clear code.
    * (This shouldn't happen, but we'll try and decode it anyway...)
    */
    oc = fc = 0;

    /* Allocate space for the decode buffer */
    buf = (unsigned char*)malloc(linewidth + 1);
    if (buf == NULL) return(OUT_OF_MEMORY);

    /* Set up the stack pointer and decode buffer pointer */
    sp = stack;
    bufptr = buf;
    bufcnt = linewidth;

    /* This is the main loop.  For each code we get we pass through the
    * linked list of prefix codes, pushing the corresponding "character" for
    * each code onto the stack.  When the list reaches a single "character"
    * we push that on the stack too, and then start unstacking each
    * character for output in the correct order.  Special handling is
    * included for the clear code, and the whole thing ends when we get
    * an ending code.
    */
    while ((c = get_next_code()) != ending)
    {
        /* If we had a file error, return without completing the decode*/
        if (c < 0)
        {
            free(buf);
            return(0);
        }
        /* If the code is a clear code, reinitialize all necessary items.*/
        if (c == clear)
        {
            curr_size = (short)(size + 1);
            slot = newcodes;
            top_slot = (short)(1 << curr_size);

            /* Continue reading codes until we get a non-clear code
            * (Another unlikely, but possible case...)
            */
            while ((c = get_next_code()) == clear);

            /* If we get an ending code immediately after a clear code
            * (Yet another unlikely case), then break out of the loop.
            */
            if (c == ending) break;

            /* Finally, if the code is beyond the range of already set codes,
            * (This one had better NOT happen...  I have no idea what will
            * result from this, but I doubt it will look good...) then set it
            * to color zero.
            */
            if (c >= slot) c = 0;
            oc = fc = c;

            /* And let us not forget to put the char into the buffer... And
            * if, on the off chance, we were exactly one pixel from the end
            * of the line, we have to send the buffer to the out_line()
            * routine...
            */
            *bufptr++ = (unsigned char)c;
            if (--bufcnt == 0)
            {
                if ((ret = (short)out_line(buf, linewidth)) < 0)
                {
                    free(buf);
                    return(ret);
                }
                bufptr = buf;
                bufcnt = linewidth;
            }
        }
        else
        {
            /* In this case, it's not a clear code or an ending code, so
            * it must be a code code...  So we can now decode the code into
            * a stack of character codes. (Clear as mud, right?)
            */
            code = c;

            /* Here we go again with one of those off chances...  If, on the
            * off chance, the code we got is beyond the range of those already
            * set up (Another thing which had better NOT happen...) we trick
            * the decoder into thinking it actually got the last code read.
            * (Hmmn... I'm not sure why this works...  But it does...)
            */
            if (code >= slot && sp < (stack + MAX_CODES - 1))
            {
                if (code > slot)
                    *bad_code_count += 1;
                code = oc;
                *sp++ = (unsigned char)fc;
            }

            /* Here we scan back along the linked list of prefixes, pushing
            * helpless characters (ie. suffixes) onto the stack as we do so.
            */
            while (code >= newcodes && sp < (stack + MAX_CODES - 1))
            {
                *sp++ = suffix[code];
                code = prefix[code];
            }

            /* Push the last character on the stack, and set up the new
            * prefix and suffix, and if the required slot number is greater
            * than that allowed by the current bit size, increase the bit
            * size.  (NOTE - If we are all full, we *don't* save the new
            * suffix and prefix...  I'm not certain if this is correct...
            * it might be more proper to overwrite the last code...
            */
            *sp++ = (unsigned char)code;
            if (slot < top_slot)
            {
                suffix[slot] = (unsigned char)(fc = (unsigned char)code);
                prefix[slot++] = oc;
                oc = c;
            }
            if (slot >= top_slot)
            {
                if (curr_size < 12)
                {
                    top_slot <<= 1;
                    ++curr_size;
                }
            }

            /* Now that we've pushed the decoded string (in reverse order)
            * onto the stack, lets pop it off and put it into our decode
            * buffer...  And when the decode buffer is full, write another
            * line...
            */
            while (sp > stack)
            {
                *bufptr++ = *(--sp);
                if (--bufcnt == 0)
                {
                    if ((ret = (short)out_line(buf, linewidth)) < 0)
                    {
                        free(buf);
                        return(ret);
                    }
                    bufptr = buf;
                    bufcnt = linewidth;
                }
            }
        }
    }
    ret = 0;
    if (bufcnt != linewidth)
    {
        ret = (short)out_line(buf, (linewidth - bufcnt));
    }
    free(buf);
    return(ret);
}



short init_exp(short size)
{
    curr_size = (short)(size + 1);
    top_slot = (short)(1 << curr_size);
    clear = (short)(1 << size);
    ending = (short)(clear + 1);
    slot = newcodes = (short)(ending + 1);
    navail_bytes = nbits_left = 0;

    memset(stack, 0, MAX_CODES + 1);
    memset(prefix, 0, MAX_CODES + 1);
    memset(suffix, 0, MAX_CODES + 1);
    return(0);
}


/* get_next_code()
* - gets the next code from the GIF file.  Returns the code, or else
* a negative number in case of file errors...
*/
short get_next_code()
{
    short i, x;
    unsigned int ret;

    if (nbits_left == 0)
    {
        if (navail_bytes <= 0)
        {
            /* Out of bytes in current block, so read next block */
            pbytes = byte_buff;
            if ((navail_bytes = (short)get_byte()) < 0)
            {
                return(navail_bytes);
            }

            if (navail_bytes)
            {
                for (i = 0; i < navail_bytes; ++i)
                {
                    if ((x = (signed short)get_byte()) < 0) return(x);
                    byte_buff[i] = (unsigned char)x;
                }
            }
        }
        b1 = *pbytes++;
        nbits_left = 8;
        --navail_bytes;
    }

    if (navail_bytes < 0) return ending; // prevent deadlocks (thanks to Mike Melnikov)

    ret = b1 >> (8 - nbits_left);
    while (curr_size > nbits_left)
    {
        if (navail_bytes <= 0)
        {
            /* Out of bytes in current block, so read next block*/
            pbytes = byte_buff;
            if ((navail_bytes = (short)get_byte()) < 0)
                return(navail_bytes);
            else if (navail_bytes)
            {
                for (i = 0; i < navail_bytes; ++i)
                {
                    if ((x = (short)get_byte()) < 0) return(x);
                    byte_buff[i] = (unsigned char)x;
                }
            }
        }
        b1 = *pbytes++;
        ret |= b1 << nbits_left;
        nbits_left += 8;
        --navail_bytes;
    }
    nbits_left = (short)(nbits_left - curr_size);
    ret &= code_mask[curr_size];
    return((short)(ret));
}
