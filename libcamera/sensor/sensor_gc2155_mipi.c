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

#define GC2155_MIPI_I2C_ADDR_W		0x3c
#define GC2155_MIPI_I2C_ADDR_R		0x3c

#define SENSOR_GAIN_SCALE		16

static uint32_t shutter_flag = 0;

#define GC2155_MIPI_2Lane

static unsigned long set_preview_mode(unsigned long preview_mode);
static unsigned long GC2155_MIPI_PowerOn(unsigned long power_on);
static unsigned long GC2155_MIPI_Identify(unsigned long param);
static unsigned long Gc2155_GetResolutionTrimTab(unsigned long param);
static unsigned long GC2155_MIPI_BeforeSnapshot(unsigned long param);
static unsigned long GC2155_MIPI_ExtFunc(unsigned long ctl_param);
static unsigned long GC2155_MIPI_After_Snapshot(unsigned long param);
static unsigned long set_brightness(unsigned long level);
static unsigned long set_contrast(unsigned long level);
static unsigned long set_saturation(unsigned long level);
static unsigned long set_image_effect(unsigned long effect_type);
static unsigned long read_gain_scale(unsigned long value);
static unsigned long set_GC2155_MIPI_ev(unsigned long level);
static unsigned long set_GC2155_MIPI_awb(unsigned long mode);
static unsigned long set_GC2155_MIPI_anti_flicker(unsigned long mode);
static unsigned long set_GC2155_MIPI_video_mode(unsigned long mode);
static void GC2155_MIPI_set_shutter();
static unsigned long GC2155_MIPI_StreamOn(unsigned long param);
static unsigned long GC2155_MIPI_StreamOff(unsigned long param);
static unsigned long GC2155_close_flag(unsigned long param);

 typedef enum
{
	FLICKER_50HZ = 0,
	FLICKER_60HZ,
	FLICKER_MAX
}FLICKER_E;

static uint32_t s_GC2155_sensor_close_flag = 0;

SENSOR_REG_T GC2155_MIPI_YUV_COMMON[]=
{
	{0xfe, 0xf0},
	{0xfe, 0xf0},
	{0xfe, 0xf0},
	{0xfc, 0x06},
	{0xf6, 0x00},
	{0xf7, 0x1d},
	{0xf8, 0x84},
	{0xfa, 0x00},
	{0xf9, 0x8e},
	{0xf2, 0x00},

	/*ISP reg*/
	{0xfe , 0x00},
	{0x03 , 0x04},
	{0x04 , 0xe2},
	{0x09 , 0x00},
	{0x0a , 0x00},
	{0x0b , 0x00},
	{0x0c , 0x00},
	{0x0d , 0x04},
	{0x0e , 0xc0},
	{0x0f , 0x06},
	{0x10 , 0x50},
	{0x12 , 0x2e},
	{0x17 , 0x14},
	{0x18 , 0x02},
	{0x19 , 0x0e},
	{0x1a , 0x01},
	{0x1b , 0x4b},
	{0x1c , 0x07},
	{0x1d , 0x10},
	{0x1e , 0x98},
	{0x1f , 0x78},
	{0x20 , 0x05},
	{0x21 , 0x40},
	{0x22 , 0xf0},
	{0x24 , 0x16},
	{0x25 , 0x01},
	{0x26 , 0x10},
	{0x2d , 0x40},
	{0x30 , 0x01},
	{0x31 , 0x90},
	{0x33 , 0x04},
	{0x34 , 0x01},

	/*ISP reg*/
	{0xfe , 0x00},
	{0x80 , 0xff},
	{0x81 , 0x2c},
	{0x82 , 0xfa},
	{0x83 , 0x00},
	{0x84 , 0x03}, //01
	{0x85 , 0x0c}, //porter 08
	{0x86 , 0x02},
	{0x89 , 0x03},
	{0x8a , 0x00},
	{0x8b , 0x00},
	{0xb0 , 0x55},
	{0xc3 , 0x11}, //00
	{0xc4 , 0x20},
	{0xc5 , 0x30},
	{0xc6 , 0x38},
	{0xc7 , 0x40},
	{0xec , 0x02},
	{0xed , 0x04},
	{0xee , 0x60},
	{0xef , 0x90},
	{0xb6 , 0x01},
	{0x90 , 0x01},
	{0x91 , 0x00},
	{0x92 , 0x00},
	{0x93 , 0x00},
	{0x94 , 0x00},
	{0x95 , 0x04},
	{0x96 , 0xb0},
	{0x97 , 0x06},
	{0x98 , 0x40},

	/*0xBLK*/
	{0xfe , 0x00},
	{0x18 , 0x02},
	{0x40 , 0x42},
	{0x41 , 0x00},
	{0x43 , 0x54},
	{0x5e , 0x00},
	{0x5f , 0x00},
	{0x60 , 0x00},
	{0x61 , 0x00},
	{0x62 , 0x00},
	{0x63 , 0x00},
	{0x64 , 0x00},
	{0x65 , 0x00},
	{0x66 , 0x20},
	{0x67 , 0x20},
	{0x68 , 0x20},
	{0x69 , 0x20},
	{0x6a , 0x08},
	{0x6b , 0x08},
	{0x6c , 0x08},
	{0x6d , 0x08},
	{0x6e , 0x08},
	{0x6f , 0x08},
	{0x70 , 0x08},
	{0x71 , 0x08},
	{0x72 , 0xf0},
	{0x7e , 0x3c},
	{0x7f , 0x00},
	{0xfe , 0x00},

	/*AEC*/
	{0xfe , 0x01},
	{0x01 , 0x08},
	{0x02 , 0xc0},
	{0x03 , 0x04},
	{0x04 , 0x90},
	{0x05 , 0x30},
	{0x06 , 0x98},
	{0x07 , 0x28},
	{0x08 , 0x6c},
	{0x09 , 0x00},
	{0x0a , 0xc2},
	{0x0b , 0x11},
	{0x0c , 0x10},
	{0x13 , 0x2d},
	{0x17 , 0x00},
	{0x1c , 0x11},
	{0x1e , 0x61},
	{0x1f , 0x30},
	{0x20 , 0x40},
	{0x22 , 0x80},
	{0x23 , 0x20},

	{0x12 , 0x35},
	{0x15 , 0x50},
	{0x10 , 0x31},
	{0x3e , 0x28},
	{0x3f , 0xe0},
	{0x40 , 0xe0},
	{0x41 , 0x08},

	{0xfe , 0x02},
	{0x0f , 0x05},

	/*INTPEE*/
	{0xfe , 0x02},
	{0x90 , 0x6c},
	{0x91 , 0x03},
	{0x92 , 0xc4},
	{0x97 , 0x64},
	{0x98 , 0x88},
	{0x9d , 0x08},
	{0xa2 , 0x11},
	{0xfe , 0x00},

	/*DNDD*/
	{0xfe , 0x02},
	{0x80 , 0xc1},
	{0x81 , 0x08},
	{0x82 , 0x05},
	{0x83 , 0x04},
	{0x84 , 0x0a},
	{0x86 , 0x80},
	{0x87 , 0x30},
	{0x88 , 0x15},
	{0x89 , 0x80},
	{0x8a , 0x60},
	{0x8b , 0x30},

	/*ASDE*/
	{0xfe , 0x01},
	{0x21 , 0x14},
	{0xfe , 0x02},
	{0x3c , 0x06},
	{0x3d , 0x40},
	{0x48 , 0x30},
	{0x49 , 0x06},
	{0x4b , 0x08},
	{0x4c , 0x20},
	{0xa3 , 0x50},
	{0xa4 , 0x30},
	{0xa5 , 0x40},
	{0xa6 , 0x80},
	{0xab , 0x40},
	{0xae , 0x0c},
	{0xb3 , 0x42},
	{0xb4 , 0x24},
	{0xb6 , 0x50},
	{0xb7 , 0x01},
	{0xb9 , 0x28},
	{0xfe , 0x00},

	/*gamma1*/
	{0xfe , 0x02},
	{0x10 , 0x0d},
	{0x11 , 0x12},
	{0x12 , 0x17},
	{0x13 , 0x1c},
	{0x14 , 0x27},
	{0x15 , 0x34},
	{0x16 , 0x44},
	{0x17 , 0x55},
	{0x18 , 0x6e},
	{0x19 , 0x81},
	{0x1a , 0x91},
	{0x1b , 0x9c},
	{0x1c , 0xaa},
	{0x1d , 0xbb},
	{0x1e , 0xca},
	{0x1f , 0xd5},
	{0x20 , 0xe0},
	{0x21 , 0xe7},
	{0x22 , 0xed},
	{0x23 , 0xf6},
	{0x24 , 0xfb},
	{0x25 , 0xff},

	/*gamma2*/
	{0xfe , 0x02},
	{0x26 , 0x0d},
	{0x27 , 0x12},
	{0x28 , 0x17},
	{0x29 , 0x1c},
	{0x2a , 0x27},
	{0x2b , 0x34},
	{0x2c , 0x44},
	{0x2d , 0x55},
	{0x2e , 0x6e},
	{0x2f , 0x81},
	{0x30 , 0x91},
	{0x31 , 0x9c},
	{0x32 , 0xaa},
	{0x33 , 0xbb},
	{0x34 , 0xca},
	{0x35 , 0xd5},
	{0x36 , 0xe0},
	{0x37 , 0xe7},
	{0x38 , 0xed},
	{0x39 , 0xf6},
	{0x3a , 0xfb},
	{0x3b , 0xff},

	/*YCP*/
	{0xfe , 0x02},
	{0xd1 , 0x28},
	{0xd2 , 0x28},
	{0xdd , 0x14},
	{0xde , 0x88},
	{0xed , 0x80},

	/*LSC*/
	{0xfe , 0x01},
	{0xc2 , 0x1f},
	{0xc3 , 0x13},
	{0xc4 , 0x0e},
	{0xc8 , 0x16},
	{0xc9 , 0x0f},
	{0xca , 0x0c},
	{0xbc , 0x52},
	{0xbd , 0x2c},
	{0xbe , 0x27},
	{0xb6 , 0x47},
	{0xb7 , 0x32},
	{0xb8 , 0x30},
	{0xc5 , 0x00},
	{0xc6 , 0x00},
	{0xc7 , 0x00},
	{0xcb , 0x00},
	{0xcc , 0x00},
	{0xcd , 0x00},
	{0xbf , 0x0e},
	{0xc0 , 0x00},
	{0xc1 , 0x00},
	{0xb9 , 0x08},
	{0xba , 0x00},
	{0xbb , 0x00},
	{0xaa , 0x0a},
	{0xab , 0x0c},
	{0xac , 0x0d},
	{0xad , 0x02},
	{0xae , 0x06},
	{0xaf , 0x05},
	{0xb0 , 0x00},
	{0xb1 , 0x05},
	{0xb2 , 0x02},
	{0xb3 , 0x04},
	{0xb4 , 0x04},
	{0xb5 , 0x05},
	{0xd0 , 0x00},
	{0xd1 , 0x00},
	{0xd2 , 0x00},
	{0xd6 , 0x02},
	{0xd7 , 0x00},
	{0xd8 , 0x00},
	{0xd9 , 0x00},
	{0xda , 0x00},
	{0xdb , 0x00},
	{0xd3 , 0x00},
	{0xd4 , 0x00},
	{0xd5 , 0x00},
	{0xa4 , 0x04},
	{0xa5 , 0x00},
	{0xa6 , 0x77},
	{0xa7 , 0x77},
	{0xa8 , 0x77},
	{0xa9 , 0x77},
	{0xa1 , 0x80},
	{0xa2 , 0x80},

	{0xfe , 0x01},
	{0xdc , 0x35},
	{0xdd , 0x28},
	{0xdf , 0x0d},
	{0xe0 , 0x70},
	{0xe1 , 0x78},
	{0xe2 , 0x70},
	{0xe3 , 0x78},
	{0xe6 , 0x90},
	{0xe7 , 0x70},
	{0xe8 , 0x90},
	{0xe9 , 0x70},
	{0xfe , 0x00},

	/*AWB*/
	{0xfe , 0x01},
	{0x4f , 0x00},
	{0x4f , 0x00},
	{0x4b , 0x01},
	{0x4f , 0x00},
	{0x4c , 0x01},
	{0x4d , 0x71},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x91},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x50},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x70},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x90},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0xb0},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0xd0},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x4f},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x6f},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x8f},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0xaf},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0xcf},
	{0x4e , 0x02},
	{0x4c , 0x01},
	{0x4d , 0x6e},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x8e},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xae},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xce},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x4d},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x6d},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x8d},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xad},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xcd},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x4c},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x6c},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x8c},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xac},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xcc},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xec},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x4b},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x6b},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x8b},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0xab},
	{0x4e , 0x03},
	{0x4c , 0x01},
	{0x4d , 0x8a},
	{0x4e , 0x04},
	{0x4c , 0x01},
	{0x4d , 0xaa},
	{0x4e , 0x04},
	{0x4c , 0x01},
	{0x4d , 0xca},
	{0x4e , 0x04},
	{0x4c , 0x01},
	{0x4d , 0xa9},
	{0x4e , 0x04},
	{0x4c , 0x01},
	{0x4d , 0xc9},
	{0x4e , 0x04},
	{0x4c , 0x01},
	{0x4d , 0xcb},
	{0x4e , 0x05},
	{0x4c , 0x01},
	{0x4d , 0xeb},
	{0x4e , 0x05},
	{0x4c , 0x02},
	{0x4d , 0x0b},
	{0x4e , 0x05},
	{0x4c , 0x02},
	{0x4d , 0x2b},
	{0x4e , 0x05},
	{0x4c , 0x02},
	{0x4d , 0x4b},
	{0x4e , 0x05},
	{0x4c , 0x01},
	{0x4d , 0xea},
	{0x4e , 0x05},
	{0x4c , 0x02},
	{0x4d , 0x0a},
	{0x4e , 0x05},
	{0x4c , 0x02},
	{0x4d , 0x2a},
	{0x4e , 0x05},
	{0x4c , 0x02},
	{0x4d , 0x6a},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0x29},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0x49},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0x69},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0x89},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0xa9},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0xc9},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0x48},
	{0x4e , 0x06},
	{0x4c , 0x02},
	{0x4d , 0x68},
	{0x4e , 0x06},
	{0x4c , 0x03},
	{0x4d , 0x09},
	{0x4e , 0x07},
	{0x4c , 0x02},
	{0x4d , 0xa8},
	{0x4e , 0x07},
	{0x4c , 0x02},
	{0x4d , 0xc8},
	{0x4e , 0x07},
	{0x4c , 0x02},
	{0x4d , 0xe8},
	{0x4e , 0x07},
	{0x4c , 0x03},
	{0x4d , 0x08},
	{0x4e , 0x07},
	{0x4c , 0x03},
	{0x4d , 0x28},
	{0x4e , 0x07},
	{0x4c , 0x02},
	{0x4d , 0x87},
	{0x4e , 0x07},
	{0x4c , 0x02},
	{0x4d , 0xa7},
	{0x4e , 0x07},
	{0x4c , 0x02},
	{0x4d , 0xc7},
	{0x4e , 0x07},
	{0x4c , 0x02},
	{0x4d , 0xe7},
	{0x4e , 0x07},
	{0x4c , 0x03},
	{0x4d , 0x07},
	{0x4e , 0x07},
	{0x4f , 0x01},
	{0xfe , 0x01},

	{0x50 , 0x80},
	{0x51 , 0xa8},
	{0x52 , 0x57},
	{0x53 , 0x38},
	{0x54 , 0xc7},
	{0x56 , 0x0e},
	{0x58 , 0x08},
	{0x5b , 0x00},
	{0x5c , 0x74},
	{0x5d , 0x8b},
	{0x61 , 0xd3},
	{0x62 , 0x90},
	{0x63 , 0xaa},
	{0x65 , 0x04},
	{0x67 , 0xb2},
	{0x68 , 0xac},
	{0x69 , 0x00},
	{0x6a , 0xb2},
	{0x6b , 0xac},
	{0x6c , 0xdc},
	{0x6d , 0xb0},
	{0x6e , 0x30},
	{0x6f , 0x40},
	{0x70 , 0x05},
	{0x71 , 0x80},
	{0x72 , 0x80},
	{0x73 , 0x30},
	{0x74 , 0x01},
	{0x75 , 0x01},
	{0x7f , 0x02},//porter 08
	{0x76 , 0x70},
	{0x77 , 0x48},
	{0x78 , 0xa0},
	{0xfe , 0x00},

	/*CC*/
	{0xfe , 0x02},
	{0xc0 , 0x01},
	{0xc1 , 0x4a},
	{0xc2 , 0xf3},
	{0xc3 , 0xfc},
	{0xc4 , 0xe4},
	{0xc5 , 0x48},
	{0xc6 , 0xec},
	{0xc7 , 0x45},
	{0xc8 , 0xf8},
	{0xc9 , 0x02},
	{0xca , 0xfe},
	{0xcb , 0x42},
	{0xcc , 0x00},
	{0xcd , 0x45},
	{0xce , 0xf0},
	{0xcf , 0x00},
	{0xe3 , 0xf0},
	{0xe4 , 0x45},
	{0xe5 , 0xe8},

	/*ABS*/
	{0xfe , 0x01},
	{0x9f , 0x42},
	{0xfe , 0x00},

	/*OUTPUT*/
	{0xfe, 0x00},
	{0xf2, 0x00},

	/*frame rate 50Hz*/
	{0xfe , 0x00},
	{0x05 , 0x01},
	{0x06 , 0x56},
	{0x07 , 0x00},
	{0x08 , 0x32},
	{0xfe , 0x01},
	{0x25 , 0x00},
	{0x26 , 0xfa},
	{0x27 , 0x04},
	{0x28 , 0xe2}, //20fps
	{0x29 , 0x06},
	{0x2a , 0xd6}, //16fps
	{0x2b , 0x07},
	{0x2c , 0xd0}, //12fps
	{0x2d , 0x0b},
	{0x2e , 0xb8}, //8fps
	{0xfe , 0x00},

	/*MIPI*/
	{0xfe, 0x03},
	{0x01, 0x87},
	{0x02, 0x11}, //modify by gc 22->11->00
	{0x03, 0x11}, //modify by gc 12->11->10
	{0x04, 0x01},
	{0x05, 0x00},
	{0x06, 0x88},
	{0x11, 0x1e},
	{0x12, 0x80},
	{0x13, 0x0c},
	{0x15, 0x10},
	{0x17, 0xf0},

	{0x21, 0x03}, //modify by gc 01->03
	{0x22, 0x01}, //modify by gc 02->01
	{0x23, 0x04}, //modify by gc 01->04
	{0x24, 0x10},
	{0x25, 0x10}, //add by gc
	{0x26, 0x03}, //add by gc
	{0x29, 0x01}, //modify by gc
	{0x2a, 0x03},
	{0x2b, 0x03}, //add by gc
	{0xfe, 0x00},
};

SENSOR_REG_T GC2155_MIPI_YUV_800x600[]=
{
	{0xfe, 0x03},
#ifdef GC2155_MIPI_2Lane
	{0x10, 0x85},
#else
	{0x10, 0x84},
#endif
	{0xfe, 0x00},
	{0xf7, 0x35},
	//{0xf8, 0x84},
	{0xfd, 0x01},

	/*crop window*/
	{0xfe , 0x00},
	{0x99 , 0x11},
	{0x9a , 0x06},
	{0x9b , 0x00},
	{0x9c , 0x00},
	{0x9d , 0x00},
	{0x9e , 0x00},
	{0x9f , 0x00},
	{0xa0 , 0x00},
	{0xa1 , 0x00},
	{0xa2  ,0x00},
	{0x90 , 0x01},
	{0x91 , 0x00},
	{0x92 , 0x00},
	{0x93 , 0x00},
	{0x94 , 0x00},
	{0x95 , 0x02},
	{0x96 , 0x58},
	{0x97 , 0x03},
	{0x98 , 0x20},

	/*AWB*/
	{0xfe , 0x00},
	{0xec , 0x01},
	{0xed , 0x02},
	{0xee , 0x30},
	{0xef , 0x48},
	{0xfe , 0x01},
	{0x74 , 0x00},

	/*AEC*/
	{0xfe , 0x01},
	{0x01 , 0x04},
	{0x02 , 0x60},
	{0x03 , 0x02},
	{0x04 , 0x48},
	{0x05 , 0x18},
	{0x06 , 0x4c},
	{0x07 , 0x14},
	{0x08 , 0x36},
	{0x0a , 0xc0},
	{0x21 , 0x14},
	{0xfe , 0x00},

	/*gamma*/
	{0xfe , 0x00},
	{0xc3 , 0x11},
	{0xc4 , 0x20},
	{0xc5 , 0x30},
	{0xfe , 0x00},

	/*mipi*/
	{0xfe , 0x03},
	{0x12 , 0x40},
	{0x13 , 0x06},
#ifdef GC2155_MIPI_2Lane
	{0x04 , 0x90},
	{0x05 , 0x01},
#else
	{0x04 , 0x01},
	{0x05 , 0x00},
#endif
	{0xfe , 0x00},
};



SENSOR_REG_T GC2155_MIPI_YUV_1280x960[]=
{
	{0xfe, 0x03},
#ifdef GC2155_MIPI_2Lane
	{0x10, 0x85},// output disable
#else
	{0x10, 0x84},
#endif
	{0xfe, 0x00},
	{0xf7, 0x1d},
	//{0xf8, 0x84},
	{0xfd, 0x00},

	/*crop window*/
	{0xfe , 0x00},
	{0x99 , 0x55},
	{0x9a , 0x06},
	{0x9b , 0x00},
	{0x9c , 0x00},
	{0x9d , 0x01},
	{0x9e , 0x23},
	{0x9f , 0x00},
	{0xa0 , 0x00},
	{0xa1 , 0x01},
	{0xa2 , 0x23},
	{0x90 , 0x01},
	{0x91 , 0x00},
	{0x92 , 0x00},
	{0x93 , 0x00},
	{0x94 , 0x00},
	{0x95 , 0x03},
	{0x96 , 0xc0},
	{0x97 , 0x05},
	{0x98 , 0x00},

	/*AWB*/
	{0xfe , 0x00},
	{0xec , 0x02},
	{0xed , 0x04},
	{0xee, 0x60},
	{0xef , 0x90},
	{0xfe , 0x01},
	{0x74 , 0x01},

	/*AEC*/
	{0xfe , 0x01},
	{0x01 , 0x08},
	{0x02 , 0xc0},
	{0x03 , 0x04},
	{0x04 , 0x90},
	{0x05 , 0x30},
	{0x06 , 0x98},
	{0x07 , 0x28},
	{0x08 , 0x6c},
	{0x0a , 0xc2},
	{0x21 , 0x14},

	/*gamma*/
	{0xfe , 0x00},
	{0xc3 , 0x11},
	{0xc4 , 0x20},
	{0xc5 , 0x30},
	{0xfe , 0x00},

	/*mipi*/
	{0xfe , 0x03},
	{0x12 , 0x00},
	{0x13 , 0x0a},
	{0x04 , 0x40},
	{0x05 , 0x01},
	{0xfe , 0x00},
};

SENSOR_REG_T GC2155_MIPI_YUV_1600x1200[]=
{
	{0xfe, 0x03},
#ifdef GC2155_MIPI_2Lane
	{0x10, 0x85},// output disable
#else
	{0x10, 0x84},
#endif
	{0xfe, 0x00},
	{0xf7, 0x1d},
	//{0xf8, 0x84},
	{0xfd, 0x00},

	/*crop window*/
	{0xfe , 0x00},
	{0x99 , 0x11},
	{0x9a , 0x06},
	{0x9b , 0x00},
	{0x9c , 0x00},
	{0x9d , 0x00},
	{0x9e , 0x00},
	{0x9f , 0x00},
	{0xa0 , 0x00},
	{0xa1 , 0x00},
	{0xa2  ,0x00},
	{0x90 , 0x01},
	{0x91 , 0x00},
	{0x92 , 0x00},
	{0x93 , 0x00},
	{0x94 , 0x00},
	{0x95 , 0x04},
	{0x96 , 0xb0},
	{0x97 , 0x06},
	{0x98 , 0x40},

	/*AWB*/
	{0xfe , 0x00},
	{0xec , 0x02},
	{0xed , 0x04},
	{0xee , 0x60},
	{0xef , 0x90},
	{0xfe , 0x01},
	{0x74 , 0x01},

	/*AEC*/
	{0xfe , 0x01},
	{0x01 , 0x08},
	{0x02 , 0xc0},
	{0x03 , 0x04},
	{0x04 , 0x90},
	{0x05 , 0x30},
	{0x06 , 0x98},
	{0x07 , 0x28},
	{0x08 , 0x6c},
	{0x0a , 0xc2},
	{0x21 , 0x14},

	/*gamma*/
	{0xfe , 0x00},
	{0xc3 , 0x11},
	{0xc4 , 0x20},
	{0xc5 , 0x30},
	{0xfe , 0x00},

	/*mipi*/
	{0xfe , 0x03},
	{0x12 , 0x80},
	{0x13 , 0x0c},
	{0x04 , 0x01},
	{0x05 , 0x00},

	{0xfe , 0x00},
};

static SENSOR_REG_TAB_INFO_T s_GC2155_MIPI_resolution_Tab_YUV[]=
{
	{ADDR_AND_LEN_OF_ARRAY(GC2155_MIPI_YUV_COMMON), 0, 0, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//{ADDR_AND_LEN_OF_ARRAY(GC2155_MIPI_YUV_800x600), 800, 600, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(GC2155_MIPI_YUV_1280x960), 1280, 960, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(GC2155_MIPI_YUV_1600x1200), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

static SENSOR_TRIM_T s_Gc2155_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0,0, {0, 0, 0, 0}},
	//{0, 0, 800, 600, 68, 490, 0x03b8, {0, 0, 800, 600}},
	{0, 0, 1280, 960, 68, 496, 0x03b8, {0, 0, 1280, 960}},
	{0, 0, 1600, 1200, 68, 496, 0x03b8, {0, 0, 1600, 1200}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

static SENSOR_VIDEO_INFO_T s_gc2155_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{0, 0, 0, 0}, {30, 30, 68, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {30, 30, 68, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {30, 30, 68, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static SENSOR_IOCTL_FUNC_TAB_T s_GC2155_MIPI_ioctl_func_tab =
{
	// Internal
	PNULL,
	GC2155_MIPI_PowerOn,
	PNULL,
	GC2155_MIPI_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	Gc2155_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	set_brightness,
	set_contrast,
	PNULL,			//set_sharpness
	set_saturation,			//set_saturation

	set_preview_mode,
	set_image_effect,

	GC2155_MIPI_BeforeSnapshot,
	GC2155_MIPI_After_Snapshot,

	PNULL,

	PNULL,			//read_ev_value
	PNULL,			//write_ev_value
	PNULL,			//read_gain_value
	PNULL,			//write_gain_value
	PNULL,			//read_gain_scale
	PNULL,			//set_frame_rate
	PNULL,
	PNULL,
	set_GC2155_MIPI_awb,
	PNULL,
	PNULL,
	set_GC2155_MIPI_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	GC2155_MIPI_ExtFunc,//PNULL,///
	set_GC2155_MIPI_anti_flicker,
	set_GC2155_MIPI_video_mode,
	PNULL,
	PNULL,
	PNULL,			//get_status
	GC2155_MIPI_StreamOn,
	GC2155_MIPI_StreamOff,
	PNULL,
	PNULL,
	GC2155_close_flag,
};

SENSOR_INFO_T g_GC2155_MIPI_yuv_info =
{
	GC2155_MIPI_I2C_ADDR_W,				// salve i2c write address
	GC2155_MIPI_I2C_ADDR_R, 				// salve i2c read address

	SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
							// bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
							// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,			// bit0: 0:negative; 1:positive -> polarily of pixel clock
							// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
							// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
							// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL|\
	SENSOR_IMAGE_EFFECT_BLACKWHITE|\
	SENSOR_IMAGE_EFFECT_RED|\
	SENSOR_IMAGE_EFFECT_GREEN|\
	SENSOR_IMAGE_EFFECT_BLUE|\
	SENSOR_IMAGE_EFFECT_YELLOW|\
	SENSOR_IMAGE_EFFECT_NEGATIVE|\
	SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,						// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
							// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,			// reset pulse level
	100,						// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,			// 1: high level valid; 0: low level valid

	1,						// count of identify code
	{{0xf0, 0x21},					// supply two code to identify sensor.
	 {0xf1, 0x55}},					// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,				// voltage of avdd

	1600,						// max width of source image
	1200,						// max height of source image
	"GC2155_MIPI",					// name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,			// define in SENSOR_IMAGE_FORMAT_E enum,
							// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YVYU,		// pattern of input image form sensor;

	s_GC2155_MIPI_resolution_Tab_YUV,			// point to resolution table information structure
	&s_GC2155_MIPI_ioctl_func_tab,			// point to ioctl function table

	PNULL,						// information and table about Rawrgb sensor
	PNULL,						// extend information about sensor
	SENSOR_AVDD_1800MV,				// iovdd
	SENSOR_AVDD_1800MV,				// dvdd
	1,						// skip frame num before preview
	1,						// skip frame num before capture
	0,
	0,

	0,
	0,
	0,
	0,
	0,
#ifdef GC2155_MIPI_2Lane
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 8, 1},
#else
	{SENSOR_INTERFACE_TYPE_CSI2, 1, 8, 1},
#endif
	s_gc2155_video_info,
	1,						// skip frame num while change setting
};

static void GC2155_MIPI_WriteReg( uint8_t  subaddr, uint8_t data )
{

	Sensor_WriteReg_8bits(subaddr, data);


}

static uint8_t GC2155_MIPI_ReadReg( uint8_t  subaddr)
{
	uint8_t value = 0;

	value = Sensor_ReadReg( subaddr);

	return value;
}

static unsigned long GC2155_MIPI_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_GC2155_MIPI_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_GC2155_MIPI_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_GC2155_MIPI_yuv_info.iovdd_val;
	BOOLEAN power_down = g_GC2155_MIPI_yuv_info.power_down_level;
	BOOLEAN reset_level = g_GC2155_MIPI_yuv_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(5000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(5000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(5000);
		//step 1 power up DVDD
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(5000);
		Sensor_PowerDown(!power_down);
		// Reset sensor
		usleep(5000);
		Sensor_SetResetLevel(!reset_level);
		usleep(5000);
		CMR_LOGE("eric GC2155_MIPI\n");
	} else {
		Sensor_PowerDown(power_down);
		usleep(5000);
		Sensor_SetResetLevel(reset_level);
		usleep(5000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(5000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(5000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		usleep(5000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(5000);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

static unsigned long GC2155_MIPI_Identify(unsigned long param)
{
#define GC2155_MIPI_PID_ADDR1	0xf0
#define GC2155_MIPI_PID_ADDR2	0xf1
#define GC2155_MIPI_SENSOR_ID	0x2155

	uint16_t sensor_id = 0;
	uint8_t pid_value = 0;
	uint8_t ver_value = 0;
	int i;
	BOOLEAN ret_value = SENSOR_FAIL;

	for(i=0;i<3;i++) {
		sensor_id = GC2155_MIPI_ReadReg(GC2155_MIPI_PID_ADDR1) << 8;
		usleep(1000);
		sensor_id |= GC2155_MIPI_ReadReg(GC2155_MIPI_PID_ADDR2);
		usleep(1000);
		SENSOR_PRINT("%s sensor_id is %x\n", __func__, sensor_id);
		if (sensor_id == GC2155_MIPI_SENSOR_ID) {
			SENSOR_PRINT("the main sensor is GC2155_MIPI\n");
			return SENSOR_SUCCESS;
		}
	}
	return ret_value;
}

static unsigned long Gc2155_GetResolutionTrimTab(unsigned long param)
{
	return (unsigned long) s_Gc2155_Resolution_Trim_Tab;
}

static void GC2155_MIPI_set_shutter()
{
	uint32_t   shutter = 0 ;

	GC2155_MIPI_WriteReg(0xfe,0x00);
#ifdef GC2155_MIPI_2Lane
	GC2155_MIPI_WriteReg(0xfa,0x00);
#else
	GC2155_MIPI_WriteReg(0xfa,0x11);
#endif
	GC2155_MIPI_WriteReg(0xb6,0x00);//AEC CLOSE
	shutter = (GC2155_MIPI_ReadReg(0x03)<<8 )|( GC2155_MIPI_ReadReg(0x04));

	shutter = shutter / 2;

	if (shutter < 1) {
		shutter = 1;
	}
	GC2155_MIPI_WriteReg(0x03, (shutter >> 8)&0xff);/* Shutter */
	GC2155_MIPI_WriteReg(0x04, shutter&0xff);
}

SENSOR_REG_T GC2155_MIPI_brightness_tab[][4]=
{
	{{0xfe , 0x02},{0xd5 , 0xc0},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0xe0},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0xf0},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x00},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x10},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x20},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x02},{0xd5 , 0x30},{0xfe , 0x00},{0xff , 0xff}}
};

static unsigned long set_brightness(unsigned long level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2155_MIPI_brightness_tab[level];

	if(level>6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		GC2155_MIPI_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

SENSOR_REG_T GC2155_MIPI_ev_tab[][4]=
{
	{{0xfe , 0x01},{0x13 , 0x15},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x20},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x25},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x2d},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x35},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x40},{0xfe , 0x00},{0xff , 0xff}},
	{{0xfe , 0x01},{0x13 , 0x45},{0xfe , 0x00},{0xff , 0xff}}
};

static unsigned long set_GC2155_MIPI_ev(unsigned long level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2155_MIPI_ev_tab[level];

	if(level>6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||(0xFF != sensor_reg_ptr[i].reg_value) ; i++) {
		GC2155_MIPI_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

static unsigned long set_GC2155_MIPI_anti_flicker(unsigned long param )
{
	shutter_flag = 0;
	SENSOR_PRINT("anti_flicker_mode:0x%x", param);
	switch (param) {
	case FLICKER_50HZ:
		GC2155_MIPI_WriteReg(0x05 , 0x01);//hb
		GC2155_MIPI_WriteReg(0x06 , 0x56);
		GC2155_MIPI_WriteReg(0x07 , 0x00);//vb
		GC2155_MIPI_WriteReg(0x08 , 0x32);
		GC2155_MIPI_WriteReg(0xfe , 0x01);
		GC2155_MIPI_WriteReg(0x25 , 0x00);//step
		GC2155_MIPI_WriteReg(0x26 , 0xfa);
		GC2155_MIPI_WriteReg(0x27 , 0x04);//level1
		GC2155_MIPI_WriteReg(0x28 , 0xe2);
		GC2155_MIPI_WriteReg(0x29 , 0x06);//level2
		GC2155_MIPI_WriteReg(0x2a , 0xd6);
		GC2155_MIPI_WriteReg(0x2b , 0x07);//6e8//level3 640
		GC2155_MIPI_WriteReg(0x2c , 0xd0);
		GC2155_MIPI_WriteReg(0x2d , 0x0b);//level4
		GC2155_MIPI_WriteReg(0x2e , 0xb8);
		GC2155_MIPI_WriteReg(0xfe , 0x00);
		break;

	case FLICKER_60HZ:
		GC2155_MIPI_WriteReg(0x05 , 0x01);//hb
		GC2155_MIPI_WriteReg(0x06 , 0x58);
		GC2155_MIPI_WriteReg(0x07 , 0x00);//vb
		GC2155_MIPI_WriteReg(0x08 , 0x32);
		GC2155_MIPI_WriteReg(0xfe , 0x01);
		GC2155_MIPI_WriteReg(0x25 , 0x00);//step
		GC2155_MIPI_WriteReg(0x26 , 0xd0);
		GC2155_MIPI_WriteReg(0x27 , 0x04);//level1
		GC2155_MIPI_WriteReg(0x28 , 0xe0);
		GC2155_MIPI_WriteReg(0x29 , 0x06);//level2
		GC2155_MIPI_WriteReg(0x2a , 0x80);
		GC2155_MIPI_WriteReg(0x2b , 0x08);//6e8//level3 640
		GC2155_MIPI_WriteReg(0x2c , 0x20);
		GC2155_MIPI_WriteReg(0x2d , 0x0b);//level4
		GC2155_MIPI_WriteReg(0x2e , 0x60);
		GC2155_MIPI_WriteReg(0xfe , 0x00);
		break;

	default:
		break;
	}

	return 0;
}

static const SENSOR_REG_T gc2155_video_mode_tab[][4]=
{
	/* preview mode:highest 20 fps*/
	{
		{0xfe, 0x01},
		{0x3c, 0x40},
		{0xfe, 0x00},
		{0xff, 0xff}
	},
	/* video mode: if use 20 fps*/
	{
		{0xfe, 0x01},
		{0x3c, 0x00},
		{0xfe, 0x00},
		{0xff, 0xff}
	}
};

static unsigned long set_GC2155_MIPI_video_mode(unsigned long mode)
{
	SENSOR_REG_T_PTR sensor_reg_ptr = (SENSOR_REG_T_PTR)gc2155_video_mode_tab[mode];
	uint16_t i;

	SENSOR_PRINT("0x%02x", mode);

	if (mode>1)
		return 0;

	for (i = 0x00; (0xff != sensor_reg_ptr[i].reg_addr) || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

SENSOR_REG_T GC2155_MIPI_awb_tab[][6]=
{
	//Auto
	{
		{0xfe , 0x00},
		{0xfe , 0x00},
		{0xfe , 0x00},
		{0xfe , 0x00},
		{0x82 , 0xfe},
		{0xff , 0xff}
	},

	//Office
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x40},//40
		{0xb4 , 0x4b},//40
		{0xb5 , 0x9e},//a8
		{0xff , 0xff}
	},

	//U30  //not use
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x50},
		{0xb4 , 0x40},
		{0xb5 , 0xa8},
		{0xff , 0xff}
	},

	//CWF  //not use
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x50},
		{0xb4 , 0x40},
		{0xb5 , 0xa8},
		{0xff , 0xff}
	},

	//HOME
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x49},//a0
		{0xb4 , 0x40},//45
		{0xb5 , 0x86},//40
		{0xff , 0xff}
	},

	//SUN:
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x4d},
		{0xb4 , 0x40},//40
		{0xb5 , 0x63},
		{0xff , 0xff}
	},

	//CLOUD:
	{
		{0xfe , 0x00},
		{0x82 , 0xfc},
		{0xb3 , 0x55},//7c
		{0xb4 , 0x40},//40
		{0xb5 , 0x4f},//50
		{0xff , 0xff}
	}
};

static unsigned long set_GC2155_MIPI_awb(unsigned long mode)
{
	uint8_t awb_en_value;
	uint16_t i;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2155_MIPI_awb_tab[mode];


	if(mode>6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		GC2155_MIPI_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

SENSOR_REG_T GC2155_MIPI_contrast_tab[][4]=
{
	{{0xfe,0x02}, {0xd3,0x30}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x34}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x38}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x40}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x44}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x48}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd3,0x50}, {0xfe,0x00}, {0xff,0xff}}
};

static unsigned long set_contrast(unsigned long level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr;

	sensor_reg_ptr = (SENSOR_REG_T*)GC2155_MIPI_contrast_tab[level];

	if(level>6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		GC2155_MIPI_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

SENSOR_REG_T GC2155_MIPI_saturation_tab[][5]=
{
	{{0xfe,0x02}, {0xd1,0x10},{0xd2,0x10}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd1,0x18},{0xd2,0x18}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd1,0x20},{0xd2,0x20}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd1,0x28},{0xd2,0x28}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd1,0x30},{0xd2,0x30}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd1,0x38},{0xd2,0x38}, {0xfe,0x00}, {0xff,0xff}},
	{{0xfe,0x02}, {0xd1,0x40},{0xd2,0x40}, {0xfe,0x00}, {0xff,0xff}}
};

SENSOR_REG_T GC2155_MIPI_fps_tab[][18]=
{
	{// 8-15.4
		{0xfe, 0x00},{0x05, 0x02},{0x06, 0x2d},{0x07, 0x01},{0x08, 0x1c},{0xfe, 0x01},{0x25, 0x00},{0x26, 0xd4},{0x27, 0x0a},{0x28, 0xc4},/* 15.4*/
		{0x29, 0x0c},{0x2a, 0x6c},/* 13.4*/{0x2b, 0x0f},{0x2c, 0xbc},/*10.6*/{0x2d, 0x14},{0x2e, 0xb4},/*8*/{0xfe, 0x00},{0xff, 0xff},
	},
	{//8.6-22
		{0xfe, 0x00},{0x05, 0x02},{0x06, 0x2d},{0x07, 0x01},{0x08, 0x1c},{0xfe, 0x01},{0x25, 0x00},{0x26, 0xd4},{0x27, 0x07},{0x28, 0x74},/* 22*/
		{0x29, 0x0a},{0x2a, 0xc4},/*15.4*/{0x2b, 0x0f},{0x2c, 0xbc},/*10.6*/{0x2d, 0x13},{0x2e, 0x0c},/*8.6*/{0xfe, 0x00},{0xff, 0xff},
	},
	{//8.6-30fps
		{0xfe, 0x00},{0x05, 0x02},{0x06, 0x2d},{0x07, 0x01},{0x08, 0x1c},{0xfe, 0x01},{0x25, 0x00},{0x26, 0xd4},{0x27, 0x05},{0x28, 0xcc},/* 30*/
		{0x29, 0x07},{0x2a, 0x74},/*22*/{0x2b, 0x0f},{0x2c, 0xbc},/*10.6*/{0x2d, 0x13},{0x2e, 0x0c},/*8.6*/{0xfe, 0x00},{0xff, 0xff},
	},
	{//15,15
		{0xfe, 0x00},{0x05, 0x01},{0x06, 0x56},{0x07, 0x01},{0x08, 0xae},{0xfe, 0x01},{0x25, 0x00},{0x26, 0xfa},{0x27, 0x05},{0x28, 0xdc},
		{0x29, 0x05},{0x2a, 0xdc},/*22*/{0x2b, 0x05},{0x2c, 0xdc},/*10.6*/{0x2d, 0x05},{0x2e, 0xdc},/*8.6*/{0xfe, 0x00},{0xff, 0xff},
	},
	{//30.03fps
		{0xfe, 0x00},{0x05, 0x01},{0x06, 0x03},{0x07, 0x00},{0x08, 0x14},{0xfe, 0x01},{0x25, 0x01},{0x26, 0x78},{0x27, 0x04},{0x28, 0x68},/* 30*/
		{0x29, 0x04},{0x2a, 0x48},/*30.03*/{0x2b, 0x04},{0x2c, 0x68},/*30.03*/{0x2d, 0x04},{0x2e, 0x68},/*30.03*/{0xfe, 0x00},{0xff, 0xff},
	},
	{//16.6-19.5
		{0xfe, 0x00},{0x05, 0x01},{0x06, 0x56},{0x07, 0x00},{0x08, 0x32},{0xfe, 0x01},{0x25, 0x00},{0x26, 0xfa},{0x27, 0x04},{0x28, 0xe2},/* 30*/
		{0x29, 0x04},{0x2a, 0xe2},/*22*/{0x2b, 0x05},{0x2c, 0xdc},/*10.6*/{0x2d, 0x05},{0x2e, 0xdc},/*8.6*/{0xfe, 0x00},{0xff, 0xff},
	},
};
static unsigned long set_saturation(unsigned long level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr;

	sensor_reg_ptr = (SENSOR_REG_T*)GC2155_MIPI_saturation_tab[level];

	if(level>6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		GC2155_MIPI_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}
static unsigned long set_preview_mode(unsigned long preview_mode)
{
	SENSOR_PRINT("set_preview_mode: preview_mode = %d\n", preview_mode);

	set_GC2155_MIPI_anti_flicker(0);
	switch (preview_mode) {
	case DCAMERA_ENVIRONMENT_NORMAL:
		//YCP_saturation
		GC2155_MIPI_WriteReg(0xfe , 0x01);
		GC2155_MIPI_WriteReg(0x3c , 0x40);
		GC2155_MIPI_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: DCAMERA_ENVIRONMENT_NORMAL\n");
		break;

	case 1://DCAMERA_ENVIRONMENT_NIGHT://1
		//YCP_saturation
		GC2155_MIPI_WriteReg(0xfe , 0x01);
		GC2155_MIPI_WriteReg(0x3c , 0x60);
		GC2155_MIPI_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: DCAMERA_ENVIRONMENT_NIGHT\n");
		break;

	case 3://SENSOR_ENVIROMENT_PORTRAIT://3
		//YCP_saturation
		GC2155_MIPI_WriteReg(0xfe , 0x01);
		GC2155_MIPI_WriteReg(0x3c , 0x40);
		GC2155_MIPI_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: SENSOR_ENVIROMENT_PORTRAIT\n");
		break;

	case 4://SENSOR_ENVIROMENT_LANDSCAPE://4
		//nightmode disable
		GC2155_MIPI_WriteReg(0xfe , 0x01);
		GC2155_MIPI_WriteReg(0x3c , 0x40);
		GC2155_MIPI_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: SENSOR_ENVIROMENT_LANDSCAPE\n");
		break;

	case 2://SENSOR_ENVIROMENT_SPORTS://2
		//nightmode disable
		//YCP_saturation
		GC2155_MIPI_WriteReg(0xfe , 0x01);
		GC2155_MIPI_WriteReg(0x3c , 0x40);
		GC2155_MIPI_WriteReg(0xfe , 0x00);
		SENSOR_PRINT("set_preview_mode: SENSOR_ENVIROMENT_SPORTS\n");
		break;

	default:
		break;
	}

	SENSOR_Sleep(10);
	return 0;
}

SENSOR_REG_T GC2155_MIPI_image_effect_tab[][2]=
{
	//Normal
	{{0x83 , 0xe0},{0xff , 0xff}},
	//BLACK&WHITE
	{{0x83 , 0x12},{0xff , 0xff}},
	//RED
	{{0x83 , 0x12},{0xff , 0xff}},
	//GREEN
	{{0x83 , 0x52},{0xff , 0xff}},
	//BLUE
	{{0x83 , 0x62},{0xff , 0xff}},
	//YELLOW
	{{0x83 , 0x72},{0xff , 0xff}},
	//NEGATIVE
	{{0x83 , 0x01},{0xff , 0xff}},
	//SEPIA
	{{0x83 , 0x82},{0xff , 0xff}}
};

static unsigned long set_image_effect(unsigned long effect_type)
{
	uint16_t i;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC2155_MIPI_image_effect_tab[effect_type];
	if(effect_type>7)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

static unsigned long GC2155_MIPI_After_Snapshot(unsigned long param)
{
	GC2155_MIPI_WriteReg(0xfa,0x00);
	GC2155_MIPI_WriteReg(0xb6,0x01);
	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}

static uint32_t GC2155_MIPI_SetFps(unsigned long param)
	{
		uint32_t rtn = SENSOR_SUCCESS;
		SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;
		SENSOR_REG_T* sensor_reg_ptr;
		uint16_t i,level;
		uint8_t min_fps = ext_ptr->param;
	    uint8_t max_fps = ext_ptr->param2;
		SENSOR_PRINT("SENSOR_GC2155_MIPI: GC2155_MIPI_SetFps min_fps: %d, max_fps: %d", min_fps,max_fps);
		if((5==min_fps)&&(15==max_fps))
			level = 0;
		else if((5==min_fps)&&(20==max_fps))
			level = 1;
		else if((5==min_fps)&&(30==max_fps))
			level = 2;
		else if((15==min_fps)&&(15==max_fps))
			level = 3;
		else if((30==min_fps)&&(30==max_fps))
			level = 4;
		else if((15==min_fps)&&(30==max_fps))
			level = 5;
		else
			return SENSOR_FAIL;

		GC2155_MIPI_WriteReg(0xfe,0x00);

		if(level==4)
			GC2155_MIPI_WriteReg(0xf8,0x86);
		else
			GC2155_MIPI_WriteReg(0xf8,0x84);

		sensor_reg_ptr = (SENSOR_REG_T*)GC2155_MIPI_fps_tab[level];
		for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
			GC2155_MIPI_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
		}
		return rtn;
	}
static unsigned long GC2155_MIPI_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT_HIGH("0x%x", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_SET_FPS:
		rtn = GC2155_MIPI_SetFps(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}
static unsigned long GC2155_MIPI_BeforeSnapshot(unsigned long sensor_snapshot_mode)
{
	sensor_snapshot_mode &= 0xffff;
	Sensor_SetMode((uint32_t)sensor_snapshot_mode);
	Sensor_SetMode_WaitDone();

	switch (sensor_snapshot_mode) {
	case SENSOR_MODE_PREVIEW_ONE:
		SENSOR_PRINT("Capture VGA Size");
		break;
	case SENSOR_MODE_SNAPSHOT_ONE_FIRST:
	case SENSOR_MODE_SNAPSHOT_ONE_SECOND:
#ifdef GC2155_MIPI_2Lane
		SENSOR_PRINT("GC2155_MIPI_2Lane_beforesnapshoot ");
#else
		GC2155_MIPI_set_shutter();
		SENSOR_PRINT("GC2155_MIPI_1Lane_beforesnapshoot ");
#endif
		break;
	default:
		break;
	}

	SENSOR_PRINT("SENSOR_GC2155: Before Snapshot");
	return 0;
}

static unsigned long read_gain_scale(unsigned long value)
{
	return SENSOR_GAIN_SCALE;
}

static unsigned long GC2155_MIPI_StreamOn(unsigned long param)
{
	SENSOR_PRINT("Start");
	usleep(100*1000);
#ifdef GC2155_MIPI_2Lane
	GC2155_MIPI_WriteReg(0xfe, 0x03);
	GC2155_MIPI_WriteReg(0x10, 0x95);
	GC2155_MIPI_WriteReg(0xfe, 0x00);
#else
	GC2155_MIPI_WriteReg(0xfe, 0x03);
	GC2155_MIPI_WriteReg(0x10, 0x94);
	GC2155_MIPI_WriteReg(0xfe, 0x00);
#endif
	usleep(20*1000);
	return 0;
}

static unsigned long GC2155_MIPI_StreamOff(unsigned long param)
{
	unsigned char value;
	unsigned int sleep_time = 0;
	SENSOR_PRINT("Stop");
	usleep(20*1000);

	value = Sensor_ReadReg(0x10);

#ifdef GC2155_MIPI_2Lane
	if (value == 0x95) {
		GC2155_MIPI_WriteReg(0xfe, 0x03);
		GC2155_MIPI_WriteReg(0x10, 0x85);
		GC2155_MIPI_WriteReg(0xfe, 0x00);
		if (!s_GC2155_sensor_close_flag) {
			sleep_time = 100*1000;
			usleep(sleep_time);
		}
	} else {
		GC2155_MIPI_WriteReg(0xfe, 0x03);
		GC2155_MIPI_WriteReg(0x10, 0x85);
		GC2155_MIPI_WriteReg(0xfe, 0x00);
	}
#else
	if (value == 0x94) {
		GC2155_MIPI_WriteReg(0xfe, 0x03);
		GC2155_MIPI_WriteReg(0x10, 0x84);
		GC2155_MIPI_WriteReg(0xfe, 0x00);
		if (!s_GC2155_sensor_close_flag) {
			sleep_time = 100*1000;
			usleep(sleep_time);
		}
	} else {
		GC2155_MIPI_WriteReg(0xfe, 0x03);
		GC2155_MIPI_WriteReg(0x10, 0x84);
		GC2155_MIPI_WriteReg(0xfe, 0x00);
	}
#endif

	s_GC2155_sensor_close_flag = 0;

	SENSOR_PRINT("X sleep_time=%dus", sleep_time);

	return 0;
}

static uint32_t GC2155_set_sensor_close_flag()
{
	uint32_t rtn = SENSOR_SUCCESS;

	s_GC2155_sensor_close_flag = 1;

	return rtn;
}

static unsigned long GC2155_close_flag(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_gc2155: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
			rtn = GC2155_set_sensor_close_flag();
			break;
		default:
			break;
	}

	return rtn;
}
