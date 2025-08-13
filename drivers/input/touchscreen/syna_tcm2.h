/* SPDX-License-Identifier: GPL-2.0
 *
 * Synaptics TouchCom touchscreen driver
 *
 * Copyright (C) 2017-2020 Synaptics Incorporated. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION DOES
 * NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES, SYNAPTICS'
 * TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT EXCEED ONE HUNDRED U.S.
 * DOLLARS.
 */

#ifndef _SYNAPTICS_TCM2_DRIVER_H_
#define _SYNAPTICS_TCM2_DRIVER_H_

#define REMOVE_OPLUS_FUNCTION 1
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/major.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/input/mt.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/crc32.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>

#define _EIO        (-EIO)
#define _ENOMEM     (-ENOMEM)
#define _EINVAL     (-EINVAL)
#define _ENODEV     (-ENODEV)
#define _ETIMEDOUT  (-ETIMEDOUT)

#define GET_BIT(var, pos) \
	(((var) & (1 << (pos))) >> (pos))

#define ATOMIC_SET(atomic, value) \
	atomic_set(&atomic, value)

#define ATOMIC_GET(atomic) \
	atomic_read(&atomic)

#define PLATFORM_DRIVER_NAME "synaptics_tcm_hbp"

#define TOUCH_INPUT_NAME "touchpanel"
#define TOUCH_INPUT_PHYS_PATH "synaptics_tcm/touch_input"

#define CHAR_DEVICE_NAME "tcm_hbp"
#define CHAR_DEVICE_MODE (0x0600)

#define SIG_DISPLAY_ON  44
#define SIG_DISPLAY_OFF 45
#define SIG_FINGER_DOWN 46

#undef MAX_DEVICE_VERSION_LENGTH
#define MAX_DEVICE_VERSION_LENGTH 256

#define SYNAPTICS_TCM_DRIVER_ID (1 << 0)
#define SYNAPTICS_TCM_DRIVER_VERSION 1
#define SYNAPTICS_TCM_DRIVER_SUBVER "4.0"

#define ABS_TOUCH_COST_TIME_KERNEL  0x21
#define ABS_TOUCH_COST_TIME_ALGO    0x22
#define ABS_TOUCH_COST_TIME_DAEMON  0x23
#define MAX_TOUCH_COST_TIME         1000 * 1000

#define TX_NUM 17
#define RX_NUM 38

#define IRQ_COST_TIME_OVER_5MS 5000
#define IRQ_COST_TIME_OVER_10MS 10000
#define IRQ_COST_TIME_OVER_20MS 20000
#define IRQ_COST_TIME_OVER_50MS 50000

#define FW_UPDATE_COMPLETE_TIMEOUT  msecs_to_jiffies(40*1000)

#define GESTURE_MODE_SWITCH_RETRY_TIMES     5
#define MAX_HEALTH_REPORT_LEN 50

#define HAS_SYSFS_INTERFACE
#define HAS_REFLASH_FEATURE
#define HAS_ROMBOOT_REFLASH_FEATURE
#define HAS_TESTING_FEATURE

#define TYPE_B_PROTOCOL

#define RESET_ON_RESUME

#define REPORT_TOUCH_WIDTH
#define RESUME_EARLY_UNBLANK

#define REPORT_TYPES (256)
#define EFP_ENABLE	(1)
#define EFP_DISABLE (0)

#define TCM_CONNECT_IN_PROBE

#define DTAP_DETECT     0x01
#define SWIPE_DETECT    0x02
#define TRIANGLE_DETECT 0x03
#define CIRCLE_DETECT   0x04
#define VEE_DETECT      0x05
#define HEART_DETECT    0x07
#define UNICODE_DETECT  0x08
#define STAP_DETECT     0x10
#define M_UNICODE       0x6d
#define S_UNICODE       0x73
#define W_UNICODE       0x77

#define REPORT_TIMEOUT_MS       1000

#define TOUCH_HOLD_DOWN 0x80
#define TOUCH_HOLD_UP   0x81
#define FINGERPRINT_ERR_REPORT   0x84

#define TOUCH_BIT_CHECK           0x3FF

#define TCM_V1_MESSAGE_MARKER 0xa5
#define TCM_V1_MESSAGE_PADDING 0x5a

#define IS_APP_FW_MODE(mode) \
	(mode == MODE_APPLICATION_FIRMWARE)

#define IS_NOT_APP_FW_MODE(mode) \
	(!IS_APP_FW_MODE(mode))

#define IS_BOOTLOADER_MODE(mode) \
	((mode == MODE_BOOTLOADER) || \
	(mode == MODE_TDDI_BOOTLOADER)  || \
	(mode == MODE_TDDI_HDL_BOOTLOADER) || \
	(mode == MODE_MULTICHIP_TDDI_BOOTLOADER))

#define IS_ROM_BOOTLOADER_MODE(mode) \
	(mode == MODE_ROMBOOTLOADER)
	
#define MAX_NUM_OBJECTS (10)

#define MAX_SIZE_GESTURE_DATA (8)

#define MAX_SIZE_CONFIG_ID (16)

#define MAX_SIZE_GRIP_INFO (4)

#define RD_RETRY_US_MIN (5000)
#define RD_RETRY_US_MAX (10000)

#define WR_DELAY_US_MIN (500)
#define WR_DELAY_US_MAX (1000)

#define TAT_DELAY_US_MIN (100)
#define TAT_DELAY_US_MAX (200)

#define TCM_MSG_CRC_LENGTH (2)
#define TCM_EXTRA_RC_LENGTH (1)

#define RECORD_POINTS_COUNT 5

#define FIRMWARE_MODE_BL_MAX (2)
#define FWUPDATE_BL_MAX (3)

#define MESSAGE_HEADER_SIZE (4)

#define CMD_RESPONSE_TIMEOUT_MS (3000)
#define CMD_RESPONSE_POLLING_DELAY_MS (20)

#define RESET_DELAY_MS (200)

#define POLLING_ENABLE_IRQ_DELAY_MS   5

#define RESP_IN_ATTN (0)
#define RESP_IN_POLLING (CMD_RESPONSE_POLLING_DELAY_MS)

#define RD_CHUNK_SIZE (0)
#define WR_CHUNK_SIZE (1024)

typedef enum tp_dev {
	TP_OFILM,
	TP_BIEL,
	TP_TRULY,
	TP_BOE,
	TP_G2Y,
	TP_TPK,
	TP_JDI,
	TP_TIANMA,
	TP_SAMSUNG,
	TP_DSJM,
	TP_BOE_B8,
	TP_INNOLUX,
	TP_HIMAX_DPT,
	TP_AUO,
	TP_DEPUTE,
	TP_HUAXING,
	TP_HLT,
	TP_DJN,
	TP_VXN,
	TP_UNKNOWN,
} tp_dev;

enum tcm_app_status {
	APP_STATUS_OK = 0x00,
	APP_STATUS_BOOTING = 0x01,
	APP_STATUS_UPDATING = 0x02,
	APP_STATUS_BAD_APP_CONFIG = 0xff,
};

enum tcm_firmware_protocol {
	TOUCHCOMM_NONE = 0,
	TOUCHCOMM_V1 = 1,
	TOUCHCOMM_V2 = 2,
};

enum tcm_report_type {
	REPORT_IDENTIFY = 0x10,
	REPORT_TOUCH = 0x11,
	REPORT_DELTA = 0x12,
	REPORT_RAW = 0x13,
	REPORT_DEBUG = 0x14,
	REPORT_HBP_ACTIVE_FRAME = 0x23,
	REPORT_LOG = 0x9f,
	REPORT_POWER_STATE_INFO = 0xFE,
};

enum tcm_firmware_mode {
	MODE_UNKNOWN = 0x00,
	MODE_APPLICATION_FIRMWARE = 0x01,
	MODE_HOSTDOWNLOAD_FIRMWARE = 0x02,
	MODE_ROMBOOTLOADER = 0x04,
	MODE_BOOTLOADER = 0x0b,
	MODE_TDDI_BOOTLOADER = 0x0c,
	MODE_TDDI_HDL_BOOTLOADER = 0x0d,
	MODE_PRODUCTIONTEST_FIRMWARE = 0x0e,
	MODE_MULTICHIP_TDDI_BOOTLOADER = 0xab,
};

enum tcm_status_code {
	STATUS_IDLE = 0x00,
	STATUS_OK = 0x01,
	STATUS_CONTINUED_READ = 0x03,
	STATUS_NO_REPORT_AVAILABLE = 0x04,
	STATUS_ACK = 0x07,
	STATUS_RETRY_REQUESTED = 0x08,
	STATUS_CMD_FAILED = 0x09,
	STATUS_RECEIVE_BUFFER_OVERFLOW = 0x0c,
	STATUS_PREVIOUS_COMMAND_PENDING = 0x0d,
	STATUS_NOT_IMPLEMENTED = 0x0e,
	STATUS_ERROR = 0x0f,
	STATUS_PACKET_CORRUPTED = 0xfe,
	STATUS_INVALID = 0xff,
};

enum tcm_command {
	CMD_NONE = 0x00,
	CMD_CONTINUE_WRITE = 0x01,
	CMD_IDENTIFY = 0x02,
	CMD_RESET = 0x04,
	CMD_ENABLE_REPORT = 0x05,
	CMD_DISABLE_REPORT = 0x06,
	CMD_TCM2_ACK = 0x07,
	CMD_TCM2_RETRY = 0x08,
	CMD_TCM2_SET_MAX_READ_LENGTH = 0x09,
	CMD_TCM2_GET_REPORT = 0x0a,
	CMD_GET_BOOT_INFO = 0x10,
	CMD_ERASE_FLASH = 0x11,
	CMD_WRITE_FLASH = 0x12,
	CMD_READ_FLASH = 0x13,
	CMD_RUN_APPLICATION_FIRMWARE = 0x14,
	CMD_SPI_MASTER_WRITE_THEN_READ = 0x15,
	CMD_REBOOT_TO_ROM_BOOTLOADER = 0x16,
	CMD_RUN_BOOTLOADER_FIRMWARE = 0x1f,
	CMD_GET_APPLICATION_INFO = 0x20,
	CMD_GET_STATIC_CONFIG = 0x21,
	CMD_SET_STATIC_CONFIG = 0x22,
	CMD_GET_DYNAMIC_CONFIG = 0x23,
	CMD_SET_DYNAMIC_CONFIG = 0x24,
	CMD_GET_TOUCH_REPORT_CONFIG = 0x25,
	CMD_SET_TOUCH_REPORT_CONFIG = 0x26,
	CMD_REZERO = 0x27,
	CMD_COMMIT_CONFIG = 0x28,
	CMD_DESCRIBE_DYNAMIC_CONFIG = 0x29,
	CMD_PRODUCTION_TEST = 0x2a,
	CMD_SET_CONFIG_ID = 0x2b,
	CMD_ENTER_DEEP_SLEEP = 0x2c,
	CMD_EXIT_DEEP_SLEEP = 0x2d,
	CMD_GET_TOUCH_INFO = 0x2e,
	CMD_GET_DATA_LOCATION = 0x2f,
	CMD_DOWNLOAD_CONFIG = 0x30,
	CMD_ENTER_PRODUCTION_TEST_MODE = 0x31,
	CMD_GET_FEATURES = 0x32,
	CMD_GET_ROMBOOT_INFO = 0x40,
	CMD_WRITE_PROGRAM_RAM = 0x41,
	CMD_ROMBOOT_RUN_BOOTLOADER_FIRMWARE = 0x42,
	CMD_SPI_MASTER_WRITE_THEN_READ_EXTENDED = 0x43,
	CMD_ENTER_IO_BRIDGE_MODE = 0x44,
	CMD_ROMBOOT_DOWNLOAD = 0x45,
};

enum dynamic_config_id {
	DC_UNKNOWN_1 = 0x00,
	DC_NO_DOZE,
	DC_DISABLE_NOISE_MITIGATION_1,
	DC_INHIBIT_FREQUENCY_SHIFT,
	DC_REQUESTED_FREQUENCY,
	DC_DISABLE_HSYNC_1,
	DC_REZERO_ON_EXIT_DEEP_SLEEP_1,
	DC_CHARGER_CONNECTED,
	DC_NO_BASELINE_RELAXATION,
	DC_IN_WAKEUP_GESTURE_MODE,
	DC_STIMULUS_FINGERS,
	DC_GRIP_SUPPRESSION_ENABLED,
	DC_ENABLE_THICK_GLOVE_1,
	DC_ENABLE_GLOVE_1,
	DC_PS_STATUS = 0xC1,
	DC_DISABLE_ESD = 0xC2,
	DC_FREQUENCE_HOPPING = 0xD2,
	DC_TOUCH_HOLD = 0xD4,
	DC_ERROR_PRIORITY = 0xD5,
	DC_NOISE_LENGTH = 0xD6,
	DC_GRIP_CONDTION_ZONE = 0xD8,
	DC_GRIP_SPECIAL_ZONE_X = 0xD9,
	DC_GRIP_SPECIAL_ZONE_Y = 0xDA,
	DC_GRIP_SPECIAL_ZONE_L = 0xDB,
	DC_GRIP_ROATE_TO_HORIZONTAL_LEVEL = 0xDC,
	DC_DARK_ZONE_ENABLE = 0xDD,
	DC_GRIP_ENABLED = 0xDE,
	DC_GRIP_DARK_ZONE_X = 0xDF,
	DC_GRIP_DARK_ZONE_Y = 0xE0,
	DC_GRIP_ABS_DARK_X = 0xE1,
	DC_GRIP_ABS_DARK_Y = 0xE2,
	DC_GRIP_ABS_DARK_U = 0xE3,
	DC_GRIP_ABS_DARK_V = 0xE4,
	DC_GRIP_ABS_DARK_SEL = 0xE5,
	DC_SET_REPORT_FRE = 0xE6,
	DC_GESTURE_MASK = 0xFE,
	DC_LOW_TEMP_ENABLE = 0xFD,
};

enum bus_connection {
	BUS_TYPE_NONE,
	BUS_TYPE_I2C,
	BUS_TYPE_SPI,
	BUS_TYPE_I3C,
};

enum gesture_classification {
	GESTURE_ID_NONE = 0,
	GESTURE_ID_DOUBLE_TAP = 1,
	GESTURE_ID_SWIPE = 2,
	GESTURE_ID_ACTIVE_SINGLE_TAP = 0x10,
	GESTURE_ID_ACTIVE_TAP_AND_HOLD = 0x80,
};

enum object_classification {
	LIFT = 0,
	FINGER = 1,
	GLOVED_OBJECT = 2,
	STYLUS = 3,
	ERASER = 4,
	SMALL_OBJECT = 5,
	PALM = 6,
	EDGE_TOUCHED = 8,
	HOVER_OBJECT = 9,
	NOP = -1,
};

enum touch_report_code {
	TOUCH_REPORT_END = 0x00,
	TOUCH_REPORT_FOREACH_ACTIVE_OBJECT = 0x01,
	TOUCH_REPORT_FOREACH_OBJECT = 0x02,
	TOUCH_REPORT_FOREACH_END = 0x03,
	TOUCH_REPORT_PAD_TO_NEXT_BYTE = 0x04,
	TOUCH_REPORT_TIMESTAMP = 0x05,
	TOUCH_REPORT_OBJECT_N_INDEX = 0x06,
	TOUCH_REPORT_OBJECT_N_CLASSIFICATION = 0x07,
	TOUCH_REPORT_OBJECT_N_X_POSITION = 0x08,
	TOUCH_REPORT_OBJECT_N_Y_POSITION = 0x09,
	TOUCH_REPORT_OBJECT_N_Z = 0x0a,
	TOUCH_REPORT_OBJECT_N_X_WIDTH = 0x0b,
	TOUCH_REPORT_OBJECT_N_Y_WIDTH = 0x0c,
	TOUCH_REPORT_OBJECT_N_TX_POSITION_TIXELS = 0x0d,
	TOUCH_REPORT_OBJECT_N_RX_POSITION_TIXELS = 0x0e,
	TOUCH_REPORT_0D_BUTTONS_STATE = 0x0f,
	TOUCH_REPORT_GESTURE_ID = 0x10,
	TOUCH_REPORT_FRAME_RATE = 0x11,
	TOUCH_REPORT_POWER_IM = 0x12,
	TOUCH_REPORT_CID_IM = 0x13,
	TOUCH_REPORT_RAIL_IM = 0x14,
	TOUCH_REPORT_CID_VARIANCE_IM = 0x15,
	TOUCH_REPORT_NSM_FREQUENCY_INDEX = 0x16,
	TOUCH_REPORT_NSM_STATE = 0x17,
	TOUCH_REPORT_NUM_OF_ACTIVE_OBJECTS = 0x18,
	TOUCH_REPORT_CPU_CYCLES_USED_SINCE_LAST_FRAME = 0x19,
	TOUCH_REPORT_FACE_DETECT = 0x1a,
	TOUCH_REPORT_GESTURE_DATA = 0x1b,
	TOUCH_REPORT_FORCE_MEASUREMENT = 0x1c,
	TOUCH_REPORT_FINGERPRINT_AREA_MEET = 0x1d,
	TOUCH_REPORT_SENSING_MODE = 0x1e,
	TOUCH_REPORT_KNOB_DATA = 0x24,
	TOUCH_REPORT_CUSTOM_GESTURE_INFO = 0xC6,
	TOUCH_REPORT_CUSTOM_GESTURE_COORDINATE = 0xC7,
	TOUCH_REPORT_CUSTOM_GRIP_INFO = 0xCB,
};

enum power_state {
	PWR_OFF = 0,
	PWR_ON,
	LOW_PWR,
	PWR_UNKNOWN,
};

enum sub_power_state {
	SUB_NONE = 0,
	SUB_PWR_RESUMING,
	SUB_PWR_RESUME_DONE,
	SUB_PWR_EARLY_SUSPENDING,
	SUB_PWR_SUSPENDING,
	SUB_PWR_SUSPEND_DONE,
};

enum tcm_command_status {
	CMD_STATE_IDLE = 0,
	CMD_STATE_BUSY = 1,
	CMD_STATE_ERROR = -1,
};

enum driver_state {
	IS_PROBE = 0,
	IS_SHUTDOWN,
	IS_REMOVE,
};

enum power_supply {
	PSU_REGULATOR = 0,
	PSU_GPIO,
};

struct tcm_buffer {
	unsigned char *buf;
	unsigned int buf_size;
	unsigned int data_length;
	struct mutex buf_mutex;
	unsigned char ref_cnt;
};

typedef enum {
    TP_RATE_START,
    TP_RATE_CALC,
    TP_RATE_CLEAR,
} tp_rate;

struct Coordinate {
	int x;
	int y;
};

struct swipes_record {
	int count;
	struct Coordinate start_points[RECORD_POINTS_COUNT];
	struct Coordinate end_points[RECORD_POINTS_COUNT];
};

struct points_record {
	int count;
	struct Coordinate points[RECORD_POINTS_COUNT];
};

static inline unsigned int le2_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
	       (unsigned int)src[1] * 0x100;
}

static inline unsigned int le4_to_uint(const unsigned char *src)
{
	return (unsigned int)src[0] +
	       (unsigned int)src[1] * 0x100 +
	       (unsigned int)src[2] * 0x10000 +
	       (unsigned int)src[3] * 0x1000000;
}

struct touchpanel_event {
	int touchpanel_id;
	int x;
	int y;
	int fid;
	char type;
	int touch_state;
	int area_rate;
};

struct tcm_v1_message_header {
	union {
		struct {
			unsigned char marker;
			unsigned char code;
			unsigned char length[2];
		};
		unsigned char data[MESSAGE_HEADER_SIZE];
	};
};

struct tcm_application_info {
	unsigned char version[2];
	unsigned char status[2];
	unsigned char static_config_size[2];
	unsigned char dynamic_config_size[2];
	unsigned char app_config_start_write_block[2];
	unsigned char app_config_size[2];
	unsigned char max_touch_report_config_size[2];
	unsigned char max_touch_report_payload_size[2];
	unsigned char customer_config_id[MAX_SIZE_CONFIG_ID];
	unsigned char max_x[2];
	unsigned char max_y[2];
	unsigned char max_objects[2];
	unsigned char num_of_buttons[2];
	unsigned char num_of_image_rows[2];
	unsigned char num_of_image_cols[2];
	unsigned char has_hybrid_data[2];
	unsigned char num_of_force_elecs[2];
};

struct tcm_identification_info {
	unsigned char version;
	unsigned char mode;
	unsigned char part_number[16];
	unsigned char build_id[4];
	unsigned char max_write_size[2];
	unsigned char max_read_size[2];
	unsigned char reserved[6];
};

struct tcm_gesture_data_blob {
	union {
		struct {
			unsigned char tap_x[2];
			unsigned char tap_y[2];
		};
		struct {
			unsigned char swipe_x[2];
			unsigned char swipe_y[2];
			unsigned char swipe_direction[2];
		};
		unsigned char data[MAX_SIZE_GESTURE_DATA];
	};
};

struct tcm_grip_info_blob {
	unsigned char data[MAX_SIZE_GRIP_INFO];
};

struct tcm_objects_data_blob {
	unsigned char status;
	unsigned int x_pos;
	unsigned int y_pos;
	unsigned int x_width;
	unsigned int y_width;
	unsigned int z;
	unsigned int tx_pos;
	unsigned int rx_pos;
	unsigned int custom_data[5];
	struct tcm_grip_info_blob grip_info;
};

struct tcm_touch_data_blob {
	unsigned int obji;
	unsigned int num_of_active_objects;
	struct tcm_objects_data_blob object_data[MAX_NUM_OBJECTS];
	unsigned int gesture_id;
	struct tcm_gesture_data_blob gesture_data;
	unsigned int timestamp;
	unsigned int buttons_state;
	unsigned int frame_rate;
	unsigned int force_data;
	unsigned int power_im;
	unsigned int cid_im;
	unsigned int rail_im;
	unsigned int cid_variance_im;
	unsigned int nsm_frequency;
	unsigned int nsm_state;
	unsigned int sensing_mode;
};

struct tcm_message_data_blob {
	atomic_t command_status;
	unsigned char command;
	unsigned char status_report_code;
	unsigned int payload_length;
	unsigned char response_code;
	unsigned char report_code;
	unsigned char seq_toggle;
	unsigned int default_resp_reading;
	struct completion cmd_completion;
	struct tcm_buffer in;
	struct tcm_buffer out;
	struct tcm_buffer temp;
	struct mutex cmd_mutex;
	struct mutex rw_mutex;
	bool predict_reads;
	unsigned int predict_length;
	bool has_crc;
	unsigned short crc_bytes;
	bool has_extra_rc;
	unsigned char rc_byte;
};

struct syna_hw_bus_data {
	unsigned char type;
	unsigned int rd_chunk_size;
	unsigned int wr_chunk_size;
	unsigned int frequency_hz;
	unsigned int i2c_addr;
	unsigned int spi_mode;
	unsigned int spi_byte_delay_us;
	unsigned int spi_block_delay_us;
	struct mutex io_mutex;
	int switch_gpio;
	int switch_state;
	struct pinctrl          *pinctrl;
	struct pinctrl_state    *pin_spi_mode_active;
	struct pinctrl_state    *pin_spi_mode_suspend;
	struct pinctrl_state    *pin_cs_high;
	struct pinctrl_state    *pin_cs_low;
};

struct syna_hw_attn_data {
	int irq_gpio;
	int irq_on_state;
	unsigned long irq_flags;
	int irq_id;
	bool irq_enabled;
	struct mutex irq_en_mutex;
};

struct syna_hw_rst_data {
	int reset_gpio;
	int reset_on_state;
	unsigned int reset_delay_ms;
	unsigned int reset_active_ms;
	struct mutex reset_en_mutex;
};

struct syna_hw_pwr_data {
	int psu;
	int vdd_gpio;
	int avdd_gpio;
	int power_on_state;
	unsigned int power_on_delay_ms;
	unsigned int vdd;
	unsigned int vled;
	unsigned int vio;
	unsigned int vddtx;
	const char *vdd_reg_name;
	void *vdd_reg_dev;
	const char *avdd_reg_name;
	void *avdd_reg_dev;
};

struct syna_hw_interface {
	void *pdev;
	struct syna_hw_bus_data bdata_io;
	struct syna_hw_attn_data bdata_attn;
	struct syna_hw_rst_data bdata_rst;
	struct syna_hw_pwr_data bdata_pwr;
	int (*ops_power_on)(struct syna_hw_interface *hw_if, bool en);
	void (*ops_hw_reset)(struct syna_hw_interface *hw_if);
	int (*ops_bus_setup)(struct syna_hw_interface *hw_if, struct syna_hw_bus_data *config);
	int (*ops_read_data)(struct syna_hw_interface *hw_if, unsigned char *rd_data, unsigned int rd_len);
	int (*ops_write_data)(struct syna_hw_interface *hw_if, unsigned char *wr_data, unsigned int wr_len);
	int (*ops_enable_irq)(struct syna_hw_interface *hw_if, bool en);
	int (*ops_wait_irq)(struct syna_hw_interface *hw_if, unsigned int timeout_ms);

};

int syna_hw_interface_init(void);
void syna_hw_interface_exit(void);

struct tcm_boot_info {
	unsigned char version;
	unsigned char status;
	unsigned char asic_id[2];
	unsigned char write_block_size_words;
	unsigned char erase_page_size_words[2];
	unsigned char max_write_payload_size[2];
	unsigned char last_reset_reason;
	unsigned char pc_at_time_of_last_reset[2];
	unsigned char boot_config_start_block[2];
	unsigned char boot_config_size_blocks[2];
	unsigned char display_config_start_block[4];
	unsigned char display_config_length_blocks[2];
	unsigned char backup_display_config_start_block[4];
	unsigned char backup_display_config_length_blocks[2];
	unsigned char custom_otp_start_block[2];
	unsigned char custom_otp_length_blocks[2];
};

struct tcm_dev {
	unsigned char protocol;
	unsigned char dev_mode;
	unsigned int packrat_number;
	unsigned int max_x;
	unsigned int max_y;
	unsigned int max_objects;
	unsigned int rows;
	unsigned int cols;
	unsigned char config_id[MAX_SIZE_CONFIG_ID];
	unsigned int max_wr_size;
	unsigned int max_rd_size;
	bool bypass_max_wr_rd_setup;
	bool is_sleep;
	struct syna_hw_interface *hw_if;
	unsigned int firmware_mode_count;
	unsigned int upload_flag;
	unsigned int error_state_count;
	struct tcm_identification_info id_info;
	struct tcm_application_info app_info;
	struct tcm_boot_info boot_info;
	struct tcm_buffer report_buf;
	struct tcm_buffer resp_buf;
	struct tcm_buffer external_buf;
	struct tcm_buffer touch_config;
	struct tcm_message_data_blob msg_data;
	atomic_t firmware_flashing;
	int (*read_message)(struct tcm_dev *tcm_dev,
			unsigned char *status_report_code);
	int (*write_message)(struct tcm_dev *tcm_dev,
			unsigned char command, unsigned char *payload,
			unsigned int payload_length, unsigned char *resp_code,
			unsigned int delay_ms_resp);
};

struct syna_tcm {

	int tp_index;
	struct tcm_dev *tcm_dev;
	struct completion report_complete;
	struct platform_device *pdev;
	struct tcm_touch_data_blob tp_data;
	struct mutex tp_event_mutex;
	struct mutex extif_mutex;
	struct mutex		mutex;
	unsigned char prev_obj_status[MAX_NUM_OBJECTS];
	struct tcm_buffer event_data;
	struct syna_hw_interface *hw_if;
	pid_t isr_pid;
	bool irq_wake;
	int irq_cost_time;
	struct cdev char_dev;
	dev_t char_dev_num;
	int char_dev_ref_count;
	struct class *device_class;
	struct device *device;
	struct kobject *sysfs_dir;
	struct proc_dir_entry *prEntry_tp;
	struct proc_dir_entry *prEntry_debug_tp;
	struct input_dev *input_dev;
	struct input_params {
		unsigned int max_x;
		unsigned int max_y;
		unsigned int max_objects;
	} input_dev_params;
	int tx_num;
	int rx_num;
	int dts_max_x;
	int dts_max_y;
	unsigned int firmware_max_x;
	unsigned int firmware_max_y;
	char *fw_name_fae;
	char algo_version[MAX_DEVICE_VERSION_LENGTH];
	int firmware_update_type;
	struct completion fw_complete;
	struct delayed_work reflash_work;
	struct workqueue_struct *reflash_workqueue;
	pid_t proc_pid;
	struct task_struct *proc_task;
	int probe_done;
	int pwr_state;
	int sub_pwr_state;
	bool slept_in_early_suspend;
	bool lpwg_enabled;
	bool is_attn_redirecting;
	unsigned char fb_ready;
	bool is_connected;
	bool hbp_enabled;
	int daemon_state;
	int primary_timestamp_enabled;
	int driver_current_state;
	unsigned int waiting_frame;
	unsigned int wait_for_ioctl_operation;
	unsigned int use_short_frame_waiting;
	bool snr_read_support;
	bool freq_hop_simulate_support;
	unsigned short gesture_type;
	unsigned short touch_and_hold;
	bool fp_active;
	int aging_test;
	int aging_mode;
	void *chip_data;
	bool in_test_process;
	unsigned int fifo_remaining_frame;
	struct list_head frame_fifo_queue;
	wait_queue_head_t wait_frame;
	unsigned char report_to_queue[REPORT_TYPES];
	struct work_struct speed_up_work;
	struct workqueue_struct *speedup_resume_wq;
	bool bus_ready;
	wait_queue_head_t wait;
	int (*dev_connect)(struct syna_tcm *tcm);
	int (*dev_disconnect)(struct syna_tcm *tcm);
	int (*dev_set_up_app_fw)(struct syna_tcm *tcm);
	int (*dev_resume)(struct device *dev);
	int (*dev_suspend)(struct device *dev);
};

int syna_dev_disable_hbp_mode(struct syna_tcm *tcm);
void syna_dev_update_lpwg_status(struct syna_tcm *tcm);
void syna_send_signal(struct syna_tcm *tcm, int signal_num);
int syna_dev_enable_lowpwr_gesture(struct syna_tcm *tcm, bool en);
int syna_tcm_get_touch_data(const unsigned char *report,
		unsigned int report_size, unsigned int offset,
		unsigned int bits, unsigned int *data);
int syna_tcm_preserve_touch_report_config(struct tcm_dev *tcm_dev);
int syna_tcm_parse_touch_report(struct tcm_dev *tcm_dev,
		unsigned char *report, unsigned int report_size,
		struct tcm_touch_data_blob *touch_data);
void syna_tcm_remove_device(struct tcm_dev *tcm_dev);
int syna_tcm_enable_predict_reading(struct tcm_dev *tcm_dev, bool en);
int syna_tcm_allocate_device(struct tcm_dev **ptcm_dev_ptr,
		struct syna_hw_interface *hw_if, unsigned int resp_reading);
int syna_tcm_switch_fw_mode(struct tcm_dev *tcm_dev,
		unsigned char mode, unsigned int fw_switch_delay);
int syna_tcm_set_dynamic_config(struct tcm_dev *tcm_dev,
		unsigned char id, unsigned short value,
		unsigned int delay_ms_resp);
int syna_tcm_detect_device(struct tcm_dev *tcm_dev);
int syna_tcm_reset(struct tcm_dev *tcm_dev);
int syna_tcm_sleep(struct tcm_dev *tcm_dev, bool en);
int syna_tcm_get_event_data(struct tcm_dev *tcm_dev,
		unsigned char *code,
		struct tcm_buffer *report);
int syna_tcm_get_app_info(struct tcm_dev *tcm_dev,
		struct tcm_application_info *app_info);

int syna_cdev_create_sysfs(struct syna_tcm *ptcm,
		struct platform_device *pdev);

void syna_cdev_remove_sysfs(struct syna_tcm *ptcm);

void syna_cdev_redirect_attn(struct syna_tcm *ptcm);

static inline void syna_pal_mutex_lock(struct mutex *ptr) {
	mutex_lock(ptr);
}

static inline void syna_pal_mutex_unlock(struct mutex *ptr) {
	mutex_unlock(ptr);
}

static inline int syna_pal_mem_cpy(void *dest, unsigned int dest_size, const void *src, unsigned int src_size, unsigned int num) {
	if (dest == NULL || src == NULL)
		return -1;

	if (num > dest_size || num > src_size)
		return -1;

	memcpy(dest, src, num);

	return 0;
}

static inline void *syna_pal_mem_alloc(unsigned int num, unsigned int size) {
	if ((int)(num * size) <= 0)
		return NULL;

	return kcalloc(num, size, GFP_KERNEL);
}

static inline int secure_memcpy(unsigned char *dest, unsigned int dest_size,
                                const unsigned char *src, unsigned int src_size,
                                unsigned int count)
{
    if (dest == NULL || src == NULL)
        return -EINVAL;

    if (count > dest_size || count > src_size) {
        pr_err("%s: src_size = %d, dest_size = %d, count = %d\n",
               __func__, src_size, dest_size, count);
        return -EINVAL;
    }

    memcpy((void *)dest, (const void *)src, count);

    return 0;
}

static inline unsigned short syna_tcm_crc16(unsigned char *p,
	unsigned int len, unsigned short val)
{
	unsigned short r = val;
	static unsigned short crc16_table[256] = {
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
		0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
		0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
		0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
		0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
		0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
		0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
		0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
		0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
		0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
		0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
		0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
		0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
		0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
		0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
		0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
		0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
		0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
		0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
		0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
		0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
		0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
		0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
		0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
		0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
		0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
		0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
		0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
		0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
		0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
	};

	if (len == 0)
		return r;

	while (len--)
		r = (r << 8) ^ crc16_table[(r >> 8) ^ *p++];

	return r;
}

static inline int syna_tcm_buf_realloc(struct tcm_buffer *pbuf,
		unsigned int size)
{
	int retval;
	unsigned char *temp_src;
	unsigned int temp_size = 0;

	if (!pbuf) 
		return -1;

	if (size > pbuf->buf_size) {
		temp_src = pbuf->buf;
		temp_size = pbuf->buf_size;

		pbuf->buf = syna_pal_mem_alloc(size, sizeof(unsigned char));
		if (!(pbuf->buf)) {
			kfree(temp_src);
			pbuf->buf_size = 0;
			return -1;
		}

		retval = syna_pal_mem_cpy(pbuf->buf,
				size,
				temp_src,
				temp_size,
				temp_size);
		if (retval < 0) {
			kfree(temp_src);
			kfree(pbuf->buf);
			pbuf->buf_size = 0;
			return retval;
		}

		kfree(temp_src);
		pbuf->buf_size = size;
	}

	return 0;
}

static inline int syna_tcm_buf_alloc(struct tcm_buffer *pbuf,
		unsigned int size)
{
	if (!pbuf)
		return -1;

	if (size > pbuf->buf_size) {
		if (pbuf->buf)
			kfree(pbuf->buf);

		pbuf->buf = syna_pal_mem_alloc(size, sizeof(unsigned char));
		if (!(pbuf->buf)) {
			pbuf->buf_size = 0;
			pbuf->data_length = 0;
			return -1;
		}
		pbuf->buf_size = size;
	}

	memset(pbuf->buf, 0x00, pbuf->buf_size);
	pbuf->data_length = 0;

	return 0;
}

static inline int syna_tcm_buf_copy(struct tcm_buffer *dest,
		struct tcm_buffer *src)
{
	int retval = 0;

	if (dest->buf_size < src->data_length) {
		retval = syna_tcm_buf_alloc(dest, src->data_length + 1);
		if (retval < 0)
			return retval;
	}

	retval = syna_pal_mem_cpy(dest->buf,
			dest->buf_size,
			src->buf,
			src->buf_size,
			src->data_length);
	if (retval < 0)
		return retval;

	dest->data_length = src->data_length;

	return retval;
}

static inline void syna_tcm_buf_lock(struct tcm_buffer *pbuf)
{
	syna_pal_mutex_lock(&pbuf->buf_mutex);
	pbuf->ref_cnt++;
}

static inline void syna_tcm_buf_unlock(struct tcm_buffer *pbuf)
{
	pbuf->ref_cnt--;
	syna_pal_mutex_unlock(&pbuf->buf_mutex);
}

static inline unsigned int syna_pal_le2_to_uint(const unsigned char *src) {
	return (unsigned int)src[0] + (unsigned int)src[1] * 0x100;
}

static inline int syna_pal_completion_wait_for(struct completion *ptr, unsigned int timeout_ms) {
	int retval;

	retval = wait_for_completion_timeout(ptr, msecs_to_jiffies(timeout_ms));
	if (retval == 0)
		return -1;

	return 0;
}

static inline void syna_tcm_buf_release(struct tcm_buffer *pbuf)
{
	kfree(pbuf->buf);
	pbuf->buf_size = 0;
	pbuf->data_length = 0;
	pbuf->ref_cnt = 0;
}

static inline void syna_tcm_buf_init(struct tcm_buffer *pbuf)
{
	pbuf->buf_size = 0;
	pbuf->data_length = 0;
	pbuf->ref_cnt = 0;
	pbuf->buf = NULL;
	mutex_init(&pbuf->buf_mutex);
}

static inline int syna_tcm_read(struct tcm_dev *tcm_dev,
	unsigned char *rd_data, unsigned int rd_len)
{
	struct syna_hw_interface *hw_if;

	if (!tcm_dev)
		return _EINVAL;

	hw_if = tcm_dev->hw_if;
	if (!hw_if->ops_read_data)
		return _ENODEV;

	return hw_if->ops_read_data(hw_if, rd_data, rd_len);
}

static inline int syna_tcm_write(struct tcm_dev *tcm_dev,
	unsigned char *wr_data, unsigned int wr_len)
{
	struct syna_hw_interface *hw_if;

	if (!tcm_dev)
		return _EINVAL;

	hw_if = tcm_dev->hw_if;
	if (!hw_if->ops_write_data)
		return _ENODEV;

	return hw_if->ops_write_data(hw_if, wr_data, wr_len);
}

static inline unsigned int syna_pal_ceil_div(unsigned int dividend, unsigned int divisor) {
	return (dividend + divisor - 1) / divisor;
}

static inline void syna_pal_completion_complete(struct completion *ptr) {
	complete(ptr);
}

static inline unsigned int syna_pal_le4_to_uint(const unsigned char *src) {
	return (unsigned int)src[0] + (unsigned int)src[1] * 0x100 + (unsigned int)src[2] * 0x10000 + (unsigned int)src[3] * 0x1000000;
}

void syna_cdev_update_report_queue(struct syna_tcm *tcm,
		unsigned char code, struct tcm_buffer *pevent_data);
void syna_cdev_update_power_state_report_queue(struct syna_tcm *tcm, bool wakeup);




#define INIT_BUFFER(buffer, is_clone) \
	mutex_init(&buffer.buf_mutex); \

#define LOCK_BUFFER(buffer) \
	mutex_lock(&buffer.buf_mutex)

#define UNLOCK_BUFFER(buffer) \
	mutex_unlock(&buffer.buf_mutex)

#define RELEASE_BUFFER(buffer) \
	do { \
        if (buffer.clone == false) { \
			kfree(buffer.buf); \
			buffer.buf_size = 0; \
			buffer.data_length = 0; \
        } \
	} while (0)

struct syna_tcm_report {
	unsigned char id;
	struct tcm_buffer buffer;
};

static inline int syna_tcm_alloc_mem(struct tcm_buffer *buffer,
					 unsigned int size)
{
	if (size > buffer->buf_size) {
		if (buffer->buf != NULL) {
			kfree(buffer->buf);
		}
		buffer->buf = kzalloc(size, GFP_KERNEL);

		if (!(buffer->buf)) {
			buffer->buf_size = 0;
			buffer->data_length = 0;
			return -ENOMEM;
		}
		buffer->buf_size = size;
	}

	memset(buffer->buf, 0, buffer->buf_size);
	buffer->data_length = 0;

	return 0;
}

static inline int tp_memcpy(void *dest, unsigned int dest_size,
			    void *src, unsigned int src_size,
			    unsigned int count)
{
	if (dest == NULL || src == NULL) {
		return -EINVAL;
	}

	if (count > dest_size || count > src_size) {
		return -EINVAL;
	}

	memcpy(dest, src, count);

	return 0;
}

#endif /* end of _SYNAPTICS_TCM2_DRIVER_H_ */

