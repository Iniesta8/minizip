/* mz_strm_bzip.h -- Stream for bzip inflate/deflate
   Version 2.0.1, October 16th, 2017
   part of the MiniZip project

   Copyright (C) 2012-2017 Nathan Moinvaziri
      https://github.com/nmoinvaz/minizip

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#ifndef _MZ_STREAM_BZIP_H
#define _MZ_STREAM_BZIP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/

int32_t mz_stream_bzip_open(void *stream, const char *filename, int32_t mode);
int32_t mz_stream_bzip_is_open(void *stream);
int32_t mz_stream_bzip_read(void *stream, void *buf, int32_t size);
int32_t mz_stream_bzip_write(void *stream, const void *buf, int32_t size);
int64_t mz_stream_bzip_tell(void *stream);
int32_t mz_stream_bzip_seek(void *stream, int64_t offset, int32_t origin);
int32_t mz_stream_bzip_close(void *stream);
int32_t mz_stream_bzip_error(void *stream);

int32_t mz_stream_bzip_get_prop_int64(void *stream, int32_t prop, int64_t *value);
int32_t mz_stream_bzip_set_prop_int64(void *stream, int32_t prop, int64_t value);

void*   mz_stream_bzip_create(void **stream);
void    mz_stream_bzip_delete(void **stream);

void*   mz_stream_bzip_get_interface(void);

/***************************************************************************/

#ifdef __cplusplus
}
#endif

#endif