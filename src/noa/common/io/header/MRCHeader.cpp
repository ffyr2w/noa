#include <cstring>  // memcpy, memset
#include <thread>   // std::this_thread::sleep_for
#include <chrono>   // std::chrono::milliseconds

#include "noa/Session.h"
#include "noa/common/OS.h"
#include "noa/common/io/header/MRCHeader.h"

namespace noa::io::details {
    void MRCHeader::shape(size4_t new_shape) {
        if (m_open_mode & OpenMode::READ) {
            if (m_open_mode & OpenMode::WRITE) {
                Session::logger.warn("MRCHeader: changing the shape of the data in "
                                     "read|write mode might corrupt the file");
            } else {
                NOA_THROW("Trying to change the shape of the data in read mode is not allowed. "
                          "Hint: to fix the header of a file, open it in read|write mode");
            }
        }
        m_header.shape = new_shape;
    }

    void MRCHeader::dtype(io::DataType data_type) {
        if (m_open_mode & io::READ) {
            if (m_open_mode & OpenMode::WRITE) {
                Session::logger.warn("MRCHeader: changing the data type of the file in "
                                     "read|write mode might corrupt the file");
            } else {
                NOA_THROW("Trying to change the data type of the file in read mode is not allowed. "
                          "Hint: to fix the header of a file, open it in read|write mode");
            }
        }
        switch (data_type) {
            case DataType::UINT4:
            case DataType::INT8:
            case DataType::UINT8:
            case DataType::INT16:
            case DataType::UINT16:
            case DataType::FLOAT16:
            case DataType::FLOAT32:
            case DataType::CFLOAT32:
            case DataType::CINT16:
                m_header.data_type = data_type;
                break;
            default:
                NOA_THROW("Data type {} is not supported", data_type);
        }
    }

    void MRCHeader::pixelSize(float3_t new_pixel_size) {
        if (m_open_mode & io::READ) {
            if (m_open_mode & OpenMode::WRITE) {
                Session::logger.warn("MRCHeader: changing the pixel size of the file in "
                                     "read|write mode might corrupt the file");
            } else {
                NOA_THROW("Trying to change the pixel size of the file in read mode is not allowed. "
                          "Hint: to fix the header of a file, open it in read|write mode");
            }
        }
        if (all(new_pixel_size >= 0))
            m_header.pixel_size = new_pixel_size;
        else
            NOA_THROW("The pixel size should be positive, got {}", new_pixel_size);
    }

    std::string MRCHeader::infoString(bool brief) const noexcept {
        if (brief)
            return string::format("Shape: {}; Pixel size: {:.3f}", m_header.shape, m_header.pixel_size);

        return string::format("Format: MRC File\n"
                              "Shape (batches, depth, height, width): {}\n"
                              "Pixel size (depth, height, width): {:.3f}\n"
                              "Data type: {}\n"
                              "Labels: {}\n"
                              "Extended header: {} bytes",
                              m_header.shape,
                              m_header.pixel_size,
                              m_header.data_type,
                              m_header.nb_labels,
                              m_header.extended_bytes_nb);
    }

    void MRCHeader::open_(const path_t& filename, open_mode_t open_mode) {
        close_();

        bool overwrite = open_mode & io::TRUNC || !(open_mode & io::READ);
        bool exists;
        try {
            exists = os::existsFile(filename);
            if (open_mode & io::WRITE) {
                if (exists)
                    os::backup(filename, overwrite);
                else if (overwrite)
                    os::mkdir(filename.parent_path());
            }
        } catch (...) {
            NOA_THROW("OS failure when trying to open the file");
        }

        m_open_mode = open_mode | io::BINARY;
        m_open_mode &= ~(io::APP | io::ATE);

        for (uint32_t it{0}; it < 5; ++it) {
            m_fstream.open(filename, io::toIOSBase(m_open_mode));
            if (m_fstream.is_open()) {
                if (exists && !overwrite) // case 1 or 2
                    readHeader_();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        m_fstream.clear();
        NOA_THROW("Failed to open the file");
    }

    void MRCHeader::readHeader_() {
        byte_t buffer[1024];
        m_fstream.seekg(0);
        m_fstream.read(reinterpret_cast<char*>(buffer), 1024);
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("File stream error. Could not read the header");
        }

        // Endianness.
        // Some software use 68-65, but the CCPEM standard is using 68-68...
        char stamp[4];
        std::memcpy(&stamp, buffer + 212, 4);
        if ((stamp[0] == 68 && stamp[1] == 65 && stamp[2] == 0 && stamp[3] == 0) ||
            (stamp[0] == 68 && stamp[1] == 68 && stamp[2] == 0 && stamp[3] == 0)) /* little */
            m_header.is_endian_swapped = isBigEndian();
        else if (stamp[0] == 17 && stamp[1] == 17 && stamp[2] == 0 && stamp[3] == 0) /* big */
            m_header.is_endian_swapped = !isBigEndian();
        else
            NOA_THROW("Invalid data. Endianness was not recognized."
                      "Should be [68,65,0,0], [68,68,0,0] or [17,17,0,0], got [{},{},{},{}]",
                      stamp[0], stamp[1], stamp[2], stamp[3]);

        // If data is swapped, some parts of the buffer need to be swapped as well.
        if (m_header.is_endian_swapped)
            swapHeader_(buffer);

        // Read & Write mode: we'll need to update the header when closing the file, so save the buffer.
        // The endianness will be swapped back to the original endianness of the file.
        if (m_open_mode & OpenMode::WRITE) {
            if (!m_header.buffer)
                m_header.buffer = std::make_unique<byte_t[]>(1024);
            std::memcpy(m_header.buffer.get(), buffer, 1024);
        }

        // Get the header from file.
        int32_t mode, imod_stamp, imod_flags, space_group;
        int3_t shape, grid_size, order;
        float3_t cell_size;
        {
            std::memcpy(shape.get(), buffer, 12);
            std::memcpy(&mode, buffer + 12, 4);
            // 16-24: sub-volume (nxstart, nystart, nzstart).
            std::memcpy(grid_size.get(), buffer + 28, 12);
            std::memcpy(cell_size.get(), buffer + 40, 12);
            // 52-64: alpha, beta, gamma.
            std::memcpy(order.get(), buffer + 64, 12);
            std::memcpy(&m_header.min, buffer + 76, 4);
            std::memcpy(&m_header.max, buffer + 80, 4);
            std::memcpy(&m_header.mean, buffer + 84, 4);
            std::memcpy(&space_group, buffer + 88, 4);
            std::memcpy(&m_header.extended_bytes_nb, buffer + 92, 4);
            // 96-98: creatid, extra data, extType, nversion, extra data, nint, nreal, extra data.
            std::memcpy(&imod_stamp, buffer + 152, 4);
            std::memcpy(&imod_flags, buffer + 156, 4);
            // 160-208: idtype, lens, nd1, nd2, vd1, vd2, tiltangles, origin(x,y,z).
            // 208-212: cmap.
            // 212-216: stamp.
            std::memcpy(&m_header.std, buffer + 216, 4);
            std::memcpy(&m_header.nb_labels, buffer + 220, 4);
            //224-1024: labels.
        }

        // Set the 4D shape:
        const int ndim = shape.flip().ndim();
        if (any(shape < 1))
            NOA_THROW("Invalid data. Logical shape should be greater than zero, got nx,ny,nz:{}", shape);
        if (ndim <= 2) {
            if (any(grid_size != shape)) {
                NOA_THROW("1D or 2D data detected. The logical shape should be equal to the grid size. "
                          "Got nx,ny,nz:{}, mx,my,mz:{}", shape, grid_size);
            }
            m_header.shape = {1, 1, shape[1], shape[0]};
        } else { // ndim == 3
            if (space_group == 0) { // stack of images
                // FIXME We should check grid_size[2] == 1, but some packages ignore this, so do nothing for now.
                //       Maybe at least warn?
                if (shape[0] != grid_size[0] || shape[1] != grid_size[1]) {
                    NOA_THROW("2D stack of images detected (ndim=3, group=0). The two innermost dimensions of the "
                              "logical shape and the grid size should be equal. Got nx,ny,nz:{}, mx,my,mz:{}",
                              shape, grid_size);
                }
                m_header.shape = {shape[2], 1, shape[1], shape[0]};
            } else if (space_group == 1) { // volume
                if (any(grid_size != shape)) {
                    NOA_THROW("3D volume detected (ndim=3, group=1). The logical shape should be equal to the "
                              "grid size. Got nx,ny,nz:{}, mx,my,mz:{}", shape, grid_size);
                }
                m_header.shape = {1, shape[2], shape[1], shape[0]};
            } else if (space_group >= 401 && space_group <= 630) { // stack of volume
                // grid_size[2] = secs per vol, shape[2] = total sections
                if (shape[2] % grid_size[2]) {
                    NOA_THROW("Stack of 3D volumes detected. The total sections (nz:{}) should be divisible by the "
                              "number of sections per volume (mz:{})", shape[2], grid_size[2]);
                } else if (shape[0] != grid_size[0] || shape[1] != grid_size[1]) {
                    NOA_THROW("Stack of 3D volumes detected. The first two dimensions of the shape and the grid size "
                              "should be equal. Got nx,ny,nz:{}, mx,my,mz:{}", shape, grid_size);
                }
                m_header.shape = {shape[2] / grid_size[2], grid_size[2], shape[1], shape[0]};
            } else {
                NOA_THROW("Data shape is not recognized. Got nx,ny,nz:{}, mx,my,mz:{}, group:",
                          shape, grid_size, space_group);
            }
        }

        // Set the pixel size:
        m_header.pixel_size = cell_size / float3_t(grid_size);
        m_header.pixel_size = m_header.pixel_size.flip();
        if (any(m_header.pixel_size < 0))
            NOA_THROW("Invalid data. Pixel size should not be negative, got {}", m_header.pixel_size);

        if (m_header.extended_bytes_nb < 0)
            NOA_THROW("Invalid data. Extended header size should be positive, got {}", m_header.extended_bytes_nb);

        // Convert mode to data type:
        switch (mode) {
            case 0:
                if (imod_stamp == 1146047817 && imod_flags & 1)
                    m_header.data_type = DataType::UINT8;
                else
                    m_header.data_type = DataType::INT8;
                break;
            case 1:
                m_header.data_type = DataType::INT16;
                break;
            case 2:
                m_header.data_type = DataType::FLOAT32;
                break;
            case 3:
                m_header.data_type = DataType::CINT16;
                break;
            case 4:
                m_header.data_type = DataType::CFLOAT32;
                break;
            case 6:
                m_header.data_type = DataType::UINT16;
                break;
            case 12:
                m_header.data_type = DataType::FLOAT16;
                break;
            case 16:
                NOA_THROW("MRC mode 16 is not currently supported");
            case 101:
                m_header.data_type = DataType::UINT4;
                break;
            default:
                NOA_THROW("Invalid data. MRC mode not recognized, got {}", mode);
        }

        // Map order: enforce row-major ordering, i.e. x=1, y=2, z=3.
        if (all(order != int3_t{1, 2, 3})) {
            if (any(order < 1) || any(order > 3) || math::sum(order) != 6)
                NOA_THROW("Invalid data. Map order should be (1,2,3), got {}", order);
            NOA_THROW("Map order {} is not supported. Only (1,2,3) is supported", order);
        }
    }

    void MRCHeader::close_() {
        if (!m_fstream.is_open())
            return;

        // Writing mode: the header should be updated before closing the file.
        if (m_open_mode & io::WRITE) {
            // Writing & reading mode: the instance didn't create the file,
            // the header was saved by readHeader_().
            if (m_open_mode & io::READ) {
                writeHeader_(m_header.buffer.get());
            } else {
                byte_t buffer[1024];
                defaultHeader_(buffer);
                writeHeader_(buffer);
            }
        }
        m_fstream.close();
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("File stream error. Could not close the file");
        }
    }

    void MRCHeader::defaultHeader_(byte_t* buffer) {
        std::memset(buffer, 0, 1024); // Set everything to 0.
        auto* buffer_ptr = reinterpret_cast<char*>(buffer);

        // Set the unused flags which do not default to 0.
        // The used bytes will be set before closing the file by writeHeader_().
        float angles[3] = {90, 90, 90};
        std::memcpy(buffer + 52, angles, 12);

        int32_t order[3] = {1, 2, 3};
        std::memcpy(buffer + 64, order, 12);

        buffer_ptr[104] = 'S';
        buffer_ptr[105] = 'E';
        buffer_ptr[106] = 'R';
        buffer_ptr[107] = 'I';

        buffer_ptr[208] = 'M';
        buffer_ptr[209] = 'A';
        buffer_ptr[210] = 'P';
        buffer_ptr[211] = ' ';

        // With new data, the endianness is always set to the endianness of the CPU.
        if (isBigEndian()) {
            buffer_ptr[212] = 17; // 16 * 1 + 1
            buffer_ptr[213] = 17;
        } else {
            buffer_ptr[212] = 68; // 16 * 4 + 4
            buffer_ptr[213] = 68;
        }
        buffer_ptr[214] = 0;
        buffer_ptr[215] = 0;
    }

    void MRCHeader::writeHeader_(byte_t* buffer) {
        // Data type.
        int32_t mode{}, imod_stamp{0}, imod_flags{0};
        switch (m_header.data_type) {
            case DataType::UINT8:
                mode = 0;
                imod_stamp = 1146047817;
                imod_flags &= 1;
                break;
            case DataType::INT8:
                mode = 0;
                break;
            case DataType::INT16:
                mode = 1;
                break;
            case DataType::FLOAT32:
                mode = 2;
                break;
            case DataType::CINT16:
                mode = 3;
                break;
            case DataType::CFLOAT32:
                mode = 4;
                break;
            case DataType::UINT16:
                mode = 6;
                break;
            case DataType::FLOAT16:
                mode = 12;
                break;
            case DataType::UINT4:
                mode = 101;
                break;
            default:
                NOA_THROW("The data type {} is not supported", m_header.data_type);
        }

        int space_group;
        int3_t shape, grid_size;
        float3_t cell_size;
        int4_t bdhw_shape(m_header.shape); // can be empty if nothing was written...
        const int ndim = bdhw_shape.ndim();
        if (ndim <= 3) { // 1D, 2D image, or 3D volume
            shape = grid_size = int3_t{bdhw_shape.get(1)}.flip();
            space_group = ndim == 3 ? 1 : 0;
        } else { // ndim == 4
            if (bdhw_shape[1] == 1) { // treat as stack of 2D images
                shape[0] = grid_size[0] = bdhw_shape[3];
                shape[1] = grid_size[1] = bdhw_shape[2];
                shape[2] = bdhw_shape[0];
                grid_size[2] = 1;
                space_group = 0;
            } else { // treat as stack of volume
                shape[0] = grid_size[0] = bdhw_shape[3];
                shape[1] = grid_size[1] = bdhw_shape[2];
                shape[2] = bdhw_shape[1] * bdhw_shape[0]; // total sections
                grid_size[2] = bdhw_shape[1]; // sections per volume
                space_group = 401;
            }
        }
        cell_size = float3_t{grid_size} * m_header.pixel_size.flip();

        // Updating the buffer.
        std::memcpy(buffer + 0, shape.get(), 12);
        std::memcpy(buffer + 12, &mode, 4);
        // 16-24: sub-volume (nxstart, nystart, nzstart) -> 0 or unchanged.
        std::memcpy(buffer + 28, grid_size.get(), 12);
        std::memcpy(buffer + 40, cell_size.get(), 12);
        // 52-64: alpha, beta, gamma -> 90,90,90 or unchanged.
        // 64-76: mapc, mapr, maps -> 1,2,3 (anything else is not supported).
        std::memcpy(buffer + 76, &m_header.min, 4);
        std::memcpy(buffer + 80, &m_header.max, 4);
        std::memcpy(buffer + 84, &m_header.mean, 4);
        {
            int tmp; // if it is a volume stack, don't overwrite.
            std::memcpy(&tmp, buffer + 88, 4);
            if (!(tmp > 401 && space_group == 401))
                std::memcpy(buffer + 88, &space_group, 4);
        }
        std::memcpy(buffer + 92, &m_header.extended_bytes_nb, 4); // 0 or unchanged.
        // 96-98: creatid -> 0 or unchanged.
        // 98-104: extra data -> 0 or unchanged.
        // 104-108: extType -> "SERI" or unchanged.
        // 108-112: nversion -> 0 or unchanged.
        // 112-128: extra data -> 0 or unchanged.
        // 128-132: nint, nreal -> 0 or unchanged.
        // 132-152: extra data -> 0 or unchanged.
        std::memcpy(buffer + 152, &imod_stamp, 4);
        std::memcpy(buffer + 156, &imod_flags, 4);
        // 160-208: idtype, lens, nd1, nd2, vd1, vd2, tiltangles, origin(x,y,z) -> 0 or unchanged.
        // 208-212: cmap -> "MAP " or unchanged.
        // 212-216: stamp -> [68,65,0,0] or [17,17,0,0], or unchanged.
        std::memcpy(buffer + 216, &m_header.std, 4);
        std::memcpy(buffer + 220, &m_header.nb_labels, 4); // 0 or unchanged.
        //224-1024: labels -> 0 or unchanged.

        // Swap back the header to its original endianness.
        if (m_header.is_endian_swapped)
            swapHeader_(buffer);

        // Write the buffer.
        m_fstream.seekp(0);
        m_fstream.write(reinterpret_cast<char*>(buffer), 1024);
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("File stream error. Could not write the header before closing the file");
        }
    }

    DataType MRCHeader::closestSupportedDataType_(DataType data_type) {
        switch (data_type) {
            case INT8:
                return INT8;
            case UINT8:
                return UINT8;
            case INT16:
                return INT16;
            case UINT16:
                return UINT16;
            case FLOAT16:
                return FLOAT16;
            case INT32:
            case UINT32:
            case INT64:
            case UINT64:
            case FLOAT32:
            case FLOAT64:
                return FLOAT32;
            case CFLOAT16:
            case CFLOAT32:
            case CFLOAT64:
                return CFLOAT32;
            default:
                NOA_THROW("{} is not a valid type", data_type);
        }
    }

    void MRCHeader::read(void* output, DataType data_type, size_t start, size_t end, bool clamp) {
        if (m_header.data_type == DataType::UINT4)
            NOA_THROW("This function does not support the 4bits format (mode 101). Use readSlice or readAll instead");

        const size_t bytes_offset = serializedSize(m_header.data_type, start);
        const auto offset = offset_() + static_cast<long>(bytes_offset);
        m_fstream.seekg(offset);
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("Could not seek to the desired offset ({})", offset);
        }
        NOA_ASSERT(end >= start);
        const size_t elements_to_read = end - start;
        const size4_t shape_to_read{1, 1, 1, elements_to_read};
        deserialize(m_fstream, m_header.data_type,
                    output, data_type, shape_to_read.strides(),
                    shape_to_read, clamp, m_header.is_endian_swapped);
    }

    void MRCHeader::readSlice(void* output, size4_t strides, size4_t shape,
                              DataType data_type, size_t start, bool clamp) {
        if (m_header.shape[2] != shape[2] || m_header.shape[3] != shape[3])
            NOA_THROW("The file shape {} is not compatible with the output shape {}", m_header.shape, shape);

        // Read either a 2D slice from a stack of 2D images or from a 3D volume.
        const bool file_is_volume = m_header.shape[0] == 1 && m_header.shape[1] > 1;
        if (file_is_volume && shape[0] != 1)
            NOA_THROW("The file shape {} describes a 3D volume. To read a slice from that volume, the provided "
                      "shape should have a batch of 1, but got shape {}", m_header.shape, shape);
        else if (!file_is_volume && shape[1] != 1)
            NOA_THROW("The file shape {} describes a (stack of) 2D image(s). To read a slice, the provided "
                      "shape should have a depth of 1, but got shape {}", m_header.shape, shape);

        // Make sure it doesn't go out of bound.
        if (m_header.shape[file_is_volume] <= start + shape[file_is_volume]) {
            NOA_THROW("The file has less slices ({}) that what is about to be read (start:{}, count:{})",
                      m_header.shape[file_is_volume], start, shape[file_is_volume]);
        }

        const auto elements_per_slice = m_header.shape[2] * m_header.shape[3];
        const size_t bytes_per_slice = serializedSize(m_header.data_type,
                                                      static_cast<size_t>(elements_per_slice),
                                                      m_header.shape[3]);
        const long offset = offset_() + static_cast<long>(start * bytes_per_slice);
        m_fstream.seekg(offset);
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("Could not seek to the desired offset ({})", offset);
        }
        deserialize(m_fstream, m_header.data_type,
                    output, data_type, strides,
                    shape, clamp, m_header.is_endian_swapped);
    }

    void MRCHeader::readSlice(void* output, DataType data_type, size_t start, size_t end, bool clamp) {
        NOA_ASSERT(end >= start);
        size4_t slice_shape{1, 1, m_header.shape[2], m_header.shape[3]};
        slice_shape[m_header.shape[1] != 1] = end - start; // if volume, set the depth, otherwise set the batch
        readSlice(output, slice_shape.strides(), slice_shape, data_type, start, clamp);
    }

    void MRCHeader::readAll(void* output, size4_t strides, size4_t shape, DataType data_type, bool clamp) {
        if (any(shape != m_header.shape))
            NOA_THROW("The file shape {} is not compatible with the output shape {}", m_header.shape, shape);

        m_fstream.seekg(offset_());
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("Could not seek to the desired offset ({})", offset_());
        }
        deserialize(m_fstream, m_header.data_type,
                    output, data_type, strides,
                    shape, clamp, m_header.is_endian_swapped);
    }

    void MRCHeader::readAll(void* output, DataType data_type, bool clamp) {
        return readAll(output, m_header.shape.strides(), m_header.shape, data_type, clamp);
    }

    void MRCHeader::write(const void* input, DataType data_type, size_t start, size_t end, bool clamp) {
        if (m_header.data_type == DataType::DATA_UNKNOWN)
            m_header.data_type = closestSupportedDataType_(data_type);

        if (m_header.data_type == DataType::UINT4)
            NOA_THROW("This function does not support the 4bits format (mode 101). Use writeSlice or writeAll instead");

        const size_t bytes_offset = serializedSize(m_header.data_type, start);
        const auto offset = offset_() + static_cast<long>(bytes_offset);
        m_fstream.seekp(offset);
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("Could not seek to the desired offset ({})", offset);
        }
        NOA_ASSERT(end >= start);
        const size_t elements_to_write = end - start;
        const size4_t shape_to_write{1, 1, 1, elements_to_write};
        serialize(input, data_type, shape_to_write.strides(), shape_to_write,
                  m_fstream, m_header.data_type,
                  clamp, m_header.is_endian_swapped);
    }

    void MRCHeader::writeSlice(const void* input, size4_t strides, size4_t shape,
                               DataType data_type, size_t start, bool clamp) {
        if (m_header.data_type == DataType::DATA_UNKNOWN)
            m_header.data_type = closestSupportedDataType_(data_type);

        // For writing a slice, it's best if we require the shape to be already set.
        if (any(m_header.shape == 0)) {
            NOA_THROW("The shape of the file is not set or is empty. "
                      "Set the shape first, and then write a slice to the file");
        } else if (m_header.shape[2] != shape[2] || m_header.shape[3] != shape[3]) {
            NOA_THROW("The file shape {} is not compatible with the input shape {}", m_header.shape, shape);
        }

        // Extract either a 2D slice from a stack of 2D images or from a 3D volume.
        const bool file_is_volume = m_header.shape[0] == 1 && m_header.shape[1] > 1;
        if (file_is_volume && shape[0] != 1)
            NOA_THROW("The file shape {} describes a 3D volume. To write a slice of that volume, the provided "
                      "shape should have a batch of 1, but got shape {}", m_header.shape, shape);
        else if (!file_is_volume && shape[1] != 1)
            NOA_THROW("The file shape {} describes a (stack of) 2D image(s). To write a 2D slice, the provided "
                      "shape should have a depth of 1, but got shape {}", m_header.shape, shape);

        // Make sure it doesn't go out of bound.
        if (m_header.shape[file_is_volume] <= start + shape[file_is_volume]) {
            NOA_THROW("The file has less slices ({}) that what is about to be written (start:{}, count:{})",
                      m_header.shape[file_is_volume], start, shape[file_is_volume]);
        }

        const auto elements_per_slice = m_header.shape[2] * m_header.shape[3];
        const size_t bytes_per_slice = serializedSize(m_header.data_type,
                                                      static_cast<size_t>(elements_per_slice),
                                                      m_header.shape[3]);
        const long offset = offset_() + static_cast<long>(start * bytes_per_slice);
        m_fstream.seekp(offset);
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("Could not seek to the desired offset ({})", offset);
        }
        serialize(input, data_type, strides, shape,
                  m_fstream, m_header.data_type,
                  clamp, m_header.is_endian_swapped);
    }

    void MRCHeader::writeSlice(const void* input, DataType data_type, size_t start, size_t end, bool clamp) {
        NOA_ASSERT(end >= start);
        size4_t slice_shape{1, 1, m_header.shape[2], m_header.shape[3]};
        slice_shape[m_header.shape[1] != 1] = end - start; // if volume, set the depth, otherwise set the batch
        return writeSlice(input, slice_shape.strides(), slice_shape, data_type, start, clamp);
    }

    void MRCHeader::writeAll(const void* input, size4_t strides, size4_t shape, DataType data_type, bool clamp) {
        if (m_header.data_type == DataType::DATA_UNKNOWN) // first write, set the data type
            m_header.data_type = closestSupportedDataType_(data_type);

        if (all(m_header.shape == 0)) // first write, set the shape
            m_header.shape = shape;
        else if (any(shape != m_header.shape))
            NOA_THROW("The file shape {} is not compatible with the input shape {}", m_header.shape, shape);

        m_fstream.seekp(offset_());
        if (m_fstream.fail()) {
            m_fstream.clear();
            NOA_THROW("Could not seek to the desired offset ({})", offset_());
        }
        serialize(input, data_type, strides, shape,
                  m_fstream, m_header.data_type,
                  clamp, m_header.is_endian_swapped);
    }

    void MRCHeader::writeAll(const void* input, DataType data_type, bool clamp) {
        if (any(m_header.shape == 0)) {
            NOA_THROW("The shape of the file is not set or is empty. "
                      "Set the shape first, and then write something to the file");
        }
        return writeAll(input, m_header.shape.strides(), m_header.shape, data_type, clamp);
    }
}
