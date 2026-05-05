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

#endif  // FEATHERDOC_PDFIO_ZLIB_COMPAT_ZLIB_H
