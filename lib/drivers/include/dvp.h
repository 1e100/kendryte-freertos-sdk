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
#ifndef _DRIVER_DVP_H
#define _DRIVER_DVP_H

#ifdef __cplusplus
extern "C" {
#endif

/* clang-format off */
/**
 * @brief       DVP object
 */
struct dvp_t
{
    volatile uint32_t dvp_cfg;
    volatile uint32_t r_addr;
    volatile uint32_t g_addr;
    volatile uint32_t b_addr;
    volatile uint32_t cmos_cfg;
    volatile uint32_t sccb_cfg;
    volatile uint32_t sccb_ctl;
    volatile uint32_t axi;
    volatile uint32_t sts;
    volatile uint32_t reverse;
    volatile uint32_t rgb_addr;
} __attribute__((packed, aligned(4)));

/* DVP Config Register */
#define DVP_CFG_START_INT_ENABLE                0x00000001
#define DVP_CFG_FINISH_INT_ENABLE               0x00000002
#define DVP_CFG_AI_OUTPUT_ENABLE                0x00000004
#define DVP_CFG_DISPLAY_OUTPUT_ENABLE           0x00000008
#define DVP_CFG_AUTO_ENABLE                     0x00000010
#define DVP_CFG_BURST_SIZE_4BEATS               0x00000100
#define DVP_CFG_FORMAT_MASK                     0x00000600
#define DVP_CFG_RGB_FORMAT                      0x00000000
#define DVP_CFG_YUV_FORMAT                      0x00000200
#define DVP_CFG_Y_FORMAT                        0x00000600
#define DVP_CFG_HREF_BURST_NUM_MASK             0x000FF000
#define DVP_CFG_HREF_BURST_NUM(x)               ((x) << 12)
#define DVP_CFG_LINE_NUM_MASK                   0x3FF00000
#define DVP_CFG_LINE_NUM(x)                     ((x) << 20)

/* DVP CMOS Config Register */
#define DVP_CMOS_CLK_DIV_MASK                   0x000000FF
#define DVP_CMOS_CLK_DIV(x)                     ((x) << 0)
#define DVP_CMOS_CLK_ENABLE                     0x00000100
#define DVP_CMOS_RESET                          0x00010000
#define DVP_CMOS_POWER_DOWN                     0x01000000

/* DVP SCCB Config Register */
#define DVP_SCCB_BYTE_NUM_MASK                  0x00000003
#define DVP_SCCB_BYTE_NUM_2                     0x00000001
#define DVP_SCCB_BYTE_NUM_3                     0x00000002
#define DVP_SCCB_BYTE_NUM_4                     0x00000003
#define DVP_SCCB_SCL_LCNT_MASK                  0x0000FF00
#define DVP_SCCB_SCL_LCNT(x)                    ((x) << 8)
#define DVP_SCCB_SCL_HCNT_MASK                  0x00FF0000
#define DVP_SCCB_SCL_HCNT(x)                    ((x) << 16)
#define DVP_SCCB_RDATA_BYTE(x)                  ((x) >> 24)

/* DVP SCCB Control Register */
#define DVP_SCCB_WRITE_ENABLE                   0x00000001
#define DVP_SCCB_DEVICE_ADDRESS(x)              ((x) << 0)
#define DVP_SCCB_REG_ADDRESS(x)                 ((x) << 8)
#define DVP_SCCB_WDATA_BYTE0(x)                 ((x) << 16)
#define DVP_SCCB_WDATA_BYTE1(x)                 ((x) << 24)

/* DVP AXI Register */
#define DVP_AXI_GM_MLEN_MASK                    0x000000FF
#define DVP_AXI_GM_MLEN_1BYTE                   0x00000000
#define DVP_AXI_GM_MLEN_4BYTE                   0x00000003

/* DVP STS Register */
#define DVP_STS_FRAME_START                     0x00000001
#define DVP_STS_FRAME_START_WE                  0x00000002
#define DVP_STS_FRAME_FINISH                    0x00000100
#define DVP_STS_FRAME_FINISH_WE                 0x00000200
#define DVP_STS_DVP_EN                          0x00010000
#define DVP_STS_DVP_EN_WE                       0x00020000
#define DVP_STS_SCCB_EN                         0x01000000
#define DVP_STS_SCCB_EN_WE                      0x02000000
/* clang-format on */

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_DVP_H */
