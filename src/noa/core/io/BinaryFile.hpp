#pragma once

#include "noa/core/Config.hpp"

#ifdef NOA_IS_OFFLINE
#include <filesystem>
#include <fstream>
#include "noa/core/Traits.hpp"
#include "noa/core/Exception.hpp"
#include "noa/core/io/OS.hpp"
#include "noa/core/string/Format.hpp"

namespace noa::io {
    /// Binary file. This is also meant to be used as a temporary file.
    ///     - the data is not formatted on reads and writes.
    ///     - the filename and path can be generated automatically.
    ///     - the file can be automatically deleted after closing.
    class BinaryFile {
    public:
        /// Generate an unused filename.
        static Path generate_filename() {
            Path out(noa::io::temporary_directory() / "");
            while (true) {
                const int tag = 10000 + std::rand() / (99999 / (99999 - 10000 + 1) + 1); // 5 random digits
                out.replace_filename(fmt::format("tmp_{}.bin", tag));
                if (not noa::io::is_file(out))
                    break;
            }
            return out;
        }

    public:
        /// Creates an empty instance. Use open() to open a file.
        BinaryFile() = default;

        /// Stores the path and opens the file. \see open() for more details.
        BinaryFile(Path path, OpenMode open_mode, bool close_delete = false)
                : m_path(std::move(path)), m_delete(close_delete) {
            open_(open_mode);
        }

        /// Generates a temporary filename and opens the file.
        explicit BinaryFile(OpenMode open_mode, bool close_delete = false)
                : m_path(generate_filename()), m_delete(close_delete) {
            open_(open_mode);
        }

        /// Close the currently opened file (if any) and opens a new file.
        /// \param path         Filename of the file to open.
        /// \param open_mode    Should be one or a combination of the following ("binary" is always considered on):
        ///                     \c read:                    File should exists.
        ///                     \c read-write:              File should exists.     Backup copy.
        ///                     \c write, write-truncate:   Overwrite the file.     Backup move.
        ///                     \c read-write-truncate:     Overwrite the file.     Backup move.
        /// \param close_delete Whether the file should be deleted after closing.
        ///
        /// \throws Exception   If any of the following cases:
        ///         - If the file does not exist and \p open_mode is set to read or read-write.
        ///         - If the permissions do not match the \p open_mode.
        ///         - If failed to close the file before starting (if any).
        ///         - If an underlying OS error was raised.
        void open(Path path, OpenMode open_mode, bool close_delete = false) {
            close();
            m_path = std::move(path);
            m_delete = close_delete;
            open_(open_mode);
        }

        /// Close the currently opened file (if any) and opens a new file with a automatically generated path/name.
        void open(OpenMode open_mode, bool close_delete = false) {
            close();
            m_path = generate_filename();
            m_delete = close_delete;
            open_(open_mode);
        }

        template<typename T>
        void read(T* output, i64 offset, i64 elements) {
            m_fstream.seekg(offset);
            if (m_fstream.fail())
                panic("File: {}. Could not seek to the desired offset ({} bytes)", m_path, offset);
            const i64 bytes = elements * sizeof(T);
            m_fstream.read(reinterpret_cast<char*>(output), bytes);
            if (m_fstream.fail())
                panic("File stream error. Failed while reading {} bytes from {}", bytes, m_path);
        }

        template<typename T>
        void write(T* output, i64 offset, i64 elements) {
            m_fstream.seekp(offset);
            if (m_fstream.fail())
                panic("File: {}. Could not seek to the desired offset ({} bytes)", m_path, offset);
            const i64 bytes = elements * sizeof(T);
            m_fstream.write(reinterpret_cast<char*>(output), bytes);
            if (m_fstream.fail())
                panic("File stream error. Failed while writing {} bytes from {}", bytes, m_path);
        }

        void close() {
            m_fstream.close();
            if (m_fstream.fail())
                panic("File: {}. File stream error. Could not close the file", m_path);
            if (m_delete)
                noa::io::remove(m_path);
        }

        void flush() { m_fstream.flush(); }
        bool exists() { return noa::io::is_file(m_path); }
        i64 size() { return noa::io::file_size(m_path); }
        void clear_flags() { m_fstream.clear(); }

        [[nodiscard]] std::fstream& fstream() noexcept { return m_fstream; }
        [[nodiscard]] const fs::path& path() const noexcept { return m_path; }
        [[nodiscard]] bool bad() const noexcept { return m_fstream.bad(); }
        [[nodiscard]] bool eof() const noexcept { return m_fstream.eof(); }
        [[nodiscard]] bool fail() const noexcept { return m_fstream.fail(); }
        [[nodiscard]] bool is_open() const noexcept { return m_fstream.is_open(); }
        [[nodiscard]] explicit operator bool() const noexcept { return !m_fstream.fail(); }

        ~BinaryFile() noexcept(false) {
            try {
                close();
            } catch (...) {
                if (!std::uncaught_exceptions()) {
                    std::rethrow_exception(std::current_exception());
                }
            }
        }

    private:
        void open_(OpenMode open_mode, const std::source_location& location = std::source_location::current());

    private:
        std::fstream m_fstream{};
        Path m_path{};
        bool m_delete{false};
    };
}
#endif
