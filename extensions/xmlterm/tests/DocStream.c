/* DocStream.c: Display complete HTML document using IFRAME */

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
  char ch;
  char* cookie;

  cookie = getenv("LTERM_COOKIE");  /* Get security cookie */
  if (cookie == NULL)
    cookie = "";

  /* XMLTerm escape sequence signalling start of a HTML document */
  printf("\033{1S%s\n", cookie);

  printf(" <HTML><BODY> \
<FORM> \
<IMG align=center src='chrome://navigator/skin/animthrob_single.gif'> \
   <B>Please click a button</B> <BR> \
<INPUT ID='button-b#' TYPE=button VALUE='Bold' \
  onClick=\"return HandleEvent(event, 'click', 'sendln','#','b')\"> \
<INPUT ID='button-e#' TYPE=button VALUE='Emphasis' \
  onClick=\"return HandleEvent(event, 'click', 'sendln','#','e')\"> \
<INPUT ID='button-q#' TYPE=button VALUE='Quit' \
  onClick=\"return HandleEvent(event, 'click', 'sendln','#','q')\"> \
<BR></FORM> \
</BODY></HTML>");

  /* XMLTerm escape sequence signalling end of stream */
  printf("%c", '\0');

  while((ch = getchar())){ /* Poll for data generated by button click events */
    switch (ch) {
    case 'b':
      printf("\033{S%s\n<B>Hello World!</B><BR> %c", cookie, '\0');
      break;
    case 'e':
      printf("\033{S%s\n<EM>Hello World!</EM><BR> %c", cookie, '\0');
      break;
    case 'q':
      return 0;
      break;
    default:
      break;
    }
  }

  return 0;
}
