/*
 * FeatherDoc local zlib compatibility shim for PDFio.
 *
 * PDFio includes <zlib.h>. FeatherDoc already vendors miniz through the ZIP
 * layer, and miniz provides the zlib subset PDFio needs for Flate streams.
 */

#ifndef FEATHERDOC_PDFIO_ZLIB_COMPAT_ZLIB_H
#define FEATHERDOC_PDFIO_ZLIB_COMPAT_ZLIB_H

#ifndef MINIZ_HEADER_FILE_ONLY
#define MINIZ_HEADER_FILE_ONLY 1
#endif

#include <miniz.h>

/*
 * miniz exposes a high ZLIB_VERNUM, but it does not support zlib's
 * inflateInit2(stream, 0) auto-window mode that libpng enables for zlib
 * >= 1.2.4.  Advertise a conservative zlib level so libpng uses inflateInit()
 * and inflateReset(), which map cleanly onto miniz.
 */
#undef ZLIB_VERSION
#undef ZLIB_VERNUM
#undef ZLIB_VER_MAJOR
#undef ZLIB_VER_MINOR
#undef ZLIB_VER_REVISION
#undef ZLIB_VER_SUBREVISION
#define ZLIB_VERSION "1.2.3-miniz"
#define ZLIB_VERNUM 0x1230
#define ZLIB_VER_MAJOR 1
#define ZLIB_VER_MINOR 2
#define ZLIB_VER_REVISION 3
#define ZLIB_VER_SUBREVISION 0

#endif  // FEATHERDOC_PDFIO_ZLIB_COMPAT_ZLIB_H
