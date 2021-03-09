/* Copyright (C) 2001-2020 Artifex Software, Inc.
   All Rights Reserved.

   This software is provided AS-IS with no warranty, either express or
   implied.

   This software is distributed under license and may not be copied,
   modified or distributed except as expressly authorized under the terms
   of the license contained in the file LICENSE in this distribution.

   Refer to licensing information at http://www.artifex.com or contact
   Artifex Software, Inc.,  1305 Grant Avenue - Suite 200, Novato,
   CA 94945, U.S.A., +1(415)492-9861, for further information.
*/


/* combined internal header for the XPS interpreter */

#include "memory_.h"
#include "math_.h"
#include "string_.h"
#include <stdlib.h>
#include <ctype.h> /* for toupper() */

/* Include zlib early to avoid offsetof redef problems on windows */
#include "zlib.h"

#include "stdint_.h"

#include "gp.h"

#include "gsgc.h"
#include "gstypes.h"
#include "gsstate.h"
#include "gsmatrix.h"
#include "gscoord.h"
#include "gsmemory.h"
#include "gsparam.h"
#include "gsdevice.h"
#include "scommon.h"
#include "gdebug.h"
#include "gserrors.h"
#include "gspaint.h"
#include "gspath.h"
#include "gsimage.h"
#include "gscspace.h"
#include "gsptype1.h"
#include "gscolor2.h"
#include "gscolor3.h"
#include "gsutil.h"
#include "gsicc.h"

#include "gstrans.h"

#include "gxpath.h"     /* gsshade.h depends on it */
#include "gxfixed.h"    /* gsshade.h depends on it */
#include "gxmatrix.h"   /* gxtype1.h depends on it */
#include "gsshade.h"
#include "gsfunc.h"
#include "gsfunc3.h"    /* we use stitching and exponential interp */

#include "gxfont.h"
#include "gxchar.h"
#include "gxcolor2.h" /* Required for definition of gs_pattern1_instance_t */
#include "gxtype1.h"
#include "gxfont1.h"
#include "gxfont42.h"
#include "gxfcache.h"
#include "gxgstate.h"

#include "gzstate.h"
#include "gzpath.h"
#include "gzcpath.h"

#include "gsicc_manage.h"
#include "gscms.h"
#include "gsicc_cache.h"
#include "gxpcolor.h"
#include "gxdevsop.h"       /* For special ops */
#include "gstext.h"         /* for gs_text_enum_t */

#include "gxtmap.h"
#include "gxfmap.h"

#ifndef PDF_CONTEXT
#define PDF_CONTEXT

#define BUF_SIZE 2048

#include "pdf_types.h"

/*
 * The interpreter context.
 */

/* Warnings and errors. The difference between a warning and an error is that we use a warning where
 * the file is technically illegal but we can be certain as to the real intent. At the time of writing
 * the only case is also a perfect example; the use of an inline image filter abbreviation (eg A85)
 * on a stream or object which is not an inline image. Although technically incorrect, its obvious
 * how to deal with this.
 */
typedef enum pdf_error_flag_e {
    E_PDF_NOERROR = 0,
    E_PDF_NOHEADER = 1,
    E_PDF_NOHEADERVERSION = E_PDF_NOHEADER << 1,
    E_PDF_NOSTARTXREF = E_PDF_NOHEADER << 2,
    E_PDF_BADSTARTXREF = E_PDF_NOHEADER << 3,
    E_PDF_BADXREFSTREAM = E_PDF_NOHEADER << 4,
    E_PDF_BADXREF = E_PDF_NOHEADER << 5,
    E_PDF_SHORTXREF = E_PDF_NOHEADER << 6,
    E_PDF_MISSINGENDSTREAM = E_PDF_NOHEADER << 7,
    E_PDF_UNKNOWNFILTER = E_PDF_NOHEADER << 8,
    E_PDF_MISSINGWHITESPACE = E_PDF_NOHEADER << 9,
    E_PDF_MALFORMEDNUMBER = E_PDF_NOHEADER << 10,
    E_PDF_UNESCAPEDSTRING = E_PDF_NOHEADER << 11,
    E_PDF_BADOBJNUMBER = E_PDF_NOHEADER << 12,
    E_PDF_MISSINGENDOBJ = E_PDF_NOHEADER << 13,
    E_PDF_TOKENERROR = E_PDF_NOHEADER << 14,
    E_PDF_KEYWORDTOOLONG = E_PDF_NOHEADER << 15,
    E_PDF_BADPAGETYPE = E_PDF_NOHEADER << 16,
    E_PDF_CIRCULARREF = E_PDF_NOHEADER << 17,
    E_PDF_UNREPAIRABLE = E_PDF_NOHEADER << 18,
    E_PDF_REPAIRED = E_PDF_NOHEADER << 19,
    E_PDF_BADSTREAM = E_PDF_NOHEADER << 20,
    E_PDF_MISSINGOBJ = E_PDF_NOHEADER << 21,
    E_PDF_BADPAGEDICT = E_PDF_NOHEADER << 22,
    E_PDF_OUTOFMEMORY = E_PDF_NOHEADER << 23,
    E_PDF_PAGEDICTERROR = E_PDF_NOHEADER << 24,
    E_PDF_STACKUNDERFLOWERROR = E_PDF_NOHEADER << 25,
    E_PDF_BADSTREAMDICT = E_PDF_NOHEADER << 26,
    E_PDF_INHERITED_STREAM_RESOURCE = E_PDF_NOHEADER << 27,
    E_PDF_DEREF_FREE_OBJ = E_PDF_NOHEADER << 28
} pdf_error_flag;

typedef enum pdf_warning_flag_e {
    W_PDF_NOWARNING = 0,
    W_PDF_BAD_XREF_SIZE = 1,
    W_PDF_BAD_INLINEFILTER = W_PDF_BAD_XREF_SIZE << 1,
    W_PDF_BAD_INLINECOLORSPACE = W_PDF_BAD_INLINEFILTER << 1,
    W_PDF_BAD_INLINEIMAGEKEY = W_PDF_BAD_INLINECOLORSPACE << 1,
    W_PDF_IMAGE_ERROR = W_PDF_BAD_INLINEIMAGEKEY << 1,
    W_PDF_BAD_IMAGEDICT = W_PDF_IMAGE_ERROR << 1,
    W_PDF_TOOMANYQ = W_PDF_BAD_IMAGEDICT << 1,
    W_PDF_TOOMANYq = W_PDF_TOOMANYQ << 1,
    W_PDF_STACKGARBAGE = W_PDF_TOOMANYq << 1,
    W_PDF_STACKUNDERFLOW = W_PDF_STACKGARBAGE << 1,
    W_PDF_GROUPERROR = W_PDF_STACKUNDERFLOW << 1,
    W_PDF_OPINVALIDINTEXT = W_PDF_GROUPERROR << 1,
    W_PDF_NOTINCHARPROC = W_PDF_GROUPERROR << 1,
    W_PDF_NESTEDTEXTBLOCK = W_PDF_NOTINCHARPROC << 1,
    W_PDF_ETNOTEXTBLOCK = W_PDF_NESTEDTEXTBLOCK << 1,
    W_PDF_TEXTOPNOBT = W_PDF_ETNOTEXTBLOCK  << 1,
    W_PDF_DEGENERATETM = W_PDF_TEXTOPNOBT << 1,
    W_PDF_BADICC_USE_ALT = W_PDF_DEGENERATETM << 1,
    W_PDF_BADICC_USECOMPS = W_PDF_BADICC_USE_ALT << 1,
    W_PDF_BADTRSWITCH = W_PDF_BADICC_USECOMPS << 1,
    W_PDF_BADSHADING = W_PDF_BADTRSWITCH << 1,
    W_PDF_BADPATTERN = W_PDF_BADSHADING << 1,
    W_PDF_NONSTANDARD_OP = W_PDF_BADPATTERN << 1,
    W_PDF_NUM_EXPONENT = W_PDF_NONSTANDARD_OP << 1,
    W_PDF_STREAM_HAS_CONTENTS = W_PDF_NUM_EXPONENT << 1,
    W_PDF_STREAM_BAD_DECODEPARMS = W_PDF_STREAM_HAS_CONTENTS << 1,
    W_PDF_MASK_ERROR = W_PDF_STREAM_BAD_DECODEPARMS << 1
} pdf_warning_flag;

typedef enum pdf_crypt_filter_e {
    CRYPT_NONE,     /* Not an encrypted file */
    CRYPT_IDENTITY, /* Encrypted file, but no encryption on this object type */
    CRYPT_V1,     /* 40-bit RC4 */
    CRYPT_V2,     /* 128-bit RC4 */
    CRYPT_AESV2,  /* 128-bit AES */
    CRYPT_AESV3,  /* 256-bit AES */
} pdf_crypt_filter;


typedef enum pdf_overprint_control_e {
    PDF_OVERPRINT_ENABLE = 0,/* Default */
    PDF_OVERPRINT_DISABLE,
    PDF_OVERPRINT_SIMULATE
} pdf_overprint_control_t;

#define INITIAL_STACK_SIZE 32
#define MAX_STACK_SIZE 524288
#define MAX_OBJECT_CACHE_SIZE 200
#define INITIAL_LOOP_TRACKER_SIZE 32

typedef struct pdf_transfer_s {
    gs_mapping_proc proc;	/* typedef is in gxtmap.h */
    frac values[transfer_map_size];
} pdf_transfer_t;

/* Items we want preserved around content stream executions */
typedef struct stream_save_s {
    gs_offset_t stream_offset;
    int gsave_level;
    int stack_count;
    gs_matrix intial_CTM;
    int group_depth;
} stream_save;

typedef struct name_entry_s {
    char *name;
    int len;
    unsigned int index;
    void *next;
} pdfi_name_entry_t;

typedef struct cmd_args_s {
    /* These are various command line switches, the list is not yet complete */
    int first_page;             /* -dFirstPage= */
    int last_page;              /* -dLastPage= */
    bool pdfdebug;
    bool pdfstoponerror;
    bool pdfstoponwarning;
    bool notransparency;
    bool nocidfallback;
    bool no_pdfmark_outlines; /* can be overridden to true if multi-page output */
    bool no_pdfmark_dests;    /* can be overridden to true if multi-page output */
    bool pdffitpage;
    bool usecropbox;
    bool useartbox;
    bool usebleedbox;
    bool usetrimbox;
    bool printed;
    bool showacroform;
    bool showannots;
    bool dopdfmarks;
    bool preserveannots;
    bool nouserunit;
    bool renderttnotdef;
    bool pdfinfo;
    pdf_overprint_control_t overprint_control;     /* Overprint -- enabled, disabled, simulated */
    char *PageList;
} cmd_args_t;

typedef struct encryption_state_s {
    /* Encryption, passwords and filter details */
    bool is_encrypted;
    int V;
    int Length;
    char *Password;
    int PasswordLen;
    int R;
    /* Revision 1-4 have O and E being 32 bytes, revision 5 and 6 48 bytes */
    char O[48];
    char U[48];
    /* OE and UE are used for revision 5 and 6 encryption */
    char OE[32];
    char UE[32];
    int P;
    pdf_string *EKey;
    bool EncryptMetadata;
    pdf_crypt_filter StrF;
    pdf_crypt_filter StmF;
    /* decrypting strings is complicated :-(
     * Streams are easy, because they can't be in compressed ObjStms, and they
     * have to be indirect objects. Strings can be indirect references or directly
     * defined, can be in compressed ObjStms and can appear inside content streams.
     * When they are in content streams we don't decrypt them, because the *stream*
     * was already decrypted. So when strings are directly or indirectly defined,
     * and *NOT* defined as part of a content stream, and not in an Objstm, we
     * need to decrypt them. We can handle the checking for ObjStm in the actual
     * decryption routine, where we also handle picking out the object number of the
     * enclosing parent, if its a directly defined string, but we cannot tell
     * whether we are executing a content stream or not, so we need to know that. This
     * flag is set whenever we are executing a content stream, it is temporarily reset
     * by pdfi_dereference() because indirect references can't appear in a content stream
     * so we know we need to decrypt any strings that are indirectly referenced. Note that
     * Form handling needs to set this flag for the duration of a Form content stream,
     * because we can execute Forms outside a page context (eg Annotations).
     */
    bool decrypt_strings;
} encryption_state_t;

typedef struct page_state_s {
    /* Page level PDF objects */
    pdf_dict *CurrentPageDict;      /* Last-ditch resource lookup */
    /* Page leve 'Default' transfer functions, black generation and under colour removal */
    pdf_transfer_t DefaultTransfers[4];
    pdf_transfer_t DefaultBG;
    pdf_transfer_t DefaultUCR;
    /* This tracks whether the current page uses transparency features */
    bool has_transparency;
    /* This tracks how many spots are on the current page */
    int num_spots;
    /* Does this page need overprint support? */
    bool needs_OP;
    /* Does this page have OP=true in ExtGState? */
    bool has_OP;
    /* Are we simulating overprint on this page? */
    bool simulate_op;
    double Size[4];
    double UserUnit;
} page_state_t;

typedef struct text_state_s {
    /* we need the text enumerator in order to call gs_text_setcharwidth() for d0 and d1 */
    gs_text_enum_t *current_enum;
    /* Detect if we are inside a text block at any time. Nested text blocks are illegal and certain
     * marking operations are illegal inside text blocks. We also manipulate this when rendering
     * type 3 BuildChar procedures, as those marking operations are legal in a BuildChar, even
     * when we are in a text block.
     */
    int BlockDepth;
    /* This is to determine if we get Type 3 Charproc operators (d0 and d1) outside
     * a Type 3 BuildChar.
     */
    bool inside_CharProc;
    /* We need to know if we're in a type 3 CharProc which has executed a 'd1' operator.
     * Colour operators are technically invalid if we are in a 'd1' context and we must
     * ignore them.
     */
    bool CharProc_is_d1;
} text_state_t;

typedef struct device_state_s {
    /* Parameters/capabilities of the selected device */
    /* Needed to determine whether we need to reset the device to handle any spots
     * and whether we need to prescan the PDF file to determine how many spot colourants
     * (if any) are used in the file.
     */
    bool spot_capable;
    /* for avoiding charpath with pdfwrite */
    bool preserve_tr_mode;
    /* Are SMask's preserved by device (pdfwrite) */
    bool preserve_smask;
    bool ForOPDFRead;
    bool pdfmark;
    /* These are derived from the device parameters rather than extracted from the device */
    /* But this is a convenient place to keep them. */
    /* Does current output device handle pdfmark */
    bool writepdfmarks;
    /* Should annotations be preserved or marked for current output device? */
    bool annotations_preserved;
} device_state_t;

typedef struct pdf_context_s
{
    void *instance;
    gs_memory_t *memory;

    /* command line argument storage */
    cmd_args_t args;

    /* Encryption state */
    encryption_state_t encryption;

    /* Text and text state parameters */
    text_state_t text;

    /* The state of the current page being processed */
    page_state_t page;

    device_state_t device_state;


    /* PDF interpreter state */

    /* State for handling the wacky W and W* operators */
    bool clip_active;
    bool do_eoclip;

    /* Doing a high level form for pdfwrite (annotations) */
    bool PreservePDFForm;

    /* Optional things from Root */
    pdf_dict *OCProperties;

    /* Optional/Marked Content stuff */
    void *OFFlevels;
    uint64_t BMClevel;

    /* Bitfields recording whether any errors or warnings were encountered */
    pdf_error_flag pdf_errors;
    pdf_warning_flag pdf_warnings;

    /* We need a gs_font_dir for gs_definefotn() */
    gs_font_dir * font_dir;
    /* Obviously we need a graphics state */
    gs_gstate *pgs;
    /* This is currently used for Patterns, but I suspect needs to be changed to use
     * 'the enclosing context'
     */
    gs_gstate *DefaultQState;


    /* The input PDF filename and the stream for it */
    char *filename;
    pdf_c_stream *main_stream;

    /* Length of the main file */
    gs_offset_t main_stream_length;
    /* offset to the xref table */
    gs_offset_t startxref;

    /* Track whether file is a hybrid. Initially prefer XRefStm but
     * if we fail to read the structure using an XRefStm, try again
     * using the xref
     */
    bool prefer_xrefstm;
    bool is_hybrid;
    /* If we've already repaired the file once, and it still fails, don't try to repair it again */
    bool repaired;

    /* The HeaderVersion is the declared version from the PDF header, but this
     * can be overridden by later trailer dictionaries, so the FinalVersion is
     * the version as finally read from the file. Note we don't currently use
     * these for anything, we might in future emit warnings if PDF files use features
     * inconsistent with the FinalVersion.
     */
    float HeaderVersion, FinalVersion;

    /* Document level PDF objects */
    xref_table_t *xref_table;
    pdf_dict *Trailer;
    pdf_dict *Root;
    pdf_dict *Info;
    pdf_dict *PagesTree;
    uint64_t num_pages;
    uint32_t *page_array; /* cache of page dict object_num's for pdfmark Dest */


    /* Interpreter level PDF objects */

    /* The interpreter operand stack */
    uint32_t stack_size;
    pdf_obj **stack_bot;
    pdf_obj **stack_top;
    pdf_obj **stack_limit;

    /* The object cache */
    uint32_t cache_entries;
    pdf_obj_cache_entry *cache_LRU;
    pdf_obj_cache_entry *cache_MRU;

    /* The loop detection state */
    uint32_t loop_detection_size;
    uint32_t loop_detection_entries;
    uint64_t *loop_detection;

    /* Used to set the 'parent' stream of a stream that gets created by dereferencing
     * We should not need this but badly fromed PDF files can use Resources defined in
     * an earlier (non-Page) stream object, and Acrobat handles this, so we need to.
     * We could haev done this more neatly if we'd known this during design :-(
     */
    pdf_stream *current_stream;
    stream_save current_stream_save;

    /* A name table :-( */
    pdfi_name_entry_t *name_table;

    pdf_dict *pdffontmap;

    /* These function pointers can be replaced by ones intended to replicate
     * PostScript functionality when running inside the Ghostscript PostScript
     * interpreter.
     */
    int (*end_page) (struct pdf_context_s *ctx);
    int (*get_glyph_name)(gs_font *font, gs_glyph index, gs_const_string *pstr);
    int (*get_glyph_index)(gs_font *font, byte *str, uint size, uint *glyph);

#if REFCNT_DEBUG
    uint64_t UID;
#endif
#if CACHE_STATISTICS
    uint64_t hits;
    uint64_t misses;
    uint64_t compressed_hits;
    uint64_t compressed_misses;
#endif
}pdf_context;

#define OBJ_CTX(o) ((pdf_context *)(o->ctx))
#define OBJ_MEMORY(o) OBJ_CTX(o)->memory

pdf_context *pdfi_create_context(gs_memory_t *pmem);
int pdfi_free_context(gs_memory_t *pmem, pdf_context *ctx);

int pdfi_get_name_index(pdf_context *ctx, char *name, int len, unsigned int *returned);
int pdfi_name_from_index(pdf_context *ctx, int index, unsigned char **name, unsigned int *len);
int pdfi_separation_name_from_index(gs_gstate *pgs, gs_separation_name index, unsigned char **name, unsigned int *len);
int pdfi_open_pdf_file(pdf_context *ctx, char *filename);
int pdfi_set_input_stream(pdf_context *ctx, stream *stm);
int pdfi_process_pdf_file(pdf_context *ctx, char *filename);
int pdfi_close_pdf_file(pdf_context *ctx);

#endif
