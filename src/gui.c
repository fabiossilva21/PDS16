#include <stdio.h>
#include <term.h>
#include <stdlib.h>

unsigned int getTermWidth(){
        char const *const term = getenv( "TERM" );
        if (!term) {
                fprintf(stderr, "TERM environment variable not set\n");
                return 0;
        }

        char term_buf[1024];
        switch (tgetent(term_buf, term)) {
                case -1:
                fprintf( stderr, "tgetent() failed: terminfo database not found\n" );
                return 0;
                case 0:
                fprintf( stderr, "tgetent() failed: TERM=%s not found\n", term );
                return 0;
        } // switch

        int const cols = tgetnum("co");  // number of (co)lumns
        if (cols == -1) {
                fprintf( stderr, "tgetnum() failed\n" );
                return 0;
        }
        return cols;
}

unsigned int getTermHeight(){
        char const *const term = getenv( "TERM" );
        if (!term) {
                fprintf(stderr, "TERM environment variable not set\n");
                return 0;
        }

        char term_buf[1024];
        switch (tgetent(term_buf, term)) {
                case -1:
                fprintf( stderr, "tgetent() failed: terminfo database not found\n" );
                return 0;
                case 0:
                fprintf( stderr, "tgetent() failed: TERM=%s not found\n", term );
                return 0;
        } // switch

        int const cols = tgetnum("li");  // number of (co)lumns
        if (cols == -1) {
                fprintf( stderr, "tgetnum() failed\n" );
                return 0;
        }
        return cols;
}
