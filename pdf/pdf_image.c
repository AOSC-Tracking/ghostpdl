/* Copyright (C) 2001-2018 Artifex Software, Inc.
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

/* Image operations for the PDF interpreter */

#include "pdf_int.h"
#include "pdf_stack.h"
#include "pdf_image.h"
#include "pdf_file.h"
#include "pdf_dict.h"

extern int pdf_dict_from_stack(pdf_context *ctx);

int pdf_BI(pdf_context *ctx)
{
    return pdf_mark_stack(ctx, PDF_DICT_MARK);
}

static int do_image(pdf_context *ctx, gs_color_space *pcs, int bpc, int comps, int width, int height, pdf_stream *new_stream)
{
    gs_image_enum *penum;
    gs_image_t gsimage;
    int code;
    unsigned char Buffer[1024];
    uint64_t toread;

    memset(&gsimage, 0, sizeof(gsimage));
    gs_image_t_init(&gsimage, pcs);
    gsimage.ColorSpace = pcs;
    gsimage.BitsPerComponent = bpc;
    gsimage.Width = width;
    gsimage.Height = height;

    gsimage.ImageMatrix.xx = (float)width;
    gsimage.ImageMatrix.yy = (float)(height * -1);
    gsimage.ImageMatrix.ty = (float)height;
/*    gsimage.ImageMatrix.xx = image->xres / 96.0;
    gsimage.ImageMatrix.yy = image->yres / 96.0;*/

    dmprintf(ctx->memory, "in do_image");
    gsimage.Interpolate = 1;

    penum = gs_image_enum_alloc(ctx->memory, "xps_parse_image_brush (gs_image_enum_alloc)");
    if (!penum)
        return_error(gs_error_VMerror);

    if ((code = gs_image_init(penum, &gsimage, false, false, ctx->pgs)) < 0)
        return code;

    toread = (((width * comps * bpc) + 7) / 8) * height;

    do {
        uint count, used, index;

        if (toread > 1024) {
            code = pdf_read_bytes(ctx, (byte *)Buffer, 1, 1024, new_stream);
            count = 1024;
        } else {
            code = pdf_read_bytes(ctx, (byte *)Buffer, 1, toread, new_stream);
            count = toread;
        }
        toread -= count;
        index = 0;
        do {
            if ((code = gs_image_next(penum, (byte *)&Buffer[index], count, &used)) < 0)
                return code;
            count -= used;
            index += used;
        } while (count);
    } while(toread && new_stream->eof == false);


    gs_image_cleanup_and_free_enum(penum, ctx->pgs);

    return 0;
}

int pdf_ID(pdf_context *ctx, pdf_stream *source)
{
    pdf_name *n = NULL;
    pdf_dict *d = NULL;
    pdf_stream *new_stream;
    int64_t Height, Width, BPC;
    int i, code, comps = 0, byteswide, total;
    byte c;
    pdf_obj *Mask;
    gs_color_space  *pcs = NULL;

    code = pdf_dict_from_stack(ctx);
    if (code < 0)
        return code;

    d = (pdf_dict *)ctx->stack_top[-1];
    pdf_countup(d);
    pdf_pop(ctx, 1);

    /* Need to process it here. Punt for now */
    code = pdf_dict_get_int(ctx, d, "Height", &Height);
    if (code == gs_error_undefined)
        code = pdf_dict_get_int(ctx, d, "H", &Height);
    if (code < 0) {
        pdf_countdown(d);
        return code;
    }

    code = pdf_dict_get_int(ctx, d, "Width", &Width);
    if (code == gs_error_undefined)
        code = pdf_dict_get_int(ctx, d, "W", &Width);
    if (code < 0) {
        pdf_countdown(d);
        return code;
    }

    code = pdf_dict_get_int(ctx, d, "BitsPerComponent", &BPC);
    if (code == gs_error_undefined)
        code = pdf_dict_get_int(ctx, d, "BPC", &BPC);
    if (code < 0) {
        pdf_countdown(d);
        return code;
    }

    code = pdf_dict_get_type(ctx, d, "ImageMask", PDF_BOOL, &Mask);
    if (code == gs_error_undefined)
        code = pdf_dict_get_type(ctx, d, "IM", PDF_BOOL, &Mask);
    if (code == 0) {
        if (((pdf_bool *)Mask)->value == true)
            comps = 1;
        pdf_countdown(Mask);
    } else {
        if (code != gs_error_undefined) {
            pdf_countdown(d);
            return code;
        }
    }

    code = pdf_dict_get_type(ctx, d, "ColorSpace", PDF_NAME, (pdf_obj **)&n);
    if (code == gs_error_undefined)
        code = pdf_dict_get_type(ctx, d, "CS", PDF_NAME, (pdf_obj **)&n);
    if (code < 0) {
        if (comps == 0) {
            gx_device *dev = gs_currentdevice_inline(ctx->pgs);
            comps = dev->color_info.num_components;
        }
    } else {
        switch(n->length){
            case 1:
                if (memcmp(n->data, "G", 1) == 0){
                    comps = 1;
                    pcs = gs_cspace_new_DeviceGray(ctx->memory);
                    (void)pcs->type->install_cspace(pcs, ctx->pgs);
                } else {
                    if (memcmp(n->data, "I", 1) == 0){
                        comps = 1;
                        pcs = gs_currentcolorspace(ctx->pgs);
                    } else {
                        pdf_countdown(n);
                        pdf_countdown(d);
                        return_error(gs_error_syntaxerror);
                    }
                }
                break;
            case 3:
                if (memcmp(n->data, "RGB", 3) == 0){
                    comps = 3;
                    pcs = gs_cspace_new_DeviceRGB(ctx->memory);
                    (void)pcs->type->install_cspace(pcs, ctx->pgs);
                } else {
                    pdf_countdown(n);
                    pdf_countdown(d);
                    return_error(gs_error_syntaxerror);
                }
                break;
            case 4:
                if (memcmp(n->data, "CMYK", 4) == 0){
                    comps = 4;
                    pcs = gs_cspace_new_DeviceCMYK(ctx->memory);
                    (void)pcs->type->install_cspace(pcs, ctx->pgs);
                } else {
                    pdf_countdown(n);
                    pdf_countdown(d);
                    return_error(gs_error_syntaxerror);
                }
                break;
            case 7:
                if (memcmp(n->data, "Indexed", 7) == 0){
                    comps = 1;
                } else {
                    pdf_countdown(n);
                    pdf_countdown(d);
                    return_error(gs_error_syntaxerror);
                }
                break;
            case 9:
                if (memcmp(n->data, "DeviceRGB", 9) == 0){
                    comps = 3;
                    pcs = gs_cspace_new_DeviceRGB(ctx->memory);
                    (void)pcs->type->install_cspace(pcs, ctx->pgs);
                } else {
                    pdf_countdown(n);
                    pdf_countdown(d);
                    return_error(gs_error_syntaxerror);
                }
                break;
            case 10:
                if (memcmp(n->data, "DeviceGray", 10) == 0){
                    comps = 1;
                    pcs = gs_cspace_new_DeviceGray(ctx->memory);
                    (void)pcs->type->install_cspace(pcs, ctx->pgs);
                } else {
                    if (memcmp(n->data, "DeviceCMYK", 10) == 0){
                        comps = 4;
                        pcs = gs_cspace_new_DeviceCMYK(ctx->memory);
                        (void)pcs->type->install_cspace(pcs, ctx->pgs);
                    } else {
                        pdf_countdown(n);
                        pdf_countdown(d);
                        return_error(gs_error_syntaxerror);
                    }
                }
                break;
            default:
                pdf_countdown(n);
                pdf_countdown(d);
                return_error(gs_error_syntaxerror);
        }
        pdf_countdown(n);
    }

    code = pdf_filter(ctx, d, source, &new_stream, true);
    if (code < 0) {
        pdf_countdown(d);
        return code;
    }

    pdf_countdown(d);

    if (pcs == NULL) {
        byteswide = ((Width * comps * BPC) + 7) / 8;
        total = byteswide * Height;

        for (i=0;i < total;i++) {
            code = pdf_read_bytes(ctx, &c, 1, 1, new_stream);
            if (code < 0) {
                pdf_close_file(ctx, new_stream);
                return code;
            }
        }
    } else {
        do_image(ctx, pcs, BPC, comps, Width, Height, new_stream);
    }
    rc_decrement_cs(pcs, "gs_setrgbcolor");
    pdf_close_file(ctx, new_stream);
    return 0;
}

int pdf_EI(pdf_context *ctx)
{
    pdf_clearstack(ctx);
    return 0;
}

int pdf_find_XObject_resource(pdf_context *ctx, pdf_name *name, pdf_dict *stream_dict, pdf_dict *page_dict, pdf_obj **o)
{
    char Key[256];
    pdf_dict *Resources, *XObjects;
    int code;

    *o = NULL;
    memcpy(Key, name->data, name->length);
    Key[name->length] = 0x00;

    code = pdf_dict_get(stream_dict, "Resources", &Resources);
    if (code == 0) {
        code = pdf_dict_get(Resources, "XObject", &XObjects);
        if (code == 0) {
            pdf_countdown(Resources);
            code = pdf_dict_get(XObjects, Key, o);
            pdf_countdown(XObjects);
            if (code == 0)
                return code;
        }
    }
    code = pdf_dict_get(page_dict, "Resources", &Resources);
    if (code < 0)
        return code;
    }
    code = pdf_dict_get(Resources, "XObject", &XObjects);
    pdf_countdown(Resources);
    if (code < 0)
        return code;

    code = pdf_dict_get(XObjects, Key, o);
    pdf_countdown(XObjects);
    return code;
    return 0;
}

int pdf_Do(pdf_context *ctx, pdf_dict *stream_dict, pdf_dict *page_dict)
{
    int code;
    pdf_name *n = NULL;
    pdf_obj *o;

    if (ctx->stack_top - ctx->stack_bot < 1) {
        if (ctx->pdfstoponerror) {
            return_error(gs_error_stackunderflow);
        }
        return 0;
    }
    n = (pdf_name *)ctx->stack_top[-1];
    if (n->type != PDF_NAME) {
        pdf_pop(ctx, 1);
        if (ctx->pdfstoponerror) {
            return_error(gs_error_typecheck);
        }
        return 0;
    }

    code = pdf_find_XObject_resource(ctx, n, stream_dict, page_dict, &o);
    if (code < 0) {
        pop(ctx, 1);
        return code;
    }
    return 0;
}
