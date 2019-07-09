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

/* Text operations for the PDF interpreter */

#include "pdf_int.h"
#include "pdf_text.h"
#include "pdf_image.h"
#include "pdf_stack.h"

#include "gsstate.h"
#include "gsmatrix.h"

int pdfi_BT(pdf_context *ctx)
{
    int code;
    gs_matrix m;

    gs_make_identity(&m);
    code = gs_settextmatrix(ctx->pgs, &m);
    if (code < 0)
        return code;

    code = gs_settextlinematrix(ctx->pgs, &m);
    if (code < 0)
        return code;

    code = gs_settextspacing(ctx->pgs, (double)0.0);
    if (code < 0)
        return code;

    code = gs_settextleading(ctx->pgs, (double)0.0);
    if (code < 0)
        return code;

    gs_settextrenderingmode(ctx->pgs, 0);

    code = gs_setwordspacing(ctx->pgs, (double)0.0);
    if (code < 0)
        return code;

    code = gs_settexthscaling(ctx->pgs, (double)100.0);
    if (code < 0)
        return code;

    code = gs_moveto(ctx->pgs, 0, 0);
    return code;
}

int pdfi_ET(pdf_context *ctx)
{
    return 0;
}

int pdfi_T_star(pdf_context *ctx)
{
    return 0;
}

int pdfi_Tc(pdf_context *ctx)
{
    int code = 0;
    pdf_num *n = NULL;

    if (pdfi_count_stack(ctx) < 1)
        return_error(gs_error_stackunderflow);
    n = (pdf_num *)ctx->stack_top[-1];

    if (n->type == PDF_INT)
        code = gs_settextspacing(ctx->pgs, (double)n->value.i);
    else {
        if (n->type == PDF_REAL)
            code = gs_settextspacing(ctx->pgs, n->value.d);
        else
            code = gs_note_error(gs_error_typecheck);
    }
    pdfi_pop(ctx, 1);
    return code;
}

int pdfi_Td(pdf_context *ctx)
{
    if (pdfi_count_stack(ctx) >= 2)
        pdfi_pop(ctx, 2);
    else
        pdfi_clearstack(ctx);
    return 0;
}

int pdfi_TD(pdf_context *ctx)
{
    if (pdfi_count_stack(ctx) >= 2)
        pdfi_pop(ctx, 2);
    else
        pdfi_clearstack(ctx);
    return 0;
}

int pdfi_Tj(pdf_context *ctx)
{
    int code;
    gs_text_enum_t *penum;
    gs_text_params_t text;
    pdf_string *s = NULL;

    if (pdfi_count_stack(ctx) < 1)
        return_error(gs_error_stackunderflow);

    s = (pdf_string *)ctx->stack_top[-1];
    if (s->type != PDF_STRING)
        return_error(gs_error_typecheck);

    if (ctx->pgs->font == NULL) {
        pdfi_pop(ctx, 1);
        return_error(gs_error_invalidfont);
    }

    text.operation = TEXT_FROM_STRING | TEXT_DO_DRAW | TEXT_RETURN_WIDTH;
    text.data.chars = (const gs_char *)s->data;
    text.size = s->length;
    code = gs_text_begin(ctx->pgs, &text, ctx->memory, &penum);
    if (code >= 0) {
        ctx->current_text_enum = penum;
        code = gs_text_process(penum);
        gs_text_release(penum, "pdfi_Tj");
        ctx->current_text_enum = NULL;
    }
    pdfi_pop(ctx, 1);

    return code;
}

int pdfi_TJ(pdf_context *ctx)
{
    if (pdfi_count_stack(ctx) >= 1)
        pdfi_pop(ctx, 1);
    return 0;
}

int pdfi_TL(pdf_context *ctx)
{
    int code = 0;
    pdf_num *n = NULL;

    if (pdfi_count_stack(ctx) < 1)
        return_error(gs_error_stackunderflow);
    n = (pdf_num *)ctx->stack_top[-1];

    if (n->type == PDF_INT)
        code = gs_settextleading(ctx->pgs, (double)n->value.i);
    else {
        if (n->type == PDF_REAL)
            code = gs_settextleading(ctx->pgs, n->value.d);
        else
            code = gs_note_error(gs_error_typecheck);
    }
    pdfi_pop(ctx, 1);
    return code;
}

int pdfi_Tm(pdf_context *ctx)
{
    int code = 0, i;
    double m[6];
    pdf_num *n = NULL;

    if (pdfi_count_stack(ctx) < 6) {
        pdfi_clearstack(ctx);
        return_error(gs_error_stackunderflow);
    }
    for (i = 1;i < 7;i++) {
        n = (pdf_num *)ctx->stack_top[-1 * i];
        if (n->type == PDF_INT)
            m[6 - i] = (double)n->value.i;
        else {
            if (n->type == PDF_REAL)
                m[6 - i] = n->value.d;
            else {
                pdfi_pop(ctx, 6);
                return_error(gs_error_typecheck);
            }
        }
    }
    pdfi_pop(ctx, 6);
    code = gs_settextmatrix(ctx->pgs, (gs_matrix *)&m);
    if (code < 0)
        return code;

    code = gs_settextlinematrix(ctx->pgs, (gs_matrix *)&m);
    return code;
}

int pdfi_Tr(pdf_context *ctx)
{
    int code = 0, mode = 0;
    pdf_num *n = NULL;

    if (pdfi_count_stack(ctx) < 1)
        return_error(gs_error_stackunderflow);
    n = (pdf_num *)ctx->stack_top[-1];

    if (n->type == PDF_INT)
        mode = n->value.i;
    else {
        if (n->type == PDF_REAL)
            mode = (int)n->value.d;
        else {
            pdfi_pop(ctx, 1);
            return_error(gs_error_typecheck);
        }
    }
    pdfi_pop(ctx, 1);

    if (mode < 0 || mode > 7)
        code = gs_note_error(gs_error_rangecheck);
    else
        gs_settextrenderingmode(ctx->pgs, mode);

    return code;
}

int pdfi_Ts(pdf_context *ctx)
{
    int code = 0;
    pdf_num *n = NULL;

    if (pdfi_count_stack(ctx) < 1)
        return_error(gs_error_stackunderflow);
    n = (pdf_num *)ctx->stack_top[-1];

    if (n->type == PDF_INT)
        code = gs_settextrise(ctx->pgs, (double)n->value.i);
    else {
        if (n->type == PDF_REAL)
            code = gs_settextrise(ctx->pgs, n->value.d);
        else
            code = gs_note_error(gs_error_typecheck);
    }
    pdfi_pop(ctx, 1);
    return code;
}

int pdfi_Tw(pdf_context *ctx)
{
    int code = 0;
    pdf_num *n = NULL;

    if (pdfi_count_stack(ctx) < 1)
        return_error(gs_error_stackunderflow);
    n = (pdf_num *)ctx->stack_top[-1];

    if (n->type == PDF_INT)
        code = gs_setwordspacing(ctx->pgs, (double)n->value.i);
    else {
        if (n->type == PDF_REAL)
            code = gs_setwordspacing(ctx->pgs, n->value.d);
        else
            code = gs_note_error(gs_error_typecheck);
    }
    pdfi_pop(ctx, 1);
    return code;
}

int pdfi_Tz(pdf_context *ctx)
{
    int code = 0;
    pdf_num *n = NULL;

    if (pdfi_count_stack(ctx) < 1)
        return_error(gs_error_stackunderflow);
    n = (pdf_num *)ctx->stack_top[-1];

    if (n->type == PDF_INT)
        code = gs_settexthscaling(ctx->pgs, (double)n->value.i);
    else {
        if (n->type == PDF_REAL)
            code = gs_settexthscaling(ctx->pgs, n->value.d);
        else
            code = gs_note_error(gs_error_typecheck);
    }
    pdfi_pop(ctx, 1);
    return code;
}

int pdfi_singlequote(pdf_context *ctx)
{
    if (pdfi_count_stack(ctx) >= 1)
        pdfi_pop(ctx, 1);
    return 0;
}

int pdfi_doublequote(pdf_context *ctx)
{
    if (pdfi_count_stack(ctx) >= 3)
        pdfi_pop(ctx, 3);
    return 0;
}