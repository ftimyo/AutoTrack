#include "Public.h"
#include <cstdio>
#include <cstdlib>

void abortError( const int line, const char *file, const char *msg) 
{
	if( msg==0 )
		fprintf(stderr, "%s %d: ERROR\n", file, line );
	else
		fprintf(stderr, "%s %d: ERROR: %s\n", file, line, msg );
	exit(0);
}