/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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

#include "nsCOMPtr.h"
#include "nsIMimeEmitter.h"
#include "mimerosetta.h"
#include "mimemsg.h"
#include "mimemoz2.h"
#include "prmem.h"
#include "prio.h"
#include "plstr.h"
#include "msgCore.h"
#include "prlog.h"
#include "nsCRT.h"
#include "nsMimeStringResources.h"
#include "nsMimeTypes.h"

#define MIME_SUPERCLASS mimeContainerClass
MimeDefClass(MimeMessage, MimeMessageClass, mimeMessageClass,
			 &MIME_SUPERCLASS);

static int MimeMessage_initialize (MimeObject *);
static void MimeMessage_finalize (MimeObject *);
static int MimeMessage_add_child (MimeObject *, MimeObject *);
static int MimeMessage_parse_begin (MimeObject *);
static int MimeMessage_parse_line (char *, PRInt32, MimeObject *);
static int MimeMessage_parse_eof (MimeObject *, PRBool);
static int MimeMessage_close_headers (MimeObject *obj);
static int MimeMessage_write_headers_html (MimeObject *);

#ifdef MOZ_SECURITY
HG56268
#endif /* MOZ_SECURITY */

#ifdef XP_UNIX
extern void MimeHeaders_do_unix_display_hook_hack(MimeHeaders *);
#endif /* XP_UNIX */

#if defined(DEBUG) && defined(XP_UNIX)
static int MimeMessage_debug_print (MimeObject *, PRFileDesc *, PRInt32 depth);
#endif

extern MimeObjectClass mimeMultipartClass;

static int
MimeMessageClassInitialize(MimeMessageClass *clazz)
{
  MimeObjectClass    *oclass = (MimeObjectClass *)    clazz;
  MimeContainerClass *cclass = (MimeContainerClass *) clazz;

  PR_ASSERT(!oclass->class_initialized);
  oclass->initialize  = MimeMessage_initialize;
  oclass->finalize    = MimeMessage_finalize;
  oclass->parse_begin = MimeMessage_parse_begin;
  oclass->parse_line  = MimeMessage_parse_line;
  oclass->parse_eof   = MimeMessage_parse_eof;
  cclass->add_child   = MimeMessage_add_child;

#if defined(DEBUG) && defined(XP_UNIX)
  oclass->debug_print = MimeMessage_debug_print;
#endif
  return 0;
}


static int
MimeMessage_initialize (MimeObject *object)
{
  return ((MimeObjectClass*)&MIME_SUPERCLASS)->initialize(object);
}

static void
MimeMessage_finalize (MimeObject *object)
{
  MimeMessage *msg = (MimeMessage *)object;
  if (msg->hdrs)
	MimeHeaders_free(msg->hdrs);
  msg->hdrs = 0;
  ((MimeObjectClass*)&MIME_SUPERCLASS)->finalize(object);
}

static int
MimeMessage_parse_begin (MimeObject *obj)
{
  int status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_begin(obj);
  if (status < 0) return status;

  /* Messages have separators before the headers, except for the outermost
	 message. */
  return MimeObject_write_separator(obj);
}


static int
MimeMessage_parse_line (char *line, PRInt32 length, MimeObject *obj)
{
  MimeMessage *msg = (MimeMessage *) obj;
  int status = 0;

  PR_ASSERT(line && *line);
  if (!line || !*line) return -1;

#ifdef MOZ_SECURITY
  HG11013
#endif /* MOZ_SECURITY */

  /* If we already have a child object, then we're done parsing headers,
	 and all subsequent lines get passed to the inferior object without
	 further processing by us.  (Our parent will stop feeding us lines
	 when this MimeMessage part is out of data.)
   */
  if (msg->container.nchildren)
	{
	  MimeObject *kid = msg->container.children[0];
	  PRBool nl;
	  PR_ASSERT(kid);
	  if (!kid) return -1;

	  /* Don't allow MimeMessage objects to not end in a newline, since it
		 would be inappropriate for any following part to appear on the same
		 line as the last line of the message.

		 #### This assumes that the only time the `parse_line' method is
		 called with a line that doesn't end in a newline is when that line
		 is the last line.
	   */
	  nl = (length > 0 && (line[length-1] == CR || line[length-1] == LF));

#ifdef MIME_DRAFTS
	  if ( !mime_typep (kid, (MimeObjectClass*) &mimeMessageClass) &&
		   obj->options &&
		   obj->options->decompose_file_p &&
		   ! obj->options->is_multipart_msg &&
		   obj->options->decompose_file_output_fn )
		{
		  if (!obj->options->dexlate_p) {
			  status = obj->options->decompose_file_output_fn (line,
															   length,
													 obj->options->stream_closure);
			  if (status < 0) return status;
			  if (!nl) {
				status = obj->options->decompose_file_output_fn (MSG_LINEBREAK,
																 MSG_LINEBREAK_LEN,
													 obj->options->stream_closure);
				if (status < 0) return status;
			  }
			  return status;
		  }
		}
#endif /* MIME_DRAFTS */


	  if (nl)
		return kid->clazz->parse_buffer (line, length, kid);
	  else
		{
		  /* Hack a newline onto the end. */
		  char *s = (char *)PR_MALLOC(length + MSG_LINEBREAK_LEN + 1);
		  if (!s) return MIME_OUT_OF_MEMORY;
		  nsCRT::memcpy(s, line, length);
		  PL_strcpy(s + length, MSG_LINEBREAK);
		  status = kid->clazz->parse_buffer (s, length + MSG_LINEBREAK_LEN, kid);
		  PR_Free(s);
		  return status;
		}
	}

  /* Otherwise we don't yet have a child object, which means we're not
	 done parsing our headers yet.
   */
  if (!msg->hdrs)
	{
	  msg->hdrs = MimeHeaders_new();
	  if (!msg->hdrs) return MIME_OUT_OF_MEMORY;
	}

#ifdef MIME_DRAFTS
  if ( obj->options &&
	     obj->options->decompose_file_p &&
	     ! obj->options->is_multipart_msg &&
	     obj->options->done_parsing_outer_headers &&
	     obj->options->decompose_file_output_fn ) 
  {
  	status =  obj->options->decompose_file_output_fn( line, length,
											                                obj->options->stream_closure );
  	if (status < 0) 
      return status;
  }
#endif /* MIME_DRAFTS */

  status = MimeHeaders_parse_line(line, length, msg->hdrs);
  if (status < 0) return status;

  /* If this line is blank, we're now done parsing headers, and should
	 examine our content-type to create our "body" part.
   */
  if (*line == CR || *line == LF)
	{
	  status = MimeMessage_close_headers(obj);
	  if (status < 0) return status;
	}

  return 0;
}

static int
MimeMessage_close_headers (MimeObject *obj)
{
  MimeMessage *msg = (MimeMessage *) obj;
  int status = 0;
  char *ct = 0;			/* Content-Type header */
  MimeObject *body;

  if (msg->hdrs)
	{
	  PRBool outer_p = !obj->headers; /* is this the outermost message? */


#ifdef MIME_DRAFTS
	  if (outer_p &&
		  obj->options &&
          (obj->options->decompose_file_p || obj->options->caller_need_root_headers) &&
		  obj->options->decompose_headers_info_fn)
		{
#ifdef MOZ_SECURITY	
HG09091
#endif /* MOZ_SECURITY */			  
          if (!obj->options->caller_need_root_headers || (obj == obj->options->state->root))
		  	status = obj->options->decompose_headers_info_fn (
												 obj->options->stream_closure,
															 msg->hdrs );
		}
#endif /* MIME_DRAFTS */


	  /* If this is the outermost message, we need to run the
		 `generate_header' callback.  This happens here instead of
		 in `parse_begin', because it's only now that we've parsed
		 our headers.  However, since this is the outermost message,
		 we have yet to write any HTML, so that's fine.
	   */
	  if (outer_p &&
		  obj->output_p &&
		  obj->options &&
		  obj->options->write_html_p &&
		  obj->options->generate_header_html_fn)
		{
		  int lstatus = 0;
		  char *html = 0;

		  /* The generate_header_html_fn might return HTML, so it's important
			 that the output stream be set up with the proper type before we
			 make the MimeObject_write() call below. */
		  if (!obj->options->state->first_data_written_p)
			{
			  lstatus = MimeObject_output_init (obj, TEXT_HTML);
			  if (lstatus < 0) return lstatus;
			  PR_ASSERT(obj->options->state->first_data_written_p);
			}

		  html = obj->options->generate_header_html_fn(NULL,
												   obj->options->html_closure,
													   msg->hdrs);
		  if (html)
			{
			  lstatus = MimeObject_write(obj, html, nsCRT::strlen(html), PR_FALSE);
			  PR_Free(html);
			  if (lstatus < 0) return lstatus;
			}
		}


	  /* Find the content-type of the body of this message.
	   */
	  {
		PRBool ok = PR_TRUE;
		char *mv = MimeHeaders_get (msg->hdrs, HEADER_MIME_VERSION,
									PR_TRUE, PR_FALSE);

#ifdef REQUIRE_MIME_VERSION_HEADER
		/* If this is the outermost message, it must have a MIME-Version
		   header with the value 1.0 for us to believe what might be in
		   the Content-Type header.  If the MIME-Version header is not
		   present, we must treat this message as untyped.
		 */
		ok = (mv && !nsCRT::strcmp(mv, "1.0"));
#else
		/* #### actually, we didn't check this in Mozilla 2.0, and checking
		   it now could cause some compatibility nonsense, so for now, let's
		   just believe any Content-Type header we see.
		 */
		ok = PR_TRUE;
#endif

		if (ok)
		  {
			ct = MimeHeaders_get (msg->hdrs, HEADER_CONTENT_TYPE, PR_TRUE, PR_FALSE);

			/* If there is no Content-Type header, but there is a MIME-Version
			   header, then assume that this *is* in fact a MIME message.
			   (I've seen messages with

				  MIME-Version: 1.0
				  Content-Transfer-Encoding: quoted-printable

			   and no Content-Type, and we should treat those as being of type
			   MimeInlineTextPlain rather than MimeUntypedText.)
			 */
			if (mv && !ct)
			  ct = nsCRT::strdup(TEXT_PLAIN);
		  }

		PR_FREEIF(mv);  /* done with this now. */
	  }

#ifdef MOZ_SECURITY
    HG67023
#endif /* MOZ_SECURITY */

	  /* Emit the HTML for this message's headers.  Do this before
		 creating the object representing the body.
	   */
	  if (obj->output_p &&
		  obj->options &&
		  obj->options->write_html_p)
		{
		  /* If citation headers are on, and this is not the outermost message,
			 turn them off. */
		  if (obj->options->headers == MimeHeadersCitation && !outer_p)
			obj->options->headers = MimeHeadersSome;

		  /* Emit a normal header block. */
		  status = MimeMessage_write_headers_html(obj);
		  if (status < 0) return status;
		}
	  else if (obj->output_p)
		{
		  /* Dump the headers, raw. */
		  status = MimeObject_write(obj, "", 0, PR_FALSE);  /* initialize */
		  if (status < 0) return status;
		  status = MimeHeaders_write_raw_headers(msg->hdrs, obj->options,
												 obj->options->dexlate_p);
		  if (status < 0) return status;
		}

#ifdef XP_UNIX
	  if (outer_p && obj->output_p)
		/* Kludge from mimehdrs.c */
		MimeHeaders_do_unix_display_hook_hack(msg->hdrs);
#endif /* XP_UNIX */
	}

  /* Never put out a separator after a message header block. */
  if (obj->options && obj->options->state)
	obj->options->state->separator_suppressed_p = PR_TRUE;

#ifdef MIME_DRAFTS
  if ( !obj->headers &&    /* outer most message header */
	   obj->options && 
	   obj->options->decompose_file_p && 
	   ct )
	obj->options->is_multipart_msg = PL_strstr(ct, "multipart/") != NULL;
#endif /* MIME_DRAFTS */


  body = mime_create(ct, msg->hdrs, obj->options);

  PR_FREEIF(ct);
  if (!body) return MIME_OUT_OF_MEMORY;
  status = ((MimeContainerClass *) obj->clazz)->add_child (obj, body);
  if (status < 0)
	{
	  mime_free(body);
	  return status;
	}

  /* Now that we've added this new object to our list of children,
	 start its parser going. */
  status = body->clazz->parse_begin(body);
  if (status < 0) return status;

  // Now notify the emitter if this is the outer most message, unless
  // it is a part that is not the head of the message. If it's a part, 
  // we need to figure out the content type/charset of the part
  //
  PRBool outer_p = !obj->headers;	/* is this the outermost message? */

  if (
      (outer_p) &&
      ( obj->options->part_to_load == NULL )
     )
  {
    char  *charset = NULL;
    char  *lct = MimeHeaders_get (msg->hdrs, HEADER_CONTENT_TYPE,
									                PR_FALSE, PR_FALSE);
		if (lct)
      charset = MimeHeaders_get_parameter (lct, "charset", NULL, NULL);

    char  *msgID = MimeHeaders_get (msg->hdrs, HEADER_MESSAGE_ID,
									                  PR_FALSE, PR_FALSE);

    char  *outCharset = NULL;
    if (!obj->options->force_user_charset)  /* Only convert if the user prefs is false */
      outCharset = "UTF-8";

    mimeEmitterStartBody(obj->options, (obj->options->headers == MimeHeadersNone), msgID, outCharset);
    PR_FREEIF(msgID);
    PR_FREEIF(lct);
    PR_FREEIF(charset);
  }

  return 0;
}



static int 
MimeMessage_parse_eof (MimeObject *obj, PRBool abort_p)
{
  int status;
  PRBool outer_p;
  MimeMessage *msg = (MimeMessage *)obj;
  if (obj->closed_p) return 0;
  
  /* Run parent method first, to flush out any buffered data. */
  status = ((MimeObjectClass*)&MIME_SUPERCLASS)->parse_eof(obj, abort_p);
  if (status < 0) return status;

  outer_p = !obj->headers;	/* is this the outermost message? */

  // Once we get to the end of parsing the message, we will notify
  // the emitter that we are done the the body.

  // Mark the end of the mail body if we are actually emitting the
  // body of the message (i.e. not Header ONLY)
  if ( 
      (outer_p) &&
      ( obj->options && (obj->options->part_to_load == NULL) ) &&
      (obj->options->headers != MimeHeadersOnly)
     )
    mimeEmitterEndBody(obj->options);

  if (outer_p &&
	  obj->options &&
	  obj->options->write_html_p &&
	  obj->options->generate_footer_html_fn)
	{
	  char *html =
		obj->options->generate_footer_html_fn (NULL,
											   obj->options->html_closure,
											   msg->hdrs);
	  if (html)
		{
		  int lstatus = MimeObject_write(obj, html, nsCRT::strlen(html), PR_FALSE);
		  PR_Free(html);
		  if (lstatus < 0) return lstatus;
		}
	}

#ifdef MIME_DRAFTS
  if ( obj->options &&
	   obj->options->decompose_file_p &&
	   obj->options->done_parsing_outer_headers &&
	   ! obj->options->is_multipart_msg &&
#ifdef MOZ_SECURITY
	   HG00234
#endif /* MOZ_SECURITY */
	   obj->options->decompose_file_close_fn ) {
	status = obj->options->decompose_file_close_fn (
											   obj->options->stream_closure );

	if ( status < 0 ) return status;
  }
#endif /* MIME_DRAFTS */


  /* Put out a separator after every message/rfc822 object. */
  if (!abort_p)
	{
	  status = MimeObject_write_separator(obj);
	  if (status < 0) return status;
	}

  return 0;
}


static int
MimeMessage_add_child (MimeObject *parent, MimeObject *child)
{
  MimeContainer *cont = (MimeContainer *) parent;
  PR_ASSERT(parent && child);
  if (!parent || !child) return -1;

  /* message/rfc822 containers can only have one child. */
  PR_ASSERT(cont->nchildren == 0);
  if (cont->nchildren != 0) return -1;

#ifdef MIME_DRAFTS
  if ( parent->options &&
	   parent->options->decompose_file_p &&
	   ! parent->options->is_multipart_msg &&
#ifdef MOZ_SECURITY
	   HG00234
#endif /* MOZ_SECURITY */
	   parent->options->decompose_file_init_fn ) {
	int status = 0;
	status = parent->options->decompose_file_init_fn (
											  parent->options->stream_closure,
											  ((MimeMessage*)parent)->hdrs );
	if ( status < 0 ) return status;
  }
#endif /* MIME_DRAFTS */
  
  return ((MimeContainerClass*)&MIME_SUPERCLASS)->add_child (parent, child);
}


static int
MimeMessage_write_headers_html (MimeObject *obj)
{
  MimeMessage     *msg = (MimeMessage *) obj;
  int             status;

#ifdef MOZ_SECURITY
  HG33391
#endif /* MOZ_SECURITY */  

  if (!obj->options || !obj->options->output_fn)
  	return 0;

  PR_ASSERT(obj->output_p && obj->options->write_html_p);

  // To support the no header option! Make sure we are not
  // suppressing headers on included email messages...
  if ( (obj->options->headers == MimeHeadersNone) &&
       (obj == obj->options->state->root) )
    return 0;

  if (!obj->options->state->first_data_written_p)
	{
	  status = MimeObject_output_init (obj, TEXT_HTML);
	  if (status < 0) 
    {
      mimeEmitterEndHeader(obj->options);
      return status;
    }
	  PR_ASSERT(obj->options->state->first_data_written_p);
	}

  // Start the header parsing by the emitter
  char *msgID = MimeHeaders_get (msg->hdrs, HEADER_MESSAGE_ID,
									                  PR_FALSE, PR_FALSE);
  PRBool outer_p = !obj->headers; /* is this the outermost message? */
  mimeEmitterStartHeader(obj->options, 
                            outer_p, 
                            (obj->options->headers == MimeHeadersOnly),
                            msgID,
                            "UTF-8");
  PR_FREEIF(msgID);

#ifdef MOZ_SECURITY
    HG00919 
#endif /* MOZ_SECURITY */

  status = MimeHeaders_write_all_headers (msg->hdrs, obj->options, FALSE);
  if (status < 0) 
  {
    mimeEmitterEndHeader(obj->options);
    return status;
  }

  if (msg->xlation_stamped_p)
  {
#ifdef MOZ_SECURITY
    HG11995
#endif /* MOZ_SECURITY */	  
  }
  else
  {
  /* If we're not writing a xlation stamp, and this is the outermost
  message, then now is the time to run the post_header_html_fn.
  (Otherwise, it will be run when the xlation-stamp is finally
  closed off, in MimeXlateed_emit_buffered_child() or
  MimeMultipartSigned_emit_child().)
	   */
    if (obj->options &&
      obj->options->state &&
      obj->options->generate_post_header_html_fn &&
      !obj->options->state->post_header_html_run_p)
    {
      char *html = 0;
      PR_ASSERT(obj->options->state->first_data_written_p);
      html = obj->options->generate_post_header_html_fn(NULL,
                                          obj->options->html_closure,
                                          msg->hdrs);
      obj->options->state->post_header_html_run_p = PR_TRUE;
      if (html)
      {
        status = MimeObject_write(obj, html, nsCRT::strlen(html), PR_FALSE);
        PR_Free(html);
        if (status < 0) 
        {
          mimeEmitterEndHeader(obj->options);
          return status;
        }
      }
    }    
  }

  mimeEmitterEndHeader(obj->options);

  // rhp:
  // For now, we are going to parse the entire message, even if we are
  // only interested in headers...why? Well, because this is the only
  // way to build the attachment list. Now we will have the attachment
  // list in the output being created by the XML emitter. If we ever
  // want to go back to where we were before, just uncomment the conditional
  // and it will stop at header parsing.
  //
  // if (obj->options->headers == MimeHeadersOnly)
  //   return -1;
  // else

  return 0;
}


#if defined(DEBUG) && defined(XP_UNIX)
static int
MimeMessage_debug_print (MimeObject *obj, PRFileDesc *stream, PRInt32 depth)
{
  MimeMessage *msg = (MimeMessage *) obj;
  char *addr = mime_part_address(obj);
  int i;
  for (i=0; i < depth; i++)
	PR_Write(stream, "  ", 2);
/*
  fprintf(stream, "<%s %s%s 0x%08X>\n",
		  obj->clazz->class_name,
		  addr ? addr : "???",
		  (msg->container.nchildren == 0 ? " (no body)" : ""),
		  (PRUint32) msg);
*/
  PR_FREEIF(addr);

#if 0
  if (msg->hdrs)
	{
	  char *s;

	  depth++;

# define DUMP(HEADER) \
	  for (i=0; i < depth; i++)												\
        PR_Write(stream, "  ", 2);												\
	  s = MimeHeaders_get (msg->hdrs, HEADER, PR_FALSE, PR_TRUE);
/**
    \
	  PR_Write(stream, HEADER ": %s\n", s ? s : "");							\
**/

	  PR_FREEIF(s)

      DUMP(HEADER_SUBJECT);
      DUMP(HEADER_DATE);
      DUMP(HEADER_FROM);
      DUMP(HEADER_TO);
      /* DUMP(HEADER_CC); */
      DUMP(HEADER_NEWSGROUPS);
      DUMP(HEADER_MESSAGE_ID);
# undef DUMP

	  PR_Write(stream, "\n", 1);
	}
#endif

  PR_ASSERT(msg->container.nchildren <= 1);
  if (msg->container.nchildren == 1)
	{
	  MimeObject *kid = msg->container.children[0];
	  int status = kid->clazz->debug_print (kid, stream, depth+1);
	  if (status < 0) return status;
	}
  return 0;
}
#endif
