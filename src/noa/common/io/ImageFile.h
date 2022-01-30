/// \file noa/common/files/ImageFile.h
/// \brief ImageFile class. The interface to specialized image files.
/// \author Thomas - ffyr2w
/// \date 19 Dec 2020

#pragma once

#include "noa/common/Definitions.h"
#include "noa/common/Exception.h"
#include "noa/common/Types.h"
#include "noa/common/io/IO.h"
#include "noa/common/io/header/Header.h"

// TODO(TF) Add JPEG, PNG and EER file format. The doc might need to be updated for the read/write functions.
// TODO(TF) TIFF format is not tested! Add tests!

namespace noa::io {
    /// Manipulate image file.
    /// \details
    ///     ImageFile is the unified interface to manipulate image file formats. Only MRC and TIFF files are
    ///     currently supported. TIFF files only support 2D image(s).
    ///     The 4D rightmost shape always puts the batch (number of volumes or number of images) in the outermost
    ///     dimension. As such, stack of 2D images are noted as {batch, 1, height, width}.
    ///     Whether dealing with an image, a stack of images, a volume, or a stack of volumes, this object will
    ///     always use the rightmost ordering, i.e. row/width major order, during (de)serialization. Rightmost shapes
    ///     {batch, depth, height, width} are used regardless of the actual order of the data in the file and the
    ///     data passed to ImageFile should always be in the rightmost order. As such, reading operations might
    ///     flip the data around before returning it, and writing operations will always write the data assuming
    ///     it matches the rightmost order. Consequently, indexing image files is the same as indexing C-style arrays.
    /// \example
    /// \code
    /// const size4_t shape = file.shape(); // {1,60,124,124}
    /// const size_t end = at(0, 0, shape[2] - 1, shape[3] - 1, shape.stride()); // end of first YX slice
    /// assert(end == 15375);
    /// file.read(output, 0, end); // which is equivalent to:
    /// file.readSlice(output, 0, 1);
    /// \endcode
    class ImageFile {
    public:
        /// Creates an empty instance. Use open(), otherwise all other function calls will be ignored.
        ImageFile() = default;

        /// Opens the image file.
        /// \param filename     Path of the image file to open. The file format is deduced from the extension.
        /// \param mode         Open mode. See open() for more details.
        template<typename T>
        NOA_HOST ImageFile(T&& filename, open_mode_t mode);

        /// Opens the image file.
        /// \param filename     Path of the image file to open.
        /// \param file_format  File format used for this file.
        /// \param mode         Open mode. See open() for more details.
        template<typename T>
        NOA_HOST ImageFile(T&& filename, Format file_format, open_mode_t mode);

        ~ImageFile() noexcept(false);
        ImageFile(const ImageFile&) = delete;
        ImageFile(ImageFile&&) = default;

    public:
        /// (Re)Opens the file.
        /// \param filename     Path of the file to open.
        /// \param mode         Open mode. Should be one of the following combination:
        ///                     1) READ:                File should exists.
        ///                     2) READ|WRITE:          File should exists.     Backup copy.
        ///                     3) WRITE or WRITE|TRUNC Overwrite the file.     Backup move.
        ///                     4) READ|WRITE|TRUNC:    Overwrite the file.     Backup move.
        ///
        /// \throws Exception   If any of the following cases:
        ///         - If the file does not exist and \p mode is \c io::READ or \c io::READ|io::WRITE.
        ///         - If the permissions do not match the \p mode.
        ///         - If the file format is not recognized, nor supported.
        ///         - If failed to close the file before starting (if any).
        ///         - If an underlying OS error was raised.
        ///         - If the file header could not be read.
        ///
        /// \note Internally, BINARY is always considered on.
        ///       On the other hand, APP and ATE are always considered off.
        ///       Changing any of these bits has no effect.
        /// \note TIFF files don't support modes 2 and 4.
        template<typename T>
        NOA_HOST void open(T&& filename, uint mode);

        /// Closes the file. In writing mode and for some file format,
        /// there can be write operation to save buffered data.
        NOA_HOST void close();

        /// Whether the file is open.
        [[nodiscard]] NOA_HOST bool isOpen() const noexcept;
        NOA_HOST explicit operator bool() const noexcept;

        /// Returns the file format.
        [[nodiscard]] NOA_HOST Format format() const noexcept;
        [[nodiscard]] NOA_HOST bool isMRC() const noexcept;
        [[nodiscard]] NOA_HOST bool isTIFF() const noexcept;
        [[nodiscard]] NOA_HOST bool isEER() const noexcept;
        [[nodiscard]] NOA_HOST bool isJPEG() const noexcept;
        [[nodiscard]] NOA_HOST bool isPNG() const noexcept;

        /// Gets the path.
        [[nodiscard]] NOA_HOST const path_t& path() const noexcept;

        /// Returns a (brief) description of the file data.
        [[nodiscard]] NOA_HOST std::string info(bool brief) const noexcept;

        /// Gets the rightmost shape of the data.
        [[nodiscard]] NOA_HOST size4_t shape() const noexcept;

        /// Sets the rightmost shape of the data. In pure read mode,
        /// this is usually not allowed and is likely to throw an exception.
        NOA_HOST void shape(size4_t shape);

        /// Gets the rightmost pixel size of the data.
        /// Passing 0 for one or more dimensions is allowed.
        [[nodiscard]] NOA_HOST float3_t pixelSize() const noexcept;

        /// Sets the rightmost pixel size of the data. In pure read mode,
        /// this is usually not allowed and is likely to throw an exception.
        NOA_HOST void pixelSize(float3_t pixel_size);

        /// Gets the type of the serialized data.
        [[nodiscard]] NOA_HOST DataType dtype() const noexcept;

        /// Sets the type of the serialized data. This will affect all future writing operation.
        /// In read mode, this is usually not allowed and is likely to throw an exception.
        NOA_HOST void dtype(DataType data_type);

        /// Gets the statistics of the data.
        /// Some fields might be unset (one should use the has*() function of stats_t before getting the values).
        [[nodiscard]] NOA_HOST stats_t stats() const noexcept;

        /// Sets the statistics of the data. Depending on the open mode and
        /// file format, this might have no effect on the file.
        NOA_HOST void stats(stats_t stats);

        /// Deserializes some elements from the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        /// \param[out] output  Output array where the deserialized elements are saved.
        ///                     Should be able to hold at least \p end - \p start elements.
        /// \param start        Position, in the file, where the deserialization starts, in \p T elements.
        /// \param end          Position, in the file, where the deserialization stops, in \p T elements.
        /// \param clamp        Whether the deserialized values should be clamped to fit the output type \p T.
        ///                     If false, out of range values are undefined.
        /// \warning This is only currently supported for MRC files.
        template<typename T>
        NOA_HOST void read(T* output, size_t start, size_t end, bool clamp = true);

        /// Deserializes some lines from the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        /// \param[out] output  Output array where the deserialized lines are saved.
        ///                     Should be able to hold at least \p end - \p start lines.
        /// \param start        Line, in the file, where the deserialization starts, in \p T elements.
        /// \param end          Line, in the file, where the deserialization stops, in \p T elements.
        /// \param clamp        Whether the deserialized values should be clamped to fit the output type \p T.
        ///                     If false, out of range values are undefined.
        /// \warning This is only currently supported for MRC files.
        template<typename T>
        NOA_HOST void readLine(T* output, size_t start, size_t end, bool clamp = true);

        /// Deserializes some shape (i.e. 3D tile) from the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        /// \param[out] output  Output array where the deserialized shape is saved.
        ///                     Should be able to hold at least a \p shape.
        /// \param offset       Rightmost offset, in the file, where the deserialization starts.
        /// \param shape        Rightmost shape, in \p T elements, to deserialize.
        /// \param clamp        Whether the deserialized values should be clamped to fit the output type \p T.
        ///                     If false, out of range values are undefined.
        /// \todo This is currently not supported.
        template<typename T>
        NOA_HOST void readShape(T* output, size4_t offset, size4_t shape, bool clamp = true);

        /// Deserializes some slices from the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        /// \param[out] output  Output array where the deserialized slices are saved.
        ///                     Should be able to hold at least \p end - \p start slices.
        /// \param start        Slice, in the file, where the deserialization starts, in \p T elements.
        /// \param end          Slice, in the file, where the deserialization stops, in \p T elements.
        /// \param clamp        Whether the deserialized values should be clamped to fit the output type \p T.
        ///                     If false, out of range values are undefined.
        template<typename T>
        NOA_HOST void readSlice(T* output, size_t start, size_t end, bool clamp = true);

        /// Deserializes the entire file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        /// \param[out] output  Output array where the deserialized values are saved.
        ///                     Should be able to hold the entire shape.
        /// \param clamp        Whether the deserialized values should be clamped to fit the output type \p T.
        ///                     If false, out of range values are undefined.
        template<typename T>
        NOA_HOST void readAll(T* output, bool clamp = true);

        /// Serializes some elements into the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        ///                     If the file data type is UINT4, \p T should not be complex.
        ///                     If the file data type is complex, \p T should be complex.
        /// \param[in] input    Input array to serialize. Read from index 0 to index (\p end - start).
        /// \param start        Position, in the file, where the serialization starts, in \p T elements.
        /// \param end          Position, in the file, where the serialization stops, in \p T elements.
        /// \param clamp        Whether the input values should be clamped to fit the file data type.
        ///                     If false, out of range values are undefined.
        /// \warning This is only supported for MRC files.
        template<typename T>
        NOA_HOST void write(const T* input, size_t start, size_t end, bool clamp = true);

        /// Serializes some lines into the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        ///                     If the file data type is UINT4, \p T should not be complex.
        ///                     If the file data type is complex, \p T should be complex.
        /// \param[in] input    Input array to serialize. Read from index 0 to index (\p end - start).
        /// \param start        Line, in the file, where the serialization starts, in \p T elements.
        /// \param end          Line, in the file, where the serialization stops, in \p T elements.
        /// \param clamp        Whether the input values should be clamped to fit the file data type.
        ///                     If false, out of range values are undefined.
        template<typename T>
        NOA_HOST void writeLine(const T* input, size_t start, size_t end, bool clamp = true);

        /// Serializes some lines into the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        ///                     If the file data type is UINT4, \p T should not be complex.
        ///                     If the file data type is complex, \p T should be complex.
        /// \param[in] input    Input array to serialize. An entire contiguous \p shape is read from this array.
        /// \param offset       Rightmost offset, in the file, where the serialization starts.
        /// \param shape        Rightmost shape, in \p T elements, to serialize.
        /// \param clamp        Whether the input values should be clamped to fit the file data type.
        ///                     If false, out of range values are undefined.
        /// \todo This is currently not supported.
        template<typename T>
        NOA_HOST void writeShape(const T* input, size4_t offset, size4_t shape, bool clamp = true);

        /// Serializes some slices into the file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        ///                     If the file data type is UINT4, \p T should not be complex.
        ///                     If the file data type is complex, \p T should be complex.
        /// \param[in] input    Input array to serialize. Read from index 0 to index (\p end - start).
        /// \param start        Slice, in the file, where the serialization starts, in \p T elements.
        /// \param end          Slice, in the file, where the serialization stops, in \p T elements.
        /// \param clamp        Whether the input values should be clamped to fit the file data type.
        ///                     If false, out of range values are undefined.
        template<typename T>
        NOA_HOST void writeSlice(const T* input, size_t start, size_t end, bool clamp = true);

        /// Serializes the entire file.
        /// \tparam T           Any data type (integer, floating-point, complex). See traits::is_data.
        ///                     If the file data type is UINT4, \p T should not be complex.
        ///                     If the file data type is complex, \p T should be complex.
        /// \param[in] input    Input array to serialize. An entire shape is read from this array.
        /// \param clamp        Whether the input values should be clamped to fit the file data type.
        ///                     If false, out of range values are undefined.
        template<typename T>
        NOA_HOST void writeAll(const T* input, bool clamp = true);

    private:
        path_t m_path{};
        std::unique_ptr<details::Header> m_header{};
        Format m_header_format{};
        bool m_is_open{};

    private:
        NOA_HOST void setHeader_(Format new_format);
        NOA_HOST static Format getFormat_(const path_t& extension) noexcept;
        NOA_HOST void open_(open_mode_t mode);
        NOA_HOST void close_();
    };
}

#define NOA_IMAGEFILE_INL_
#include "noa/common/io/ImageFile.inl"
#undef NOA_IMAGEFILE_INL_
