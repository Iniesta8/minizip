/* mz_strm.c -- Stream interface
   Version 2.7.4, November 6, 2018
   part of the MiniZip project

   Copyright (C) 2010-2018 Nathan Moinvaziri
     https://github.com/nmoinvaz/minizip

   This program is distributed under the terms of the same license as zlib.
   See the accompanying LICENSE file for the full text of the license.
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "mz.h"
#include "mz_strm.h"

/***************************************************************************/

int32_t mz_stream_open(void *stream, const char *path, int32_t mode)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->open == NULL)
        return MZ_STREAM_ERROR;
    return strm->vtbl->open(strm, path, mode);
}

int32_t mz_stream_is_open(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->is_open == NULL)
        return MZ_STREAM_ERROR;
    return strm->vtbl->is_open(strm);
}

int32_t mz_stream_read(void *stream, void *buf, int32_t size)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->read == NULL)
        return MZ_PARAM_ERROR;
    if (mz_stream_is_open(stream) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->vtbl->read(strm, buf, size);
}

static int32_t mz_stream_read_value(void *stream, uint64_t *value, int32_t len)
{
    uint8_t buf[8];
    int32_t n = 0;
    int32_t i = 0;

    *value = 0;
    if (mz_stream_read(stream, buf, len) == len)
    {
        for (n = 0; n < len; n += 1, i += 8)
            *value += ((uint64_t)buf[n]) << i;
    }
    else if (mz_stream_error(stream))
        return MZ_STREAM_ERROR;
    else
        return MZ_END_OF_STREAM;

    return MZ_OK;
}

int32_t mz_stream_read_uint8(void *stream, uint8_t *value)
{
    int32_t err = MZ_OK;
    uint64_t value64 = 0;

    *value = 0;
    err = mz_stream_read_value(stream, &value64, sizeof(uint8_t));
    if (err == MZ_OK)
        *value = (uint8_t)value64;
    return err;
}

int32_t mz_stream_read_uint16(void *stream, uint16_t *value)
{
    int32_t err = MZ_OK;
    uint64_t value64 = 0;

    *value = 0;
    err = mz_stream_read_value(stream, &value64, sizeof(uint16_t));
    if (err == MZ_OK)
        *value = (uint16_t)value64;
    return err;
}

int32_t mz_stream_read_uint32(void *stream, uint32_t *value)
{
    int32_t err = MZ_OK;
    uint64_t value64 = 0;

    *value = 0;
    err = mz_stream_read_value(stream, &value64, sizeof(uint32_t));
    if (err == MZ_OK)
        *value = (uint32_t)value64;
    return err;
}

int32_t mz_stream_read_int64(void *stream, int64_t *value)
{
    return mz_stream_read_value(stream, (uint64_t *)value, sizeof(uint64_t));
}

int32_t mz_stream_read_uint64(void *stream, uint64_t *value)
{
    return mz_stream_read_value(stream, value, sizeof(uint64_t));
}

int32_t mz_stream_write(void *stream, const void *buf, int32_t size)
{
    mz_stream *strm = (mz_stream *)stream;
    if (size == 0)
        return size;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->write == NULL)
        return MZ_PARAM_ERROR;
    if (mz_stream_is_open(stream) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->vtbl->write(strm, buf, size);
}

static int32_t mz_stream_write_value(void *stream, uint64_t value, int32_t len)
{
    uint8_t buf[8];
    int32_t n = 0;

    for (n = 0; n < len; n += 1)
    {
        buf[n] = (uint8_t)(value & 0xff);
        value >>= 8;
    }

    if (value != 0)
    {
        // Data overflow - hack for ZIP64 (X Roche)
        for (n = 0; n < len; n += 1)
            buf[n] = 0xff;
    }

    if (mz_stream_write(stream, buf, len) != len)
        return MZ_STREAM_ERROR;

    return MZ_OK;
}

int32_t mz_stream_write_uint8(void *stream, uint8_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint8_t));
}

int32_t mz_stream_write_uint16(void *stream, uint16_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint16_t));
}

int32_t mz_stream_write_uint32(void *stream, uint32_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint32_t));
}

int32_t mz_stream_write_int64(void *stream, int64_t value)
{
    return mz_stream_write_value(stream, (uint64_t)value, sizeof(uint64_t));
}

int32_t mz_stream_write_uint64(void *stream, uint64_t value)
{
    return mz_stream_write_value(stream, value, sizeof(uint64_t));
}

int32_t mz_stream_write_chars(void *stream, const char *value, uint8_t null_terminate)
{
    int32_t len = (int32_t)strlen(value);
    if (null_terminate)
        len += 1;
    return mz_stream_write(stream, value, len);
}

int32_t mz_stream_copy(void *target, void *source, int32_t len)
{
    return mz_stream_copy_stream(target, NULL, source, NULL, len);
}

int32_t mz_stream_copy_stream(void *target, mz_stream_write_cb write_cb, void *source, mz_stream_read_cb read_cb, int32_t len)
{
    uint8_t buf[16384];
    int32_t bytes_to_copy = 0;
    int32_t read = 0;
    int32_t written = 0;

    if (write_cb == NULL)
        write_cb = mz_stream_write;
    if (read_cb == NULL)
        read_cb = mz_stream_read;

    while (len > 0)
    {
        bytes_to_copy = len;
        if (bytes_to_copy > (int32_t)sizeof(buf))
            bytes_to_copy = sizeof(buf);
        read = read_cb(source, buf, bytes_to_copy);
        if (read <= 0)
            return MZ_STREAM_ERROR;
        written = write_cb(target, buf, read);
        if (written != read)
            return MZ_STREAM_ERROR;
        len -= read;
    }

    return MZ_OK;
}

int64_t mz_stream_tell(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->tell == NULL)
        return MZ_PARAM_ERROR;
    if (mz_stream_is_open(stream) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->vtbl->tell(strm);
}

int32_t mz_stream_seek(void *stream, int64_t offset, int32_t origin)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->seek == NULL)
        return MZ_PARAM_ERROR;
    if (mz_stream_is_open(stream) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->vtbl->seek(strm, offset, origin);
}

int32_t mz_stream_close(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->close == NULL)
        return MZ_PARAM_ERROR;
    if (mz_stream_is_open(stream) != MZ_OK)
        return MZ_STREAM_ERROR;
    return strm->vtbl->close(strm);
}

int32_t mz_stream_error(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->error == NULL)
        return MZ_PARAM_ERROR;
    return strm->vtbl->error(strm);
}

int32_t mz_stream_set_base(void *stream, void *base)
{
    mz_stream *strm = (mz_stream *)stream;
    strm->base = (mz_stream *)base;
    return MZ_OK;
}

void* mz_stream_get_interface(void *stream)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL)
        return NULL;
    return (void *)strm->vtbl;
}

int32_t mz_stream_get_prop_int64(void *stream, int32_t prop, int64_t *value)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->get_prop_int64 == NULL)
        return MZ_PARAM_ERROR;
    return strm->vtbl->get_prop_int64(stream, prop, value);
}

int32_t mz_stream_set_prop_int64(void *stream, int32_t prop, int64_t value)
{
    mz_stream *strm = (mz_stream *)stream;
    if (strm == NULL || strm->vtbl == NULL || strm->vtbl->set_prop_int64 == NULL)
        return MZ_PARAM_ERROR;
    return strm->vtbl->set_prop_int64(stream, prop, value);
}

void *mz_stream_create(void **stream, mz_stream_vtbl *vtbl)
{
    if (stream == NULL)
        return NULL;
    if (vtbl == NULL || vtbl->create == NULL)
        return NULL;
    return vtbl->create(stream);
}

void mz_stream_delete(void **stream)
{
    mz_stream *strm = NULL;
    if (stream == NULL)
        return;
    strm = (mz_stream *)*stream;
    if (strm != NULL && strm->vtbl != NULL && strm->vtbl->destroy != NULL)
        strm->vtbl->destroy(stream);
    *stream = NULL;
}

/***************************************************************************/

typedef struct mz_stream_raw_s {
    mz_stream   stream;
    int64_t     total_in;
    int64_t     total_out;
    int64_t     max_total_in;
} mz_stream_raw;

/***************************************************************************/

int32_t mz_stream_raw_open(void *stream, const char *path, int32_t mode)
{
    MZ_UNUSED(stream);
    MZ_UNUSED(path);
    MZ_UNUSED(mode);

    return MZ_OK;
}

int32_t mz_stream_raw_is_open(void *stream)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    return mz_stream_is_open(raw->stream.base);
}

int32_t mz_stream_raw_read(void *stream, void *buf, int32_t size)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    int32_t bytes_to_read = size;
    int32_t read = 0;

    if (raw->max_total_in > 0)
    {
        if ((raw->max_total_in - raw->total_in) < size)
            bytes_to_read = (int32_t)(raw->max_total_in - raw->total_in);
    }

    read = mz_stream_read(raw->stream.base, buf, bytes_to_read);

    if (read > 0)
        raw->total_in += read;

    return read;
}

int32_t mz_stream_raw_write(void *stream, const void *buf, int32_t size)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    int32_t written = mz_stream_write(raw->stream.base, buf, size);
    if (written > 0)
        raw->total_out += written;
    return written;
}

int64_t mz_stream_raw_tell(void *stream)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    return mz_stream_tell(raw->stream.base);
}

int32_t mz_stream_raw_seek(void *stream, int64_t offset, int32_t origin)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    return mz_stream_seek(raw->stream.base, offset, origin);
}

int32_t mz_stream_raw_close(void *stream)
{
    MZ_UNUSED(stream);

    return MZ_OK;
}

int32_t mz_stream_raw_error(void *stream)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    return mz_stream_error(raw->stream.base);
}

int32_t mz_stream_raw_get_prop_int64(void *stream, int32_t prop, int64_t *value)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    switch (prop)
    {
    case MZ_STREAM_PROP_TOTAL_IN:
        *value = raw->total_in;
        return MZ_OK;
    case MZ_STREAM_PROP_TOTAL_OUT:
        *value = raw->total_out;
        return MZ_OK;
    }
    return MZ_EXIST_ERROR;
}

int32_t mz_stream_raw_set_prop_int64(void *stream, int32_t prop, int64_t value)
{
    mz_stream_raw *raw = (mz_stream_raw *)stream;
    switch (prop)
    {
    case MZ_STREAM_PROP_TOTAL_IN_MAX:
        raw->max_total_in = value;
        return MZ_OK;
    }
    return MZ_EXIST_ERROR;
}

/***************************************************************************/

static mz_stream_vtbl mz_stream_raw_vtbl = {
    mz_stream_raw_open,
    mz_stream_raw_is_open,
    mz_stream_raw_read,
    mz_stream_raw_write,
    mz_stream_raw_tell,
    mz_stream_raw_seek,
    mz_stream_raw_close,
    mz_stream_raw_error,
    mz_stream_raw_create,
    mz_stream_raw_delete,
    mz_stream_raw_get_prop_int64,
    mz_stream_raw_set_prop_int64
};

/***************************************************************************/

void *mz_stream_raw_create(void **stream)
{
    mz_stream_raw *raw = NULL;

    raw = (mz_stream_raw *)MZ_ALLOC(sizeof(mz_stream_raw));
    if (raw != NULL)
    {
        memset(raw, 0, sizeof(mz_stream_raw));
        raw->stream.vtbl = &mz_stream_raw_vtbl;
    }
    if (stream != NULL)
        *stream = raw;

    return raw;
}

void mz_stream_raw_delete(void **stream)
{
    mz_stream_raw *raw = NULL;
    if (stream == NULL)
        return;
    raw = (mz_stream_raw *)*stream;
    if (raw != NULL)
        MZ_FREE(raw);
    *stream = NULL;
}
