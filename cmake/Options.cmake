# ---------------------------------------------------------------------------------------
# Compiler/General options
# ---------------------------------------------------------------------------------------

# These *_ENABLE_* options are project wide (i.e. for all targets).
option(NOA_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(NOA_ENABLE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(NOA_ENABLE_IPO "Enable Interprocedural Optimization, aka Link Time Optimization (LTO)" OFF)
option(NOA_ENABLE_CCACHE "Enable ccache if available" OFF)
option(NOA_ENABLE_PCH "Build using precompiled header to speed up compilation time" ON)
#option(NOA_ENABLE_SINGLE_PRECISION "Use single precision floating-points whenever possible" ON)
#option(BUILD_SHARED_LIBS "Build the library as a shared library." OFF)
option(NOA_ENABLE_PROFILER "Enable the profiler" OFF)

# ---------------------------------------------------------------------------------------
# Dependency options
# ---------------------------------------------------------------------------------------

# CUDA
# ====

option(NOA_BUILD_CUDA "Use the CUDA GPU backend" ON)
option(NOA_CUDA_USE_STATIC_LIBS "Use the CUDA static libraries" ON)
set(NOA_CUDA_ARCH
        52 60 61 75 86
        CACHE STRING "List of architectures to generate device code for. Default=  \"52 60 61 75 85\""
        FORCE)

# FFTW
# ====

# Project specific system environmental variables:
#   NOA_FFTW_LIBRARIES: Path(s) of the fftw3 libraries. Ignored if NOA_FFTW_USE_OWN=OFF
#   NOA_FFTW_INCLUDE:   Path(s) of the fftw3 header(s). Ignored if NOA_FFTW_USE_OWN=OFF
option(NOA_FFTW_USE_STATIC_LIBS "Use the FFTW static libraries" ON)
option(NOA_FFTW_USE_OWN "Use your own libraries. If false, fetch them from the web" ON)

# TIFF
# ====

#option(NOA_TIFF_USE_STATIC_LIBS "Use the TIFF static libraries" ON)
#option(NOA_TIFF_USE_OWN "Use your own libraries. If false, fetch them from the web" OFF)

# ---------------------------------------------------------------------------------------
# Target options
# ---------------------------------------------------------------------------------------
option(NOA_BUILD_TESTS "Build tests" ${NOA_IS_MASTER})
option(NOA_BUILD_BENCHMARKS "Build benchmarks" ${NOA_IS_MASTER})
#option(NOA_BUILD_DOC "Build Doxygen-Sphinx documentation" OFF)
#option(NOA_PACKAGING "Generate packaging" OFF)
