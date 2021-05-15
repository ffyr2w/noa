#pragma once

#include "noa/Definitions.h"
#include "noa/gpu/cuda/Types.h"
#include "noa/gpu/cuda/util/Stream.h"

namespace Noa::CUDA::Memory {
    /* ------------------------- */
    /* --- Using coordinates --- */
    /* ------------------------- */

    /**
     * Extracts from the input array one or multiple subregions at variable locations.
     * @tparam T                    (u)short, (u)int, (u)long, (u)long long, float, double.
     * @param[in] input             Input array to use for the extraction.
     * @param input_shape           Physical {fast, medium, slow} shape of @a input.
     * @param[out] subregions       Output subregions. At least `subregion_count * getElements(subregion_shape)` elements.
     * @param subregion_shape       Physical {fast, medium, slow} shape of one subregion.
     * @param[in] subregion_centers Center of the subregions, corresponding to @a input_shape. One per subregion.
     * @param subregion_count       Number of subregions.
     * @param border_mode           Border mode applied to the elements falling out of the input bounds.
     *                              Should be BORDER_NOTHING, BORDER_ZERO or BORDER_VALUE.
     * @param border_value          Border value. Only used if @a border_mode == BORDER_VALUE.
     *
     * @throw If @a border_mode is not BORDER_NOTHING, BORDER_ZERO or BORDER_VALUE.
     * @note  @a input == @a output is not valid.
     */
    template<typename T>
    NOA_HOST void extract(const T* input, size_t pitch_input, size3_t input_shape,
                          T* subregions, size_t pitch_subregion, size3_t subregion_shape,
                          const size3_t* subregion_centers, uint subregion_count,
                          BorderMode border_mode, T border_value, Stream& stream);

    template<typename T>
    NOA_IH void extract(const T* input, size3_t input_shape,
                        T* subregions, size3_t subregion_shape,
                        const size3_t* subregion_centers, uint subregion_count,
                        BorderMode border_mode, T border_value, Stream& stream) {
        extract(input, input_shape.x, input_shape,
                subregions, subregion_shape.x, subregion_shape, subregion_centers, subregion_count,
                border_mode, border_value, stream);
    }

    /// Extracts a subregion from the input array.
    template<typename T>
    NOA_HOST void extract(const T* input, size_t pitch_input, size3_t input_shape,
                          T* subregion, size_t pitch_subregion, size3_t subregion_shape, size3_t subregion_center,
                          BorderMode border_mode, T border_value, Stream& stream);

    template<typename T>
    NOA_IH void extract(const T* input, size3_t input_shape,
                          T* subregion, size3_t subregion_shape, size3_t subregion_center,
                          BorderMode border_mode, T border_value, Stream& stream) {
        extract(input, input_shape.x, input_shape,
                subregion, subregion_shape.x, subregion_shape, subregion_center,
                border_mode, border_value, stream);
    }

    /**
     * Insert a subregion into the output array.
     * @tparam T                (u)short, (u)int, (u)long, (u)long long, float, double.
     * @param[in] subregion     Subregion to insert into @a output.
     * @param subregion_shape   Physical {fast, medium, slow} shape of @a subregion.
     * @param subregion_center  Center of the subregion, corresponding to @a output_shape.
     * @param[out] output       Output array.
     * @param output_shape      Physical {fast, medium, slow} shape of @a subregion.
     *
     * @note The subregion can be (partially) out of the @a output bounds.
     * @note @a subregion == @a output is not valid.
     * @note Insert multiple regions is currently not supported since it is unclear
     *       what should be done in case of overlapping subregions.
     */
    template<typename T>
    NOA_HOST void insert(const T* subregion, size_t subregion_pitch, size3_t subregion_shape, size3_t subregion_center,
                         T* output, size_t output_pitch, size3_t output_shape, Stream& stream);

    template<typename T>
    NOA_IH void insert(const T* subregion, size3_t subregion_shape, size3_t subregion_center,
                         T* output, size3_t output_shape, Stream& stream) {
        insert(subregion, subregion_shape.x, subregion_shape, subregion_center,
               output, output_shape.x, output_shape, stream);
    }

    /* ------------------- */
    /* --- Using a map --- */
    /* ------------------- */

    /**
     * Extracts the linear indexes where @a mask is > 0. These indexes are referred to as a map.
     * @tparam T                (u)short, (u)int, (u)long, (u)long long, float, double.
     * @param[in] mask          Mask to read
     * @param elements          Size of @a mask, in elements.
     * @param threshold         Threshold value. Elements greater than is value are added to the map.
     * @return                  1: the map. The calling scope becomes the owner of this map.
     *                          2: the size of the map, in elements.
     * @param[in,out] stream    Stream on which to enqueue this function.
     *                          The stream will be synchronized when the function returns.
     */
    template<typename T>
    NOA_HOST std::pair<size_t*, size_t> getMap(const T* mask, size_t elements, T threshold, Stream& stream);

    /// Extracts the linear indexes where @a mask is > 0. These indexes are referred to as a map.
    /// @note This overload is for padded layout and is otherwise identical to the overload for contiguous layout.
    template<typename T>
    NOA_HOST std::pair<size_t*, size_t> getMap(const T* mask, size_t mask_pitch, size3_t mask_shape,
                                               T threshold, Stream& stream);

    /**
     * Extracts elements from the input array(s) into the output array(s) at the indexes saved in the map.
     * @tparam T                (u)short, (u)int, (u)long, (u)long long, float, double.
     * @param[in] i_sparse      Input arrays to extract the elements from. One per batch.
     * @param i_sparse_elements Size, in elements, of one input array (i.e. size of one batch).
     * @param[out] o_dense      Output arrays where the extracted elements are saved (in the same order as
     *                          specified in @a map). One per batch.
     * @param o_dense_elements  Size, in elements, of one output array, which is also the size of @a i_map.
     * @param[in] i_map         Indexes corresponding to one input array. The same map is applied to every batch.
     *                          Should be at least @a o_dense_elements elements.
     * @param batches           Number of batches in @a i_sparse and @a o_dense.
     * @param[in,out] stream    Stream on which to enqueue this function.
     *                          The stream will be synchronized when the function returns.
     */
    template<typename T>
    NOA_HOST void extract(const T* i_sparse, size_t i_sparse_elements, T* o_dense, size_t o_dense_elements,
                          const size_t* i_map, uint batches, Stream& stream);

    /**
     * Inserts elements from the input array(s) into the output array(s) at the indexes saved in the map.
     * @tparam T                (u)short, (u)int, (u)long, (u)long long, float, double.
     * @param[in] i_dense       Input arrays to insert. One per batch.
     * @param i_dense_elements  Size, in elements, of one input array, which is also the size of @a map.
     * @param[out] o_sparse     Output arrays corresponding to @a map.
     * @param o_sparse_elements Size, in elements, of one output array.
     * @param[in] i_map         Indexes of @a o_sparse where the elements in @a i_dense should be inserted.
     *                          The same map is applied to every batch.
     *                          Should be at least @a i_dense_elements elements.
     * @param batches           Number of batches in @a i_dense and @a o_sparse.
     * @param[in,out] stream    Stream on which to enqueue this function.
     */
    template<typename T>
    NOA_HOST void insert(const T* i_dense,  size_t i_dense_elements, T* o_sparse, size_t o_sparse_elements,
                         const size_t* i_map, uint batches, Stream& stream);
}
