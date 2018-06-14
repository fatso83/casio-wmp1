#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <signal.h>

#include <globaldefs.h>

#ifdef INCLUDE_STD_ULCASE
void strucase (char *s) {
   register unsigned char *p = (unsigned char *)s;
   while (*p != 0) *p++ = toupper(*p);
 }

void strlcase (char *str) {
   char *p = str;

   while (*p) {
      *p = tolower(*p);
      ++p;
    }
 }
#endif
#ifdef INCLUDE_STD_MAIN
   /* Global variables, available on any project */
   APIRET rc;
   INT Signal_Caught;
   INT Signal_Interrupted;
   INT Signal_Terminate;

   USHORT maincode (int argc, char *argv[]);

   /* Our own default Signal-Handler */
   static void sighandler (int sig) {
       switch (sig) {
         case SIGINT: {
          Signal_Interrupted = 1; Signal_Caught = 1;
          fputs ("SIGINT!\n", stdout);
          break; }
         case SIGTERM: {
          Signal_Terminate = 1; Signal_Caught = 1;
          fputs ("SIGTERM!\n", stdout);
           break; }
        }
    }

   /* SIGINT  - Interrupt Ctrl-Break
      SIGTERM - Terminate Signal */

   /* Execution begins here, because we need to set some things up, before
       giving control to our actual project */

   int main (int argc, char **argv) {
      if (SIG_ERR == signal(SIGINT, sighandler)) return 1;
      rc = maincode (argc, argv);
      return rc;
    }
#endif
