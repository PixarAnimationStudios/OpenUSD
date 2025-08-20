//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_FILE_IO_H
#define PXR_USD_SDF_FILE_IO_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/fileVersion.h"
#include "pxr/usd/ar/ar.h"

#include "pxr/usd/ar/writableAsset.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <ostream>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

class SdfSpec;

// ArWritableAsset implementation that writes to a std::ostream.
class Sdf_StreamWritableAsset
    : public ArWritableAsset
{
public:
    SDF_API
    explicit Sdf_StreamWritableAsset(std::ostream& out)
    : _offset(0)
    , _out(out)
    {
    }

    SDF_API
    virtual ~Sdf_StreamWritableAsset();

    bool Close() override
    {
        _out.flush();
        _offset = 0;
        return true;
    }

    size_t Write(const void* buffer, size_t count, size_t offset) override
    {
        if (offset != _offset) {
            // The caller wants to seek. This may fail depending on the
            // ostream. Streams to pipes or terminals are not seekable. If the
            // seek fails, it may even raise an exception so handle it
            // carefully.
            try {
                _out.seekp(offset);
            } catch (const std::ios_base::failure&) {
                // Ignore the exception. We're going to check for failure below.
            }

            if (_out.fail()) {
                // We could not seek. Do not output any bytes.
                return 0;
            }

            _offset = offset;
        }

        _out.write(static_cast<const char*>(buffer), count);
        _offset += count;

        return count;
    }

private:
    size_t _offset;
    std::ostream& _out;
};

// Class for managing reading and writing multiple versions of text files.
class Sdf_TextOutput
{
public:
    explicit Sdf_TextOutput(std::ostream& out,
                          const std::string& name)
    : Sdf_TextOutput(std::make_shared<Sdf_StreamWritableAsset>(out), name)
    { }

    explicit Sdf_TextOutput(std::shared_ptr<ArWritableAsset>&& asset,
                          const std::string& name)
        : _asset(std::move(asset))
        , _offset(0)
        , _buffer(new char[BUFFER_SIZE])
        , _bufferPos(0)
        , _name(name)
    { }

    ~Sdf_TextOutput()
    {
        if (_asset) {
            Close();
        }
    }

    Sdf_TextOutput(const Sdf_TextOutput&) = delete;
    const Sdf_TextOutput& operator=(const Sdf_TextOutput&) = delete;

    // Close the output, flushing contents to destination.
    bool Close()
    {
        if (!_asset) {
            return true;
        }

        const bool ok = _FlushBuffer() &&
                        _UpdateHeader() &&
                        _asset->Close();
        _asset.reset();
        return ok;
    }

    // Write given \p str to output.
    bool Write(const std::string& str)
    {
        return _Write(str.c_str(), str.length());
    }

    // Write NUL-terminated character string \p str to output.
    bool Write(const char* str)
    {
        return _Write(str, strlen(str));
    }

    // Write the header of a text file. This should be the first output when
    // writing a USD text file. The header consists of a cookie and a
    // version. Because the version may be updated while writing the file if
    // advanced features are encountered, the header is padded with extra white
    // space to allow it to be safely overwritten with a new version if
    // necessary.
    //
    // If version is supplied and is valid then it will be the version written
    // and become the version of the output. If version is not supplied or is
    // not valid, the existing output version will be used.
    //
    SDF_API
    bool WriteHeader(const std::string& cookie,
                     const SdfFileVersion version = SdfFileVersion());

    // Inform the writer that the output stream requires the given version (or
    // newer) to represent all the features in the layer.  This allows the
    // writer to start with a conservative version assumption and promote to
    // newer versions only as required by the data stream contents.
    SDF_API
    bool RequestWriteVersionUpgrade(const SdfFileVersion& ver, std::string reason);

private:
    // Potentially update the version string in the header of the output
    // file. This should be the last output when writing a usda text file and
    // is invoked automatically by \c Close.
    //
    // If \c RequestWriteVersionUpgrade has upgraded the version, this method
    // will update the header at the beginning of the file. Not all outputs
    // support seeking (like terminals or pipes) so a runtime error will be
    // emitted if the version needs to be updated but cannot be.
    SDF_API
    bool _UpdateHeader();

    bool _Write(const char* str, size_t strLength)
    {
        // Much of the text format writing code writes small number of
        // characters at a time. Buffer writes to batch writes into larger
        // chunks.
        while (strLength != 0) {
            const size_t numAvail = BUFFER_SIZE - _bufferPos;
            const size_t numToCopy = std::min(numAvail, strLength);
            memcpy(_buffer.get() + _bufferPos, str, numToCopy);

            _bufferPos += numToCopy;
            str += numToCopy;
            strLength -= numToCopy;

            if (_bufferPos == BUFFER_SIZE) {
                if (!_FlushBuffer()) {
                    return false;
                }
            }
        }
        
        return true;
    }

    bool _FlushBuffer()
    {
        if (_bufferPos == 0) {
            return true;
        }

        const size_t nWritten = _asset->Write(
            _buffer.get(), _bufferPos, _offset);

        if (nWritten != _bufferPos) {
            TF_RUNTIME_ERROR("Failed to write bytes");
            return false;
        }
        _offset += nWritten;
        _bufferPos = 0;
        return true;
    };

    bool _Seek(size_t pos) {
        if (!_FlushBuffer()) {
            return false;
        }

        // We don't actually seek, we just arrange for the next output
        // to occur at pos. If pos is out of range or the output does
        // not support seeking, the next write operation may fail.
        _offset = pos;

        return true;
    }

    std::shared_ptr<ArWritableAsset> _asset;
    size_t _offset;

    const size_t BUFFER_SIZE = 4096;
    std::unique_ptr<char[]> _buffer;
    size_t _bufferPos;

    std::string _cookie;
    SdfFileVersion _writtenVersion;
    SdfFileVersion _requestedVersion;
    std::string _name;
};

// Helper class for writing out strings for the text file format
// into a single string. 
class Sdf_StringOutput
    : public Sdf_TextOutput
{
public:
    explicit Sdf_StringOutput()
    : Sdf_TextOutput(_str, "<string>")
    {
    }

    // Closes the output and returns the text output as a string.
    std::string GetString()
    {
        Close();
        return _str.str();
    }

private:
    std::stringstream _str;
};

// Write the provided \a spec to \a out indented \a indent levels.
bool Sdf_WriteToStream(const SdfSpec &spec, std::ostream& out, size_t indent);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
