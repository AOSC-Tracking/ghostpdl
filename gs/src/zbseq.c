/* Copyright (C) 1990, 1995, 1997, 1998 Aladdin Enterprises.  All rights reserved.

   This file is part of Aladdin Ghostscript.

   Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of Aladdin Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute Aladdin Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/*Id: zbseq.c  */
/* Level 2 binary object sequence operators */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "ialloc.h"
#include "btoken.h"
#include "store.h"

/* Current binary format (in iscan.c) */
extern ref ref_binary_object_format;

/* System and user name arrays. */
ref binary_token_names;		/* array of size 2 */
private ref *const binary_token_names_p = &binary_token_names;

/* Import the Level 2 scanner extensions. */
typedef struct scanner_state_s scanner_state;
extern int scan_binary_token(P3(stream *, ref *, scanner_state *));
extern int (*scan_btoken_proc) (P3(stream *, ref *, scanner_state *));

/* Initialize the binary token machinery. */
private void
zbseq_init(void)
{
    /* Initialize fake system and user name tables. */
    /* PostScript code will install the real system name table. */
    ialloc_ref_array(&binary_token_names, 0 /*a_noaccess */ , 2,
		     "binary token names");
    make_empty_array(system_names_p, a_readonly);
    make_empty_array(user_names_p, a_all);
    gs_register_ref_root(imemory, NULL, (void **)&binary_token_names_p,
			 "binary token names");

    /* Set up Level 2 scanning constants. */
    scan_btoken_proc = scan_binary_token;
}

/* <names> .installsystemnames - */
private int
zinstallsystemnames(register os_ptr op)
{
    if (r_space(op) != avm_global)
	return_error(e_invalidaccess);
    check_read_type(*op, t_shortarray);
    ref_assign_old(NULL, system_names_p, op, ".installsystemnames");
    pop(1);
    return 0;
}

/* - currentobjectformat <int> */
private int
zcurrentobjectformat(register os_ptr op)
{
    push(1);
    *op = ref_binary_object_format;
    return 0;
}

/* <int> setobjectformat - */
private int
zsetobjectformat(register os_ptr op)
{
    check_type(*op, t_integer);
    if (op->value.intval < 0 || op->value.intval > 4)
	return_error(e_rangecheck);
    ref_assign_old(NULL, &ref_binary_object_format, op, "setobjectformat");
    pop(1);
    return 0;
}

/* <ref_offset> <char_offset> <obj> <string8> .bosobject */
/*   <ref_offset'> <char_offset'> <string8> */
/*
 * This converts a single object to its binary object sequence
 * representation, doing the dirty work of printobject and writeobject.
 * (The main control is in PostScript code, so that we don't have to worry
 * about interrupts or callouts in the middle of writing the various data
 * items.)  Note that this may or may not modify the 'unused' field.
 */

private int
zbosobject(os_ptr op)
{
    int code;

    check_type(op[-3], t_integer);
    check_type(op[-2], t_integer);
    check_write_type(*op, t_string);
    if (r_size(op) < 8)
	return_error(e_rangecheck);
    code = encode_binary_token(op - 1, &op[-3].value.intval,
			       &op[-2].value.intval, op->value.bytes);
    if (code < 0)
	return code;
    op[-1] = *op;
    r_set_size(op - 1, 8);
    pop(1);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zbseq_l2_op_defs[] =
{
    op_def_begin_level2(),
    {"1.installsystemnames", zinstallsystemnames},
    {"0currentobjectformat", zcurrentobjectformat},
    {"1setobjectformat", zsetobjectformat},
    {"4.bosobject", zbosobject},
    op_def_end(zbseq_init)
};
