/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

/*
 * Utility functions called by various backends.
 */ 

#include "xpidl.h"

/* XXXbe static */ char OOM[] = "ERROR: out of memory\n";

void *
xpidl_malloc(size_t nbytes)
{
    void *p = malloc(nbytes);
    if (!p) {
        fputs(OOM, stderr);
        exit(1);
    }
    return p;
}

#ifdef XP_MAC
static char *strdup(const char *c)
{
	char	*newStr = malloc(strlen(c) + 1);
	if (newStr)
	{
		strcpy(newStr, c);
	}
	return newStr;
}
#endif

char *
xpidl_strdup(const char *s)
{
    char *ns = strdup(s);
    if (!ns) {
        fputs(OOM, stderr);
        exit(1);
    }
    return ns;
}

void
xpidl_write_comment(TreeState *state, int indent)
{
    fprintf(state->file, "%*s/* ", indent, "");
    IDL_tree_to_IDL(state->tree, state->ns, state->file,
                    IDLF_OUTPUT_NO_NEWLINES |
                    IDLF_OUTPUT_NO_QUALIFY_IDENTS |
                    IDLF_OUTPUT_PROPERTIES);
    fputs(" */\n", state->file);
}

/*
 * Print an iid to into a supplied buffer; the buffer should be at least
 * UUID_LENGTH bytes.
 */
gboolean
xpidl_sprint_iid(nsID *id, char iidbuf[])
{
    int printed;

    printed = sprintf(iidbuf,
                       "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                       (PRUint32) id->m0, (PRUint32) id->m1,(PRUint32) id->m2,
                       (PRUint32) id->m3[0], (PRUint32) id->m3[1],
                       (PRUint32) id->m3[2], (PRUint32) id->m3[3],
                       (PRUint32) id->m3[4], (PRUint32) id->m3[5],
                       (PRUint32) id->m3[6], (PRUint32) id->m3[7]);

#ifdef SPRINTF_RETURNS_STRING
    return (printed && strlen((char *)printed) == 36);
#else
    return (printed == 36);
#endif
}

/* We only parse the {}-less format.  (xpidl_header never has, so we're safe.) */
static const char nsIDFmt2[] =
  "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";

/*
 * Parse a uuid string into an nsID struct.  We cannot link against libxpcom,
 * so we re-implement nsID::Parse here.
 */
gboolean
xpidl_parse_iid(nsID *id, const char *str)
{
    PRInt32 count = 0;
    PRInt32 n1, n2, n3[8];
    PRInt32 n0, i;

    assert(str != NULL);
    
    if (strlen(str) != 36) {
        return FALSE;
    }
     
#ifdef DEBUG_shaver_iid
    fprintf(stderr, "parsing iid   %s\n", str);
#endif

    count = sscanf(str, nsIDFmt2,
                   &n0, &n1, &n2,
                   &n3[0],&n3[1],&n3[2],&n3[3],
                   &n3[4],&n3[5],&n3[6],&n3[7]);

    id->m0 = (PRInt32) n0;
    id->m1 = (PRInt16) n1;
    id->m2 = (PRInt16) n2;
    for (i = 0; i < 8; i++) {
      id->m3[i] = (PRInt8) n3[i];
    }

#ifdef DEBUG_shaver_iid
    if (count == 11) {
        fprintf(stderr, "IID parsed to ");
        print_IID(id, stderr);
        fputs("\n", stderr);
    }
#endif
    return (gboolean)(count == 11);
}

/*
 * Common method verification code, called by *op_dcl in the various backends.
 */
gboolean
verify_method_declaration(IDL_tree method_tree)
{
    struct _IDL_OP_DCL *op = &IDL_OP_DCL(method_tree);
    IDL_tree iface;
    gboolean scriptable_interface;

    if (op->f_varargs) {
        /* We don't currently support varargs. */
        IDL_tree_error(method_tree, "varargs are not currently supported\n");
        return FALSE;
    }

    /* 
     * Verify that we've been called on an interface, and decide if the
     * interface was marked [scriptable].
     */
    if (IDL_NODE_UP(method_tree) && IDL_NODE_UP(IDL_NODE_UP(method_tree)) &&
        IDL_NODE_TYPE(iface = IDL_NODE_UP(IDL_NODE_UP(method_tree))) 
        == IDLN_INTERFACE)
    {
        scriptable_interface =
            (IDL_tree_property_get(IDL_INTERFACE(iface).ident, "scriptable")
             != NULL);
    } else {
        IDL_tree_error(method_tree, "verify_op_dcl called on a non-interface?");
        return FALSE;
    }

    /*
     * Require that any method in an interface marked as [scriptable], that
     * *isn't* scriptable because it refers to some native type, be marked
     * [noscript] or [notxpcom].
     */
    if (scriptable_interface &&
        IDL_tree_property_get(op->ident, "notxpcom") == NULL &&
        IDL_tree_property_get(op->ident, "noscript") == NULL)
    {
        IDL_tree iter;

        /* Loop through the parameters and check. */
        for (iter = op->parameter_dcls; iter; iter = IDL_LIST(iter).next) {
            IDL_tree param_type =
                IDL_PARAM_DCL(IDL_LIST(iter).data).param_type_spec;
            IDL_tree simple_decl =
                IDL_PARAM_DCL(IDL_LIST(iter).data).simple_declarator;

            /*
             * Reject this method if a parameter is native and isn't marked
             * with either nsid or iid_is.
             */
            if (UP_IS_NATIVE(param_type) &&
                IDL_tree_property_get(param_type, "nsid") == NULL &&
                IDL_tree_property_get(simple_decl, "iid_is") == NULL)
            {
                IDL_tree_error(method_tree,
                               "methods in [scriptable] interfaces which are "
                               "non-scriptable because they refer to native "
                               "types (parameter \"%s\") must be marked "
                               "[noscript]\n", IDL_IDENT(simple_decl).str);
                return FALSE;
            }
        }
        
        /* How about the return type? */
        if (op->op_type_spec != NULL && UP_IS_NATIVE(op->op_type_spec)) {
            IDL_tree_error(method_tree,
                           "methods in [scriptable] interfaces which are "
                           "non-scriptable because they return native "
                           "types must be marked [noscript]\n");
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * Verify that a native declaration has an associated C++ expression, i.e. that
 * it's of the form native <idl-name>(<c++-name>)
 */
gboolean
check_native(TreeState *state)
{
    char *native_name;
    /* require that native declarations give a native type */
    if (IDL_NATIVE(state->tree).user_type) 
        return TRUE;
    native_name = IDL_IDENT(IDL_NATIVE(state->tree).ident).str;
    IDL_tree_error(state->tree,
                   "``native %s;'' needs C++ type: ``native %s(<C++ type>);''",
                   native_name, native_name);
    return FALSE;
}

/*
 * Print a GSList as char strings to a file.
 */
void
printlist(FILE *outfile, GSList *slist)
{
    guint i;
    guint len = g_slist_length(slist);

    for(i = 0; i < len; i++) {
        fprintf(outfile, 
                "%s\n", (char *)g_slist_nth_data(slist, i));
    }
}

void
xpidl_list_foreach(IDL_tree p, IDL_tree_func foreach, gpointer user_data)
{
    IDL_tree_func_data tfd;

    while (p) {
        struct _IDL_LIST *list = &IDL_LIST(p);
        tfd.tree = list->data;
        if (!foreach(&tfd, user_data))
            return;
        p = list->next;
    }
}
