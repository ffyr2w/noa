/**
 * @file ImageFile.h
 * @brief ImageFile class. The interface to specialized image files.
 * @author Thomas - ffyr2w
 * @date 19 Dec 2020
 */
#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <ios>
#include <filesystem>
#include <string>

#include "noa/Types.h"
#include "noa/Errno.h"
#include "noa/util/IO.h"
#include "noa/util/IntX.h"
#include "noa/util/FloatX.h"

namespace Noa {
    /**
     * The ImageFile tries to offer an uniform API to work with all image files
     * (e.g. @a MRCFile, @a TIFFile, @a EERFile, etc.).
     */
    class ImageFile {
    public:
        ImageFile() = default;
        virtual ~ImageFile() = default;

        /**
         * Returns an ImageFile with a path, but the file is NOT opened.
         * @return  One of the derived ImageFile or nullptr if the extension is not recognized.
         * @warning Before opening the file, check that the returned pointer is valid.
         *
         * @note    @c new(std::nothrow) could be used to prevent a potential bad_alloc, but in this case
         *          returning a nullptr could also be because the extension is not recognized...
         *
         * @note    The previous implementation was using an opaque pointer, but this is much simpler and probably
         *          safer since it is obvious that we are dealing with a unique_ptr and that it is necessary to check
         *          whether or not it is a nullptr before using it.
         */
        [[nodiscard]] static std::unique_ptr<ImageFile> get(const std::string& extension);

        [[nodiscard]] inline static std::unique_ptr<ImageFile> get(const path_t& extension) {
            return get(extension.string());
        }

        // Below are the functions that derived classes should override.
        //  ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓   ↓
    public:
        /**
         * (Re)Opens the file.
         * @param[in] mode      Should be one or a combination of the following:
         *                      @c in:              Read.           File should exists.
         *                      @c in|out:          Read & Write.   File should exists.     Backup copy.
         *                      @c out, out|trunc:  Write.          Overwrite the file.     Backup move.
         *                      @c in|out|trunc:    Read & Write.   Overwrite the file.     Backup move.
         * @param[in] wait      Wait for the file to exist for 10*3s, otherwise wait for 5*10ms.
         * @return              Any of the following error number:
         *                      @c Errno::invalid_state, if the image file type is not recognized.
         *                      @c Errno::fail_close, if failed to close the file before starting.
         *                      @c Errno::fail_open, if failed to open the file.
         *                      @c Errno::fail_os, if an underlying OS error was raised.
         *                      @c Errno::fail_read, if the
         *                      @c Errno::not_supported, if the file format is not supported.
         *                      @c Errno::invalid_data, if the file is not recognized.
         *                      @c Errno::good, otherwise.
         *
         * @note                Internally, the @c std::ios::binary is always considered on. On the
         *                      other hand, @c std::ios::app and @c std::ios::ate are always
         *                      considered off. Changing any of these bits has no effect.
         */
        virtual Errno open(openmode_t mode, bool wait) = 0;

        /** Resets the path and opens the file. */
        virtual Errno open(const path_t&, openmode_t, bool) = 0;

        /** Resets the path and opens the file. */
        virtual Errno open(path_t&&, openmode_t, bool) = 0;

        /** Whether or not the file is open. */
        [[nodiscard]] virtual bool isOpen() const = 0;

        /** Closes the file. For some file format, there can be write operation to save buffered data. */
        virtual Errno close() = 0;

        /** Gets the path. */
        [[nodiscard]] virtual const path_t* path() const noexcept = 0;

        [[nodiscard]] virtual size3_t getShape() const = 0;
        virtual Errno setShape(size3_t) = 0;

        [[nodiscard]] virtual Float3<float> getPixelSize() const = 0;
        virtual Errno setPixelSize(Float3<float>) = 0;

        [[nodiscard]] virtual std::string toString(bool) const = 0;

        /** Returns the underlying data type. */
        virtual IO::DataType getDataType() const = 0;

        /** Sets the data type for all future writing operation. */
        virtual Errno setDataType(IO::DataType) = 0;

        /**
         * Reads the entire data into @a ptr_out. The ordering is (x=1, y=2, z=3).
         * @param[out] ptr_out   Output array. Should be at least equal to @c Math::elements(getShape()) * 4.
         * @return               Any returned @c Errno from IO::readFloat().
         * @note The underlying data should be a real (as opposed to complex) type.
         */
        virtual Errno readAll(float* ptr_out) = 0;

        /**
         * Reads the entire data into @a out. The ordering is (x=1, y=2, z=3).
         * @param[out] ptr_out   Output array. Should be at least equal to @c Math::elements(getShape()) * 8.
         * @return               Any @c Errno returned by IO::readComplexFloat().
         *                       Errno::not_supported, if the file format does not support complex data.
         * @note The underlying data should be a complex type.
         */
        virtual Errno readAll(cfloat_t* ptr_out) = 0;

        /**
         * Reads slices (z sections) from the file data and stores the data into @a out.
         * @note Slices refer to whatever dimension is in the last order, but since the only
         *       data ordering allowed is (x=1, y=2, z=3), slices are effectively z sections.
         * @note The underlying data should be a real (as opposed to complex) type.
         *
         * @param[out] ptr_out  Output float array. It should be large enough to contain the desired
         *                      data, that is `Math::elements(getShape()) * 4 * z_count` bytes.
         * @param z_pos         Slice to start reading from.
         * @param z_count       Number of slices to read.
         * @return Errno        Any @c Errno returned by IO::readFloat().
         */
        virtual Errno readSlice(float* ptr_out, size_t z_pos, size_t z_count) = 0;

        /**
         * Reads slices (z sections) from the file data and stores the data into @a out.
         * @note The underlying data should be a complex type.
         *
         * @param[out] ptr_out  Output float array. It should be large enough to contain the desired
         *                      data, that is `Math::elements(getShape()) * 8 * z_count` bytes.
         * @param z_pos         Slice to start reading from.
         * @param z_count       Number of slices to read.
         * @return Errno        Any @c Errno returned by IO::readComplexFloat().
         *                      Errno::not_supported, if the file format does not support complex data.
         */
        virtual Errno readSlice(cfloat_t* ptr_out, size_t z_pos, size_t z_count) = 0;

        /**
         * Writes the entire file. The ordering is expected to be (x=1, y=2, z=3).
         * @param[in] ptr_in    Array to write. Should be at least `Math::elements(getShape()) * 4` bytes.
         * @return Errno        Any @c Errno returned by IO::writeFloat().
         * @note The underlying data should be a real (as opposed to complex) type.
         */
        virtual Errno writeAll(const float* ptr_in) = 0;

        /**
         * Writes the entire file. The ordering is expected to be (x=1, y=2, z=3).
         * @param[in] ptr_in    Array to serialize. Should be at least `Math::elements(getShape()) * 8` bytes.
         * @return Errno        Any @c Errno returned by IO::writeComplexFloat().
         * @note The underlying data should be a complex type.
         */
        virtual Errno writeAll(const cfloat_t* ptr_in) = 0;

        /**
         * Writes slices (z sections) from @a out into the file.
         * @param[out] ptr_out  Array to serialize.
         * @param z_pos         Slice to start.
         * @param z_count       Number of slices to serialize.
         * @return Errno        Any @c Errno returned by IO::writeFloat().
         * @note The underlying data should be a real (as opposed to complex) type.
         */
        virtual Errno writeSlice(const float* ptr_out, size_t z_pos, size_t z_count) = 0;

        /**
         * Writes slices (z sections) from @a out into the file.
         * @param[out] ptr_out  Array to serialize.
         * @param z_pos         Slice to start.
         * @param z_count       Number of slices to serialize.
         * @return Errno        Any @c Errno returned by IO::writeComplexFloat().
         * @note The underlying data should be a complex type.
         */
        virtual Errno writeSlice(const cfloat_t* ptr_out, size_t z_pos, size_t z_count) = 0;
    };
}
