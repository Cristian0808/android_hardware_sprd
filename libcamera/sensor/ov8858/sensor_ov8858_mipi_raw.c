/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
//#include "sensor_ov8858_raw_param_v3.c"
#include "newparam/sensor_ov8858_raw_param_main.c"
#else
#include "sensor_ov8858_raw_param_v2.c"
#endif
#include "sensor_ov8858_r2a_otp.c"
#include "packet_convert.c"

#define DW9714_VCM_SLAVE_ADDR (0x18>>1)
#define BU64241GWZ_VCM_SLAVE_ADDR (0x18 >> 1)

#define ov8858_I2C_ADDR_W        (0x6c>>1)
#define ov8858_I2C_ADDR_R         (0x6c>>1)

#define ov8858_RAW_PARAM_COM  0x0000

#define ov8858_MIN_FRAME_LEN_PRV  0x5e8
#define ov8858_4_LANES
static int s_ov8858_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;
static int s_exposure_time = 0;

LOCAL unsigned long _ov8858_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _ov8858_PowerOn(uint32_t power_on);
LOCAL uint32_t _ov8858_Identify(uint32_t param);
LOCAL uint32_t _ov8858_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _ov8858_after_snapshot(uint32_t param);
LOCAL uint32_t _ov8858_StreamOn(uint32_t param);
LOCAL uint32_t _ov8858_StreamOff(uint32_t param);
LOCAL uint32_t _ov8858_write_exposure(uint32_t param);
LOCAL uint32_t _ov8858_write_gain(uint32_t param);
LOCAL uint32_t _ov8858_write_af(uint32_t param);
LOCAL uint32_t _ov8858_flash(uint32_t param);
LOCAL uint32_t _ov8858_ExtFunc(unsigned long ctl_param);
LOCAL int _ov8858_get_VTS(void);
LOCAL int _ov8858_set_VTS(int VTS);
LOCAL uint32_t _ov8858_ReadGain(uint32_t param);
LOCAL uint32_t _ov8858_set_video_mode(uint32_t param);
LOCAL int _ov8858_get_shutter(void);
LOCAL uint32_t _ov8858_com_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov8858_dw9714_SRCInit(uint32_t mode);
LOCAL unsigned long _ov8858_GetExifInfo(unsigned long param);
LOCAL uint32_t _ov8858_read_otp_gain(uint32_t *param);
LOCAL unsigned long _ov8858_access_val(unsigned long param);
LOCAL uint32_t _ov8858_ex_write_exposure(uint32_t param);
LOCAL uint32_t _ov8858_test_pattern(void);
LOCAL uint32_t _ov8858_read_otp(unsigned long param);
LOCAL uint32_t _ov8858_get_golden_data(unsigned long param);

LOCAL const struct raw_param_info_tab s_ov8858_raw_param_tab[]={
	{ov8858_RAW_PARAM_COM, &s_ov8858_mipi_raw_info, ov8858_otp_identify_otp,ov8858_otp_update_otp},
	//{ov8858_RAW_PARAM_COM, &s_ov8858_mipi_raw_info, _ov8858_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov8858_mipi_raw_info_ptr=NULL;

static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;
static uint32_t s_ov8858_sensor_close_flag = 0;

LOCAL const SENSOR_REG_T ov8858_common_init[] = {
#if 0
	//@@ SIZE_1408X792_60FPS_MIPI_4LANE(Cropping)
	{0x0103, 0x01}, // Change 42 to 6c when copy the setting
	{0x0100, 0x00},
	{SENSOR_WRITE_DELAY, 0x0a},  //;delay 5ms
	{0x0302, 0x1a},
	{0x0303, 0x00},
	{0x0304, 0x03},
	{0x030d, 0x1d},
	{0x030e, 0x00},
	{0x030f, 0x09},
	{0x0312, 0x01},
	{0x031e, 0x0c},
	{0x3600, 0x00},
	{0x3601, 0x00},
	{0x3602, 0x00},
	{0x3603, 0x00},
	{0x3604, 0x22},
	{0x3605, 0x30},
	{0x3606, 0x00},
	{0x3607, 0x20},
	{0x3608, 0x11},
	{0x3609, 0x28},
	{0x360a, 0x00},
	{0x360b, 0x06},
	{0x360c, 0xdc},
	{0x360d, 0x40},
	{0x360e, 0x0c},
	{0x360f, 0x20},
	{0x3610, 0x07},
	{0x3611, 0x20},
	{0x3612, 0x88},
	{0x3613, 0x80},
	{0x3614, 0x58},
	{0x3615, 0x00},
	{0x3616, 0x4a},
	{0x3617, 0xb0},
	{0x3618, 0x56},
	{0x3619, 0x70},
	{0x361a, 0x99},
	{0x361b, 0x00},
	{0x361c, 0x07},
	{0x361d, 0x00},
	{0x361e, 0x00},
	{0x361f, 0x00},
	{0x3638, 0xff},
	{0x3633, 0x0c},
	{0x3634, 0x0c},
	{0x3635, 0x0c},
	{0x3636, 0x0c},
	{0x3645, 0x13},
	{0x3646, 0x83},
	{0x364a, 0x07},
	{0x3015, 0x01},
	{0x3018, 0x72},
	{0x3020, 0x93},
	{0x3022, 0x01},
	{0x3031, 0x0a},
	{0x3034, 0x00},
	{0x3106, 0x01},
	{0x3305, 0xf1},
	{0x3308, 0x00},
	{0x3309, 0x28},
	{0x330a, 0x00},
	{0x330b, 0x20},
	{0x330c, 0x00},
	{0x330d, 0x00},
	{0x330e, 0x00},
	{0x330f, 0x40},
	{0x3307, 0x04},
	{0x3500, 0x00},
	{0x3501, 0x32},
	{0x3502, 0x80},
	{0x3503, 0x00},
	{0x3505, 0x80},
	{0x3508, 0x04},
	{0x3509, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x3510, 0x00},
	{0x3511, 0x02},
	{0x3512, 0x00},
	{0x3700, 0x18},
	{0x3701, 0x0c},
	{0x3702, 0x28},
	{0x3703, 0x19},
	{0x3704, 0x14},
	{0x3705, 0x00},
	{0x3706, 0x35},
	{0x3707, 0x04},
	{0x3708, 0x24},
	{0x3709, 0x33},
	{0x370a, 0x00},
	{0x370b, 0xb5},
	{0x370c, 0x04},
	{0x3718, 0x12},
	{0x3719, 0x31},
	{0x3712, 0x42},
	{0x3714, 0x24},
	{0x371e, 0x19},
	{0x371f, 0x40},
	{0x3720, 0x05},
	{0x3721, 0x05},
	{0x3724, 0x06},
	{0x3725, 0x01},
	{0x3726, 0x06},
	{0x3728, 0x05},
	{0x3729, 0x02},
	{0x372a, 0x03},
	{0x372b, 0x53},
	{0x372c, 0xa3},
	{0x372d, 0x53},
	{0x372e, 0x06},
	{0x372f, 0x10},
	{0x3730, 0x01},
	{0x3731, 0x06},
	{0x3732, 0x14},
	{0x3733, 0x10},
	{0x3734, 0x40},
	{0x3736, 0x20},
	{0x373a, 0x05},
	{0x373b, 0x06},
	{0x373c, 0x0a},
	{0x373e, 0x03},
	{0x3755, 0x10},
	{0x3758, 0x00},
	{0x3759, 0x4c},
	{0x375a, 0x06},
	{0x375b, 0x13},
	{0x375c, 0x20},
	{0x375d, 0x02},
	{0x375e, 0x00},
	{0x375f, 0x14},
	{0x3768, 0x22},
	{0x3769, 0x44},
	{0x376a, 0x44},
	{0x3761, 0x00},
	{0x3762, 0x00},
	{0x3763, 0x00},
	{0x3766, 0xff},
	{0x376b, 0x00},
	{0x3772, 0x23},
	{0x3773, 0x02},
	{0x3774, 0x16},
	{0x3775, 0x12},
	{0x3776, 0x04},
	{0x3777, 0x00},
	{0x3778, 0x17},
	{0x37a0, 0x44},
	{0x37a1, 0x3d},
	{0x37a2, 0x3d},
	{0x37a3, 0x00},
	{0x37a4, 0x00},
	{0x37a5, 0x00},
	{0x37a6, 0x00},
	{0x37a7, 0x44},
	{0x37a8, 0x4c},
	{0x37a9, 0x4c},
	{0x3760, 0x00},
	{0x376f, 0x01},
	{0x37aa, 0x44},
	{0x37ab, 0x2e},
	{0x37ac, 0x2e},
	{0x37ad, 0x33},
	{0x37ae, 0x0d},
	{0x37af, 0x0d},
	{0x37b0, 0x00},
	{0x37b1, 0x00},
	{0x37b2, 0x00},
	{0x37b3, 0x42},
	{0x37b4, 0x42},
	{0x37b5, 0x33},
	{0x37b6, 0x00},
	{0x37b7, 0x00},
	{0x37b8, 0x00},
	{0x37b9, 0xff},
	{0x3800, 0x00},
	{0x3801, 0x0c},
	{0x3802, 0x00},
	{0x3803, 0x0c},
	{0x3804, 0x0c},
	{0x3805, 0xd3},
	{0x3806, 0x09},
	{0x3807, 0xa3},
	{0x3808, 0x05},
	{0x3809, 0x80},
	{0x380a, 0x03},
	{0x380b, 0x18},
	{0x380c, 0x03},
	{0x380d, 0xd4},
	{0x380e, 0x09},
	{0x380f, 0x3e},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3813, 0x02},
	{0x3814, 0x03},
	{0x3815, 0x01},
	{0x3820, 0x00},
	{0x3821, 0x67},
	{0x382a, 0x03},
	{0x382b, 0x01},
	{0x3830, 0x08},
	{0x3836, 0x02},
	{0x3837, 0x18},
	{0x3841, 0xff},
	{0x3846, 0x48},
	{0x3d85, 0x14},
	{0x3f08, 0x08},
	{0x3f0a, 0x80},
	{0x4000, 0xf1},
	{0x4001, 0x10},
	{0x4005, 0x10},
	{0x4002, 0x27},
	{0x4009, 0x81},
	{0x400b, 0x0c},
	{0x401b, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x01},
	{0x4021, 0x20},
	{0x4022, 0x01},
	{0x4023, 0x9f},
	{0x4024, 0x03},
	{0x4025, 0xe0},
	{0x4026, 0x04},
	{0x4027, 0x5f},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x04},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x08},
	{0x402f, 0x02},
	{0x401f, 0x00},
	{0x4034, 0x3f},
	{0x403d, 0x04},
	{0x4300, 0xff},
	{0x4301, 0x00},
	{0x4302, 0x0f},
	{0x4316, 0x00},
	{0x4500, 0x38},
	{0x4503, 0x18},
	{0x4600, 0x00},
	{0x4601, 0xaf},
	{0x481f, 0x32},
	{0x4837, 0x0a},
	{0x4850, 0x10},
	{0x4851, 0x32},
	{0x4b00, 0x2a},
	{0x4b0d, 0x00},
#if 1
	{0x4d00, 0x04},
	{0x4d01, 0x44},
	{0x4d02, 0xc1},
	{0x4d03, 0x72},
	{0x4d04, 0xb2},
	{0x4d05, 0xe3},
#else
	{0x4d00, 0x04},
	{0x4d01, 0x18},
	{0x4d02, 0xc3},
	{0x4d03, 0xff},
	{0x4d04, 0xff},
	{0x4d05, 0xff},
#endif
	{0x5000, 0xfe},
	{0x5001, 0x01},
	{0x5002, 0x08},
	{0x5003, 0x20},
	{0x5046, 0x12},
	{0x5901, 0x00},
	{0x5e00, 0x00},
	{0x5e01, 0x41},
	{0x382d, 0x7f},
	{0x4825, 0x3a},
	{0x4826, 0x40},
	{0x4808, 0x25},
#else
	//@@ SIZE_3264X2448_15FPS_MIPI_2LANE
	//100 99 3264 2448 ; Resolution
	//102 80 1
	//102 3601 5dc ;15fps
	//102 40 0 ; HDR Mode off
	//;FPGA set-up
	//c8 01 f2 ;MIPI FPGA;
	//CL 100 100;delay

	{0x103,0x01},
	//{0x303f,0x01},
	//{0x3012,0x6c},
	{SENSOR_WRITE_DELAY, 0x0a},  //;delay 5ms
	{0x100,0x00},
	{0x302,0x1e},//mipi colock
	{0x303,0x00},
	{0x304,0x03},
#if defined(ov8858_2_LANES)
	{0x30e,0x02},
#elif defined(ov8858_4_LANES)
	{0x30e,0x00},
#endif
	{0x30d,0x1e},//mipi
	{0x30f,0x04},
	{0x312,0x01},
	{0x31e,0x0c},
	{0x3600,0x00},
	{0x3601,0x00},
	{0x3602,0x00},
	{0x3603,0x00},
	{0x3604,0x22},
	{0x3605,0x20},
	{0x3606,0x00},
	{0x3607,0x20},
	{0x3608,0x11},
	{0x3609,0x28},
	{0x360a,0x00},
	{0x360b,0x05},
	{0x360c,0xd4},
	{0x360d,0x40},
	{0x360e,0x0c},
	{0x360f,0x20},
	{0x3610,0x07},
	{0x3611,0x20},
	{0x3612,0x88},
	{0x3613,0x80},
	{0x3614,0x58},
	{0x3615,0x00},
	{0x3616,0x4a},
	{0x3617,0x40},
	{0x3618,0x5a},
	{0x3619,0x70},
	{0x361a,0x99},
	{0x361b,0x0a},
	{0x361c,0x07},
	{0x361d,0x00},
	{0x361e,0x00},
	{0x361f,0x00},
	{0x3638,0xff},
	{0x3633,0x0f},
	{0x3634,0x0f},
	{0x3635,0x0f},
	{0x3636,0x12},
	{0x3645,0x13},
	{0x3646,0x83},
	{0x364a,0x07},
	{0x3015,0x00},
#if defined(ov8858_2_LANES)
	{0x3018,0x32},
#elif defined(ov8858_4_LANES)
	{0x3018,0x72},
#endif
	{0x3020,0x93},
	{0x3022,0x01},
	{0x3031,0x0a},
	{0x3034,0x00},
	{0x3106,0x01},
	{0x3305,0xf1},
	{0x3308,0x00},
	{0x3309,0x28},
	{0x330a,0x00},
	{0x330b,0x20},
	{0x330c,0x00},
	{0x330d,0x00},
	{0x330e,0x00},
	{0x330f,0x40},
	{0x3307,0x04},
	{0x3500,0x00},
	{0x3501,0x9a},
	{0x3502,0x20},
#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
	{0x3503,0x70},
#else
	{0x3503,0x80},
#endif
	{0x3505,0x80},
	{0x3508,0x02},
	{0x3509,0x00},
	{0x350c,0x00},
	{0x350d,0x80},
	{0x3510,0x00},
	{0x3511,0x02},
	{0x3512,0x00},
	{0x3700,0x30},
	{0x3701,0x18},
	{0x3702,0x50},
	{0x3703,0x32},
	{0x3704,0x28},
	{0x3705,0x00},
	{0x3706,0x82},
	{0x3707,0x08},
	{0x3708,0x48},
	{0x3709,0x66},
	{0x370a,0x01},
	{0x370b,0x82},
	{0x370c,0x07},
	{0x3718,0x14},
	{0x3719,0x31},
	{0x3712,0x44},
	{0x3714,0x24},
	{0x371e,0x31},
	{0x371f,0x7f},
	{0x3720,0x0a},
	{0x3721,0x0a},
	{0x3724,0x0c},
	{0x3725,0x02},
	{0x3726,0x0c},
	{0x3728,0x0a},
	{0x3729,0x03},
	{0x372a,0x06},
	{0x372b,0xa6},
	{0x372c,0xa6},
	{0x372d,0xa6},
	{0x372e,0x0c},
	{0x372f,0x20},
	{0x3730,0x02},
	{0x3731,0x0c},
	{0x3732,0x28},
	{0x3733,0x10},
	{0x3734,0x40},
	{0x3736,0x30},
	{0x373a,0x0a},
	{0x373b,0x0b},
	{0x373c,0x14},
	{0x373e,0x06},
	{0x3750,0x0a},
	{0x3751,0x0e},
	{0x3755,0x10},
	{0x3758,0x00},
	{0x3759,0x4c},
	{0x375a,0x0c},
	{0x375b,0x26},
	{0x375c,0x20},
	{0x375d,0x04},
	{0x375e,0x00},
	{0x375f,0x28},
	{0x3768,0x22},
	{0x3769,0x44},
	{0x376a,0x44},
	{0x3761,0x00},
	{0x3762,0x00},
	{0x3763,0x00},
	{0x3766,0xff},
	{0x376b,0x00},
	{0x3772,0x46},
	{0x3773,0x04},
	{0x3774,0x2c},
	{0x3775,0x13},
	{0x3776,0x08},
	{0x3777,0x00},
	{0x3778,0x17},
	{0x37a0,0x88},
	{0x37a1,0x7a},
	{0x37a2,0x7a},
	{0x37a3,0x00},
	{0x37a4,0x00},
	{0x37a5,0x00},
	{0x37a6,0x00},
	{0x37a7,0x88},
	{0x37a8,0x98},
	{0x37a9,0x98},
	{0x3760,0x00},
	{0x376f,0x01},
	{0x37aa,0x88},
	{0x37ab,0x5c},
	{0x37ac,0x2e},
	{0x37ad,0x33},
	{0x37ae,0x0d},
	{0x37af,0x0d},
	{0x37b0,0x00},
	{0x37b1,0x00},
	{0x37b2,0x00},
	{0x37b3,0x84},
	{0x37b4,0x42},
	{0x37b5,0x60},
	{0x37b6,0x00},
	{0x37b7,0x00},
	{0x37b8,0x00},
	{0x37b9,0xff},
	{0x3800,0x00},
	{0x3801,0x0c},
	{0x3802,0x00},
	{0x3803,0x0c},
	{0x3804,0x0c},
	{0x3805,0xd3},
	{0x3806,0x09},
	{0x3807,0xa3},
	{0x3808,0x0c},
	{0x3809,0xc0},
	{0x380a,0x09},
	{0x380b,0x90},
	{0x380c,0x07},
	{0x380d,0x94},
	{0x380e,0x09},
	{0x380f,0xaa},
	{0x3810,0x00},
	{0x3811,0x04},
	{0x3813,0x02},
	{0x3814,0x01},
	{0x3815,0x01},
	//{0x3820,0x06},//0x00->0x06
	//{0x3821,0x40},//0x46->0x40
	{0x3820,0x00},
       {0x3821,0x46},
	{0x382a,0x01},
	{0x382b,0x01},
	{0x3830,0x06},
	{0x3836,0x01},
	{0x3837,0x18},
	{0x3841,0xff},
	{0x3846,0x48},
	{0x3d85,0x16},
	{0x3d8c,0x73},
	{0x3d8d,0xde},
	{0x3f08,0x10},
	{0x4000,0xf1},
	{0x4001,0x00},
	{0x4005,0x10},
	{0x4002,0x27},
	{0x4009,0x81},
	{0x400b,0x0c},
	{0x4011,0x20},
	{0x401b,0x00},
	{0x401d,0x00},
	{0x4020,0x00},
	{0x4021,0x04},
	{0x4022,0x0c},
	{0x4023,0x60},
	{0x4024,0x0f},
	{0x4025,0x36},
	{0x4026,0x0f},
	{0x4027,0x37},
	{0x4028,0x00},
	{0x4029,0x02},
	{0x402a,0x04},
	{0x402b,0x08},
	{0x402c,0x00},
	{0x402d,0x02},
	{0x402e,0x04},
	{0x402f,0x08},
	{0x401f,0x00},
	{0x4034,0x3f},
	{0x403d,0x04},
	{0x4300,0xff},
	{0x4301,0x00},
	{0x4302,0x0f},
	{0x4316,0x00},
	{0x4503,0x18},
	{0x4600,0x01},
	{0x4601,0x97},
	{0x481f,0x32},
	{0x4837,0x16},//16
	{0x4850,0x10},
	{0x4851,0x32},
	{0x4b00,0x2a},
	{0x4b0d,0x00},
	{0x4d00,0x04},
	{0x4d01,0x18},
	{0x4d02,0xc3},
	{0x4d03,0xff},
	{0x4d04,0xff},
	{0x4d05,0xff},
	{0x5000,0xfe},
	{0x5001,0x01},
	{0x5002,0x08},
	{0x5003,0x20},
	{0x5046,0x12},
	{0x5780,0x3e},
	{0x5781,0x0f},
	{0x5782,0x44},
	{0x5783,0x02},
	{0x5784,0x01},
	{0x5785,0x00},
	{0x5786,0x00},
	{0x5787,0x04},
	{0x5788,0x02},
	{0x5789,0x0f},
	{0x578a,0xfd},
	{0x578b,0xf5},
	{0x578c,0xf5},
	{0x578d,0x03},
	{0x578e,0x08},
	{0x578f,0x0c},
	{0x5790,0x08},
	{0x5791,0x04},
	{0x5792,0x00},
	{0x5793,0x52},
	{0x5794,0xa3},
	//{0x5871,0x0d},
	//{0x5870,0x18},
	//{0x586e,0x10},
	//{0x586f,0x08},
	{0x58f8,0x3d},
	{0x5901,0x00},
	{0x5b00,0x02},
	{0x5b01,0x10},
	{0x5b02,0x03},
	{0x5b03,0xcf},
	{0x5b05,0x6c},
	{0x5e00,0x00},
	{0x5e01,0x41},
	{0x4825,0x3a},
	{0x4826,0x40},
	{0x4808,0x25},
	{0x3763,0x18},
	{0x3768,0xcc},
	{0x470b,0x28},
	{0x4202,0x00},
	{0x400d,0x10},
	{0x4040,0x07},
	{0x403e,0x08},
	{0x4041,0xc6},
	{0x3007,0x80},
	{0x400a,0x01},
	{0x3015,0x01},
	{0x0100, 0x00}
#endif
};

LOCAL const SENSOR_REG_T ov8858_1408X792_setting[] = {
	//@@ SIZE_1408X792_60FPS_MIPI_4LANE(Cropping)
	//line_time=7.04us   mipi clk = 312M
	{0x0103, 0x01}, // Change 42 to 6c when copy the setting
	{0x0100, 0x00},
	{0x0302, 0x1a},
	{0x0303, 0x00},
	{0x0304, 0x03},
	{0x030d, 0x1d},
	{0x030e, 0x00},
	{0x030f, 0x09},
	{0x0312, 0x01},
	{0x031e, 0x0c},
	{0x3600, 0x00},
	{0x3601, 0x00},
	{0x3602, 0x00},
	{0x3603, 0x00},
	{0x3604, 0x22},
	{0x3605, 0x30},
	{0x3606, 0x00},
	{0x3607, 0x20},
	{0x3608, 0x11},
	{0x3609, 0x28},
	{0x360a, 0x00},
	{0x360b, 0x06},
	{0x360c, 0xdc},
	{0x360d, 0x40},
	{0x360e, 0x0c},
	{0x360f, 0x20},
	{0x3610, 0x07},
	{0x3611, 0x20},
	{0x3612, 0x88},
	{0x3613, 0x80},
	{0x3614, 0x58},
	{0x3615, 0x00},
	{0x3616, 0x4a},
	{0x3617, 0xb0},
	{0x3618, 0x56},
	{0x3619, 0x70},
	{0x361a, 0x99},
	{0x361b, 0x00},
	{0x361c, 0x07},
	{0x361d, 0x00},
	{0x361e, 0x00},
	{0x361f, 0x00},
	{0x3638, 0xff},
	{0x3633, 0x0c},
	{0x3634, 0x0c},
	{0x3635, 0x0c},
	{0x3636, 0x0c},
	{0x3645, 0x13},
	{0x3646, 0x83},
	{0x364a, 0x07},
	{0x3015, 0x01},
	{0x3018, 0x72},
	{0x3020, 0x93},
	{0x3022, 0x01},
	{0x3031, 0x0a},
	{0x3034, 0x00},
	{0x3106, 0x01},
	{0x3305, 0xf1},
	{0x3308, 0x00},
	{0x3309, 0x28},
	{0x330a, 0x00},
	{0x330b, 0x20},
	{0x330c, 0x00},
	{0x330d, 0x00},
	{0x330e, 0x00},
	{0x330f, 0x40},
	{0x3307, 0x04},
	{0x3500, 0x00},
	{0x3501, 0x32},
	{0x3502, 0x80},
	{0x3503, 0x00},
	{0x3505, 0x80},
	{0x3508, 0x04},
	{0x3509, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x3510, 0x00},
	{0x3511, 0x02},
	{0x3512, 0x00},
	{0x3700, 0x18},
	{0x3701, 0x0c},
	{0x3702, 0x28},
	{0x3703, 0x19},
	{0x3704, 0x14},
	{0x3705, 0x00},
	{0x3706, 0x35},
	{0x3707, 0x04},
	{0x3708, 0x24},
	{0x3709, 0x33},
	{0x370a, 0x00},
	{0x370b, 0xb5},
	{0x370c, 0x04},
	{0x3718, 0x12},
	{0x3719, 0x31},
	{0x3712, 0x42},
	{0x3714, 0x24},
	{0x371e, 0x19},
	{0x371f, 0x40},
	{0x3720, 0x05},
	{0x3721, 0x05},
	{0x3724, 0x06},
	{0x3725, 0x01},
	{0x3726, 0x06},
	{0x3728, 0x05},
	{0x3729, 0x02},
	{0x372a, 0x03},
	{0x372b, 0x53},
	{0x372c, 0xa3},
	{0x372d, 0x53},
	{0x372e, 0x06},
	{0x372f, 0x10},
	{0x3730, 0x01},
	{0x3731, 0x06},
	{0x3732, 0x14},
	{0x3733, 0x10},
	{0x3734, 0x40},
	{0x3736, 0x20},
	{0x373a, 0x05},
	{0x373b, 0x06},
	{0x373c, 0x0a},
	{0x373e, 0x03},
	{0x3755, 0x10},
	{0x3758, 0x00},
	{0x3759, 0x4c},
	{0x375a, 0x06},
	{0x375b, 0x13},
	{0x375c, 0x20},
	{0x375d, 0x02},
	{0x375e, 0x00},
	{0x375f, 0x14},
	{0x3768, 0x22},
	{0x3769, 0x44},
	{0x376a, 0x44},
	{0x3761, 0x00},
	{0x3762, 0x00},
	{0x3763, 0x00},
	{0x3766, 0xff},
	{0x376b, 0x00},
	{0x3772, 0x23},
	{0x3773, 0x02},
	{0x3774, 0x16},
	{0x3775, 0x12},
	{0x3776, 0x04},
	{0x3777, 0x00},
	{0x3778, 0x17},
	{0x37a0, 0x44},
	{0x37a1, 0x3d},
	{0x37a2, 0x3d},
	{0x37a3, 0x00},
	{0x37a4, 0x00},
	{0x37a5, 0x00},
	{0x37a6, 0x00},
	{0x37a7, 0x44},
	{0x37a8, 0x4c},
	{0x37a9, 0x4c},
	{0x3760, 0x00},
	{0x376f, 0x01},
	{0x37aa, 0x44},
	{0x37ab, 0x2e},
	{0x37ac, 0x2e},
	{0x37ad, 0x33},
	{0x37ae, 0x0d},
	{0x37af, 0x0d},
	{0x37b0, 0x00},
	{0x37b1, 0x00},
	{0x37b2, 0x00},
	{0x37b3, 0x42},
	{0x37b4, 0x42},
	{0x37b5, 0x33},
	{0x37b6, 0x00},
	{0x37b7, 0x00},
	{0x37b8, 0x00},
	{0x37b9, 0xff},
	{0x3800, 0x00},
	{0x3801, 0x0c},
	{0x3802, 0x00},
	{0x3803, 0x0c},
	{0x3804, 0x0c},
	{0x3805, 0xd3},
	{0x3806, 0x09},
	{0x3807, 0xa3},
	{0x3808, 0x05},
	{0x3809, 0x80},
	{0x380a, 0x03},
	{0x380b, 0x18},
	{0x380c, 0x03},   //HTS [15:8]
	{0x380d, 0xd4},   //HTS [7:0]
	{0x380e, 0x09},   //VTS [15:8]
	{0x380f, 0x3e},  //VTS [7:0]
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3813, 0x02},
	{0x3814, 0x03},
	{0x3815, 0x01},
	{0x3820, 0x00},
	{0x3821, 0x67},
	{0x382a, 0x03},
	{0x382b, 0x01},
	{0x3830, 0x08},
	{0x3836, 0x02},
	{0x3837, 0x18},
	{0x3841, 0xff},
	{0x3846, 0x48},
	{0x3d85, 0x14},
	{0x3f08, 0x08},
	{0x3f0a, 0x80},
	{0x4000, 0xf1},
	{0x4001, 0x10},
	{0x4005, 0x10},
	{0x4002, 0x27},
	{0x4009, 0x81},
	{0x400b, 0x0c},
	{0x401b, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x01},
	{0x4021, 0x20},
	{0x4022, 0x01},
	{0x4023, 0x9f},
	{0x4024, 0x03},
	{0x4025, 0xe0},
	{0x4026, 0x04},
	{0x4027, 0x5f},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x04},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x08},
	{0x402f, 0x02},
	{0x401f, 0x00},
	{0x4034, 0x3f},
	{0x403d, 0x04},
	{0x4300, 0xff},
	{0x4301, 0x00},
	{0x4302, 0x0f},
	{0x4316, 0x00},
	{0x4500, 0x38},
	{0x4503, 0x18},
	{0x4600, 0x00},
	{0x4601, 0xaf},
	{0x481f, 0x32},
	{0x4837, 0x0a},
	{0x4850, 0x10},
	{0x4851, 0x32},
	{0x4b00, 0x2a},
	{0x4b0d, 0x00},
#if 1
	{0x4d00, 0x04},
	{0x4d01, 0x44},
	{0x4d02, 0xc1},
	{0x4d03, 0x72},
	{0x4d04, 0xb2},
	{0x4d05, 0xe3},
#else
	{0x4d00, 0x04},
	{0x4d01, 0x18},
	{0x4d02, 0xc3},
	{0x4d03, 0xff},
	{0x4d04, 0xff},
	{0x4d05, 0xff},
#endif
	{0x5000, 0xfe},
	{0x5001, 0x01},
	{0x5002, 0x08},
	{0x5003, 0x20},
	{0x5046, 0x12},
	{0x5901, 0x00},
	{0x5e00, 0x00},
	{0x5e01, 0x41},
	{0x382d, 0x7f},
	{0x4825, 0x3a},
	{0x4826, 0x40},
	{0x4808, 0x25},
};

LOCAL const SENSOR_REG_T ov8858_3264X1836_setting[] = {
	// @@ SIZE_3264X1836_30FPS_MIPI_4LANE
	//line_time=8.9us   mipi clk = 312M
	{0x0100, 0x01}, // Change 42 to 6c when copy the setting
	{0x0100, 0x00},
	{0x0302, 0x1a},
	{0x0303, 0x00},
	{0x0304, 0x03},
	{0x030d, 0x1d},
	{0x030e, 0x00},
	{0x030f, 0x04},
	{0x0312, 0x01},
	{0x031e, 0x0c},
	{0x3600, 0x00},
	{0x3601, 0x00},
	{0x3602, 0x00},
	{0x3603, 0x00},
	{0x3604, 0x22},
	{0x3605, 0x30},
	{0x3606, 0x00},
	{0x3607, 0x20},
	{0x3608, 0x11},
	{0x3609, 0x28},
	{0x360a, 0x00},
	{0x360b, 0x06},
	{0x360c, 0xdc},
	{0x360d, 0x40},
	{0x360e, 0x0c},
	{0x360f, 0x20},
	{0x3610, 0x07},
	{0x3611, 0x20},
	{0x3612, 0x88},
	{0x3613, 0x80},
	{0x3614, 0x58},
	{0x3615, 0x00},
	{0x3616, 0x4a},
	{0x3617, 0xb0},
	{0x3618, 0x56},
	{0x3619, 0x70},
	{0x361a, 0x99},
	{0x361b, 0x00},
	{0x361c, 0x07},
	{0x361d, 0x00},
	{0x361e, 0x00},
	{0x361f, 0x00},
	{0x3638, 0xff},
	{0x3633, 0x0c},
	{0x3634, 0x0c},
	{0x3635, 0x0c},
	{0x3636, 0x0c},
	{0x3645, 0x13},
	{0x3646, 0x83},
	{0x364a, 0x07},
	{0x3015, 0x01},
	{0x3018, 0x72},
	{0x3020, 0x93},
	{0x3022, 0x01},
	{0x3031, 0x0a},
	{0x3034, 0x00},
	{0x3106, 0x01},
	{0x3305, 0xf1},
	{0x3308, 0x00},
	{0x3309, 0x28},
	{0x330a, 0x00},
	{0x330b, 0x20},
	{0x330c, 0x00},
	{0x330d, 0x00},
	{0x330e, 0x00},
	{0x330f, 0x40},
	{0x3307, 0x04},
	{0x3500, 0x00},
	{0x3501, 0x74},
	{0x3502, 0x80},
	{0x3503, 0x00},
	{0x3505, 0x80},
	{0x3508, 0x02},
	{0x3509, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x3510, 0x00},
	{0x3511, 0x02},
	{0x3512, 0x00},
	{0x3700, 0x30},
	{0x3701, 0x18},
	{0x3702, 0x50},
	{0x3703, 0x32},
	{0x3704, 0x28},
	{0x3705, 0x00},
	{0x3706, 0x6a},
	{0x3707, 0x08},
	{0x3708, 0x48},
	{0x3709, 0x66},
	{0x370a, 0x01},
	{0x370b, 0x6a},
	{0x370c, 0x07},
	{0x3718, 0x14},
	{0x3719, 0x31},
	{0x3712, 0x44},
	{0x3714, 0x24},
	{0x371e, 0x31},
	{0x371f, 0x7f},
	{0x3720, 0x0a},
	{0x3721, 0x0a},
	{0x3724, 0x0c},
	{0x3725, 0x02},
	{0x3726, 0x0c},
	{0x3728, 0x0a},
	{0x3729, 0x03},
	{0x372a, 0x06},
	{0x372b, 0xa6},
	{0x372c, 0xa6},
	{0x372d, 0xa6},
	{0x372e, 0x0c},
	{0x372f, 0x20},
	{0x3730, 0x02},
	{0x3731, 0x0c},
	{0x3732, 0x28},
	{0x3733, 0x10},
	{0x3734, 0x40},
	{0x3736, 0x30},
	{0x373a, 0x0a},
	{0x373b, 0x0b},
	{0x373c, 0x14},
	{0x373e, 0x06},
	{0x3755, 0x10},
	{0x3758, 0x00},
	{0x3759, 0x4c},
	{0x375a, 0x0c},
	{0x375b, 0x26},
	{0x375c, 0x20},
	{0x375d, 0x04},
	{0x375e, 0x00},
	{0x375f, 0x28},
	{0x3768, 0x22},
	{0x3769, 0x44},
	{0x376a, 0x44},
	{0x3761, 0x00},
	{0x3762, 0x00},
	{0x3763, 0x00},
	{0x3766, 0xff},
	{0x376b, 0x00},
	{0x3772, 0x46},
	{0x3773, 0x04},
	{0x3774, 0x2c},
	{0x3775, 0x13},
	{0x3776, 0x08},
	{0x3777, 0x00},
	{0x3778, 0x16},
	{0x37a0, 0x88},
	{0x37a1, 0x7a},
	{0x37a2, 0x7a},
	{0x37a3, 0x00},
	{0x37a4, 0x00},
	{0x37a5, 0x00},
	{0x37a6, 0x00},
	{0x37a7, 0x88},
	{0x37a8, 0x98},
	{0x37a9, 0x98},
	{0x3760, 0x00},
	{0x376f, 0x01},
	{0x37aa, 0x88},
	{0x37ab, 0x5c},
	{0x37ac, 0x5c},
	{0x37ad, 0x55},
	{0x37ae, 0x19},
	{0x37af, 0x19},
	{0x37b0, 0x00},
	{0x37b1, 0x00},
	{0x37b2, 0x00},
	{0x37b3, 0x84},
	{0x37b4, 0x84},
	{0x37b5, 0x66},
	{0x37b6, 0x00},
	{0x37b7, 0x00},
	{0x37b8, 0x00},
	{0x37b9, 0xff},
	{0x3800, 0x00},
	{0x3801, 0x0c},
	{0x3802, 0x00},
	{0x3803, 0x0c},
	{0x3804, 0x0c},
	{0x3805, 0xd3},
	{0x3806, 0x09},
	{0x3807, 0xa3},
	{0x3808, 0x0c},
	{0x3809, 0xc0},
	{0x380a, 0x07},
	{0x380b, 0x2c},
	{0x380c, 0x09},  //HTS [15:8]
	{0x380d, 0xae},  //HTS [7:0]
	{0x380e, 0x07},  //VTS [15:8]
	{0x380f, 0x50}, //VTS [7:0]
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3813, 0x02},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3820, 0x00},
	{0x3821, 0x46},
	{0x382a, 0x01},
	{0x382b, 0x01},
	{0x3830, 0x06},
	{0x3836, 0x01},
	{0x3837, 0x18},
	{0x3841, 0xff},
	{0x3846, 0x48},
	{0x3d85, 0x14},
	{0x3f08, 0x10},
	{0x4000, 0xf1},
	{0x4001, 0x00},
	{0x4005, 0x10},
	{0x4002, 0x27},
	{0x4009, 0x81},
	{0x400b, 0x0c},
	{0x401b, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x00},
	{0x4021, 0x04},
	{0x4022, 0x0b},
	{0x4023, 0xc3},
	{0x4024, 0x0c},
	{0x4025, 0x36},
	{0x4026, 0x0c},
	{0x4027, 0x37},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x08},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x0c},
	{0x402f, 0x02},
	{0x401f, 0x00},
	{0x4034, 0x3f},
	{0x403d, 0x04},
	{0x4300, 0xff},
	{0x4301, 0x00},
	{0x4302, 0x0f},
	{0x4316, 0x00},
	{0x4503, 0x18},
	{0x4600, 0x01},
	{0x4601, 0x97},
	{0x481f, 0x32},
	{0x4837, 0x0a},
	{0x4850, 0x10},
	{0x4851, 0x32},
	{0x4b00, 0x2a},
	{0x4b0d, 0x00},
#if 1
	{0x4d00, 0x04},
	{0x4d01, 0x44},
	{0x4d02, 0xc1},
	{0x4d03, 0x72},
	{0x4d04, 0xb2},
	{0x4d05, 0xe3},
#else
	{0x4d00, 0x04},
	{0x4d01, 0x18},
	{0x4d02, 0xc3},
	{0x4d03, 0xff},
	{0x4d04, 0xff},
	{0x4d05, 0xff},
#endif
	{0x5000, 0xfe},
	{0x5001, 0x01},
	{0x5002, 0x08},
	{0x5003, 0x20},
	{0x5046, 0x12},
	{0x5901, 0x00},
	{0x5e00, 0x00},
	{0x5e01, 0x41},
	{0x4825, 0x3a},
	{0x4826, 0x40},
	{0x4808, 0x25},
};

LOCAL const SENSOR_REG_T ov8858_3264x2448_setting[] = {
#if 0
	//@@SIZE_3264X2448_25FPS_MIPI_4LANE
	//line_time=16.16us   mipi clk = 288M
	{0x0103, 0x01}, // Change 42 to 6c when copy the setting
	{0x0100, 0x00},
	{0x0302, 0x18},
	{0x0303, 0x00},
	{0x0304, 0x03},
	{0x030d, 0x19},
	{0x030e, 0x00},
	{0x030f, 0x04},
	{0x0312, 0x01},
	{0x031e, 0x0c},
	{0x3600, 0x00},
	{0x3601, 0x00},
	{0x3602, 0x00},
	{0x3603, 0x00},
	{0x3604, 0x22},
	{0x3605, 0x30},
	{0x3606, 0x00},
	{0x3607, 0x20},
	{0x3608, 0x11},
	{0x3609, 0x28},
	{0x360a, 0x00},
	{0x360b, 0x06},
	{0x360c, 0xdc},
	{0x360d, 0x40},
	{0x360e, 0x0c},
	{0x360f, 0x20},
	{0x3610, 0x07},
	{0x3611, 0x20},
	{0x3612, 0x88},
	{0x3613, 0x80},
	{0x3614, 0x58},
	{0x3615, 0x00},
	{0x3616, 0x4a},
	{0x3617, 0xb0},
	{0x3618, 0x56},
	{0x3619, 0x70},
	{0x361a, 0x99},
	{0x361b, 0x00},
	{0x361c, 0x07},
	{0x361d, 0x00},
	{0x361e, 0x00},
	{0x361f, 0x00},
	{0x3638, 0xff},
	{0x3633, 0x0c},
	{0x3634, 0x0c},
	{0x3635, 0x0c},
	{0x3636, 0x0c},
	{0x3645, 0x13},
	{0x3646, 0x83},
	{0x364a, 0x07},
	{0x3015, 0x01},
	{0x3018, 0x72},
	{0x3020, 0x93},
	{0x3022, 0x01},
	{0x3031, 0x0a},
	{0x3034, 0x00},
	{0x3106, 0x01},
	{0x3305, 0xf1},
	{0x3308, 0x00},
	{0x3309, 0x28},
	{0x330a, 0x00},
	{0x330b, 0x20},
	{0x330c, 0x00},
	{0x330d, 0x00},
	{0x330e, 0x00},
	{0x330f, 0x40},
	{0x3307, 0x04},
	{0x3500, 0x00},
	{0x3501, 0x9a},
	{0x3502, 0x20},
	{0x3503, 0x00},
	{0x3505, 0x80},
	{0x3508, 0x02},
	{0x3509, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x80},
	{0x3510, 0x00},
	{0x3511, 0x02},
	{0x3512, 0x00},
	{0x3700, 0x30},
	{0x3701, 0x18},
	{0x3702, 0x50},
	{0x3703, 0x32},
	{0x3704, 0x28},
	{0x3705, 0x00},
	{0x3706, 0x6a},
	{0x3707, 0x08},
	{0x3708, 0x48},
	{0x3709, 0x66},
	{0x370a, 0x01},
	{0x370b, 0x6a},
	{0x370c, 0x07},
	{0x3718, 0x14},
	{0x3719, 0x31},
	{0x3712, 0x44},
	{0x3714, 0x24},
	{0x371e, 0x31},
	{0x371f, 0x7f},
	{0x3720, 0x0a},
	{0x3721, 0x0a},
	{0x3724, 0x0c},
	{0x3725, 0x02},
	{0x3726, 0x0c},
	{0x3728, 0x0a},
	{0x3729, 0x03},
	{0x372a, 0x06},
	{0x372b, 0xa6},
	{0x372c, 0xa6},
	{0x372d, 0xa6},
	{0x372e, 0x0c},
	{0x372f, 0x20},
	{0x3730, 0x02},
	{0x3731, 0x0c},
	{0x3732, 0x28},
	{0x3733, 0x10},
	{0x3734, 0x40},
	{0x3736, 0x30},
	{0x373a, 0x0a},
	{0x373b, 0x0b},
	{0x373c, 0x14},
	{0x373e, 0x06},
	{0x3755, 0x10},
	{0x3758, 0x00},
	{0x3759, 0x4c},
	{0x375a, 0x0c},
	{0x375b, 0x26},
	{0x375c, 0x20},
	{0x375d, 0x04},
	{0x375e, 0x00},
	{0x375f, 0x28},
	{0x3768, 0x22},
	{0x3769, 0x44},
	{0x376a, 0x44},
	{0x3761, 0x00},
	{0x3762, 0x00},
	{0x3763, 0x00},
	{0x3766, 0xff},
	{0x376b, 0x00},
	{0x3772, 0x46},
	{0x3773, 0x04},
	{0x3774, 0x2c},
	{0x3775, 0x13},
	{0x3776, 0x08},
	{0x3777, 0x00},
	{0x3778, 0x16},
	{0x37a0, 0x88},
	{0x37a1, 0x7a},
	{0x37a2, 0x7a},
	{0x37a3, 0x00},
	{0x37a4, 0x00},
	{0x37a5, 0x00},
	{0x37a6, 0x00},
	{0x37a7, 0x88},
	{0x37a8, 0x98},
	{0x37a9, 0x98},
	{0x3760, 0x00},
	{0x376f, 0x01},
	{0x37aa, 0x88},
	{0x37ab, 0x5c},
	{0x37ac, 0x5c},
	{0x37ad, 0x55},
	{0x37ae, 0x19},
	{0x37af, 0x19},
	{0x37b0, 0x00},
	{0x37b1, 0x00},
	{0x37b2, 0x00},
	{0x37b3, 0x84},
	{0x37b4, 0x84},
	{0x37b5, 0x66},
	{0x37b6, 0x00},
	{0x37b7, 0x00},
	{0x37b8, 0x00},
	{0x37b9, 0xff},
	{0x3800, 0x00},
	{0x3801, 0x0c},
	{0x3802, 0x00},
	{0x3803, 0x0c},
	{0x3804, 0x0c},
	{0x3805, 0xd3},
	{0x3806, 0x09},
	{0x3807, 0xa3},
	{0x3808, 0x0c},
	{0x3809, 0xc0},
	{0x380a, 0x09},
	{0x380b, 0x90},
	{0x380c, 0x07},   //HTS [15:8]
	{0x380d, 0x94},   //HTS [7:0]
	{0x380e, 0x09},   //VTS [15:8]
	{0x380f, 0xaa},   //VTS [7:0]
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3813, 0x02},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3820, 0x00},
	{0x3821, 0x46},
	{0x382a, 0x01},
	{0x382b, 0x01},
	{0x3830, 0x06},
	{0x3836, 0x01},
	{0x3837, 0x18},
	{0x3841, 0xff},
	{0x3846, 0x48},
	{0x3d85, 0x14},
	{0x3f08, 0x10},
	{0x4000, 0xf1},
	{0x4001, 0x00},
	{0x4005, 0x10},
	{0x4002, 0x27},
	{0x4009, 0x81},
	{0x400b, 0x0c},
	{0x401b, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x00},
	{0x4021, 0x04},
	{0x4022, 0x0b},
	{0x4023, 0xc3},
	{0x4024, 0x0c},
	{0x4025, 0x36},
	{0x4026, 0x0c},
	{0x4027, 0x37},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x08},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x0c},
	{0x402f, 0x02},
	{0x401f, 0x00},
	{0x4034, 0x3f},
	{0x403d, 0x04},
	{0x4300, 0xff},
	{0x4301, 0x00},
	{0x4302, 0x0f},
	{0x4316, 0x00},
	{0x4503, 0x18},
	{0x4600, 0x01},
	{0x4601, 0x97},
	{0x481f, 0x32},
	{0x4837, 0x0a},
	{0x4850, 0x10},
	{0x4851, 0x32},
	{0x4b00, 0x2a},
	{0x4b0d, 0x00},
#if 1
	{0x4d00, 0x04},
	{0x4d01, 0x44},
	{0x4d02, 0xc1},
	{0x4d03, 0x72},
	{0x4d04, 0xb2},
	{0x4d05, 0xe3},
#else
	{0x4d00, 0x04},
	{0x4d01, 0x18},
	{0x4d02, 0xc3},
	{0x4d03, 0xff},
	{0x4d04, 0xff},
	{0x4d05, 0xff},
#endif
	{0x5000, 0xfe},
	{0x5001, 0x01},
	{0x5002, 0x08},
	{0x5003, 0x20},
	{0x5046, 0x12},
	{0x5901, 0x00},
	{0x5e00, 0x00},
	{0x5e01, 0x41},
	{0x4825, 0x3a},
	{0x4826, 0x40},
	{0x4808, 0x25},
#else
		//@@ Capture setting
	//100 99 3264 2448 ; Resolution

	//{0x0100,0x00},//;
	{0x3769,0x44},// ;
	{0x376a,0x44},// ;
	{0x3808,0x0c},//
	{0x3809,0xc0},//;d0
	{0x380a,0x09},//
	{0x380b,0x90},//;a0
	{0x380c,0x07},//
	{0x380d,0x94},//
	{0x380e,0x09},//
	{0x380f,0xaa},//;60;0d
	{0x3814,0x01},//
	//{0x3821,0x40},//
	{0x3821,0x46},//
	{0x382a,0x01},//
	{0x302b,0x01},// ;
	{0x3830,0x06},//
	{0x3836,0x01},//
	{0x4000,0xf1},//
	{0x4001,0x00},//
	{0x4022,0x0c},//
	{0x4023,0x60},//
	{0x4025,0x36},//
	{0x4027,0x37},//
	{0x402a,0x04},// ;
	{0x402b,0x08},//
	{0x402e,0x04},//
	{0x402f,0x08},//
	{0x4600,0x01},//
	{0x4601,0x97},//
	{0x382d,0xff},//
	{0x5901,0x00},// ;
	//{0x0100,0x00}
#endif
};


LOCAL SENSOR_REG_TAB_INFO_T s_ov8858_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov8858_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov8858_1408X792_setting), 1408, 792, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov8858_3264X1836_setting), 3264, 1836, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8858_3264x2448_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov8858_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0, 1408, 792, 70, 408, 2366},
	//{0, 0, 3264, 1836, 89, 528, 1872},
	//{0, 0, 3264, 2448, 269, 720, 2474, {0, 0, 3264, 2448}},
	{0, 0, 3264, 2448, 134, 720, 2474, {0, 0, 3264, 2448}},//qf
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov8858_1408x792_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};
LOCAL const SENSOR_REG_T ov8858_3264X1836_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

LOCAL const SENSOR_REG_T  s_ov8858_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

LOCAL SENSOR_VIDEO_INFO_T s_ov8858_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{30, 30, 70, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8858_1408x792_video_tab},
	//{{{30, 30, 89, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)ov8858_3264X1836_video_tab},
	{{{15, 15, 162, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8858_3264x2448_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},

	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL uint32_t _ov8858_set_video_mode(uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t         i = 0x00;
	uint32_t         mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_ov8858_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov8858_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%02x", param);
	return 0;
}


LOCAL uint32_t  BU64241GWZ_set_pos(uint16_t pos){

	uint8_t cmd_val[2] ={((pos>>8)&0x03)|0xc4,pos&0xff};

	// set pos
	CMR_LOGE("BU64241GWZ_set_pos %d",pos);
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	return 0;
}

LOCAL uint32_t  BU64241GWZ_get_otp(uint16_t *inf,uint16_t *macro){

	// get otp
	//spec not specifi otp,don't know how to get inf and macro
	return 0;
}

LOCAL uint32_t  BU64241GWZ_get_motor_pos(uint16_t *pos){

	uint8_t cmd_val[2] = {0xc4, 0x00};

	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	*pos = (cmd_val[0]&0x03)<<8;
	*pos += cmd_val[1];
	CMR_LOGI("vcm pos %d",*pos);
	return 0;
}

LOCAL uint32_t  BU64241GWZ_set_motor_bestmode(){

	uint16_t A_code=80,B_code=90;
	uint8_t rf=0x0F,slew_rate=0x02,stt=0x01,str=0x02;
	uint8_t cmd_val[2];

	//set
	cmd_val[0] = 0xcc;
	cmd_val[1] = (rf<<3)|slew_rate;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0xd4|(A_code>>8);
	cmd_val[1] = A_code&0xff;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0xdc|(B_code>>8);
	cmd_val[1] = B_code&0xff;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0xe4;
	cmd_val[1] = (str<<5)|stt;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);

	CMR_LOGI("VCM mode set");
	return 0;
}

LOCAL uint32_t  BU64241GWZ_get_test_vcm_mode(){

	uint16_t A_code=80,B_code=90,C_code=0;
	uint8_t rf=0x0F,slew_rate=0x02,stt=0x01,str=0x02;
	uint8_t cmd_val[2];

	FILE* fp = NULL;
	fp = fopen("/data/misc/media/cur_vcm_info.txt","wb");

	// read
	cmd_val[0] = 0xcc;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	rf = cmd_val[1]>>3;
	slew_rate = cmd_val[1]&0x03;
	usleep(200);
	cmd_val[0] = 0xd4;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	A_code = (cmd_val[0]&0x03)<<8;
	A_code = A_code+cmd_val[1];
	usleep(200);
	cmd_val[0] = 0xdc;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	B_code = (cmd_val[0]&0x03)<<8;
	B_code = B_code+cmd_val[1];
	usleep(200);
	cmd_val[0] = 0xc4;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	C_code = (cmd_val[0]&0x03)<<8;
	C_code = C_code+cmd_val[1];
	usleep(200);
	cmd_val[0] = 0xe4;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	str = cmd_val[1]>>5;
	stt = cmd_val[1]&0x1f;
	CMR_LOGI("VCM A_code B_code rf slew_rate stt str pos,%d %d %d %d %d %d %d",A_code,B_code,rf,slew_rate,stt,str,C_code);

	fprintf(fp,"VCM A_code B_code rf slew_rate stt str pos,%d %d %d %d %d %d %d",A_code,B_code,rf,slew_rate,stt,str,C_code);
	fflush(fp);
	fclose(fp);
	return 0;
}

LOCAL uint32_t  BU64241GWZ_set_test_vcm_mode(char* vcm_mode){

	uint16_t A_code=80,B_code=90;
	uint8_t rf=0x0F,slew_rate=0x02,stt=0x01,str=0x02;
	uint8_t cmd_val[2];
	char* p1 = vcm_mode;

	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	A_code = atoi(vcm_mode);
	vcm_mode = p1;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	B_code = atoi(vcm_mode);
	vcm_mode = p1;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	rf = atoi(vcm_mode);
	vcm_mode = p1;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	slew_rate = atoi(vcm_mode);
	vcm_mode = p1;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	stt = atoi(vcm_mode);
	vcm_mode = p1;
	while( *p1!='~'  && *p1!='\0' )
		p1++;
	*p1++ = '\0';
	str = atoi(vcm_mode);
	CMR_LOGI("VCM A_code B_code rf slew_rate stt str 1nd,%d %d %d %d %d %d",A_code,B_code,rf,slew_rate,stt,str);
	//set
	cmd_val[0] = 0xcc;
	cmd_val[1] = (rf<<3)|slew_rate;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0xd4|(A_code>>8);
	cmd_val[1] = A_code&0xff;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0xdc|(B_code>>8);
	cmd_val[1] = B_code&0xff;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	usleep(200);
	cmd_val[0] = 0xe4;
	cmd_val[1] = (str<<5)|stt;
	Sensor_WriteI2C((0x18 >> 1),(uint8_t*)&cmd_val[0],2);
	// read
	cmd_val[0] = 0xcc;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	rf = cmd_val[1]>>3;
	slew_rate = cmd_val[1]&0x03;
	usleep(200);
	cmd_val[0] = 0xd4;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	A_code = (cmd_val[0]&0x03)<<8;
	A_code = A_code+cmd_val[1];
	usleep(200);
	cmd_val[0] = 0xdc;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	B_code = (cmd_val[0]&0x03)<<8;
	B_code = B_code+cmd_val[1];
	usleep(200);
	cmd_val[0] = 0xe4;
	Sensor_ReadI2C_seq((0x18 >> 1),(uint8_t*)&cmd_val[0],1,2);
	str = cmd_val[1]>>5;
	stt = cmd_val[1]&0x1f;
	CMR_LOGI("VCM A_code B_code rf slew_rate stt str 2nd,%d %d %d %d %d %d",A_code,B_code,rf,slew_rate,stt,str);
	return 0;
}


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov8858_ioctl_func_tab = {
	PNULL,
	_ov8858_PowerOn,
	PNULL,
	_ov8858_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov8858_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov8858_set_brightness,
	PNULL, // _ov8858_set_contrast,
	PNULL,
	PNULL,			//_ov8858_set_saturation,

	PNULL, //_ov8858_set_work_mode,
	PNULL, //_ov8858_set_image_effect,

	_ov8858_BeforeSnapshot,
	_ov8858_after_snapshot,
	_ov8858_flash,
	PNULL,
	_ov8858_write_exposure,
	PNULL,
	_ov8858_write_gain,
	PNULL,
	PNULL,
	_ov8858_write_af,
	PNULL,
	PNULL, //_ov8858_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov8858_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov8858_GetExifInfo,
	_ov8858_ExtFunc,
	PNULL, //_ov8858_set_anti_flicker,
	_ov8858_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov8858_StreamOn,
	_ov8858_StreamOff,
	_ov8858_access_val,//_ov8858_cfg_otp, //
	_ov8858_ex_write_exposure,
	BU64241GWZ_set_pos,// set vcm pos
	PNULL, //get_af otp
	BU64241GWZ_get_motor_pos,// get vcm pos in register
	BU64241GWZ_set_motor_bestmode,// set vcm best mode and avoid damping
	BU64241GWZ_get_test_vcm_mode,// test whether vcm mode valid in register
	BU64241GWZ_set_test_vcm_mode,// set vcm mode and test best mode for damping
};


SENSOR_INFO_T g_ov8858_mipi_raw_info = {
	ov8858_I2C_ADDR_W,	// salve i2c write address
	ov8858_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN |
	    SENSOR_IMAGE_EFFECT_BLUE |
	    SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	5,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x300A, 0x88},		// supply two code to identify sensor.
	 {0x300B, 0x58}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"ov8858",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;
	s_ov8858_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov8858_ioctl_func_tab,	// point to ioctl function table
	&s_ov8858_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov8858_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
#if defined(ov8858_2_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
#elif defined(ov8858_4_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
#endif

	s_ov8858_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov8858_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_ov8858_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct isp_mode_param* mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	int i;
	char name[100] = {'\0'};

	isp_raw_para_update_from_file(&g_ov8858_mipi_raw_info,0);

	for (i=0; i<mode_common_ptr->block_num; i++) {
		struct isp_block_header* header = &(mode_common_ptr->block_header[i]);
		uint8_t* data = (uint8_t*)mode_common_ptr + header->offset;
		switch (header->block_id)
		{
		case	ISP_BLK_PRE_WAVELET_V1: {
				/* modify block data */
				struct sensor_pwd_param* block = (struct sensor_pwd_param*)data;
				#define _NR_PWD_PARAM_
				static struct sensor_pwd_level pwd_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_PWD_PARAM_
				block->param_ptr = (isp_uint *)pwd_param;
			}
			break;

		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;
				#define _NR_BPC_PARAM_
				static struct sensor_bpc_level bpc_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_BPC_PARAM_
				block->param_ptr = (isp_uint *)bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;
				#define _NR_BDN_PARAM_
				static struct sensor_bdn_level bdn_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_BDN_PARAM_
				block->param_ptr = (isp_uint *)bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				#define _NR_GRGB_PARAM_
				static struct sensor_grgb_v1_level grgb_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_GRGB_PARAM_
				block->param_ptr = (isp_uint *)grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				#define _NR_NLM_PARAM_
				static struct sensor_nlm_level nlm_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_NLM_PARAM_

				#define _NR_VST_PARAM_
				static struct sensor_vst_level vst_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_VST_PARAM_

				#define _NR_IVST_PARAM_
				static struct sensor_ivst_level ivst_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_IVST_PARAM_

				#define _NR_FLAT_OFFSET_PARAM_
				static struct sensor_flat_offset_level flat_offset_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_FLAT_OFFSET_PARAM_

				block->param_nlm_ptr = (isp_uint *)nlm_param;
				block->param_vst_ptr = (isp_uint *)vst_param;
				block->param_ivst_ptr = (isp_uint *)ivst_param;
				block->param_flat_offset_ptr = (isp_uint *)flat_offset_param;
			}
			break;

		case	ISP_BLK_CFA_V1: {
				/* modify block data */
				struct sensor_cfa_param_v1* block = (struct sensor_cfa_param_v1*)data;
				#define _NR_CFAE_PARAM_
				static struct sensor_cfae_level cfae_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_CFAE_PARAM_
				block->param_ptr = (isp_uint *)cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;
				#define _NR_RGB_PRECDN_PARAM_
				static struct sensor_rgb_precdn_level precdn_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_RGB_PRECDN_PARAM_
				block->param_ptr = (isp_uint *)precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;
				#define _NR_YUV_PRECDN_PARAM_
				static struct sensor_yuv_precdn_level yuv_precdn_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_YUV_PRECDN_PARAM_
				block->param_ptr = (isp_uint *)yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;
				#define _NR_PRFY_PARAM_
				static struct sensor_prfy_level prfy_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_PRFY_PARAM_
				block->param_ptr = (isp_uint *)prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;
				#define _NR_YUV_CDN_PARAM_
				static struct sensor_uv_cdn_level uv_cdn_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_YUV_CDN_PARAM_
				block->param_ptr = (isp_uint *)uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {
				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				#define _NR_EDGE_PARAM_
				static struct sensor_ee_level edge_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_EDGE_PARAM_
				block->param_ptr = (isp_uint *)edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;
				#define _NR_YUV_POSTCDN_PARAM_
				static struct sensor_uv_postcdn_level uv_postcdn_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_YUV_POSTCDN_PARAM_
				block->param_ptr = (isp_uint *)uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;
				#define _NR_IIRCNR_PARAM_
				static struct sensor_iircnr_level iir_cnr_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_IIRCNR_PARAM_
				block->param_ptr = (isp_uint *)iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				#define _NR_IIR_YRANDOM_PARAM_
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_IIR_YRANDOM_PARAM_
				block->param_ptr = (isp_uint *)iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;
				#define _NR_CCE_UV_PARAM_
				static struct sensor_cce_uvdiv_level cce_uvdiv_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_CCE_UV_PARAM_
				block->param_ptr = (isp_uint *)cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;
				#define _NR_Y_AFM_PARAM_
				static struct sensor_y_afm_level y_afm_param[] = {
					#include "newparam/isp_nr.h"
				};
				#undef _NR_Y_AFM_PARAM_
				block->param_ptr = (isp_uint *)y_afm_param;
			}
			break;

		default:
			break;
		}
	}

	return rtn;
}


LOCAL unsigned long _ov8858_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%x",  (unsigned long)s_ov8858_Resolution_Trim_Tab);
	return (unsigned long) s_ov8858_Resolution_Trim_Tab;
}
LOCAL uint32_t _ov8858_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8858_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8858_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8858_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8858_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8858_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov8858_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(2*1000);

		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		Sensor_SetIovddVoltage(iovdd_val);
		Sensor_PowerDown(!power_down);
		usleep(1000);
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);
	} else {
		usleep(1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000);
		Sensor_SetResetLevel(reset_level);
		usleep(1000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov8858: _ov8858_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8858_cfg_otp(uint32_t  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8858_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_ov8858: _ov8858_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

static uint32_t _ov8858_set_sensor_close_flag()
{
	uint32_t rtn = SENSOR_SUCCESS;

	s_ov8858_sensor_close_flag = 1;

	return rtn;
}

LOCAL unsigned long _ov8858_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8858_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_OV8858: cfg_otp E");
	SENSOR_PRINT("SENSOR_OV8858:param_ptr=%x",param_ptr);
	//Sensor_WriteReg(0x103,0x01);
	usleep(1 * 1000);

	if(!param_ptr){
		if(PNULL!=tab_ptr[module_id].cfg_otp){
			tab_ptr[module_id].cfg_otp(0);
		}
		return rtn;
	}

	SENSOR_PRINT("SENSOR_OV8858: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = _ov8858_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			rtn = _ov8858_read_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
			rtn = _ov8858_set_sensor_close_flag();
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			rtn = _ov8858_read_otp((cmr_uint)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_GOLDEN_DATA:
			rtn = _ov8858_get_golden_data((cmr_uint)param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_OV8858: cfg_otp X");

	return rtn;
}

LOCAL uint32_t _ov8858_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov8858: _ov8858_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov8858_RAW_PARAM_COM;

	if(ov8858_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov8858_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8858_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=ov8858_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov8858_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_ov8858: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_ov8858: ov8858_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov8858_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_ov8858: ov8858_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL uint32_t _ov8858_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov8858_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

extern int Sensor_Set_slave_adr(uint32_t sensor_slave_adr);


LOCAL uint32_t _ov8858_Identify(uint32_t param)
{
#define ov8858_PID_VALUE_0    0x00
#define ov8858_PID_ADDR_0     0x300A
#define ov8858_PID_VALUE_1    0x88
#define ov8858_PID_ADDR_1     0x300B
#define ov8858_VER_VALUE    0x58
#define ov8858_VER_ADDR     0x300C

	uint8_t pid_value_0 = 0x01;
	uint8_t pid_value_1 = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov8858: mipi raw identify\n");

	pid_value_0 = Sensor_ReadReg(ov8858_PID_ADDR_0);
	if (ov8858_PID_VALUE_0 == pid_value_0) {
		pid_value_1 = Sensor_ReadReg(ov8858_PID_ADDR_1);
		if (ov8858_PID_VALUE_1 == pid_value_1) {
			ver_value = Sensor_ReadReg(ov8858_VER_ADDR);
			SENSOR_PRINT("SENSOR_ov8858: Identify: PID = 0x%x, VER = 0x%x", pid_value_1, ver_value);
			if (ov8858_VER_VALUE == ver_value) {
				SENSOR_PRINT("SENSOR_ov8858: this is ov8858 sensor !");
				ret_value=_ov8858_GetRawInof();
				if(SENSOR_SUCCESS != ret_value)
				{
					SENSOR_PRINT("SENSOR_ov8858: the module is unknow error !");
				}
				Sensor_ov8858_InitRawTuneInfo();
			} else {
				SENSOR_PRINT("SENSOR_ov8858: Identify this is OV%x%x sensor !", pid_value_1, ver_value);
			}
		} else {
			SENSOR_PRINT("SENSOR_ov8858: identify fail, PID_ADDR = 0x%x,  pid_value= 0x%d", ov8858_PID_ADDR_1, pid_value_1);
		}
	} else {
		SENSOR_PRINT("SENSOR_ov8858: identify fail, PID_ADDR = 0x%x, pid_value= 0x%d", ov8858_PID_ADDR_0, pid_value_0);

	}

	return ret_value;
}

static uint32_t Sexpsure_line = 0;
static unsigned long _ov8858_write_exp_dummy(uint16_t expsure_line,
								uint16_t dummy_line, uint16_t size_index)
{
	int32_t ret_value = SENSOR_SUCCESS;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;
	uint32_t linetime = 0;


	SENSOR_PRINT("SENSOR_ov8858: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	max_frame_len=_ov8858_GetMaxFrameLine(size_index);
	SENSOR_PRINT("SENSOR_ov8858: max_frame_len:0x%x", max_frame_len);
	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+dummy_line+4)> max_frame_len) ? (expsure_line+dummy_line+4) : max_frame_len;

		if(0x00!=(0x01&frame_len))
		{
			frame_len+=0x01;
		}

		frame_len_cur = (Sensor_ReadReg(0x380e)&0xff)<<8;
		frame_len_cur |= Sensor_ReadReg(0x380f)&0xff;

		if(frame_len_cur != frame_len){
			value=(frame_len)&0xff;
			ret_value = Sensor_WriteReg(0x380f, value);
			value=(frame_len>>0x08)&0xff;
			ret_value = Sensor_WriteReg(0x380e, value);

		}
	}

	value=(expsure_line<<0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3502, value);
	value=(expsure_line>>0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3501, value);
	value=(expsure_line>>0x0c)&0x0f;
	ret_value = Sensor_WriteReg(0x3500, value);

	s_capture_shutter = expsure_line;
	linetime=s_ov8858_Resolution_Trim_Tab[size_index].line_time;
	s_exposure_time = s_capture_shutter * linetime / 10;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);


	return ret_value;
}

LOCAL uint32_t _ov8858_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t expsure_line = 0x00;
	uint32_t dummy_line = 0x00;
	uint32_t size_index=0x00;


	expsure_line = param&0xffff;
	dummy_line = (param>>0x10)&0x0fff;
	size_index = (param>>0x1c)&0x0f;

	_ov8858_write_exp_dummy(expsure_line, dummy_line, size_index);

	return ret_value;
}


LOCAL uint32_t _ov8858_ex_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t size_index=0x00;
	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;

	expsure_line = ex->exposure;
	dummy_line = ex->dummy;
	size_index = ex->size_index;

	_ov8858_write_exp_dummy(expsure_line, dummy_line, size_index);

	return ret_value;
}

LOCAL uint32_t _ov8858_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t value=0x00;
	uint32_t real_gain = 0;

	//param = Sgain;
	SENSOR_PRINT("SENSOR_ov8858: write_gain:0x%x", param);
//	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
//	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);

//	value = real_gain*8;
	value = param;
	SENSOR_PRINT("SENSOR_ov8858: write_gain:0x%x,  real_gain = 0x%x", param, value);
	real_gain = value & 0xff;
	ret_value = Sensor_WriteReg(0x3509, real_gain);/*0-7*/
	real_gain = (value>>0x08)&0x07;
	ret_value = Sensor_WriteReg(0x3508, real_gain);/*8*/

	return ret_value;
}

static uint32_t BU64241GWZ_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t a_u2Data = param&0x0ffff;
	uint8_t cmd_val[2] = {(uint8_t)(((a_u2Data >> 8) & 0x03) | 0xc0), (uint8_t)(a_u2Data & 0xff)};
	uint16_t slave_addr = BU64241GWZ_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 2;

	SENSOR_PRINT("%d", param);
	SENSOR_PRINT("BU64241GWZ_write_af");
	ret_value = Sensor_WriteI2C(slave_addr, cmd_val, cmd_len);

	return ret_value;
}

static uint32_t dw9714_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = DW9714_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 2;
	uint16_t step_4bit = 0x09;

	SENSOR_PRINT("%d", param);
	SENSOR_PRINT("dw9714_write_af");
	cmd_val[0] = (param & 0xfff0) >> 4;
	cmd_val[1] = ((param & 0x0f) << 4) | step_4bit;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	return ret_value;
}

LOCAL uint32_t _ov8858_write_af(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8858: _ov8858_write_af %d",param);
#if defined(CONFIG_VCM_BU64241GWZ)
	return BU64241GWZ_write_af(param);
#else
	return dw9714_write_af(param);
#endif
}

LOCAL uint32_t _ov8858_BeforeSnapshot(uint32_t param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov8858_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov8858_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_ov8858: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov8858: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov8858: prvline equal to capline");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;

	capture_exposure = preview_exposure * prv_linetime/cap_linetime;
	//capture_exposure *= 2;

	if(0 == capture_exposure){
		capture_exposure = 1;
	}

	if(capture_exposure > (capture_maxline - 4)){
		capture_maxline = capture_exposure + 4;
		ret_l = (unsigned char)(capture_maxline&0x0ff);
		ret_h = (unsigned char)((capture_maxline >> 8)&0xff);
		Sensor_WriteReg(0x380e, ret_h);
		Sensor_WriteReg(0x380f, ret_l);
	}
	ret_l = ((unsigned char)capture_exposure&0xf) << 4;
	ret_m = (unsigned char)((capture_exposure&0xfff) >> 4) & 0xff;
	ret_h = (unsigned char)(capture_exposure >> 12);

	Sensor_WriteReg(0x3502, ret_l);
	Sensor_WriteReg(0x3501, ret_m);
	Sensor_WriteReg(0x3500, ret_h);

	CFG_INFO:
	s_capture_shutter = _ov8858_get_shutter();
	s_capture_VTS = _ov8858_get_VTS();
	_ov8858_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8858_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8858: after_snapshot mode:%d", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8858_GetExifInfo(unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long) & sexif;
}


LOCAL uint32_t _ov8858_flash(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8858: param=%d", param);

	/* enable flash, disable in _ov8858_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8858_StreamOn(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8858: StreamOn");

	_ov8858_test_pattern();
	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL uint32_t _ov8858_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8858: StreamOff");
	unsigned char value;
	unsigned int sleep_time = 0;

	value = Sensor_ReadReg(0x0100);
	if (value == 0x01) {
		Sensor_WriteReg(0x0100, 0x00);
		if (!s_ov8858_sensor_close_flag) {
			sleep_time = 100*1000;
			usleep(sleep_time);
		}
	} else {
		Sensor_WriteReg(0x0100, 0x00);
	}

	s_ov8858_sensor_close_flag = 0;

	SENSOR_PRINT("X sleep_time=%dus", sleep_time);

	return 0;
}

int _ov8858_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

int _ov8858_set_shutter(int shutter)
{
	// write shutter, in number of line period
	int temp;

	shutter = shutter & 0xffff;

	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);

	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);

	temp = shutter>>12;
	Sensor_WriteReg(0x3500, temp);

	return 0;
}

int _ov8858_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x350b);

	return gain16;
}

int _ov8858_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x350b, temp);

	temp = gain16>>8;
	Sensor_WriteReg(0x350a, temp);

	return 0;
}

static void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_ov8858_set_gain16(capture_gain16);

	// write capture shutter
	if (capture_shutter > (capture_VTS - 10)) {
		capture_VTS = capture_shutter + 10;
		_ov8858_set_VTS(capture_VTS);
	}
	_ov8858_set_shutter(capture_shutter);
}

static uint32_t _ov8858_SetEV(unsigned long  param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_ov8858_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_ov8858: _ov8858_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_ov8858_gain,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov8858_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov8858_gain,s_capture_VTS,s_capture_shutter *2);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL uint32_t _ov8858_ExtFunc(unsigned long  ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT_HIGH("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _ov8858_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL int _ov8858_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte
	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);
	return VTS;
}

LOCAL int _ov8858_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);
	temp = VTS>>8;
	Sensor_WriteReg(0x380e, temp);
	return 0;
}
LOCAL uint32_t _ov8858_ReadGain(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	s_ov8858_gain=(int)gain;

	SENSOR_PRINT("SENSOR_ov8858: _ov8858_ReadGain gain: 0x%x", s_ov8858_gain);

	return rtn;
}

LOCAL uint32_t _ov8858_dw9714_SRCInit(uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	int i = 0;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	SENSOR_PRINT(" _ov8858_dw9714_SRCInit: mode = %d\n", mode);
	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xa1;
			cmd_val[1] = 0x0e;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x90;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}

LOCAL uint32_t _ov8858_read_otp_gain(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x3509);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x3508);/*8*/
	gain |= (value<<0x08)&0x700;

	*param = gain;

	return rtn;
}

LOCAL uint32_t _ov8858_test_pattern(void)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t pattern,gr,r,gb,b;
	int8_t i = 0;
	uint32_t solid_rgb;
	char	value[100];
	#define BASEGAIN 0x80

	property_get("debug.camera.test.pattern", value, "0");
	pattern = atoi(value);

	property_get("debug.camera.test.solid.rgb", value, "0");
	solid_rgb= atoi(value);

	property_get("debug.camera.test.pattern.gr", value, "0");
	gr = atoi(value);

	property_get("debug.camera.test.pattern.r", value, "0");
	r = atoi(value);

	property_get("debug.camera.test.pattern.b", value, "0");
	b = atoi(value);

	property_get("debug.camera.test.pattern.gb", value, "0");
	gb = atoi(value);

	SENSOR_PRINT(":_ov8858_test_pattern:pattern:%d,gr:0x%x,r:0x%x,b:0x%x,gb:0x%x\n", pattern,gr,r,b,gb);

	if(pattern){
		SENSOR_PRINT(":_ov8858_test_pattern:pattern:%d\n", pattern);
		Sensor_WriteReg(0x5e00,0x83);//enable test pattern and Black image mode
	}
	if(solid_rgb)
	{
		SENSOR_PRINT("_ov8858_test_pattern5: 0x%x", (7<<pattern) | 0x03);
		Sensor_WriteReg(0x4320,Sensor_ReadReg(0x4320)|1<<1);/*solid_color_en*/

		Sensor_WriteReg(0x4322,b>>8 & 0x03);	/*write solid b high byte*/
		Sensor_WriteReg(0x4323,b & 0xff);		/*write solid b low byte*/

		Sensor_WriteReg(0x4324,gb>>8 & 0x03);	/*write solid gb high byte*/
		Sensor_WriteReg(0x4325,gb & 0xff);		/*write solid gb low byte*/

		Sensor_WriteReg(0x4326,r>>8 & 0x03);	/*write solid r high byte*/
		Sensor_WriteReg(0x4327,r & 0xff);		/*write solid r low byte*/

		Sensor_WriteReg(0x4328,gr>>8 & 0x03);	/*write solid gr high byte*/
		Sensor_WriteReg(0x4329,gr & 0xff);		/*write solid gr low byte*/
	}
ret:
	return rtn;
}

LOCAL uint32_t _ov8858_read_otp(unsigned long param)
{
	uint32_t rtn                  = SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;
	uint32_t start_addr           = 0;
	uint32_t len                  = 0;
	uint8_t *buff                 = NULL;
	uint16_t tmp;

	if (NULL == param_ptr) {
		return -1;
	}

	CMR_LOGI("SENSOR_OV8858: _ov8858_read_otp E");


	if (SENSOR_OTP_PARAM_NORMAL == param_ptr->type) {
		//start_addr           = param_ptr->start_addr;
		param_ptr->len = 8192;
	} else if (SENSOR_OTP_PARAM_CHECKSUM == param_ptr->type) {
		//param_ptr->start_addr = (bank_select)? ov8858_OTP_LSC_CHECKSUM_ADDR_2:hi544_OTP_LSC_CHECKSUM_ADDR;
		param_ptr->len = 4;
	} else if (SENSOR_OTP_PARAM_READBYTE == param_ptr->type) {
	//start_addr           = param_ptr->start_addr;
	//len                  = param_ptr->len;
	} else if (SENSOR_OTP_PARAM_FW_VERSION == param_ptr->type) {
		//param_ptr->start_addr = (bank_select)? ov8858_OTP_MODULE_INFO_ADDR_2:hi544_OTP_MODULE_INFO_ADDR;
		param_ptr->len = 11;
	}

	//_ov8858_read_otp((unsigned long) param_ptr);

	if (NULL != param_ptr->awb.data_ptr && SENSOR_OTP_PARAM_NORMAL == param_ptr->type) {
		uint32_t real_size = 0;
		rtn = awb_package_convert(&otp_info.r_current, param_ptr->awb.data_ptr,param_ptr->awb.size, &real_size);
		param_ptr->awb.size = real_size;
		CMR_LOGI("SENSOR_ov8858: awb real_size %d", real_size);
		rtn = lsc_package_convert(&otp_info.lsc_param, param_ptr->lsc.data_ptr,param_ptr->lsc.size, &real_size);
		param_ptr->lsc.size = real_size;
		CMR_LOGI("SENSOR_ov8858: lsc real_size %d", real_size);
	}
	CMR_LOGI("SENSOR_ov8858: _ov8858_read_otp X");

	return rtn;
}

LOCAL uint32_t _ov8858_get_golden_data(unsigned long param){
	uint32_t rtn  = SENSOR_SUCCESS;
	rtn = construct_golden_data(param);
	return rtn;
}

