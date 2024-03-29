/* Copyright (C) 2001-2023 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  39 Mesa Street, Suite 108A, San Francisco,
   CA 94129, USA, for further information.
*/

/*
 * Brother HL 720 and 730 driver for Ghostscript
 *
 * Note: for the HL 760, use the HP driver.
 *
 * The original code was borrowed from the
 * HP LaserJet/DeskJet driver for Ghostscript.
 * The code specific to the Brother HL 720 was written by :
 *       Pierre-Olivier Gaillard (pierre.gaillard@hol.fr)
 * Thanks to the documentation kindly provided by :
 *        Richard Thomas <RICHARDT@brother.co.uk>
 *
 * Removal of compression code on 1/17/00 by Ross Martin
 * (ross@ross.interwrx.com, martin@walnut.eas.asu.edu)
 * enables this driver to correctly print tiger.eps on a
 * Brother MFC6550MC Fax Machine.  Change to the Horizontal
 * Offset fixes incorrect page alignment at 300dpi in
 * Landscape mode with a2ps.
 */
#include "gdevprn.h"
/* The following line is used though these printers are not PCL printers*/
/* This is because we want the paper size access function */
/* (The 720 is a simple GDI printer) */
#include "gdevpcl.h"

/*
 * You may select a default resolution of  150 (for 730), 300, or
 * 600 DPI in the makefile, or an actual resolution on
 * the gs command line.
 *
 * If the preprocessor symbol A4 is defined, the default paper size is
 * the European A4 size; otherwise it is the U.S. letter size (8.5"x11").
 *
 * You may find the following test page useful in determining the exact
 * margin settings on your printer.  It prints four big arrows which
 * point exactly to the for corners of an A4 sized paper. Of course the
 * arrows cannot appear in full on the paper, and they are truncated by
 * the margins. The margins measured on the testpage must match those
 * in gdevdjet.c.  So the testpage indicates two facts: 1) the page is
 * not printed in the right position 2) the page is truncated too much
 * because the margins are wrong. Setting wrong margins in gdevdjet.c
 * will also move the page, so both facts should be matched with the
 * real world.

%!
        newpath
        0 0 moveto 144 72 lineto 72 144 lineto
        closepath fill stroke 0 0 moveto 144 144 lineto stroke

        595.27 841.88 moveto 451.27 769.88 lineto 523.27 697.88 lineto
        closepath fill stroke 595.27 841.88 moveto 451.27 697.88 lineto stroke

        0 841.88 moveto 144 769.88 lineto 72 697.88 lineto
        closepath fill stroke 0 841.88 moveto 144 697.88 lineto stroke

        595.27 0 moveto 451.27 72 lineto 523.27 144 lineto
        closepath fill stroke 595.27 0 moveto 451.27 144 lineto stroke

        /Helvetica findfont
        14 scalefont setfont
        100 600 moveto
        (This is an A4 testpage. The arrows should point exactly to the) show
        100 580 moveto
        (corners and the margins should match those given in gdev*.c) show
        showpage

 */

#define USE_POSSIBLY_FLAWED_COMPRESSION 1

/* Type definitions */
typedef struct {
  short width;                /* physical width of the paper */
  short height;               /* physical height of the paper */
}                 PaperFormat; /* Rep. of the charateristics of a sheet of paper */

typedef unsigned char Byte; /* Rep. of elementary data unit */

/*
 * Definition of a Helper structure to handle a list of commands
 */
typedef struct {
  Byte * data;
  short maxSize;
  short current;

} ByteList;

/*
 * Type for representing a summary of the previous lines
 *
 */

typedef struct {
  short  previousSize;
  Byte*  previousData;
  short  nbBlankLines;
  short  nbLinesSent;
  short  pageWidth;
  short  pageHeight;
  short  horizontalOffset;
  short  resolution;
} Summary;

/* Constants */

/* We need a boolean : true , we got it from gdevprn.h */

/* Other constants */
static const int DumpFinished = 0;
static const int DumpContinue = 1;
static const int HL7X0_LENGTH = 5; /* Length of a command to tell the size of the data to be sent to the printer*/
static void  makeCommandsForSequence(Byte     * pSource,
                                      short      length,
                                      ByteList * pCommandList,
                                      short      offset,
                                      Byte     * pCommandCount,
                                      short      rest);

/* Auxiliary Functions */

static int dumpPage(gx_device_printer * pSource,
                      Byte              * pLineTmp,
                      ByteList          * pCommandList,
                      Summary           * pSummary
                      );
static int initSummary(gx_device_printer* pdev, Summary * s,short pw, short ph, short resolution);

static void freeSummary(gx_device_printer* pdev, Summary * s);

static void resetPreviousData(Summary * s);

static void makeFullLine( Byte      * pCurrentLine,
                           Byte      * pPreviousLine,
                           short       lineWidth,
                           ByteList  * commandsList,
                           short       horizontalOffset
                           );

/*
 * Initialize a list of Bytes structure
 */
static void initByteList(ByteList *list, Byte *array, short maxSize,short initCurrent);
static void addByte(ByteList *list,Byte value );
static void addArray(ByteList *list, Byte *source, short nb);
static void addNBytes(ByteList * list, Byte value, short nb);
static Byte * currentPosition(ByteList * list);
static void addCodedNumber(ByteList * list, short number);
static int isThereEnoughRoom(ByteList * list, short biggest);
static short roomLeft(ByteList * list);
static void dumpToPrinter(ByteList * list,gp_file * printStream);

/* Real Print function */

static int hl7x0_print_page(gx_device_printer *, gp_file *, int, int, ByteList *);

/* Define the default, maximum resolutions. */
#ifdef X_DPI
#  define X_DPI2 X_DPI
#else
#  define X_DPI 300
#  define X_DPI2 600
#endif
#ifdef Y_DPI
#  define Y_DPI2 Y_DPI
#else
#  define Y_DPI 300
#  define Y_DPI2 600
#endif

#define LETTER_WIDTH 5100
#define LEFT_MARGIN  30
/* The following table is not actually used.... */
#if 0
static const PaperFormat tableOfFormats[] = {
    /*  0 P LETTER */ { 2550, 3300 },
    /*  1 P LEGAL  */ { 2550, 4200 },
    /*  2 P EXEC   */ { 2175, 3150 },
    /*  3 P A4(78) */ { 2480, 3507 },
    /*  4 P B5     */ { 2078, 2953 },
    /*  5 P A5     */ { 1754, 2480 },
    /*  6 P MONARC */ { 1162, 2250 },
    /*  7 P COM10  */ { 1237, 2850 },
    /*  8 P DL     */ { 1299, 2598 },
    /*  9 P C5     */ { 1913, 2704 },
    /* 10 P A4Long */ { 2480, 4783 },

    /* 11 L LETTER */ { 3300, 2550 },
    /* 12 L LEGAL  */ { 4200, 2550 },
    /* 13 L EXEC   */ { 3150, 2175 },
    /* 14 L A4     */ { 3507, 2480 },
    /* 15 L B5     */ { 2952, 2078 },
    /* 16 L A5     */ { 2480, 1754 },
    /* 17 L MONARC */ { 2250, 1162 },
    /* 18 L COM10  */ { 2850, 1237 },
    /* 19 L DL     */ { 2598, 1299 },
    /* 20 L C5     */ { 2704, 1913 },
    /* 21 L A4Long */ { 4783, 2480 }
};
#endif

/* Compute the maximum length of a compressed line */
static short MaxLineLength(short resolution){
return (((156 * resolution / 150 ) * 5 )/4) + 8;
}

/* Margins are left, bottom, right, top. */
/* Quotation from original gdevdjet.c */
/* from Frans van Hoesel hoesel@rugr86.rug.nl.  */
/* A4 has a left margin of 1/8 inch and at a printing width of
 * 8 inch this give a right margin of 0.143. The 0.09 top margin is
 * not the actual margin - which is 0.07 - but compensates for the
 * inexact paperlength which is set to 117 10ths.
 * Somebody should check for letter sized paper. I left it at 0.07".
 */

/* The A4 margins are almost good */
/* The one for Letter are those of the gdevdjet.c file... */
#define HL7X0_MARGINS_A4	0.1, 0.15, 0.07, 0.05
#define HL7X0_MARGINS_LETTER 0.275, 0.20, 0.25, 0.07

/* We round up the LINE_SIZE to a multiple of a ulong for faster scanning. */
#define W sizeof(word)

/* Printer types */

#define HL720    0
#define HL730    0 /* No difference */

/* The device descriptors */
static dev_proc_open_device(hl7x0_open);
static dev_proc_close_device(hl7x0_close);
static dev_proc_print_page(hl720_print_page);

/* Since the print_page doesn't alter the device, this device can print in the background */
static void
hl7x0_initialize_device_procs(gx_device *dev)
{
    gdev_prn_initialize_device_procs_mono_bg(dev);

    set_dev_proc(dev, open_device, hl7x0_open);
    set_dev_proc(dev, close_device, hl7x0_close);
}

const gx_device_printer far_data gs_hl7x0_device =
  prn_device(hl7x0_initialize_device_procs, "hl7x0",
        DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
        X_DPI, Y_DPI,
        0, 0, 0, 0,		/* margins filled in by hl7x0_open */
        1, hl720_print_page); /* The hl720 and hl730 can both use the same print method */

/* Open the printer, adjusting the margins if necessary. */

static int
hl7x0_open(gx_device *pdev)
{	/* Change the margins if necessary. */
        static const float m_a4[4] = { HL7X0_MARGINS_A4 };
        static const float m_letter[4] = { HL7X0_MARGINS_LETTER };
        const float *m =
          (gdev_pcl_paper_size(pdev) == PAPER_SIZE_A4 ? m_a4 : m_letter);

        gx_device_set_margins(pdev, m, true);
        return gdev_prn_open(pdev);
}

/* The orders sent are those provided in the Brother DOS example */
static int
hl7x0_close(gx_device *pdev)
{
    gx_device_printer *const ppdev = (gx_device_printer *)pdev;
    int code = gdev_prn_open_printer(pdev, 1);

    if (code < 0)
        return code;
    gp_fputs("@N@N@N@N@X", ppdev->file) ;
    return gdev_prn_close_printer(pdev);
}

/* ------ Internal routines ------ */

/* The HL 720 can compress*/
static int
hl720_print_page(gx_device_printer *pdev, gp_file *prn_stream)
{
        Byte prefix[] ={
   0x1B,'%','-','1','2','3','4','5','X'
  ,'@','P','J','L',0x0A                         /* set PJL mode */
  ,'@','P','J','L',' ','E','N','T','E','R',' '
  ,'L','A','N','G','U','A','G','E'
  ,' ','=',' ','H','B','P',0x0A                 /* set GDI Printer mode */
  ,'@','L', 0x0
   };
        ByteList initCommand;
        int x_dpi = pdev->x_pixels_per_inch;
        initByteList(&initCommand,
                     prefix,         /* Array */
                     sizeof(prefix), /* Total size */
                     sizeof(prefix) - 1); /* Leave one byte free since*/
        /* we need to add the following order at the end */
        addByte(&initCommand, (Byte) ((((600/x_dpi) >> 1) \
                                                  | (((600/x_dpi) >> 1) << 2))));
        /* Put the value of the used resolution into the init string */

        return hl7x0_print_page(pdev, prn_stream, HL720, 300,
               &initCommand);
}

/* Send the page to the printer.  For speed, compress each scan line, */
/* since computer-to-printer communication time is often a bottleneck. */
static int
hl7x0_print_page(gx_device_printer *pdev, gp_file *printStream, int ptype,
  int dots_per_inch, ByteList *initCommand)
{
  int code;
        /* UTILE*/
  /* Command for a formFeed (we can't use strings because of the zeroes...)*/
  Byte FormFeed[] = {'@','G',0x00,0x00,0x01,0xFF,'@','F'};
  ByteList formFeedCommand;
  /* Main characteristics of the page */
  int line_size       = gdev_mem_bytes_per_scan_line((gx_device *)pdev);
  int x_dpi = pdev->x_pixels_per_inch;
  /*  int y_dpi = pdev->y_pixels_per_inch; */
  int num_rows = dev_print_scan_lines(pdev);
  int result;
  int sizeOfBuffer   = MaxLineLength(x_dpi) + 30;
  Byte * storage      = (Byte *) gs_malloc(pdev->memory,
                                           sizeOfBuffer + line_size,
                                           1,
                                           "hl7x0_print_page");
        /*	bool dup = pdev->Duplex; */
        /* bool dupset = pdev->Duplex_set >= 0; */
        Summary pageSummary;
        ByteList commandsBuffer;
        if ( storage == 0 )	/* can't allocate working area */
                return_error(gs_error_VMerror);
        code = initSummary(pdev,
                    &pageSummary,
                    line_size,
                    num_rows,
                    x_dpi);
        if (code < 0) {
            gs_free(pdev->memory, (char *)storage, storage_size_words, 1, "hl7X0_print_page");
            return code;
        }
        initByteList(&commandsBuffer, storage, sizeOfBuffer,0 );
        /* PLUS A MOI */
        if ( pdev->PageCount == 0 )
          {
            /* Put out init string before first page. */
            dumpToPrinter(initCommand, printStream);		/* send init to printer */

        }

        do {
          result = dumpPage(pdev,
                            storage + sizeOfBuffer, /* The line buffer is after the dump buffer */
                            &commandsBuffer,
                            &pageSummary);
            if (result < 0)
                goto xit;
          dumpToPrinter(&commandsBuffer,printStream);

        } while (result == DumpContinue);	/* NB: at end  of page, result will be DumpFinished == 0 */

        /* end raster graphics and eject page */
        initByteList(&formFeedCommand,
                     FormFeed,          /* Array */
                     sizeof(FormFeed),  /* Size in bytes */
                     sizeof(FormFeed)); /* First free byte */
        dumpToPrinter(&formFeedCommand, printStream);

xit:
        /* free temporary storage */
        freeSummary(pdev, &pageSummary);
        gs_free(pdev->memory, (char *)storage, storage_size_words, 1, "hl7X0_print_page");

        return result; /* If we reach this line, it means there was no error */
}

/*
 * Useful auxiliary declarations
 *
 */

static short stripTrailingBlanks(Byte * line, short length){
  short positionOfFirstZero = length - 1;
  while (positionOfFirstZero > 0) {
    if (line[positionOfFirstZero] != 0) {
      return positionOfFirstZero + 1;
    }
    positionOfFirstZero -- ;
  }
  return 0;
}

/*
 * Changed the horizontalOffset function 1/17/00 Ross Martin.
 * ross@ross.interwrx.com or martin@walnut.eas.asu.edu
 *
 * The equation used to muliply pixWidth by resolution/600
 * also.  This didn't work right at resolution 300; it caused
 * landscape pages produced by a2ps to be half off the
 * page, when they were not at 600dpi or on other
 * devices.  I'm not sure the equation below is exactly
 * correct, but it now looks to be pretty close visually,
 * and works correctly at 600dpi and 300dpi.
 */
static short horizontalOffset(short pixWidth,
                              short pixOffset,
                              short resolution){
return (((LETTER_WIDTH * resolution/600 - pixWidth) + pixOffset * 2) + 7) / 8;

}

/*
 * First values in a Summary
 */
static int initSummary(gx_device_printer* pdev, Summary * s,short pw, short ph, short resolution){
  s->previousSize = -1 ;
  s->previousData = gs_malloc(pdev->memory, pw, 1, "initSummary");
  s->nbBlankLines = 1;
  s->nbLinesSent = 0;
  s->pageWidth = pw; /* In Bytes */
  s->pageHeight = ph;
  s->horizontalOffset = horizontalOffset( pw * 8,LEFT_MARGIN, resolution) ;
  s->resolution = resolution;
  if (!s->previousData) {
    return_error(gs_error_VMerror);
  }
  return 0;
}

static void freeSummary(gx_device_printer* pdev, Summary * s) {
  gs_free(pdev->memory, s->previousData, s->pageWidth, 1, "freeSummary");
}

/*
 * The previous line was blank, so we need to clean the corresponding array
 */
static void resetPreviousData(Summary * s){
 memset(s->previousData,0,s->pageWidth);
}

/*
 * dumpPage :
 *
 */
static int dumpPage(gx_device_printer * pSource,
                      Byte              * pLineTmp,
                      ByteList          * pCommandList,
                      Summary           * pSummary
                      ){

  /* Declarations */
  Byte * pSaveCommandStart;
  short  lineNB;
  short usefulLength;
  short tmpLength;
  int code = 0;

  /* Initializations */
  /* Make room for size of commands buffer */
  pSaveCommandStart = currentPosition(pCommandList);
  addNBytes(pCommandList,0,HL7X0_LENGTH);
  /* pSource += pSummary->nbLinesSent * pSummary->pageWidth;*/
  /* Process all possible Lines */
  for (lineNB = pSummary->nbLinesSent /*ERROR? + nbBlankLines */ ;
       lineNB < pSummary->pageHeight ; lineNB ++ ) {
    /* Fetch the line and put it into the buffer */
    code = gdev_prn_copy_scan_lines(pSource,
                             lineNB,
                             pLineTmp,
                             pSummary->pageWidth);
    if (code < 0)
        return code;

    usefulLength =  stripTrailingBlanks(pLineTmp,pSummary->pageWidth);
    if (usefulLength != 0) {

      /* The line is not blank */
      /* Get rid of the precedent blank lines */
      if (pSummary->nbBlankLines != 0) {
        if ( isThereEnoughRoom( pCommandList, pSummary->nbBlankLines )   ) {

          addNBytes(pCommandList,0xff,pSummary->nbBlankLines);
          pSummary->nbBlankLines = 0;

        }
        else {

          short availableRoom = roomLeft(pCommandList);
          addNBytes(pCommandList,0xff,availableRoom);
          pSummary->nbBlankLines -= availableRoom;

          break ; /* We have no more room */

        }

        resetPreviousData(pSummary); /* Make sure there are zeroes for the previous line */
        pSummary->previousSize = 0; /* The previous line was empty */

      }

      /* Deal with the current line */
      if (!isThereEnoughRoom(pCommandList,MaxLineLength(pSummary->resolution))){
        break; /* We can process this line */
      }

      if (pSummary->previousSize > usefulLength){
        tmpLength = pSummary->previousSize;
      }
      else {
        tmpLength = usefulLength;
      }

      if (pSummary->previousSize == -1 ) {/* This is the first line */

        Byte *save = currentPosition(pCommandList);
        addByte(pCommandList,0); /* One byte for the number of commands */

        makeCommandsForSequence(pLineTmp,
                                tmpLength,
                                pCommandList,
                                pSummary->horizontalOffset,
                                save,
                                0);
      }
      else { /*There is a previous line */

        makeFullLine(pLineTmp,
                     pSummary->previousData,
                     tmpLength,
                     pCommandList,
                     pSummary->horizontalOffset);
      }
      /* The present line will soon be considered as "previous" */
      pSummary->previousSize = tmpLength;
      /* Update the data representing the line will soon be the "previous line" */
      memcpy(pSummary->previousData,pLineTmp,tmpLength);

    }
    else { /* the current line is blank */
      pSummary->nbBlankLines++;
    }

  /* And one more line */
    pSummary->nbLinesSent ++;
  }

  if (pCommandList->current > HL7X0_LENGTH){
    short size = pCommandList->current - HL7X0_LENGTH;
    *(pSaveCommandStart++)  = '@';
    *(pSaveCommandStart++)  = 'G';
    *(pSaveCommandStart++)  = 0;                    /* was: (Byte) (size >> 16) as per hte spec, but shorts cannot exceed 16 bits! */
    *(pSaveCommandStart++)  = (Byte) (size >> 8);
    *(pSaveCommandStart++)  = (Byte) (size);
  }
  else {  /* We only met blank lines and reached the end of the page */
    pCommandList->current = 0;
  }
  if (lineNB == pSummary->pageHeight){
    return DumpFinished;
  }
  else {
    return DumpContinue;
  }
}

/*
 *  makeFullLine :
 *  process an arbitrary line for which a former line is available
 *  The line will be split in sequences that are different from the
 * corresponding ones of the previous line. These sequences will be processed
 * by makeCommandsOfSequence.
 */

static void makeFullLine( Byte      * pCurrentLine,
                           Byte      * pPreviousLine,
                           short       lineWidth,
                           ByteList  * commandsList,
                           short       horizontalOffset
                           ){
  /* Declarations */
  Byte *pPreviousTmp;
  Byte *pCurrentTmp;
  Byte *pNumberOfCommands;
  int loopCounter;
  short remainingWidth;
  Byte *pStartOfSequence;
  /*****************/
  /* Special cases */
  /*****************/

  /* I believe this situation to be impossible */
  if (lineWidth <= 0) {
    addByte(commandsList,0xff);
    return;
  }

  /*******************/
  /* Initializations */
  /*******************/

  pNumberOfCommands = currentPosition(commandsList); /* Keep a pointer to the number of commands */
  addByte(commandsList,0); /* At the moment there are 0 commands */

  pPreviousTmp = pPreviousLine;
  pCurrentTmp = pCurrentLine;

  /* Build vector of differences with a Xor */

  for (loopCounter = lineWidth ;  0 < loopCounter ; loopCounter -- )
    *pPreviousTmp++ ^= *pCurrentTmp++;

  /* Find sequences that are different from the corresponding (i.e. vertically aligned)
   * one of the previous line. Make commands for them.
   */

  pStartOfSequence = pPreviousLine;
  remainingWidth = lineWidth;

  while (true) {

    /*
     * Disabled line-to-line compression, 1/17/00 Ross Martin
     * ross@ross.interwrx.com and/or martin@walnut.eas.asu.edu
     *
     * The compression here causes problems printing tiger.eps.
     * The problem is vertical streaks.  The printer I'm printing
     * to is a Brother MFC6550MC Fax Machine, which may be
     * slightly different from the hl720 and hl730.  Note that
     * this fax machine does support HP LaserJet 2p emulation,
     * but in order to enable it I believe one needs special
     * setup from a DOS program included with the printer.  Thus,
     * the hl7x0 driver seems a better choice.  In any case,
     * on the MFC6550MC, some files print fine with compression
     * turned on, but others such as tiger.eps print with streaks.
     * disabling the compression fixes the problem, so I haven't
     * looked any further at the cause.  It may be that the
     * compression is correct for the hl720 and hl730, and only
     * different for the MFC6550MC, or it may be that tiger.eps
     * won't print correctly with compression enabled on any
     * of these.  It may be that the problem is only with color
     * and/or grayscale prints.  YMMV.  I don't think it likely
     * that turning off compression will cause problems with
     * other printers, except that they may possibly print slower.
     */

#ifdef USE_POSSIBLY_FLAWED_COMPRESSION
    /* Count and skip bytes that are not "new" */
    while (true) {
      if (remainingWidth == 0)  /* There is nothing left to do */
        {
          return;
        }
      if (*pStartOfSequence != 0)
        break;
      pStartOfSequence ++;
      horizontalOffset ++; /* the offset takes count of the bytes that are not "new" */
      --remainingWidth;
    }
#endif

    pPreviousTmp = pStartOfSequence + 1; /* The sequence contains at least this byte */
    --remainingWidth;

    /* Find the end of the sequence of "new" bytes */

#ifdef USE_POSSIBLY_FLAWED_COMPRESSION
    while (remainingWidth != 0 && *pPreviousTmp != 0) {
      ++pPreviousTmp; /* Enlarge the sequence Of new bytes */
      --remainingWidth;
    }
#else
   pPreviousTmp += remainingWidth;
   remainingWidth = 0;
#endif

    makeCommandsForSequence(pCurrentLine + (pStartOfSequence - pPreviousLine),
                             pPreviousTmp - pStartOfSequence,
                             commandsList,
                             horizontalOffset,
                             pNumberOfCommands,
                             remainingWidth);
    if (*pNumberOfCommands == 0xfe   /* If the number of commands has reached the maximum value */
        ||                           /* or */
        remainingWidth == 0 )        /* There is nothing left to process */
    {
      return;
    }

    pStartOfSequence = pPreviousTmp + 1; /* We go on right after the sequence of "new" bytes */
    horizontalOffset = 1;
    --remainingWidth;
  } /* End of While */

} /* End of makeFullLine */

/*
 *  Declarations of functions that are defined further in the file
 */
static void makeSequenceWithoutRepeat(
                                  Byte     * pSequence,
                                  short      lengthOfSequence,
                                  ByteList * pCommandList,
                                  short      offset             );

static void makeSequenceWithRepeat(
                                  Byte     * pSequence,
                                  short      lengthOfSequence,
                                  ByteList * pCommandList,
                                  short      offset             );

/*
 * makeCommandsForSequence :
 * Process a sequence of new bytes (i.e. different from the ones on the former line)
 */

static void makeCommandsForSequence(Byte     * pSource,
                                     short      length,
                                     ByteList * pCommandList,
                                     short      offset,
                                     Byte     * pNumberOfCommands,
                                     short      rest)         {
  /* Declarations */
  Byte * pStartOfSequence;
  Byte * pEndOfSequence;
  short  remainingLength = length - 1;

  pStartOfSequence = pSource;
  pEndOfSequence = pStartOfSequence + 1;
  /*
   * Process the whole "new" Sequence that is divided into
   * repetitive and non-repetitive sequences.
   */
  while (true) {

    /* If we have already stored too many commands, make one last command with
     * everything that is left in the line and return.
     */
    if (*pNumberOfCommands == 0xfd) {
      makeSequenceWithoutRepeat(pStartOfSequence,
                        1 + remainingLength + rest,
                        pCommandList,
                        offset);
      ++*pNumberOfCommands;
      return;
    }

    /* Start with a sub-sequence without byte-repetition */
    while (true) {
      /* If we have completed the last subsequence */
      if (remainingLength == 0) {
        makeSequenceWithoutRepeat(pStartOfSequence,
                     pEndOfSequence - pStartOfSequence,
                     pCommandList,
                     offset);
        ++*pNumberOfCommands;
        return;
      }
      /* If we have discovered a repetition */
      if (*pEndOfSequence == *(pEndOfSequence - 1)) {
        break;
      }
      ++ pEndOfSequence; /* The subsequence is bigger*/
      --remainingLength;
    }
    /* If this is a sequence without repetition */
    if (pStartOfSequence != pEndOfSequence - 1) {
      makeSequenceWithoutRepeat(pStartOfSequence,
                                (pEndOfSequence - 1) - pStartOfSequence,
                                pCommandList,
                                offset);
      ++*pNumberOfCommands;
      offset = 0;
      pStartOfSequence = pEndOfSequence - 1;

      /* If we have too many commands */
      if (*pNumberOfCommands == 0xfd) {
        makeSequenceWithoutRepeat(pStartOfSequence,
                                  1 + remainingLength + rest,
                                  pCommandList,
                                  offset);
        ++*pNumberOfCommands;
        return;
      }
    } /* End If */

    /*
     * Process a subsequence that repeats the same byte
     */
    while (true) {
      /* If there is nothing left to process */
      if (remainingLength == 0) {
        makeSequenceWithRepeat(pStartOfSequence,
                               pEndOfSequence - pStartOfSequence,
                               pCommandList,
                               offset);
        ++*pNumberOfCommands;
        return;
      }
      /* If we find a different byte */
      if (*pEndOfSequence != *pStartOfSequence){
        break;
      }
      ++pEndOfSequence; /* The subsequence is yet bigger */
      --remainingLength;
    } /* End of While */
      makeSequenceWithRepeat(pStartOfSequence,
                             pEndOfSequence - pStartOfSequence,
                             pCommandList,
                             offset);
      ++*pNumberOfCommands;
      offset = 0;   /* The relative offset between two subsequences is 0 */
      pStartOfSequence = pEndOfSequence ++ ; /* we loop again from the end of this subsequence */
      --remainingLength;

  } /* End of While */

} /* End makeCommandsForSequence */

/*
 * makeSequenceWithoutRepeat
 */
static void makeSequenceWithoutRepeat(
                                  Byte     * pSequence,
                                  short      lengthOfSequence,
                                  ByteList * pCommandList,
                                  short      offset             ){
  /*
   *   Constant definitions
   */
  static const short MAX_OFFSET         = 15;
  static const short POSITION_OF_OFFSET = 3;
  static const short MAX_LENGTH         =  7;

  Byte tmpFirstByte = 0;
  Byte * pSaveFirstByte;
  short reducedLength = lengthOfSequence - 1; /* Length is alway higher than 1
                                                 Therefore a reduced value is stored
                                                 */
  /* Initialization */

  pSaveFirstByte = currentPosition(pCommandList);
  addByte( pCommandList, 0 /* Dummy value */);

  /* Computations */

  if (offset >= MAX_OFFSET) {
    addCodedNumber(pCommandList,offset - MAX_OFFSET);
    tmpFirstByte |= MAX_OFFSET << POSITION_OF_OFFSET;
  }
  else
    tmpFirstByte |= offset << POSITION_OF_OFFSET;

  if (reducedLength >= MAX_LENGTH) {
    addCodedNumber(pCommandList,reducedLength - MAX_LENGTH);
    tmpFirstByte |= MAX_LENGTH ;
  }
  else
    tmpFirstByte |= reducedLength ;
  /* Add a copy of the source sequence */

  addArray(pCommandList, pSequence, lengthOfSequence);

  /* Store the computed value of the first byte */

  *pSaveFirstByte = tmpFirstByte;

  return ;
} /* End of makeSequenceWithoutRepeat */

/*
 * makeSequenceWithRepeat
 */
static void makeSequenceWithRepeat(
                                  Byte     * pSequence,
                                  short      lengthOfSequence,
                                  ByteList * pCommandList,
                                  short      offset             ){
  /*
   *   Constant definitions
   */
  static const short MAX_OFFSET         = 3;
  static const short POSITION_OF_OFFSET = 5;
  static const short MAX_LENGTH         =  31;

  Byte tmpFirstByte = 0x80;
  Byte * pSaveFirstByte;
  short reducedLength = lengthOfSequence - 2; /* Length is always higher than 2
                                                 Therefore a reduced value is stored
                                                 */
  /* Initialization */

  pSaveFirstByte = currentPosition(pCommandList);
  addByte( pCommandList, 0 /* Dummy value */);

  /* Computations */

  if (offset >= MAX_OFFSET) {
    addCodedNumber(pCommandList, offset - MAX_OFFSET);
    tmpFirstByte |= MAX_OFFSET << POSITION_OF_OFFSET;
  }
  else
    tmpFirstByte |= offset << POSITION_OF_OFFSET;

  if (reducedLength >= MAX_LENGTH) {
    addCodedNumber(pCommandList,reducedLength - MAX_LENGTH);
    tmpFirstByte |= MAX_LENGTH ;
  }
  else
    tmpFirstByte |= reducedLength ;
  /* Add a copy the byte that is repeated throughout the sequence */

  addByte(pCommandList, *pSequence );

  /* Store the computed value of the first byte */

  *pSaveFirstByte = tmpFirstByte;

  return ;
} /* End of makeSequenceWithRepeat*/

/*
 * Initialize a list of Bytes structure
 */
static void initByteList(ByteList *list, Byte *array, short maxSize, short initCurrent) {
  list->current = initCurrent;
  list->maxSize = maxSize;
  list->data = array;
}

/*
 * Add a Byte to a list of Bytes
 */
static void addByte(ByteList *list,Byte value ) {
 if (list->current < list->maxSize)
  list->data[list->current++] = value;
 else
   eprintf("Could not add byte to command\n");
}

/*
 * Add a copy of an array to a list of Bytes
 */

static void addArray(ByteList *list, Byte *source, short nb){
  if (list->current <= list->maxSize - nb)
  {
    memcpy(list->data + list->current, source , (size_t) nb);
    list->current += nb;
  }
  else
    eprintf("Could not add byte array to command\n");
}

/*
 * Add N bytes to a list of Bytes
 */

static void addNBytes(ByteList * list, Byte value, short nb){
  int i;
  if (list->current <= list->maxSize - nb)
  {
    for (i = list->current ; i < (list->current + nb) ; i++)
      {
        list->data[i] = value;
      }
    list->current += nb;
  }
  else
    eprintf1("Could not add %d bytes to command\n",nb);
}

/*
 * Get pointer to the current byte
 */
static Byte * currentPosition(ByteList * list) {
  return &(list->data[list->current]);
}

/*
 * add a number coded in the following way :
 * q bytes with 0xff value
 * 1 byte with r value
 * where q is the quotient of the number divided by 0xff and r is the
 * remainder.
 */
static void addCodedNumber(ByteList * list, short number){
 short q = number / 0xff;
 short r = number % 0xff;

 addNBytes(list, 0xff, q);
 addByte(list,r);

}

/*
 * See if there is enough room for a set of commands of size biggest
 *
 */

static int isThereEnoughRoom(ByteList * list, short biggest){
  return ((list->maxSize-list->current) >= biggest);
}
/*
 * Tell how much room is left
 */
static short roomLeft(ByteList * list){
  return list->maxSize - list->current;
}
/*
 * Dump all commands to the printer and reset the structure
 *
 */
static void dumpToPrinter(ByteList * list,gp_file * printStream){
  short loopCounter;
  /* Actual dump */
  /* Please note that current is the first empty byte */
  for (loopCounter = 0; loopCounter < list->current; loopCounter++)
    {
      gp_fputc(list->data[loopCounter],printStream);
    }

  /* Reset of the ByteList */
  list->current = 0;
}
