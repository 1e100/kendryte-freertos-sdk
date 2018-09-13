/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _DRIVER_HARD_FFT_H
#define _DRIVER_HARD_FFT_H

#include <stdint.h>
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      FFT algorithm accelerator register
 *
 * @note       FFT algorithm accelerator register table
 *
 * | Offset    | Name           | Description                         |
 * |-----------|----------------|-------------------------------------|
 * | 0x00      | fft_input_fifo | input data fifo                     |
 * | 0x08      | fft_ctrl       | fft ctrl reg                        |
 * | 0x10      | fifo_ctrl      | fifo ctrl                           |
 * | 0x18      | intr_mask      | interrupt mask                      |
 * | 0x20      | intr_clear     | interrupt clear                     |
 * | 0x28      | fft_status     | fft status reg                      |
 * | 0x30      | fft_status_raw | fft_status_raw                      |
 * | 0x38      | fft_output_fifo| fft_output_fifo                     |
 *
 */


/**
 * @brief      input data fifo
 *
 *             No. 0 Register (0x00)
 */
typedef struct
{
    uint64_t fft_input_fifo : 64;
} __attribute__((packed, aligned(8))) fft_fft_input_fifo_t;

/**
 * @brief      fft ctrl reg
 *
 *             No. 1 Register (0x08)
 */
typedef struct
{
    uint64_t fft_point : 3;
    uint64_t fft_mode : 1;
    uint64_t fft_shift : 9;
    uint64_t fft_enable : 1;
    uint64_t dma_send : 1;
    uint64_t fft_input_mode : 2;
    uint64_t fft_data_mode : 1;
    uint64_t reserved : 46;
} __attribute__((packed, aligned(8))) fft_fft_ctrl_t;

/**
 * @brief      fifo ctrl
 *
 *             No. 2 Register (0x10)
 */
typedef struct
{
    uint64_t resp_fifo_flush_n : 1;
    uint64_t cmd_fifo_flush_n : 1;
    uint64_t gs_fifo_flush_n : 1;
    uint64_t reserved : 61;
} __attribute__((packed, aligned(8))) fft_fifo_ctrl_t;

/**
 * @brief      interrupt mask
 *
 *             No. 3 Register (0x18)
 */
typedef struct
{
    uint64_t fft_done_mask : 1;
    uint64_t reserved : 63;
} __attribute__((packed, aligned(8))) fft_intr_mask_t;

/**
 * @brief      interrupt clear
 *
 *             No. 4 Register (0x20)
 */
typedef struct
{
    uint64_t fft_done_clear : 1;
    uint64_t reserved1 : 63;
} __attribute__((packed, aligned(8))) fft_intr_clear_t;

/**
 * @brief      fft status reg
 *
 *             No. 5 Register (0x28)
 */
typedef struct
{
    uint64_t fft_done_status : 1;
    uint64_t reserved1 : 63;
} __attribute__((packed, aligned(8))) fft_fft_status_t;

/**
 * @brief      fft_status_raw
 *
 *             No. 6 Register (0x30)
 */
typedef struct
{
    uint64_t fft_done_status_raw : 1;
    uint64_t reserved : 63;
} __attribute__((packed, aligned(8))) fft_fft_status_raw_t;

/**
 * @brief      fft_output_fifo
 *
 *             No. 7 Register (0x38)
 */
typedef struct
{
    uint64_t fft_output_fifo : 64;
} __attribute__((packed, aligned(8))) fft_fft_output_fifo_t;

/**
 * @brief      Fast Fourier transform (FFT) algorithm accelerator object
 *
 *             A fast Fourier transform (FFT) algorithm computes the discrete
 *             Fourier transform (DFT) of a sequence, or its inverse (IFFT).
 *             Fourier analysis converts a signal from its original domain
 *             (often time or space) to a representation in the frequency
 *             domain and vice versa. An FFT rapidly computes such
 *             transformations by factorizing the DFT matrix into a product of
 *             sparse (mostly zero) factors.
 */
typedef struct
{
    /* No. 0 (0x00): input data fifo */
    fft_fft_input_fifo_t fft_input_fifo;
    /* No. 1 (0x08): fft ctrl reg */
    fft_fft_ctrl_t fft_ctrl;
    /* No. 2 (0x10): fifo ctrl */
    fft_fifo_ctrl_t fifo_ctrl;
    /* No. 3 (0x18): interrupt mask */
    fft_intr_mask_t intr_mask;
    /* No. 4 (0x20): interrupt clear */
    fft_intr_clear_t intr_clear;
    /* No. 5 (0x28): fft status reg */
    fft_fft_status_t fft_status;
    /* No. 6 (0x30): fft_status_raw */
    fft_fft_status_raw_t fft_status_raw;
    /* No. 7 (0x38): fft_output_fifo */
    fft_fft_output_fifo_t fft_output_fifo;
} __attribute__((packed, aligned(8))) fft_t;

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_FFT_H */

