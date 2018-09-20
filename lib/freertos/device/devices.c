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
#include <FreeRTOS.h>
#include <atomic.h>
#include <semphr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysctl.h>
#include <uarths.h>
#include "devices.h"
#include "driver.h"
#include "hal.h"

#define MAX_HANDLES 256
#define HANDLE_OFFSET 256
#define MAX_CUSTOM_DRIVERS 32

#define DEFINE_INSTALL_DRIVER(type)                     \
    static void install_##type##_drivers()              \
    {                                                   \
        driver_registry_t* head = g_##type##_drivers;   \
        while (head->name)                              \
        {                                               \
            const driver_base_t* driver = head->driver; \
            driver->install(driver->userdata);          \
            head++;                                     \
        }                                               \
    }

typedef struct
{
    driver_registry_t* driver_reg;
} _file;

static _file* handles_[MAX_HANDLES] = {0};
static driver_registry_t g_custom_drivers[MAX_CUSTOM_DRIVERS] = {0};

uintptr_t fft_file_;
uintptr_t aes_file_;
uintptr_t sha256_file_;

DEFINE_INSTALL_DRIVER(hal);
DEFINE_INSTALL_DRIVER(dma);
DEFINE_INSTALL_DRIVER(system);

driver_registry_t* find_free_driver(driver_registry_t* registry, const char* name)
{
    driver_registry_t* head = registry;
    while (head->name)
    {
        if (strcmp(name, head->name) == 0)
        {
            driver_base_t* driver = (driver_base_t*)head->driver;
            if (driver->open(driver->userdata))
                return head;
            else
                return NULL;
        }

        head++;
    }

    return NULL;
}

static driver_registry_t* install_custom_driver_core(const char* name, driver_type type, const void* driver)
{
    size_t i = 0;
    driver_registry_t* head = g_custom_drivers;
    for (i = 0; i < MAX_CUSTOM_DRIVERS; i++, head++)
    {
        if (!head->name)
        {
            head->name = strdup(name);
            head->type = type;
            head->driver = driver;
            return head;
        }
    }

    configASSERT(!"Max custom drivers exceeded.");
    return NULL;
}

void install_drivers()
{
    install_system_drivers();

    fft_file_ = io_open("/dev/fft0");
    aes_file_ = io_open("/dev/aes0");
    sha256_file_ = io_open("/dev/sha256");
}

static _file* io_alloc_file(driver_registry_t* driver_reg)
{
    if (driver_reg)
    {
        _file* file = (_file*)malloc(sizeof(_file));
        if (!file)
            return NULL;
        file->driver_reg = driver_reg;
        return file;
    }

    return NULL;
}

static _file* io_open_reg(driver_registry_t* registry, const char* name, _file** file)
{
    driver_registry_t* driver_reg = find_free_driver(registry, name);
    _file* ret = io_alloc_file(driver_reg);
    *file = ret;
    return ret;
}

/* Generic IO Implementation Helper Macros */

#define DEFINE_READ_PROXY(tl, tu)                                                  \
    if (rfile->driver_reg->type == DRIVER_##tu)                                    \
    {                                                                              \
        const tl##_driver_t* tl = (const tl##_driver_t*)rfile->driver_reg->driver; \
        return tl->read(buffer, len, tl->base.userdata);                           \
    }

#define DEFINE_WRITE_PROXY(tl, tu)                                                 \
    if (rfile->driver_reg->type == DRIVER_##tu)                                    \
    {                                                                              \
        const tl##_driver_t* tl = (const tl##_driver_t*)rfile->driver_reg->driver; \
        return tl->write(buffer, len, tl->base.userdata);                          \
    }

#define DEFINE_CONTROL_PROXY(tl, tu)                                                                            \
    if (rfile->driver_reg->type == DRIVER_##tu)                                                                 \
    {                                                                                                           \
        const tl##_driver_t* tl = (const tl##_driver_t*)rfile->driver_reg->driver;                              \
        return tl->io_control(control_code, write_buffer, write_len, read_buffer, read_len, tl->base.userdata); \
    }

static void dma_add_free();

int io_read(uintptr_t file, char* buffer, size_t len)
{
    _file* rfile = (_file*)handles_[file - HANDLE_OFFSET];
    /* clang-format off */
    DEFINE_READ_PROXY(uart, UART)
    else DEFINE_READ_PROXY(i2c_device, I2C_DEVICE)
    else DEFINE_READ_PROXY(spi_device, SPI_DEVICE)
    else
    {
        return -1;
    }
    /* clang-format on */
}

static void io_free(_file* file)
{
    if (file)
    {
        if (file->driver_reg->type == DRIVER_DMA)
            dma_add_free();

        driver_base_t* driver = (driver_base_t*)file->driver_reg->driver;
        driver->close(driver->userdata);

        free(file);
    }
}

static uintptr_t io_alloc_handle(_file* file)
{
    if (file)
    {
        size_t i, j;
        for (i = 0; i < 2; i++)
        {
            for (j = 0; j < MAX_HANDLES; j++)
            {
                if (atomic_cas(handles_ + j, 0, file) == 0)
                    return j + HANDLE_OFFSET;
            }
        }

        io_free(file);
        return 0;
    }

    return 0;
}

uintptr_t io_open(const char* name)
{
    _file* file = 0;
    if (io_open_reg(g_system_drivers, name, &file))
    {
    }
    else if (io_open_reg(g_hal_drivers, name, &file))
    {
    }

    if (file)
        return io_alloc_handle(file);
    configASSERT(file);
    return 0;
}

int io_close(uintptr_t file)
{
    if (file)
    {
        _file* rfile = (_file*)handles_[file - HANDLE_OFFSET];
        io_free(rfile);
        atomic_set(handles_ + file - HANDLE_OFFSET, 0);
    }

    return 0;
}

int io_write(uintptr_t file, const char* buffer, size_t len)
{
    _file* rfile = (_file*)handles_[file - HANDLE_OFFSET];
    /* clang-format off */
    DEFINE_WRITE_PROXY(uart, UART)
    else DEFINE_WRITE_PROXY(i2c_device, I2C_DEVICE)
    else DEFINE_WRITE_PROXY(spi_device, SPI_DEVICE)
    else
    {
        return -1;
    }
    /* clang-format on */
}

int io_control(uintptr_t file, size_t control_code, const char* write_buffer, size_t write_len, char* read_buffer, size_t read_len)
{
    _file* rfile = (_file*)handles_[file - HANDLE_OFFSET];
    DEFINE_CONTROL_PROXY(custom, CUSTOM)

    return -1;
}

/* Device IO Implementation Helper Macros */

#define COMMON_ENTRY(tl, tu)                               \
    _file* rfile = (_file*)handles_[file - HANDLE_OFFSET]; \
    configASSERT(rfile->driver_reg->type == DRIVER_##tu);  \
    const tl##_driver_t* tl = (const tl##_driver_t*)rfile->driver_reg->driver;

#define COMMON_ENTRY_FILE(file, tl, tu)                    \
    _file* rfile = (_file*)handles_[file - HANDLE_OFFSET]; \
    configASSERT(rfile->driver_reg->type == DRIVER_##tu);  \
    const tl##_driver_t* tl = (const tl##_driver_t*)rfile->driver_reg->driver;

/* UART */

void uart_config(uintptr_t file, size_t baud_rate, size_t data_width, uart_stopbit stopbit, uart_parity parity)
{
    COMMON_ENTRY(uart, UART);
    uart->config(baud_rate, data_width, stopbit, parity, uart->base.userdata);
}

/* GPIO */

size_t gpio_get_pin_count(uintptr_t file)
{
    COMMON_ENTRY(gpio, GPIO);
    return gpio->pin_count;
}

void gpio_set_drive_mode(uintptr_t file, size_t pin, gpio_drive_mode mode)
{
    COMMON_ENTRY(gpio, GPIO);
    gpio->set_drive_mode(gpio->base.userdata, pin, mode);
}

void gpio_set_pin_edge(uintptr_t file, size_t pin, gpio_pin_edge edge)
{
    COMMON_ENTRY(gpio, GPIO);
    gpio->set_pin_edge(gpio->base.userdata, pin, edge);
}

void gpio_set_onchanged(uintptr_t file, size_t pin, gpio_onchanged callback, void* userdata)
{
    COMMON_ENTRY(gpio, GPIO);
    gpio->set_onchanged(gpio->base.userdata, pin, callback, userdata);
}

gpio_pin_value gpio_get_pin_value(uintptr_t file, size_t pin)
{
    COMMON_ENTRY(gpio, GPIO);
    return gpio->get_pin_value(gpio->base.userdata, pin);
}

void gpio_set_pin_value(uintptr_t file, size_t pin, gpio_pin_value value)
{
    COMMON_ENTRY(gpio, GPIO);
    gpio->set_pin_value(gpio->base.userdata, pin, value);
}

/* I2C */

uintptr_t i2c_get_device(uintptr_t file, const char* name, size_t slave_address, size_t address_width, i2c_bus_speed_mode bus_speed_mode)
{
    COMMON_ENTRY(i2c, I2C);
    i2c_device_driver_t* driver = i2c->get_device(slave_address, address_width, bus_speed_mode, i2c->base.userdata);
    driver_registry_t* reg = install_custom_driver_core(name, DRIVER_I2C_DEVICE, driver);
    return io_alloc_handle(io_alloc_file(reg));
}

int i2c_dev_transfer_sequential(uintptr_t file, const char* write_buffer, size_t write_len, char* read_buffer, size_t read_len)
{
    COMMON_ENTRY(i2c_device, I2C_DEVICE);
    return i2c_device->transfer_sequential(write_buffer, write_len, read_buffer, read_len, i2c_device->base.userdata);
}

void i2c_config_as_slave(uintptr_t file, size_t slave_address, size_t address_width, i2c_bus_speed_mode bus_speed_mode, i2c_slave_handler* handler)
{
    COMMON_ENTRY(i2c, I2C);
    i2c->config_as_slave(slave_address, address_width, bus_speed_mode, handler, i2c->base.userdata);
}

/* I2S */

void i2s_config_as_render(uintptr_t file, const audio_format_t* format, size_t delay_ms, i2s_align_mode align_mode, size_t channels_mask)
{
    COMMON_ENTRY(i2s, I2S);
    i2s->config_as_render(format, delay_ms, align_mode, channels_mask, i2s->base.userdata);
}

void i2s_config_as_capture(uintptr_t file, const audio_format_t* format, size_t delay_ms, i2s_align_mode align_mode, size_t channels_mask)
{
    COMMON_ENTRY(i2s, I2S);
    i2s->config_as_capture(format, delay_ms, align_mode, channels_mask, i2s->base.userdata);
}

void i2s_get_buffer(uintptr_t file, char** buffer, size_t* frames)
{
    COMMON_ENTRY(i2s, I2S);
    i2s->get_buffer(buffer, frames, i2s->base.userdata);
}

void i2s_release_buffer(uintptr_t file, size_t frames)
{
    COMMON_ENTRY(i2s, I2S);
    i2s->release_buffer(frames, i2s->base.userdata);
}

void i2s_start(uintptr_t file)
{
    COMMON_ENTRY(i2s, I2S);
    i2s->start(i2s->base.userdata);
}

void i2s_stop(uintptr_t file)
{
    COMMON_ENTRY(i2s, I2S);
    i2s->stop(i2s->base.userdata);
}

/* SPI */

uintptr_t spi_get_device(uintptr_t file, const char* name, spi_mode mode, spi_frame_format frame_format, size_t chip_select_line, size_t data_bit_length)
{
    COMMON_ENTRY(spi, SPI);
    spi_device_driver_t* driver = spi->get_device(mode, frame_format, chip_select_line, data_bit_length, spi->base.userdata);
    driver_registry_t* reg = install_custom_driver_core(name, DRIVER_SPI_DEVICE, driver);
    return io_alloc_handle(io_alloc_file(reg));
}

void spi_dev_config(uintptr_t file, size_t instruction_length, size_t address_length, size_t wait_cycles, spi_addr_inst_trans_mode trans_mode)
{
    COMMON_ENTRY(spi_device, SPI_DEVICE);
    spi_device->config(instruction_length, address_length, wait_cycles, trans_mode, spi_device->base.userdata);
}

double spi_dev_set_speed(uintptr_t file, double speed)
{
    COMMON_ENTRY(spi_device, SPI_DEVICE);
    return spi_device->set_speed(speed, spi_device->base.userdata);
}

int spi_dev_transfer_full_duplex(uintptr_t file, const char* write_buffer, size_t write_len, char* read_buffer, size_t read_len)
{
    COMMON_ENTRY(spi_device, SPI_DEVICE);
    return spi_device->transfer_full_duplex(write_buffer, write_len, read_buffer, read_len, spi_device->base.userdata);
}

int spi_dev_transfer_sequential(uintptr_t file, const char* write_buffer, size_t write_len, char* read_buffer, size_t read_len)
{
    COMMON_ENTRY(spi_device, SPI_DEVICE);
    return spi_device->transfer_sequential(write_buffer, write_len, read_buffer, read_len, spi_device->base.userdata);
}

void spi_dev_fill(uintptr_t file, size_t instruction, size_t address, uint32_t value, size_t count)
{
    COMMON_ENTRY(spi_device, SPI_DEVICE);
    return spi_device->fill(instruction, address, value, count, spi_device->base.userdata);
}

/* DVP */

void dvp_config(uintptr_t file, size_t width, size_t height, int auto_enable)
{
    COMMON_ENTRY(dvp, DVP);
    dvp->config(width, height, auto_enable, dvp->base.userdata);
}

void dvp_enable_frame(uintptr_t file)
{
    COMMON_ENTRY(dvp, DVP);
    dvp->enable_frame(dvp->base.userdata);
}

size_t dvp_get_output_num(uintptr_t file)
{
    COMMON_ENTRY(dvp, DVP);
    return dvp->output_num;
}

void dvp_set_signal(uintptr_t file, dvp_signal_type type, int value)
{
    COMMON_ENTRY(dvp, DVP);
    dvp->set_signal(type, value, dvp->base.userdata);
}

void dvp_set_output_enable(uintptr_t file, size_t index, int enable)
{
    COMMON_ENTRY(dvp, DVP);
    dvp->set_output_enable(index, enable, dvp->base.userdata);
}

void dvp_set_output_attributes(uintptr_t file, size_t index, video_format format, void* output_buffer)
{
    COMMON_ENTRY(dvp, DVP);
    dvp->set_output_attributes(index, format, output_buffer, dvp->base.userdata);
}

void dvp_set_frame_event_enable(uintptr_t file, video_frame_event event, int enable)
{
    COMMON_ENTRY(dvp, DVP);
    dvp->set_frame_event_enable(event, enable, dvp->base.userdata);
}

void dvp_set_on_frame_event(uintptr_t file, dvp_on_frame_event callback, void* callback_data)
{
    COMMON_ENTRY(dvp, DVP);
    dvp->set_on_frame_event(callback, callback_data, dvp->base.userdata);
}

/* SSCB */

uintptr_t sccb_get_device(uintptr_t file, const char* name, size_t slave_address, size_t address_width)
{
    COMMON_ENTRY(sccb, SCCB);
    sccb_device_driver_t* driver = sccb->get_device(slave_address, address_width, sccb->base.userdata);
    driver_registry_t* reg = install_custom_driver_core(name, DRIVER_SCCB_DEVICE, driver);
    return io_alloc_handle(io_alloc_file(reg));
}

uint8_t sccb_dev_read_byte(uintptr_t file, uint16_t reg_address)
{
    COMMON_ENTRY(sccb_device, SCCB_DEVICE);
    return sccb_device->read_byte(reg_address, sccb_device->base.userdata);
}

void sccb_dev_write_byte(uintptr_t file, uint16_t reg_address, uint8_t value)
{
    COMMON_ENTRY(sccb_device, SCCB_DEVICE);
    sccb_device->write_byte(reg_address, value, sccb_device->base.userdata);
}

/* FFT */

void fft_complex_uint16(fft_point point, fft_direction direction, uint32_t shifts_mask, const uint16_t* input, uint16_t* output)
{
    COMMON_ENTRY_FILE(fft_file_, fft, FFT);
    fft->complex_uint16(point, direction, shifts_mask, input, output, fft->base.userdata);
}

/* AES */

void aes_decrypt(aes_parameter* aes_param)
{
    COMMON_ENTRY_FILE(aes_file_, aes, AES);
    aes->decrypt(aes_param, aes->base.userdata);
}
void aes_encrypt(aes_parameter* aes_param)
{
    COMMON_ENTRY_FILE(aes_file_, aes, AES);
    aes->encrypt(aes_param, aes->base.userdata);
}

/* SHA */

void sha256_str(const char* str, size_t length, uint8_t* hash)
{
    COMMON_ENTRY_FILE(sha256_file_, sha256, SHA256);
    sha256->sha_str(str, length, hash, sha256->base.userdata);
}

/* TIMER */

size_t timer_set_interval(uintptr_t file, size_t nanoseconds)
{
    COMMON_ENTRY(timer, TIMER);
    return timer->set_interval(nanoseconds, timer->base.userdata);
}

void timer_set_ontick(uintptr_t file, timer_ontick ontick, void* ontick_data)
{
    COMMON_ENTRY(timer, TIMER);
    timer->set_ontick(ontick, ontick_data, timer->base.userdata);
}

void timer_set_enable(uintptr_t file, int enable)
{
    COMMON_ENTRY(timer, TIMER);
    timer->set_enable(enable, timer->base.userdata);
}

/* PWM */

size_t pwm_get_pin_count(uintptr_t file)
{
    COMMON_ENTRY(pwm, PWM);
    return pwm->pin_count;
}

double pwm_set_frequency(uintptr_t file, double frequency)
{
    COMMON_ENTRY(pwm, PWM);
    return pwm->set_frequency(frequency, pwm->base.userdata);
}

double pwm_set_active_duty_cycle_percentage(uintptr_t file, size_t pin, double duty_cycle_percentage)
{
    COMMON_ENTRY(pwm, PWM);
    return pwm->set_active_duty_cycle_percentage(pin, duty_cycle_percentage, pwm->base.userdata);
}

void pwm_set_enable(uintptr_t file, size_t pin, int enable)
{
    COMMON_ENTRY(pwm, PWM);
    pwm->set_enable(pin, enable, pwm->base.userdata);
}

/* RTC */

void rtc_get_datetime(uintptr_t file, datetime_t* datetime)
{
    COMMON_ENTRY(rtc, RTC);
    rtc->get_datetime(datetime, rtc->base.userdata);
}

void rtc_set_datetime(uintptr_t file, const datetime_t* datetime)
{
    COMMON_ENTRY(rtc, RTC);
    rtc->set_datetime(datetime, rtc->base.userdata);
}

/* HAL */

static uintptr_t pic_file_;

typedef struct
{
    pic_irq_handler pic_callbacks[MAX_IRQN];
    void* callback_userdata[MAX_IRQN];
} pic_context_t;

static pic_context_t pic_context_;
static SemaphoreHandle_t dma_free_;

static void init_dma_system()
{
    size_t count = 0;
    driver_registry_t* head = g_dma_drivers;
    while (head->name)
    {
        count++;
        head++;
    }

    dma_free_ = xSemaphoreCreateCounting(count, count);
}

void install_hal()
{
    install_hal_drivers();
    pic_file_ = io_open("/dev/pic0");
    configASSERT(pic_file_);

    install_dma_drivers();
    init_dma_system();
}

/* PIC */

void pic_set_irq_enable(size_t irq, int enable)
{
    COMMON_ENTRY_FILE(pic_file_, pic, PIC);
    pic->set_irq_enable(irq, enable, pic->base.userdata);
}

void pic_set_irq_priority(size_t irq, size_t priority)
{
    COMMON_ENTRY_FILE(pic_file_, pic, PIC);
    pic->set_irq_priority(irq, priority, pic->base.userdata);
}

void pic_set_irq_handler(size_t irq, pic_irq_handler handler, void* userdata)
{
    atomic_set(pic_context_.callback_userdata + irq, userdata);
    pic_context_.pic_callbacks[irq] = handler;
}

void kernel_iface_pic_on_irq(size_t irq)
{
    pic_irq_handler handler = pic_context_.pic_callbacks[irq];
    if (handler)
        handler(pic_context_.callback_userdata[irq]);
}

/* DMA */

uintptr_t dma_open_free()
{
    configASSERT(xSemaphoreTake(dma_free_, portMAX_DELAY) == pdTRUE);

    driver_registry_t *head = g_dma_drivers, *driver_reg = NULL;
    while (head->name)
    {
        driver_base_t* driver = (driver_base_t*)head->driver;
        if (driver->open(driver->userdata))
        {
            driver_reg = head;
            break;
        }

        head++;
    }

    configASSERT(driver_reg);
    uintptr_t handle = io_alloc_handle(io_alloc_file(driver_reg));
    return handle;
}

void dma_close(uintptr_t file)
{
    io_close(file);
}

static void dma_add_free()
{
    xSemaphoreGive(dma_free_);
}

void dma_set_select_request(uintptr_t file, uint32_t request)
{
    COMMON_ENTRY(dma, DMA);
    dma->set_select_request(request, dma->base.userdata);
}

void dma_transmit_async(uintptr_t file, const volatile void* src, volatile void* dest, int src_inc, int dest_inc, size_t element_size, size_t count, size_t burst_size, SemaphoreHandle_t completion_event)
{
    COMMON_ENTRY(dma, DMA);
    dma->transmit_async(src, dest, src_inc, dest_inc, element_size, count, burst_size, completion_event, dma->base.userdata);
}

void dma_transmit(uintptr_t file, const volatile void* src, volatile void* dest, int src_inc, int dest_inc, size_t element_size, size_t count, size_t burst_size)
{
    SemaphoreHandle_t event = xSemaphoreCreateBinary();
    dma_transmit_async(file, src, dest, src_inc, dest_inc, element_size, count, burst_size, event);
    //	printf("event: %p\n", event);
    configASSERT(xSemaphoreTake(event, portMAX_DELAY) == pdTRUE);
    vSemaphoreDelete(event);
}

void dma_loop_async(uintptr_t file, const volatile void** srcs, size_t src_num, volatile void** dests, size_t dest_num, int src_inc, int dest_inc, size_t element_size, size_t count, size_t burst_size, dma_stage_completion_handler stage_completion_handler, void* stage_completion_handler_data, SemaphoreHandle_t completion_event, int* stop_signal)
{
    COMMON_ENTRY(dma, DMA);
    dma->loop_async(srcs, src_num, dests, dest_num, src_inc, dest_inc, element_size, count, burst_size, stage_completion_handler, stage_completion_handler_data, completion_event, stop_signal, dma->base.userdata);
}

/* Custom Driver */

void install_custom_driver(const char* name, const custom_driver_t* driver)
{
    install_custom_driver_core(name, DRIVER_CUSTOM, driver);
}

/* System */

uint32_t system_set_cpu_frequency(uint32_t frequency)
{
    sysctl_clock_set_clock_select(SYSCTL_CLOCK_SELECT_ACLK, SYSCTL_SOURCE_IN0);

    sysctl_pll_disable(SYSCTL_PLL0);
    sysctl_pll_enable(SYSCTL_PLL0);
    uint32_t result = sysctl_pll_set_freq(SYSCTL_PLL0, SYSCTL_SOURCE_IN0, frequency * 2);

    while (sysctl_pll_is_lock(SYSCTL_PLL0) == 0)
        sysctl_pll_clear_slip(SYSCTL_PLL0);
    sysctl_clock_enable(SYSCTL_PLL0);
    sysctl_clock_set_clock_select(SYSCTL_CLOCK_SELECT_ACLK, SYSCTL_SOURCE_PLL0);
    uart_init();
    return result;
}
