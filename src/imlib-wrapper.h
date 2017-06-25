/*
	Including <Imlib.h> has a nasty side effect:
	It pulls in <Xos.h> which defines a couple of macros like "index", "rindex", "None" or
	"Enum" which clash with the typical naming of Qt variables and parameters.

	So this wrapper file first pulls in <Imlib.h> and then removes all macros that would
	cause compile errors in this project.
*/

#ifndef IMLIB_WRAPPER_H
#define IMLIB_WRAPPER_H

#include <Imlib.h>

#undef index
#undef rindex
#undef None

#endif
