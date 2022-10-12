/*
	Including <Imlib.h> has a nasty side effect:
	It pulls in <Xos.h> which defines a couple of macros like "index", "rindex", "None" or
	"Enum" which clash with the typical naming of Qt variables and parameters.

	So this wrapper file first pulls in <Imlib.h> and then removes all macros that would
	cause compile errors in this project.
*/

#ifndef IMLIB_WRAPPER_H
#define IMLIB_WRAPPER_H

#ifdef HAVE_IMLIB1
#include <Imlib.h>
#endif
#ifdef HAVE_IMLIB2
#include <Imlib2.h>
#endif

#undef index
#undef rindex

#undef Bool
#undef Color
#undef CursorShape
#undef Expose
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef GrayScale
#undef KeyPress
#undef KeyRelease
#undef None
#undef Status
#undef Unsorted

// Although an Imlib2 'Imlib_Image' is conceptually the same as an
// Imlib1 'ImlibImage', they cannot be directly substituted because
// Imlib2 uses the type as an opaque handle (declared as a 'void *')
// while in Imlib1 it is a typedef'ed as a struct and usually passed
// around as a pointer.  So this macro is instead used as an image type.
#ifdef HAVE_IMLIB1
#define IMLIBIMAGE	ImlibImage *
#endif
#ifdef HAVE_IMLIB2
#define IMLIBIMAGE	Imlib_Image
#endif

#ifdef HAVE_IMLIB2
// Data types in Imlib1 which have a directly named equivalent in Imlib2
#define ImlibColorModifier	Imlib_Color_Modifier

// Data types in Imlib1 which are not used in Imlib2.  They are just
// defined like this so that header files will work, use of them in code
// needs to be conditional for Imlib1 only.
#define ImlibInitParams		void *
#define ImlibData		void

// Functions in Imlib1 which do much the same thing in Imlib2
// (to the current image as set in context).
#define Imlib_load_image(id, file)				imlib_load_image(file)
#define Imlib_set_image_modifier(id, im, mod)			imlib_context_set_color_modifier(*mod)
#define Imlib_create_image_from_data(id, data, alpha, w, h)	imlib_create_image_using_copied_data(w, h, reinterpret_cast<DATA32 *>(data))

// See the API documentation for Imlib_rotate_image(), the 'd' parameter
// is not actually used.
#define Imlib_rotate_image(id, im, d)		imlib_image_flip_diagonal()
#define Imlib_flip_image_horizontal(id, im)	imlib_image_flip_horizontal()
#define Imlib_flip_image_vertical(id, im)	imlib_image_flip_vertical()
#endif // HAVE_IMLIB2

// API documentation for Imlib1:  http://web.mit.edu/graphics/src/imlib-1.7/doc/
// API documentation for Imlib2:  https://docs.enlightenment.org/api/imlib2/html/

#endif
