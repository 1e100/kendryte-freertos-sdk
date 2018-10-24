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
#ifndef _FREERTOS_DRIVER_H
#define _FREERTOS_DRIVER_H

#include "object.hpp"
#include <gsl/span>

namespace sys
{
typedef enum _driver_type
{
    DRIVER_UART,
    DRIVER_GPIO,
    DRIVER_I2C,
    DRIVER_I2C_DEVICE,
    DRIVER_I2S,
    DRIVER_SPI,
    DRIVER_SPI_DEVICE,
    DRIVER_DVP,
    DRIVER_SCCB,
    DRIVER_SCCB_DEVICE,
    DRIVER_FFT,
    DRIVER_AES,
    DRIVER_SHA256,
    DRIVER_TIMER,
    DRIVER_PWM,
    DRIVER_WDT,
    DRIVER_RTC,
    DRIVER_PIC,
    DRIVER_DMAC,
    DRIVER_DMA,
    DRIVER_BLOCK_STORAGE,
    DRIVER_FILE,
    DRIVER_CUSTOM
} driver_type_t;

typedef struct tag_driver_registry
{
    const char *name;
    const void *driver;
    driver_type_t type;
} driver_registry_t;

class object_access : public virtual object
{
public:
    virtual void open() = 0;
    virtual void close() = 0;
};

class driver : public virtual object, public virtual object_access
{
public:
    virtual void install() = 0;
};

class uart_driver : public driver
{
public:
    virtual void config(uint32_t baud_rate, uint32_t databits, uart_stopbits_t stopbits, uart_parity_t parity) = 0;
    virtual size_t read(gsl::span<uint8_t> buffer) = 0;
    virtual void write(gsl::span<const uint8_t> buffer) = 0;
};

class gpio_driver : public driver
{
public:
    virtual uint32_t get_pin_count() = 0;
    virtual void set_drive_mode(uint32_t pin, gpio_drive_mode_t mode) = 0;
    virtual void set_pin_edge(uint32_t pin, gpio_pin_edge_t edge) = 0;
    virtual void set_on_changed(uint32_t pin, gpio_on_changed_t callback, void *userdata) = 0;
    virtual gpio_pin_value_t get_pin_value(uint32_t pin) = 0;
    virtual void set_pin_value(uint32_t pin, gpio_pin_value_t value) = 0;
};

class i2c_device_driver : public driver
{
public:
    virtual double set_clock_rate(double clock_rate) = 0;
    virtual size_t read(gsl::span<uint8_t> buffer) = 0;
    virtual void write(gsl::span<const uint8_t> buffer) = 0;
    virtual size_t transfer_sequential(gsl::span<const uint8_t> write_buffer, gsl::span<uint8_t> read_buffer) = 0;
};

class i2c_driver : public driver
{
public:
    virtual object_ptr<i2c_device_driver> get_device(uint32_t slave_address, uint32_t address_width) = 0;
    virtual void config_as_slave(uint32_t slave_address, uint32_t address_width, const i2c_slave_handler_t &handler) = 0;
    virtual double slave_set_clock_rate(double clock_rate) = 0;
};

class i2s_driver : public driver
{
public:
    virtual void config_as_render(const audio_format_t &format, size_t delay_ms, i2s_align_mode_t align_mode, uint32_t channels_mask) = 0;
    virtual void config_as_capture(const audio_format_t &format, size_t delay_ms, i2s_align_mode_t align_mode, uint32_t channels_mask) = 0;
    virtual void get_buffer(gsl::span<uint8_t> &buffer, uint32_t &frames) = 0;
    virtual void release_buffer(uint32_t frames) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

class spi_device_driver : public driver
{
public:
    virtual void config_non_standard(uint32_t instruction_length, uint32_t address_length, uint32_t wait_cycles, spi_inst_addr_trans_mode_t trans_mode) = 0;
    virtual double set_clock_rate(double clock_rate) = 0;
    virtual size_t read(gsl::span<uint8_t> buffer) = 0;
    virtual void write(gsl::span<const uint8_t> buffer) = 0;
    virtual size_t transfer_full_duplex(gsl::span<const uint8_t> write_buffer, gsl::span<uint8_t> read_buffer) = 0;
    virtual size_t transfer_sequential(gsl::span<const uint8_t> write_buffer, gsl::span<uint8_t> read_buffer) = 0;
    virtual void fill(uint32_t instruction, uint32_t address, uint32_t value, size_t count) = 0;
};

class spi_driver : public driver
{
public:
    virtual object_ptr<spi_device_driver> get_device(spi_mode_t mode, spi_frame_format_t frame_format, uint32_t chip_select_mask, uint32_t data_bit_length) = 0;
};

class dvp_driver : public driver
{
public:
    virtual uint32_t get_output_num() = 0;
    virtual void config(uint32_t width, uint32_t height, bool auto_enable) = 0;
    virtual void enable_frame() = 0;
    virtual void set_signal(dvp_signal_type_t type, bool value) = 0;
    virtual void set_output_enable(uint32_t index, bool enable) = 0;
    virtual void set_output_attributes(uint32_t index, video_format_t format, gsl::span<uint8_t> output_buffer) = 0;
    virtual void set_frame_event_enable(dvp_frame_event_t event, bool enable) = 0;
    virtual void set_on_frame_event(dvp_on_frame_event_t callback, void *userdata) = 0;
    virtual double xclk_set_clock_rate(double clock_rate) = 0;
};

class sccb_device_driver : public driver
{
public:
    virtual uint8_t read_byte(uint16_t reg_address) = 0;
    virtual void write_byte(uint16_t reg_address, uint8_t value) = 0;
};

class sccb_driver : public driver
{
public:
    virtual object_ptr<sccb_device_driver> get_device(uint32_t slave_address, uint32_t reg_address_width) = 0;
};

class fft_driver : public driver
{
public:
    virtual void complex_uint16(uint16_t shift, fft_direction_t direction, gsl::span<const uint64_t> input, size_t point_num, gsl::span<uint64_t> output) = 0;
};

class aes_driver : public driver
{
public:
    virtual void aes_ecb128_hard_decrypt(gsl::span<const uint8_t> input_key, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_ecb128_hard_encrypt(gsl::span<const uint8_t> input_key, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_ecb192_hard_decrypt(gsl::span<const uint8_t> input_key, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_ecb192_hard_encrypt(gsl::span<const uint8_t> input_key, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_ecb256_hard_decrypt(gsl::span<const uint8_t> input_key, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_ecb256_hard_encrypt(gsl::span<const uint8_t> input_key, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_cbc128_hard_decrypt(cbc_context_t &context, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_cbc128_hard_encrypt(cbc_context_t &context, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_cbc192_hard_decrypt(cbc_context_t &context, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_cbc192_hard_encrypt(cbc_context_t &context, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_cbc256_hard_decrypt(cbc_context_t &context, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_cbc256_hard_encrypt(cbc_context_t &context, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
    virtual void aes_gcm128_hard_decrypt(gcm_context_t &context, gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data, gsl::span<uint8_t> gcm_tag) = 0;
};

class sha256_driver : public driver
{
public:
    virtual void sha256_hard_calculate(gsl::span<const uint8_t> input_data, gsl::span<uint8_t> output_data) = 0;
};

class timer_driver : public driver
{
public:
    virtual void set_interval(size_t nanoseconds) = 0;
    virtual void set_on_tick(timer_on_tick_t on_tick, void *userdata) = 0;
    virtual void set_enable(bool enable) = 0;
};

class pwm_driver : public driver
{
public:
    virtual uint32_t get_pin_count() = 0;
    virtual double set_frequency(double frequency) = 0;
    virtual double set_active_duty_cycle_percentage(uint32_t pin, double duty_cycle_percentage) = 0;
    virtual void set_enable(uint32_t pin, bool enable) = 0;
};

class wdt_driver : public driver
{
public:
    virtual void set_response_mode(wdt_response_mode_t mode) = 0;
    virtual size_t set_timeout(size_t nanoseconds) = 0;
    virtual void set_on_timeout(wdt_on_timeout_t handler, void *userdata) = 0;
    virtual void restart_counter() = 0;
    virtual void set_enable(bool enable) = 0;
};

class rtc_driver : public driver
{
public:
    virtual void get_datetime(struct tm &datetime) = 0;
    virtual void set_datatime(const struct tm &datetime) = 0;
};

class custom_driver : public driver
{
public:
    virtual void control(uint32_t control_code, gsl::span<const uint8_t> write_buffer, gsl::span<uint8_t> read_buffer) = 0;
};

/* ===== internal drivers ======*/

void kernel_iface_pic_on_irq(uint32_t irq);

class pic_driver : public driver
{
public:
    virtual void set_irq_enable(uint32_t irq, bool enable) = 0;
    virtual void set_irq_priority(uint32_t irq, uint32_t priority) = 0;
};

class dma_driver : public driver
{
public:
    virtual void set_select_request(uint32_t request) = 0;
    virtual void config(uint32_t priority) = 0;
    virtual void transmit_async(const volatile void *src, volatile void *dest, bool src_inc, bool dest_inc, size_t element_size, size_t count, size_t burst_size, SemaphoreHandle_t completion_event) = 0;
    virtual void loop_async(const volatile void **srcs, size_t src_num, volatile void **dests, size_t dest_num, bool src_inc, bool dest_inc, size_t element_size, size_t count, size_t burst_size, dma_stage_completion_handler_t stage_completion_handler, void *stage_completion_handler_data, SemaphoreHandle_t completion_event, int *stop_signal) = 0;
};

class dmac_driver : public driver
{
public:

};

class block_storage_driver : public driver
{
public:
    virtual uint32_t get_rw_block_size() = 0;
    virtual uint32_t get_blocks_count() = 0;
    virtual void read_blocks(uint32_t start_block, uint32_t blocks_count, gsl::span<uint8_t> buffer) = 0;
    virtual void write_blocks(uint32_t start_block, uint32_t blocks_count, gsl::span<const uint8_t> buffer) = 0;
};

extern driver_registry_t g_hal_drivers[];
extern driver_registry_t g_dma_drivers[];
extern driver_registry_t g_system_drivers[];

/**
 * @brief       Install a driver
 * @param[in]   name        Specify the path to access it later
 * @param[in]   type        The type of driver
 * @param[in]   driver      The driver info
 *
 * @return      result
 *     - NULL   Fail
 *     - other  The driver registry
 */
driver_registry_t *system_install_driver(const char *name, driver_type_t type, const void *driver);
}

#endif /* _FREERTOS_DRIVER_H */
