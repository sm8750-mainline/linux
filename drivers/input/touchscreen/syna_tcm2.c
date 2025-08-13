// SPDX-License-Identifier: GPL-2.0
/*
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

#include "syna_tcm2.h"
#include <linux/sched/signal.h>
#include <linux/wait.h>

static DECLARE_WAIT_QUEUE_HEAD(state_waiter);

extern struct platform_device *syna_spi_device;

static void syna_dev_free_input_events(struct syna_tcm *tcm)
{
	struct input_dev *input_dev = tcm->input_dev;
	unsigned int idx;
	if (input_dev == NULL)
		return;
	syna_pal_mutex_lock(&tcm->tp_event_mutex);
	for (idx = 0; idx < MAX_NUM_OBJECTS; idx++) {
		input_mt_slot(input_dev, idx);
		input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 0);
	}
	input_report_key(input_dev, BTN_TOUCH, 0);
	input_report_key(input_dev, BTN_TOOL_FINGER, 0);
	input_sync(input_dev);
	syna_pal_mutex_unlock(&tcm->tp_event_mutex);

}

static void syna_dev_report_input_events(struct syna_tcm *tcm)
{
	unsigned int idx;
	unsigned int x;
	unsigned int y;
	int wx;
	int wy;
	unsigned int status;
	unsigned int touch_count;
	struct input_dev *input_dev = tcm->input_dev;
	unsigned int max_objects = tcm->tcm_dev->max_objects;
	struct tcm_touch_data_blob *touch_data;
	struct tcm_objects_data_blob *object_data;
	if (input_dev == NULL)
		return;
	syna_pal_mutex_lock(&tcm->tp_event_mutex);
	touch_data = &tcm->tp_data;
	object_data = &tcm->tp_data.object_data[0];

	if (tcm->pwr_state == LOW_PWR)
		goto exit;
	touch_count = 0;
	for (idx = 0; idx < max_objects; idx++) {
		if (tcm->prev_obj_status[idx] == LIFT &&
				object_data[idx].status == LIFT)
			status = NOP;
		else
			status = object_data[idx].status;

		switch (status) {
		case LIFT:
			input_mt_slot(input_dev, idx);
			input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 0);
			break;
		case FINGER:
		case GLOVED_OBJECT:
			x = object_data[idx].x_pos;
			y = object_data[idx].y_pos;
			wx = object_data[idx].x_width;
			wy = object_data[idx].y_width;
			input_mt_slot(input_dev, idx);
			input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 1);
			input_report_key(input_dev, BTN_TOUCH, 1);
			input_report_key(input_dev, BTN_TOOL_FINGER, 1);
			if (tcm->dts_max_x && tcm->dts_max_y) {
				input_report_abs(input_dev, ABS_MT_POSITION_X, x * tcm->dts_max_x / tcm->firmware_max_x);
				input_report_abs(input_dev, ABS_MT_POSITION_Y, y * tcm->dts_max_y / tcm->firmware_max_y);
			} else {
				input_report_abs(input_dev, ABS_MT_POSITION_X, x);
				input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
			}
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, MAX(wx, wy));
			input_report_abs(input_dev, ABS_MT_TOUCH_MINOR, MIN(wx, wy));
			touch_count++;
			break;
		default:
			break;
		}
		tcm->prev_obj_status[idx] = object_data[idx].status;
	}
	if (touch_count == 0) {
		input_report_key(input_dev, BTN_TOUCH, 0);
		input_report_key(input_dev, BTN_TOOL_FINGER, 0);
	}
	input_sync(input_dev);
exit:
	syna_pal_mutex_unlock(&tcm->tp_event_mutex);
}

static int syna_dev_create_input_device(struct syna_tcm *tcm)
{
	int retval = 0;
	struct tcm_dev *tcm_dev = tcm->tcm_dev;
	struct input_dev *input_dev = NULL;
	input_dev = input_allocate_device();
	if (input_dev == NULL)
		return -ENODEV;
	input_dev->name = TOUCH_INPUT_NAME;
	input_dev->phys = TOUCH_INPUT_PHYS_PATH;
	input_dev->id.product = SYNAPTICS_TCM_DRIVER_ID;
	input_dev->id.version = SYNAPTICS_TCM_DRIVER_VERSION;
	input_dev->dev.parent = tcm->pdev->dev.parent;
	input_set_drvdata(input_dev, tcm);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(ABS_MT_TOUCH_MAJOR, input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_TOOL_TYPE, input_dev->absbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
#endif
	set_bit(KEY_SLEEP, input_dev->keybit);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, tcm_dev->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, tcm_dev->max_y, 0, 0);
	input_mt_init_slots(input_dev, tcm_dev->max_objects, INPUT_MT_DIRECT);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_PROFILE, 0, 1, 0, 0);
	tcm->input_dev_params.max_x = tcm_dev->max_x;
	tcm->input_dev_params.max_y = tcm_dev->max_y;
	tcm->input_dev_params.max_objects = tcm_dev->max_objects;

	retval = input_register_device(input_dev);
	if (retval < 0) {
		input_free_device(input_dev);
		input_dev = NULL;
		return retval;
	}

	tcm->input_dev = input_dev;

	return 0;
}

static void syna_dev_release_input_device(struct syna_tcm *tcm)
{
	if (!tcm->input_dev)
		return;
	input_unregister_device(tcm->input_dev);
	tcm->input_dev = NULL;
}

static int syna_dev_set_up_input_device(struct syna_tcm *tcm)
{
	int retval = 0;
	if (IS_NOT_APP_FW_MODE(tcm->tcm_dev->dev_mode))
		return 0;
	syna_dev_free_input_events(tcm);
	syna_pal_mutex_lock(&tcm->tp_event_mutex);
	if (tcm->input_dev != NULL)
		syna_dev_release_input_device(tcm);
	retval = syna_dev_create_input_device(tcm);
	if (retval < 0)
		goto exit;

exit:
	syna_pal_mutex_unlock(&tcm->tp_event_mutex);
	return retval;
}

int syna_tcm_get_touch_data(const unsigned char *report,
		unsigned int report_size, unsigned int offset,
		unsigned int bits, unsigned int *data)
{
	unsigned char mask;
	unsigned char byte_data;
	unsigned int output_data;
	unsigned int bit_offset;
	unsigned int byte_offset;
	unsigned int data_bits;
	unsigned int available_bits;
	unsigned int remaining_bits;

	if (bits == 0 || bits > 32 || !report)
		return _EINVAL;

	if (offset + bits > report_size * 8) {
		*data = 0;
		return 0;
	}

	output_data = 0;
	remaining_bits = bits;

	bit_offset = offset % 8;
	byte_offset = offset / 8;

	while (remaining_bits) {
		byte_data = report[byte_offset];
		byte_data >>= bit_offset;

		available_bits = 8 - bit_offset;
		data_bits = MIN(available_bits, remaining_bits);
		mask = 0xff >> (8 - data_bits);

		byte_data &= mask;

		output_data |= byte_data << (bits - remaining_bits);

		bit_offset = 0;
		byte_offset += 1;
		remaining_bits -= data_bits;
	}

	*data = output_data;

	return 0;
}

int syna_tcm_get_event_data(struct tcm_dev *tcm_dev,
		unsigned char *code, struct tcm_buffer *data)
{
	int retval = 0;

	if (!tcm_dev || !code)
		return _EINVAL;

	retval = tcm_dev->read_message(tcm_dev,
			code);
	if (retval < 0)
		return retval;

	if (!data)
		goto exit;

	if (*code >= REPORT_IDENTIFY && *code != STATUS_INVALID) {
		if (tcm_dev->report_buf.data_length == 0)
			goto exit;

		syna_tcm_buf_lock(&tcm_dev->report_buf);

		retval = syna_tcm_buf_copy(data, &tcm_dev->report_buf);
		if (retval < 0) {
			syna_tcm_buf_unlock(&tcm_dev->report_buf);
			goto exit;
		}

		syna_tcm_buf_unlock(&tcm_dev->report_buf);
	}

	if (*code > STATUS_IDLE && *code <= STATUS_ERROR) {
		if (tcm_dev->resp_buf.data_length == 0)
			goto exit;

		syna_tcm_buf_lock(&tcm_dev->resp_buf);

		retval = syna_tcm_buf_copy(data, &tcm_dev->resp_buf);
		if (retval < 0) {
			syna_tcm_buf_unlock(&tcm_dev->resp_buf);
			goto exit;
		}

		syna_tcm_buf_unlock(&tcm_dev->resp_buf);
	}

exit:
	return retval;
}

static int syna_tcm_get_gesture_data(const unsigned char *report,
		unsigned int report_size, unsigned int offset,
		unsigned int bits, struct tcm_gesture_data_blob *gesture_data,
		unsigned int gesture_id)
{
	int retval;
	unsigned int idx;
	unsigned int data;
	unsigned int size;
	unsigned int data_end;

	if (!report)
		return _EINVAL;

	if (offset + bits > report_size * 8)
		return 0;

	data_end = offset + bits;

	size = (sizeof(gesture_data->data) / sizeof(unsigned char));

	idx = 0;
	while (offset < data_end && idx < size) {
		retval = syna_tcm_get_touch_data(report, report_size, offset, 16, &data);
		if (retval < 0) 
			return retval;
		gesture_data->data[idx++] = (unsigned char)(data & 0xff);
		gesture_data->data[idx++] = (unsigned char)((data >> 8) & 0xff);
		offset += 16;
	}

	return 0;
}

int syna_tcm_parse_touch_report(struct tcm_dev *tcm_dev,
		unsigned char *report, unsigned int report_size,
		struct tcm_touch_data_blob *touch_data)
{
	int retval;
	bool active_only;
	bool num_of_active_objects;
	unsigned char code;
	unsigned int size;
	unsigned int idx;
	unsigned int obj;
	unsigned int next;
	unsigned int data;
	unsigned int bits;
	unsigned int offset;
	unsigned int objects;
	unsigned int active_objects;
	unsigned int config_size;
	unsigned char *config_data;
	struct tcm_objects_data_blob *object_data;
	static unsigned int end_of_foreach;

	if (!tcm_dev || !report || report_size <= 0 || !touch_data || tcm_dev->max_objects == 0)
		return _EINVAL;

	object_data = touch_data->object_data;

	if (!object_data)
		return _EINVAL;

	config_data = tcm_dev->touch_config.buf;
	config_size = tcm_dev->touch_config.data_length;

	if (!config_data || config_size == 0)
		return _EINVAL;

	size = sizeof(touch_data->object_data);
	memset(touch_data->object_data, 0x00, size);

	num_of_active_objects = false;

	idx = 0;
	offset = 0;
	objects = 0;
	active_objects = 0;
	active_only = false;
	obj = 0;
	next = 0;

	while (idx < config_size) {
		code = config_data[idx++];
		switch (code) {
		case TOUCH_REPORT_END:
			goto exit;
		case TOUCH_REPORT_FOREACH_ACTIVE_OBJECT:
			obj = 0;
			next = idx;
			active_only = true;
			break;
		case TOUCH_REPORT_FOREACH_OBJECT:
			obj = 0;
			next = idx;
			active_only = false;
			break;
		case TOUCH_REPORT_FOREACH_END:
			end_of_foreach = idx;
			if (active_only) {
				if (num_of_active_objects) {
					objects++;
					obj++;
					if (objects < active_objects)
						idx = next;
				} else if (offset < report_size * 8) {
					obj++;
					idx = next;
				}
			} else {
				obj++;
				if (obj < tcm_dev->max_objects)
					idx = next;
			}
			break;
		case TOUCH_REPORT_PAD_TO_NEXT_BYTE:
			offset = syna_pal_ceil_div(offset, 8) * 8;
			break;
		case TOUCH_REPORT_TIMESTAMP:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->timestamp = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_INDEX:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			obj = data;
			touch_data->obji = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_CLASSIFICATION:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].status = (unsigned char)data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_X_POSITION:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].x_pos = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_Y_POSITION:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].y_pos = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_Z:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].z = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_X_WIDTH:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].x_width = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_Y_WIDTH:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].y_width = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_TX_POSITION_TIXELS:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].tx_pos = data;
			offset += bits;
			break;
		case TOUCH_REPORT_OBJECT_N_RX_POSITION_TIXELS:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			object_data[obj].rx_pos = data;
			offset += bits;
			break;
		case TOUCH_REPORT_NUM_OF_ACTIVE_OBJECTS:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			active_objects = data;
			num_of_active_objects = true;
			touch_data->num_of_active_objects = data;
			offset += bits;
			if (touch_data->num_of_active_objects == 0) {
				if (end_of_foreach == 0)
					return 0;

				idx = end_of_foreach;
			}
			break;
		case TOUCH_REPORT_0D_BUTTONS_STATE:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->buttons_state = data;
			offset += bits;
			break;
		case TOUCH_REPORT_GESTURE_ID:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report,
				report_size, offset, bits, &data);
			touch_data->gesture_id = data;
			offset += bits;
			if (retval < 0)
				return retval;
			break;
		case TOUCH_REPORT_GESTURE_DATA:
			bits = config_data[idx++];
			retval = syna_tcm_get_gesture_data(report,
					report_size,
					offset, bits,
					&touch_data->gesture_data,
					touch_data->gesture_id);
			offset += bits;
			if (retval < 0)
				return retval;
			break;
		case TOUCH_REPORT_FRAME_RATE:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->frame_rate = data;
			offset += bits;
			break;
		case TOUCH_REPORT_FORCE_MEASUREMENT:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->force_data = data;
			offset += bits;
			break;
		case TOUCH_REPORT_POWER_IM:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->power_im = data;
			offset += bits;
			break;
		case TOUCH_REPORT_CID_IM:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->cid_im = data;
			offset += bits;
			break;
		case TOUCH_REPORT_RAIL_IM:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->rail_im = data;
			offset += bits;
			break;
		case TOUCH_REPORT_CID_VARIANCE_IM:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->cid_variance_im = data;
			offset += bits;
			break;
		case TOUCH_REPORT_NSM_FREQUENCY_INDEX:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->nsm_frequency = data;
			offset += bits;
			break;
		case TOUCH_REPORT_NSM_STATE:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->nsm_state = data;
			offset += bits;
			break;
		case TOUCH_REPORT_SENSING_MODE:
			bits = config_data[idx++];
			retval = syna_tcm_get_touch_data(report, report_size,
					offset, bits, &data);
			if (retval < 0)
				return retval;
			touch_data->sensing_mode = data;
			offset += bits;
			break;
		default:
			bits = config_data[idx++];
			offset += bits;
			break;
		}
	}

exit:
	return 0;
}

static irqreturn_t syna_dev_isr(int irq, void *data)
{
	int retval;
	unsigned char code = 0;
	struct syna_tcm *tcm = data;
	struct syna_hw_attn_data *attn = &tcm->hw_if->bdata_attn;

	if (unlikely(gpio_get_value(attn->irq_gpio) != attn->irq_on_state))
		goto exit;

	tcm->isr_pid = current->pid;
	
	retval = syna_tcm_get_event_data(tcm->tcm_dev,
			&code,
			&tcm->event_data);
	if (retval < 0)
		goto exit;

	if (code == REPORT_TOUCH) {
		retval = syna_tcm_parse_touch_report(tcm->tcm_dev,
				tcm->event_data.buf,
				tcm->event_data.data_length,
				&tcm->tp_data);
		if (retval < 0)
			goto exit;
		syna_dev_report_input_events(tcm);
	}

exit:
	return IRQ_HANDLED;
}

static int syna_dev_request_irq(struct syna_tcm *tcm)
{
	int retval;
	struct syna_hw_attn_data *attn = &tcm->hw_if->bdata_attn;

	if (attn->irq_gpio < 0) {
		retval = -EINVAL;
		goto exit;
	}

	attn->irq_id = gpio_to_irq(attn->irq_gpio);
	retval = request_threaded_irq(attn->irq_id,
			NULL,
			syna_dev_isr,
			attn->irq_flags | IRQF_ONESHOT,
			PLATFORM_DRIVER_NAME,
			tcm);
	if (retval < 0)
		goto exit;

	attn->irq_enabled = true;
	tcm->bus_ready = true;

exit:
	return retval;
}

static void syna_dev_release_irq(struct syna_tcm *tcm)
{
	struct syna_hw_attn_data *attn = &tcm->hw_if->bdata_attn;

	if (attn->irq_id <= 0)
		return;

	if (tcm->hw_if->ops_enable_irq)
		tcm->hw_if->ops_enable_irq(tcm->hw_if, false);

	free_irq(attn->irq_id, tcm);

	attn->irq_id = 0;
	attn->irq_enabled = false;
}

int syna_tcm_preserve_touch_report_config(struct tcm_dev *tcm_dev)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned int size = 0;

	if (!tcm_dev || IS_NOT_APP_FW_MODE(tcm_dev->dev_mode))
		return _EINVAL;

	retval = tcm_dev->write_message(tcm_dev,
			CMD_GET_TOUCH_REPORT_CONFIG,
			NULL,
			0,
			&resp_code,
			tcm_dev->msg_data.default_resp_reading);
	if (retval < 0)
		goto exit;

	syna_tcm_buf_lock(&tcm_dev->resp_buf);

	size = tcm_dev->resp_buf.data_length;
	retval = syna_tcm_buf_alloc(&tcm_dev->touch_config,
			size);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_dev->resp_buf);
		goto exit;
	}

	syna_tcm_buf_lock(&tcm_dev->touch_config);

	retval = syna_pal_mem_cpy(tcm_dev->touch_config.buf,
			tcm_dev->touch_config.buf_size,
			tcm_dev->resp_buf.buf,
			tcm_dev->resp_buf.buf_size,
			size);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_dev->touch_config);
		syna_tcm_buf_unlock(&tcm_dev->resp_buf);
		goto exit;
	}

	tcm_dev->touch_config.data_length = size;

	syna_tcm_buf_unlock(&tcm_dev->touch_config);
	syna_tcm_buf_unlock(&tcm_dev->resp_buf);

exit:
	return retval;
}

int syna_tcm_get_app_info(struct tcm_dev *tcm_dev,
		struct tcm_application_info *app_info)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned int app_status;
	unsigned int resp_data_len = 0;
	unsigned int copy_size;
	struct tcm_application_info *info;

	if (!tcm_dev)
		return _EINVAL;

	if (IS_NOT_APP_FW_MODE(tcm_dev->dev_mode))
		return _EINVAL;

	retval = tcm_dev->write_message(tcm_dev,
			CMD_GET_APPLICATION_INFO,
			NULL,
			0,
			&resp_code,
			tcm_dev->msg_data.default_resp_reading);
	if (retval < 0)
		goto exit;

	resp_data_len = tcm_dev->resp_buf.data_length;
	copy_size = MIN(sizeof(tcm_dev->app_info), resp_data_len);

	info = &tcm_dev->app_info;

	retval = syna_pal_mem_cpy((unsigned char *)info,
			sizeof(struct tcm_application_info),
			tcm_dev->resp_buf.buf,
			tcm_dev->resp_buf.buf_size,
			copy_size);
	if (retval < 0)
		goto exit;

	if (app_info == NULL)
		goto show_info;

	retval = syna_pal_mem_cpy((unsigned char *)app_info,
			sizeof(struct tcm_application_info),
			tcm_dev->resp_buf.buf,
			tcm_dev->resp_buf.buf_size,
			copy_size);
	if (retval < 0)
		goto exit;

show_info:
	app_status = syna_pal_le2_to_uint(tcm_dev->app_info.status);

	if (app_status == APP_STATUS_BAD_APP_CONFIG) {
		retval = _ENODEV;
		goto exit;
	} else if (app_status != APP_STATUS_OK) {
		retval = _ENODEV;
		goto exit;
	}

	tcm_dev->max_objects = syna_pal_le2_to_uint(info->max_objects);
	tcm_dev->max_x = syna_pal_le2_to_uint(info->max_x);
	tcm_dev->max_y = syna_pal_le2_to_uint(info->max_y);

	tcm_dev->cols = syna_pal_le2_to_uint(info->num_of_image_cols);
	tcm_dev->rows = syna_pal_le2_to_uint(info->num_of_image_rows);
	syna_pal_mem_cpy((unsigned char *)tcm_dev->config_id,
			MAX_SIZE_CONFIG_ID,
			info->customer_config_id,
			MAX_SIZE_CONFIG_ID,
			MAX_SIZE_CONFIG_ID);

exit:
	return retval;
}

static int syna_dev_set_up_app_fw(struct syna_tcm *tcm)
{
	int retval = 0;
	struct tcm_dev *tcm_dev = tcm->tcm_dev;

	if (IS_NOT_APP_FW_MODE(tcm_dev->dev_mode))
		return -EINVAL;
	
	retval = syna_tcm_get_app_info(tcm_dev, &tcm_dev->app_info);
	if (retval < 0)
			return -EIO;
	if (tcm->dts_max_x && tcm->dts_max_y) {
		tcm->firmware_max_x = tcm_dev->max_x;
		tcm->firmware_max_y = tcm_dev->max_y;
		tcm_dev->max_x = tcm->dts_max_x;
		tcm_dev->max_y = tcm->dts_max_y;
	}
	retval = syna_tcm_preserve_touch_report_config(tcm_dev);
	if (retval < 0)
		return -EIO;

	return retval;
}

int syna_tcm_sleep(struct tcm_dev *tcm_dev, bool en)
{
	int retval = 0;
	unsigned char resp_code;
	unsigned char command;

	if (!tcm_dev)
		return _EINVAL;

	command = (en) ? CMD_ENTER_DEEP_SLEEP : CMD_EXIT_DEEP_SLEEP;

	retval = tcm_dev->write_message(tcm_dev,
			command,
			NULL,
			0,
			&resp_code,
			tcm_dev->msg_data.default_resp_reading);
	if (retval < 0)
		goto exit;

	tcm_dev->is_sleep = en;
	retval = 0;
exit:
	return retval;
}

static int syna_dev_enter_normal_sensing(struct syna_tcm *tcm)
{
	int retval = 0;

	if (!tcm)
		return -EINVAL;

	retval = syna_tcm_sleep(tcm->tcm_dev, false);
	if (retval < 0)
		return retval;

	return 0;
}

static int syna_dev_resume(struct device *dev)
{
	struct syna_tcm *tcm = dev_get_drvdata(dev);
	queue_work(tcm->speedup_resume_wq, &tcm->speed_up_work);
	return 0;
}

static void syna_speedup_resume(struct work_struct *work)
{
	struct syna_tcm *tcm = container_of(work, struct syna_tcm,
				     speed_up_work);
	int retval;
	struct syna_hw_interface *hw_if = tcm->hw_if;
	bool irq_enabled = true;

	if (tcm->pwr_state == PWR_ON) {
		tcm->sub_pwr_state = SUB_PWR_RESUME_DONE;
		return;
	}
	mutex_lock(&tcm->mutex);

	if (IS_REMOVE == tcm->driver_current_state) {
		mutex_unlock(&tcm->mutex);
		return;
	};

	tcm->sub_pwr_state = SUB_PWR_RESUMING;

	irq_enabled = (!hw_if->bdata_attn.irq_enabled);

	if (irq_enabled && (hw_if->ops_enable_irq))
		hw_if->ops_enable_irq(hw_if, true);
	
	retval = syna_dev_enter_normal_sensing(tcm);
	if (retval < 0)
		goto exit;
	
	tcm->pwr_state = PWR_ON;

	retval = syna_dev_set_up_app_fw(tcm);
	if (retval < 0)
		goto exit;

	retval = 0;

exit:
	tcm->sub_pwr_state = SUB_PWR_RESUME_DONE;
	tcm->slept_in_early_suspend = false;

	mutex_unlock(&tcm->mutex);
}

static int syna_dev_suspend(struct device *dev)
{
	struct syna_tcm *tcm = dev_get_drvdata(dev);
	struct syna_hw_interface *hw_if = tcm->hw_if;
	struct touchpanel_event event_data;
	bool irq_disabled = true;

	if (tcm->pwr_state != PWR_ON)
		return 0;
	mutex_lock(&tcm->mutex);

	if (IS_REMOVE == tcm->driver_current_state) {
		mutex_unlock(&tcm->mutex);
		return -EINVAL;
	};

	tcm->sub_pwr_state = SUB_PWR_SUSPENDING;
	syna_dev_free_input_events(tcm);
	tcm->pwr_state = PWR_OFF;
	memset(&event_data, 0, sizeof(struct touchpanel_event));
	irq_disabled = (!tcm->lpwg_enabled);

	if (irq_disabled && (hw_if->ops_enable_irq))
		hw_if->ops_enable_irq(hw_if, false);

	tcm->sub_pwr_state = SUB_PWR_SUSPEND_DONE;
	mutex_unlock(&tcm->mutex);

	return 0;
}

static int syna_dev_disconnect(struct syna_tcm *tcm)
{
	struct syna_hw_interface *hw_if = tcm->hw_if;
	int ret = 0;

	if (tcm->is_connected == false)
		return 0;

	cancel_delayed_work_sync(&tcm->reflash_work);

	if (hw_if->bdata_attn.irq_id)
		syna_dev_release_irq(tcm);

	if (hw_if->bdata_rst.reset_gpio >= 0) {
		ret = gpio_direction_output(hw_if->bdata_rst.reset_gpio, 0);
		if (ret)
			return -EINVAL;
	}

	syna_dev_release_input_device(tcm);

	tcm->input_dev_params.max_x = 0;
	tcm->input_dev_params.max_y = 0;
	tcm->input_dev_params.max_objects = 0;

	if (hw_if->ops_power_on)
		hw_if->ops_power_on(hw_if, false);

	tcm->pwr_state = PWR_OFF;
	tcm->is_connected = false;

	return 0;
}

static int syna_tcm_v1_read(struct tcm_dev *tcm_dev, unsigned int rd_length,
		unsigned char *buf, unsigned int buf_size, bool extra_crc)
{
	int retval;
	unsigned int max_rd_size;
	int retry;

	if (!tcm_dev || IS_ERR_OR_NULL(buf) || rd_length > buf_size)
		return _EINVAL;

	if (rd_length == 0)
		return 0;

	max_rd_size = tcm_dev->max_rd_size;

	if (max_rd_size != 0 && rd_length > max_rd_size) {
		return _EINVAL;
	}

	for (retry = 0; retry < 10; retry++) {
		retval = syna_tcm_read(tcm_dev,
				buf,
				rd_length);
		if (retval < 0)
			goto exit;

		if (buf[0] == TCM_V1_MESSAGE_MARKER)
			break;

		retval = _EIO;
		usleep_range(RD_RETRY_US_MIN, RD_RETRY_US_MAX);
	}

exit:
	return retval;
}

static int syna_tcm_v1_continued_read(struct tcm_dev *tcm_dev,
		unsigned int length)
{
	int retval = 0;
	unsigned char code;
	unsigned int idx;
	unsigned int offset;
	unsigned int chunks;
	unsigned int chunk_space;
	unsigned int xfer_length;
	unsigned int total_length;
	unsigned int remaining_length;
	struct tcm_message_data_blob *tcm_msg = NULL;
	bool last = false;

	if (!tcm_dev)
		return _EINVAL;

	tcm_msg = &tcm_dev->msg_data;

	if (length == 0 || tcm_msg->payload_length == 0)
		return 0;

	if ((length & 0xffff) == 0xffff)
		return _EINVAL;

	total_length = MESSAGE_HEADER_SIZE + tcm_msg->payload_length + 1;
	remaining_length = length + 1;

	if (tcm_msg->has_crc) {
		total_length += TCM_MSG_CRC_LENGTH;
		remaining_length += TCM_MSG_CRC_LENGTH;
	}
	if (tcm_msg->has_extra_rc) {
		total_length += TCM_EXTRA_RC_LENGTH;
		remaining_length += TCM_EXTRA_RC_LENGTH;
	}
	if (tcm_msg->has_crc || tcm_msg->has_extra_rc) {
		total_length += 1;
		remaining_length += 1;
	}

	syna_tcm_buf_lock(&tcm_msg->in);

	retval = syna_tcm_buf_realloc(&tcm_msg->in, total_length + 1);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_msg->in);
		return retval;
	}

	if (tcm_dev->max_rd_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = tcm_dev->max_rd_size - 2;

	chunks = syna_pal_ceil_div(remaining_length, chunk_space);
	chunks = chunks == 0 ? 1 : chunks;

	offset = MESSAGE_HEADER_SIZE + (tcm_msg->payload_length - length);

	syna_tcm_buf_lock(&tcm_msg->temp);

	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		last = ((idx + 1) == chunks);

		if (xfer_length == 1) {
			tcm_msg->in.buf[offset] = TCM_V1_MESSAGE_PADDING;
			offset += xfer_length;
			remaining_length -= xfer_length;
			continue;
		}

		retval = syna_tcm_buf_alloc(&tcm_msg->temp, xfer_length + 2);
		if (retval < 0)
			goto exit;
		
		retval = syna_tcm_v1_read(tcm_dev,
				xfer_length + 2,
				tcm_msg->temp.buf,
				tcm_msg->temp.buf_size,
				(tcm_msg->has_crc) && last);
		if (retval < 0)
			goto exit;

		tcm_msg->temp.data_length = xfer_length + 2;

		code = tcm_msg->temp.buf[1];

		if (code != STATUS_CONTINUED_READ) {
			retval = _EIO;
			goto exit;
		}

		retval = syna_pal_mem_cpy(&tcm_msg->in.buf[offset],
				tcm_msg->in.buf_size - offset,
				&tcm_msg->temp.buf[2],
				tcm_msg->temp.buf_size - 2,
				xfer_length);
		if (retval < 0)
			goto exit;

		offset += xfer_length;
		remaining_length -= xfer_length;
	}

exit:
	syna_tcm_buf_unlock(&tcm_msg->temp);
	syna_tcm_buf_unlock(&tcm_msg->in);

	return retval;
}

static int syna_tcm_v1_parse_idinfo(struct tcm_dev *tcm_dev,
		unsigned char *data, unsigned int size, unsigned int data_len)
{
	int retval;
	unsigned int build_id = 0;
	struct tcm_identification_info *id_info;

	if (!tcm_dev || !data || data_len == 0)
		return _EINVAL;

	id_info = &tcm_dev->id_info;

	retval = syna_pal_mem_cpy((unsigned char *)id_info,
			sizeof(struct tcm_identification_info),
			data,
			size,
			MIN(sizeof(*id_info), data_len));
	if (retval < 0)
		return retval;

	build_id = syna_pal_le4_to_uint(id_info->build_id);

	if (tcm_dev->packrat_number != build_id)
		tcm_dev->packrat_number = build_id;

	tcm_dev->dev_mode = id_info->mode;

	return 0;
}

static void syna_tcm_v1_dispatch_response(struct tcm_dev *tcm_dev)
{
	int retval;
	struct tcm_message_data_blob *tcm_msg = NULL;
	struct completion *cmd_completion = NULL;

	if (!tcm_dev)
		return;

	tcm_msg = &tcm_dev->msg_data;
	cmd_completion = &tcm_msg->cmd_completion;

	tcm_msg->response_code = tcm_msg->status_report_code;

	if (ATOMIC_GET(tcm_msg->command_status) != CMD_STATE_BUSY)
		return;

	if (tcm_msg->payload_length == 0) {
		tcm_dev->resp_buf.data_length = tcm_msg->payload_length;
		ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);
		goto exit;
	}

	syna_tcm_buf_lock(&tcm_dev->resp_buf);

	retval = syna_tcm_buf_alloc(&tcm_dev->resp_buf, tcm_msg->payload_length);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_dev->resp_buf);
		goto exit;
	}

	syna_tcm_buf_lock(&tcm_msg->in);

	retval = syna_pal_mem_cpy(tcm_dev->resp_buf.buf,
			tcm_dev->resp_buf.buf_size,
			&tcm_msg->in.buf[MESSAGE_HEADER_SIZE],
			tcm_msg->in.buf_size - MESSAGE_HEADER_SIZE,
			tcm_msg->payload_length);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_msg->in);
		syna_tcm_buf_unlock(&tcm_dev->resp_buf);
		goto exit;
	}

	tcm_dev->resp_buf.data_length = tcm_msg->payload_length;

	syna_tcm_buf_unlock(&tcm_msg->in);
	syna_tcm_buf_unlock(&tcm_dev->resp_buf);

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);

exit:
	syna_pal_completion_complete(cmd_completion);
}

static void syna_tcm_v1_dispatch_report(struct tcm_dev *tcm_dev)
{
	int retval;
	struct tcm_message_data_blob *tcm_msg = NULL;
	struct completion *cmd_completion = NULL;

	if (!tcm_dev)
		return;

	tcm_msg = &tcm_dev->msg_data;
	cmd_completion = &tcm_msg->cmd_completion;

	tcm_msg->report_code = tcm_msg->status_report_code;

	if (tcm_msg->payload_length == 0) {
		tcm_dev->report_buf.data_length = 0;
		goto exit;
	}

	syna_tcm_buf_lock(&tcm_dev->report_buf);

	retval = syna_tcm_buf_alloc(&tcm_dev->report_buf,
			tcm_msg->payload_length);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_dev->report_buf);
		goto exit;
	}

	syna_tcm_buf_lock(&tcm_msg->in);

	retval = syna_pal_mem_cpy(tcm_dev->report_buf.buf,
			tcm_dev->report_buf.buf_size,
			&tcm_msg->in.buf[MESSAGE_HEADER_SIZE],
			tcm_msg->in.buf_size - MESSAGE_HEADER_SIZE,
			tcm_msg->payload_length);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_msg->in);
		syna_tcm_buf_unlock(&tcm_dev->report_buf);
		goto exit;
	}

	tcm_dev->report_buf.data_length = tcm_msg->payload_length;

	syna_tcm_buf_unlock(&tcm_msg->in);
	syna_tcm_buf_unlock(&tcm_dev->report_buf);

	if (tcm_msg->report_code == REPORT_IDENTIFY) {
		tcm_dev->is_sleep = false; 
		syna_tcm_buf_lock(&tcm_msg->in);

		retval = syna_tcm_v1_parse_idinfo(tcm_dev,
				&tcm_msg->in.buf[MESSAGE_HEADER_SIZE],
				tcm_msg->in.buf_size - MESSAGE_HEADER_SIZE,
				tcm_msg->payload_length);
		if (retval < 0) {
			syna_tcm_buf_unlock(&tcm_msg->in);
			return;
		}

		syna_tcm_buf_unlock(&tcm_msg->in);

		if (tcm_dev->dev_mode == 0x0b) {
			tcm_dev->firmware_mode_count++;
			if (!tcm_dev->upload_flag && tcm_dev->firmware_mode_count >= FIRMWARE_MODE_BL_MAX) {
				tcm_dev->upload_flag = 1;
			}
		}
		if (ATOMIC_GET(tcm_msg->command_status) == CMD_STATE_BUSY) {
			switch (tcm_msg->command) {
			case CMD_RESET:
				tcm_msg->response_code = STATUS_OK;
				ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);
				syna_pal_completion_complete(cmd_completion);
				goto exit;
			case CMD_REBOOT_TO_ROM_BOOTLOADER:
			case CMD_RUN_BOOTLOADER_FIRMWARE:
			case CMD_RUN_APPLICATION_FIRMWARE:
			case CMD_ENTER_PRODUCTION_TEST_MODE:
			case CMD_ROMBOOT_RUN_BOOTLOADER_FIRMWARE:
				tcm_msg->response_code = STATUS_OK;
				ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);
				syna_pal_completion_complete(cmd_completion);
				goto exit;
			default:
				ATOMIC_SET(tcm_msg->command_status, CMD_STATE_ERROR);
				syna_pal_completion_complete(cmd_completion);
				goto exit;
			}
		}
	}

exit:
	return;
}

static void syna_tcm_v1_update_crc(struct tcm_dev *tcm_dev)
{
	struct tcm_message_data_blob *tcm_msg = NULL;
	int offset;

	if (!tcm_dev)
		return;

	tcm_msg = &tcm_dev->msg_data;

	if (!tcm_msg->has_crc || tcm_msg->payload_length == 0)
		return;

	syna_tcm_buf_lock(&tcm_msg->in);

	offset = MESSAGE_HEADER_SIZE + tcm_msg->payload_length;
	if (tcm_msg->in.buf_size <= offset + 1)
		return;

	if (tcm_msg->has_crc) {
		tcm_msg->crc_bytes = (unsigned short)syna_pal_le2_to_uint(&tcm_msg->in.buf[offset + 1]);

		if (tcm_msg->has_extra_rc) {
			offset += TCM_MSG_CRC_LENGTH;
			if (tcm_msg->in.buf_size >= offset + 1) {
				tcm_msg->rc_byte = tcm_msg->in.buf[offset + 1];
			}
		}
	}

	syna_tcm_buf_unlock(&tcm_msg->in);
}

static int syna_tcm_v1_read_message(struct tcm_dev *tcm_dev,
		unsigned char *status_report_code)
{
	int retval = 0;
	struct tcm_v1_message_header *header;
	struct tcm_message_data_blob *tcm_msg = NULL;
	struct mutex *rw_mutex = NULL;
	struct completion *cmd_completion = NULL;
	unsigned int len = 0;
	bool do_predict = false;
	unsigned int tmp_len;

	if (!tcm_dev)
		return _EINVAL;

	tcm_msg = &tcm_dev->msg_data;
	rw_mutex = &tcm_msg->rw_mutex;
	cmd_completion = &tcm_msg->cmd_completion;

	do_predict = (ATOMIC_GET(tcm_msg->command_status) == CMD_STATE_IDLE);

	if (status_report_code)
		*status_report_code = STATUS_INVALID;

	syna_pal_mutex_lock(rw_mutex);

	syna_tcm_buf_lock(&tcm_msg->in);

	len = MESSAGE_HEADER_SIZE;

	if (tcm_msg->predict_reads && do_predict) {
		if (tcm_msg->predict_length > 0) {
			len += tcm_msg->predict_length;
			if (tcm_msg->has_crc)
				len += TCM_MSG_CRC_LENGTH;
			if (tcm_msg->has_extra_rc)
				len += TCM_EXTRA_RC_LENGTH;
			len += 1;
		}
	}

	if (len > tcm_msg->in.buf_size) {
		retval = syna_tcm_buf_alloc(&tcm_msg->in, len);
		if (retval < 0) {
			syna_tcm_buf_unlock(&tcm_msg->in);

			tcm_msg->status_report_code = STATUS_INVALID;
			tcm_msg->payload_length = 0;
			goto exit;
		}
	}

	retval = syna_tcm_v1_read(tcm_dev,
			len,
			tcm_msg->in.buf,
			tcm_msg->in.buf_size,
			false);
	if (retval < 0) {
		syna_tcm_buf_unlock(&tcm_msg->in);

		tcm_msg->status_report_code = STATUS_INVALID;
		tcm_msg->payload_length = 0;
		goto exit;
	}

	header = (struct tcm_v1_message_header *)tcm_msg->in.buf;

	tcm_msg->status_report_code = header->code;

	tcm_msg->payload_length = syna_pal_le2_to_uint(header->length);

	syna_tcm_buf_unlock(&tcm_msg->in);

	if (tcm_msg->payload_length == 0)
		goto do_dispatch;

	tmp_len = len - MESSAGE_HEADER_SIZE;
	if (len > MESSAGE_HEADER_SIZE)
		len = (tcm_msg->payload_length > tmp_len) ?
			(tcm_msg->payload_length - tmp_len) : 0;
	else
		len = tcm_msg->payload_length;

	retval = syna_tcm_v1_continued_read(tcm_dev, len);
	if (retval < 0)
		goto exit;

do_dispatch:
	syna_tcm_buf_lock(&tcm_msg->in);

	tcm_msg->in.buf[0] = TCM_V1_MESSAGE_MARKER;
	tcm_msg->in.buf[1] = tcm_msg->status_report_code;
	tcm_msg->in.buf[2] = (unsigned char)tcm_msg->payload_length;
	tcm_msg->in.buf[3] = (unsigned char)(tcm_msg->payload_length >> 8);

	syna_tcm_buf_unlock(&tcm_msg->in);

	syna_tcm_v1_update_crc(tcm_dev);

	syna_tcm_buf_lock(&tcm_dev->external_buf);
	if (tcm_msg->payload_length > 0) {
		retval = syna_tcm_buf_alloc(&tcm_dev->external_buf,
				tcm_msg->payload_length);
		if (retval >= 0) {
			retval = syna_pal_mem_cpy(&tcm_dev->external_buf.buf[0],
				tcm_msg->payload_length,
				&tcm_msg->in.buf[MESSAGE_HEADER_SIZE],
				tcm_msg->in.buf_size - MESSAGE_HEADER_SIZE,
				tcm_msg->payload_length);
		}
	}
	tcm_dev->external_buf.data_length = tcm_msg->payload_length;
	syna_tcm_buf_unlock(&tcm_dev->external_buf);

	if (tcm_msg->status_report_code <= STATUS_ERROR ||
		tcm_msg->status_report_code == STATUS_INVALID) {
		switch (tcm_msg->status_report_code) {
		case STATUS_CONTINUED_READ:
			retval = _EIO;
			goto exit;
		case STATUS_IDLE:
			retval = 0;
			goto exit;
		default:
			break;
		}
	}

	if (tcm_msg->status_report_code >= REPORT_IDENTIFY)
		syna_tcm_v1_dispatch_report(tcm_dev);
	else
		syna_tcm_v1_dispatch_response(tcm_dev);

	if (status_report_code)
		*status_report_code = tcm_msg->status_report_code;

	if (tcm_msg->predict_reads && do_predict) {
		if (tcm_dev->max_rd_size < MESSAGE_HEADER_SIZE)
			tcm_msg->predict_length = tcm_msg->payload_length;
		else
			tcm_msg->predict_length = MIN(tcm_msg->payload_length,
				tcm_dev->max_rd_size - MESSAGE_HEADER_SIZE - 1);

		if (tcm_msg->status_report_code < REPORT_IDENTIFY)
			tcm_msg->predict_length = 0;
	}

	retval = 0;

exit:
	if (retval < 0) {
		if (ATOMIC_GET(tcm_msg->command_status) == CMD_STATE_BUSY) {
			ATOMIC_SET(tcm_msg->command_status, CMD_STATE_ERROR);
			complete(cmd_completion);
		}
	}

	syna_pal_mutex_unlock(rw_mutex);

	return retval;
}

static int syna_tcm_v1_write(struct tcm_dev *tcm_dev, unsigned char command,
		unsigned char *payload, unsigned int payload_len,
		bool extra_crc, unsigned short crc)
{
	int retval = 0;
	struct tcm_message_data_blob *tcm_msg = NULL;
	int size, buf_size;
	unsigned char crc16[TCM_MSG_CRC_LENGTH] = { 0 };

	if (!tcm_dev)
		return _EINVAL;

	tcm_msg = &tcm_dev->msg_data;

	syna_tcm_buf_lock(&tcm_msg->out);

	buf_size = payload_len + 3;
	if (extra_crc) {
		crc16[0] = (unsigned char) crc & 0xff;
		crc16[1] = (unsigned char) (crc >> 8);

		buf_size += sizeof(crc16);
	}

	retval = syna_tcm_buf_alloc(&tcm_msg->out, buf_size);
	if (retval < 0)
		goto exit;

	if (command != CMD_CONTINUE_WRITE) {
		size = payload_len + 3;

		tcm_msg->out.buf[0] = command;
		tcm_msg->out.buf[1] = (unsigned char)payload_len;
		tcm_msg->out.buf[2] = (unsigned char)(payload_len >> 8);

		if (payload_len > 0) {
			retval = syna_pal_mem_cpy(&tcm_msg->out.buf[3],
					tcm_msg->out.buf_size - 3,
					payload,
					payload_len,
					payload_len);
			if (retval < 0)
				goto exit;
		}
	} else {
		size = payload_len + 1;

		tcm_msg->out.buf[0] = CMD_CONTINUE_WRITE;

		retval = syna_pal_mem_cpy(&tcm_msg->out.buf[1],
				tcm_msg->out.buf_size - 1,
				payload,
				payload_len,
				payload_len);
		if (retval < 0)
			goto exit;
	}

	if (extra_crc) {
		retval = syna_pal_mem_cpy(&tcm_msg->out.buf[size],
				tcm_msg->out.buf_size - size,
				crc16,
				sizeof(crc16),
				sizeof(crc16)
				);
		if (retval < 0)
			goto exit;

		size += sizeof(crc16);
	}

	retval = syna_tcm_write(tcm_dev,
			tcm_msg->out.buf,
			size
			);
	if (retval < 0)
		goto exit;

exit:
	syna_tcm_buf_unlock(&tcm_msg->out);

	return retval;
}

static int syna_tcm_v1_write_message(struct tcm_dev *tcm_dev,
	unsigned char command, unsigned char *payload,
	unsigned int payload_len, unsigned char *resp_code,
	unsigned int delay_ms_resp)
{
	int retval = 0;
	unsigned int idx;
	unsigned int chunks = 0;
	unsigned int chunk_space;
	unsigned int xfer_length;
	unsigned int remaining_length;
	int timeout = 0;
	int polling_ms = 0;
	struct tcm_message_data_blob *tcm_msg = NULL;
	struct mutex *cmd_mutex = NULL;
	struct mutex *rw_mutex = NULL;
	struct completion *cmd_completion = NULL;
	bool has_irq_ctrl = false;
	bool in_polling = false;
	bool last = false;
	unsigned char tmp;
	unsigned short crc16 = 0xFFFF;

	if (!tcm_dev)
		return _EINVAL;

	tcm_msg = &tcm_dev->msg_data;
	cmd_mutex = &tcm_msg->cmd_mutex;
	rw_mutex = &tcm_msg->rw_mutex;
	cmd_completion = &tcm_msg->cmd_completion;

	if (resp_code)
		*resp_code = STATUS_INVALID;

	in_polling = (delay_ms_resp != RESP_IN_ATTN);

	has_irq_ctrl = (bool)(tcm_dev->hw_if->ops_enable_irq != NULL);
	has_irq_ctrl &= tcm_dev->hw_if->bdata_attn.irq_enabled;

	if (has_irq_ctrl && in_polling && tcm_dev->hw_if->ops_enable_irq)
		tcm_dev->hw_if->ops_enable_irq(tcm_dev->hw_if, false);

	syna_pal_mutex_lock(cmd_mutex);

	syna_pal_mutex_lock(rw_mutex);

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_BUSY);

	reinit_completion(cmd_completion);

	tcm_msg->command = command;

	remaining_length = payload_len;

	if (tcm_msg->has_crc) {
		crc16 = syna_tcm_crc16(&command, 1, crc16);
		tmp = (unsigned char)payload_len & 0xff;
		crc16 = syna_tcm_crc16(&tmp, 1, crc16);
		tmp = (unsigned char)(payload_len >> 8) & 0xff;
		crc16 = syna_tcm_crc16(&tmp, 1, crc16);
		if (payload_len > 0)
			crc16 = syna_tcm_crc16(payload, payload_len, crc16);
	}

	if (tcm_dev->max_wr_size == 0)
		chunk_space = remaining_length;
	else
		chunk_space = tcm_dev->max_wr_size - 1;

	if (chunk_space)
		chunks = syna_pal_ceil_div(remaining_length, chunk_space);

	chunks = chunks == 0 ? 1 : chunks;

	for (idx = 0; idx < chunks; idx++) {
		if (remaining_length > chunk_space)
			xfer_length = chunk_space;
		else
			xfer_length = remaining_length;

		last = ((idx + 1) == chunks);

		if (idx == 0) {
			retval = syna_tcm_v1_write(tcm_dev,
					tcm_msg->command,
					&payload[0],
					xfer_length,
					(tcm_msg->has_crc) && last,
					crc16);
		} else {
			retval = syna_tcm_v1_write(tcm_dev,
					CMD_CONTINUE_WRITE,
					&payload[idx * chunk_space],
					xfer_length,
					(tcm_msg->has_crc) && last,
					crc16);
		}

		if (retval < 0) {
			syna_pal_mutex_unlock(rw_mutex);
			goto exit;
		}

		remaining_length -= xfer_length;

		if (chunks > 1)
			usleep_range(WR_DELAY_US_MIN, WR_DELAY_US_MAX);
	}

	syna_pal_mutex_unlock(rw_mutex);

	timeout = 0;
	if (!in_polling)
		polling_ms = CMD_RESPONSE_TIMEOUT_MS;
	else
		polling_ms = delay_ms_resp;

	do {
		retval = syna_pal_completion_wait_for(cmd_completion,
				polling_ms);

		if (ATOMIC_GET(tcm_msg->command_status) == CMD_STATE_IDLE)
			goto check_response;
		if (in_polling) {
			ATOMIC_SET(tcm_msg->command_status, CMD_STATE_BUSY);

			retval = syna_tcm_v1_read_message(tcm_dev, NULL);
			if (retval < 0)
				reinit_completion(cmd_completion);
		}

		timeout += polling_ms;

	} while (timeout < CMD_RESPONSE_TIMEOUT_MS);

check_response:
	if (ATOMIC_GET(tcm_msg->command_status) != CMD_STATE_IDLE) {
		if (timeout >= CMD_RESPONSE_TIMEOUT_MS) {
			retval = _ETIMEDOUT;
			goto exit;
		} else {
			retval = _EIO;
			goto exit;
		}
	}

	if (resp_code)
		*resp_code = tcm_msg->status_report_code;

	if (tcm_msg->response_code != STATUS_OK) {
		retval = _EIO;
	} else {
		retval = 0;
	}

exit:
	tcm_msg->command = CMD_NONE;

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);

	syna_pal_mutex_unlock(cmd_mutex);

	if (has_irq_ctrl && in_polling && tcm_dev->hw_if->ops_enable_irq) {
		msleep(POLLING_ENABLE_IRQ_DELAY_MS);
		tcm_dev->hw_if->ops_enable_irq(tcm_dev->hw_if, true);
	}

	return retval;
}

static void syna_tcm_v1_set_ops(struct tcm_dev *tcm_dev)
{
	if (!tcm_dev)
		return;

	tcm_dev->read_message = syna_tcm_v1_read_message;
	tcm_dev->write_message = syna_tcm_v1_write_message;

	tcm_dev->msg_data.predict_length = 0;

	tcm_dev->protocol = TOUCHCOMM_V1;
}

static int syna_tcm_v1_set_max_rw_size(struct tcm_dev *tcm_dev)
{
	unsigned int wr_size = 0;
	struct tcm_identification_info *id_info;

	if (!tcm_dev) {
		return _EINVAL;
	}

	id_info = &tcm_dev->id_info;

	wr_size = syna_pal_le2_to_uint(id_info->max_write_size);

	if (wr_size == 0) {
		return 0;
	}

	if (wr_size != tcm_dev->max_wr_size) {
		if (tcm_dev->max_wr_size == 0)
			tcm_dev->max_wr_size = wr_size;
		else
			tcm_dev->max_wr_size = MIN(wr_size, tcm_dev->max_wr_size);
	}

	return 0;
}

static int syna_tcm_v1_detect(struct tcm_dev *tcm_dev, unsigned char *data,
		unsigned int size)
{
	int retval;
	struct tcm_v1_message_header *header;
	struct tcm_message_data_blob *tcm_msg = NULL;
	unsigned int payload_length = 0;
	unsigned char resp_code = 0;
	unsigned short default_crc = 0x5a5a;
	unsigned short default_rc = 0x5a;

	if (!tcm_dev || !data || size != MESSAGE_HEADER_SIZE)
		return _EINVAL;
	
	tcm_msg = &tcm_dev->msg_data;

	header = (struct tcm_v1_message_header *)data;

	if (header->marker != TCM_V1_MESSAGE_MARKER)
		return _ENODEV;

	tcm_msg->has_crc = true;
	tcm_dev->msg_data.crc_bytes = default_crc;
	tcm_msg->has_extra_rc = true;
	tcm_dev->msg_data.rc_byte = default_rc;

	if (header->code == REPORT_IDENTIFY) {
		payload_length = syna_pal_le2_to_uint(header->length);
		tcm_msg->payload_length = payload_length;

		retval = syna_tcm_v1_continued_read(tcm_dev,
				payload_length);
		if (retval < 0)
			return retval;
	} else {
		retval = syna_tcm_v1_write_message(tcm_dev,
				CMD_IDENTIFY,
				NULL,
				0,
				&resp_code,
				RESP_IN_POLLING);
		if (retval < 0) {
			retval = syna_tcm_v1_write_message(tcm_dev,
					CMD_RESET,
					NULL,
					0,
					&resp_code,
					RESET_DELAY_MS);
			if (retval < 0)
				return retval;
		}

		payload_length = tcm_msg->payload_length;
	}


	if (tcm_dev->dev_mode == MODE_UNKNOWN) {
		syna_tcm_buf_lock(&tcm_msg->in);

		retval = syna_tcm_v1_parse_idinfo(tcm_dev,
				&tcm_msg->in.buf[MESSAGE_HEADER_SIZE],
				tcm_msg->in.buf_size - MESSAGE_HEADER_SIZE,
				payload_length);
		if (retval < 0) {
			syna_tcm_buf_unlock(&tcm_msg->in);
			return retval;
		}

		syna_tcm_buf_unlock(&tcm_msg->in);
	}

	retval = syna_tcm_v1_set_max_rw_size(tcm_dev);
	if (retval < 0)
		return retval;

	syna_tcm_v1_update_crc(tcm_dev);
	if (tcm_dev->msg_data.crc_bytes == default_crc)
		tcm_msg->has_crc = false;
	if (tcm_dev->msg_data.rc_byte == default_rc)
		tcm_msg->has_extra_rc = false;

	syna_tcm_v1_set_ops(tcm_dev);

	return retval;
}

static int syna_tcm_detect_protocol(struct tcm_dev *tcm_dev,
		unsigned char *data, unsigned int data_len)
{
	int retval;

	if (!tcm_dev)
		return _EINVAL;

	retval = syna_tcm_v1_detect(tcm_dev, data, data_len);

	return retval;
}

int syna_tcm_detect_device(struct tcm_dev *tcm_dev)
{
	int retval = 0;
	unsigned char data[4] = { 0 };

	if (!tcm_dev)
		return _EINVAL;

	tcm_dev->dev_mode = MODE_UNKNOWN;

	data[0] = 0x07;
	retval = syna_tcm_write(tcm_dev, &data[0], 1);
	if (retval < 0)
		return _EIO;

	retval = syna_tcm_read(tcm_dev,
			data, (unsigned int)sizeof(data));
	if (retval < 0)
		return _EIO;

	retval = syna_tcm_detect_protocol(tcm_dev,
			data, (unsigned int)sizeof(data));
	if (retval < 0)
		return retval;

	if (!tcm_dev->write_message || !tcm_dev->read_message)
		return _ENODEV;

	retval = tcm_dev->dev_mode;
	return retval;
}

static int syna_dev_connect(struct syna_tcm *tcm)
{
	int retval;
	struct syna_hw_interface *hw_if = tcm->hw_if;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;
	struct tcm_dev *tcm_dev = tcm->tcm_dev;

	if (!tcm_dev)
		return -EINVAL;

	if (tcm->is_connected)
		return 0;

	if (hw_if->ops_power_on) {
		retval = hw_if->ops_power_on(hw_if, true);
		if (retval < 0)
			return -ENODEV;
	}

	if (hw_if->ops_hw_reset)
		hw_if->ops_hw_reset(hw_if);

	retval = syna_tcm_detect_device(tcm->tcm_dev);
	if (retval < 0)
		goto err_detect_dev;

	switch (retval) {
	case MODE_APPLICATION_FIRMWARE:
		retval = syna_dev_set_up_app_fw(tcm);
		if (retval >= 0) {
			retval = syna_dev_set_up_input_device(tcm);
			if (retval < 0) 
				goto err_setup_input_dev;
		}

		break;
	default:
		break;
	}

	retval = syna_dev_request_irq(tcm);
	if (retval < 0)
		goto err_request_irq;

	tcm->pwr_state = PWR_ON;
	tcm->is_connected = true;
	tcm->bus_ready = true;

	if (tcm->tcm_dev->max_wr_size != bus->wr_chunk_size)
		bus->wr_chunk_size = tcm->tcm_dev->max_wr_size;
	if (tcm->tcm_dev->max_rd_size != bus->rd_chunk_size)
		bus->rd_chunk_size = tcm->tcm_dev->max_rd_size;

	return 0;

err_request_irq:
	syna_dev_release_input_device(tcm);

err_setup_input_dev:
err_detect_dev:
	if (hw_if->ops_power_on)
		hw_if->ops_power_on(hw_if, false);

	return retval;
}

static int init_chip_dts(struct device *dev, void *chip_data)
{
	int rc = 0;
	struct device_node *np;
	struct syna_tcm *tcm = (struct syna_tcm *)chip_data;
	int tx_rx_num[2], panel_coords[2];
	np = dev->of_node;

	rc = of_property_read_u32_array(np, "touchpanel,tx-rx-num", tx_rx_num, 2);

	if (rc) {
		tcm->tx_num = TX_NUM;
		tcm->rx_num = RX_NUM;

	} else {
		tcm->tx_num = tx_rx_num[0];
		tcm->rx_num = tx_rx_num[1];
	}


	rc = of_property_read_u32_array(np, "touchpanel,panel-coords", panel_coords, 2);

	if (rc) {
		tcm->dts_max_x = 0;
		tcm->dts_max_y = 0;

	} else {
		tcm->dts_max_x = panel_coords[0];
		tcm->dts_max_y = panel_coords[1];
	}

	return 0;
}

static int syna_tcm_init_message_wrap(struct tcm_message_data_blob *tcm_msg,
		unsigned int resp_reading)
{
	syna_tcm_buf_init(&tcm_msg->in);
	syna_tcm_buf_init(&tcm_msg->out);
	syna_tcm_buf_init(&tcm_msg->temp);

	init_completion(&tcm_msg->cmd_completion);
	mutex_init(&tcm_msg->cmd_mutex);
	mutex_init(&tcm_msg->rw_mutex);

	ATOMIC_SET(tcm_msg->command_status, CMD_STATE_IDLE);
	tcm_msg->command = CMD_NONE;
	tcm_msg->status_report_code = STATUS_IDLE;
	tcm_msg->payload_length = 0;
	tcm_msg->response_code = 0;
	tcm_msg->report_code = 0;
	tcm_msg->seq_toggle = 0;

	syna_tcm_buf_lock(&tcm_msg->in);

	if (syna_tcm_buf_alloc(&tcm_msg->in, MESSAGE_HEADER_SIZE) < 0) {
		tcm_msg->in.buf_size = 0;
		tcm_msg->in.data_length = 0;
		syna_tcm_buf_unlock(&tcm_msg->in);
		return _EINVAL;
	}
	tcm_msg->in.buf_size = MESSAGE_HEADER_SIZE;

	syna_tcm_buf_unlock(&tcm_msg->in);

	tcm_msg->default_resp_reading = resp_reading;

	tcm_msg->predict_reads = false;
	tcm_msg->predict_length = 0;
	tcm_msg->has_crc = false;
	tcm_msg->crc_bytes = 0;
	tcm_msg->has_extra_rc = false;
	tcm_msg->rc_byte = 0;

	return 0;
}

int syna_tcm_allocate_device(struct tcm_dev **ptcm_dev_ptr,
		struct syna_hw_interface *hw_if, unsigned int resp_reading)
{
	int retval = 0;
	struct tcm_dev *tcm_dev = NULL;

	if (!hw_if || (!hw_if->ops_read_data || !hw_if->ops_write_data))
		return _EINVAL;

	*ptcm_dev_ptr = NULL;

	tcm_dev = (struct tcm_dev *)syna_pal_mem_alloc(
			1,
			sizeof(struct tcm_dev));
	if (!tcm_dev)
		return _ENOMEM;

	tcm_dev->hw_if = hw_if;

	tcm_dev->max_rd_size = hw_if->bdata_io.rd_chunk_size;
	tcm_dev->max_wr_size = hw_if->bdata_io.wr_chunk_size;

	tcm_dev->write_message = NULL;
	tcm_dev->read_message = NULL;

	tcm_dev->dev_mode = MODE_UNKNOWN;

	syna_tcm_buf_init(&tcm_dev->report_buf);
	syna_tcm_buf_init(&tcm_dev->resp_buf);
	syna_tcm_buf_init(&tcm_dev->external_buf);
	syna_tcm_buf_init(&tcm_dev->touch_config);

	retval = syna_tcm_init_message_wrap(&tcm_dev->msg_data, resp_reading);
	if (retval < 0)
		goto err_init_message_wrap;

	*ptcm_dev_ptr = tcm_dev;

	return 0;

err_init_message_wrap:
	syna_tcm_buf_release(&tcm_dev->touch_config);
	syna_tcm_buf_release(&tcm_dev->external_buf);
	syna_tcm_buf_release(&tcm_dev->report_buf);
	syna_tcm_buf_release(&tcm_dev->resp_buf);

	tcm_dev->hw_if = NULL;

	kfree(tcm_dev);

	return retval;
}

int syna_tcm_enable_predict_reading(struct tcm_dev *tcm_dev, bool en)
{
	if (!tcm_dev)
		return _EINVAL;

	tcm_dev->msg_data.predict_reads = en;
	tcm_dev->msg_data.predict_length = 0;

	return 0;
}

static int syna_dev_probe(struct platform_device *pdev)
{
	int retval;
	struct syna_tcm *tcm = NULL;
	struct tcm_dev *tcm_dev = NULL;
	struct syna_hw_interface *hw_if = NULL;
	struct device * syna_spi_pdev;

	hw_if = pdev->dev.platform_data;
	if (!hw_if)
		return -EINVAL;

	tcm = syna_pal_mem_alloc(1, sizeof(struct syna_tcm));
	if (!tcm)
		return -ENOMEM;
	
	retval = syna_tcm_allocate_device(&tcm_dev, hw_if, RESP_IN_POLLING);
	if (retval < 0 || !tcm_dev)
		goto err_allocate_cdev;

	tcm->tcm_dev = tcm_dev;
	tcm->pdev = pdev;
	tcm->hw_if = hw_if;

	syna_spi_pdev = syna_spi_device->dev.parent;

	retval = init_chip_dts(syna_spi_pdev, tcm);

	if (retval < 0)
		goto err_manufacture_info;
	
	syna_tcm_buf_init(&tcm->event_data);

	mutex_init(&tcm->tp_event_mutex);

	mutex_init(&tcm->mutex);
	init_completion(&tcm->fw_complete);

	tcm->is_connected = false;
	tcm->pwr_state = PWR_OFF;

	tcm->dev_connect = syna_dev_connect;
	tcm->dev_disconnect = syna_dev_disconnect;
	tcm->dev_set_up_app_fw = syna_dev_set_up_app_fw;
	tcm->dev_resume = syna_dev_resume;
	tcm->dev_suspend = syna_dev_suspend;

	tcm->wait_for_ioctl_operation = 0;
	tcm->waiting_frame = 0;
	tcm->use_short_frame_waiting = 0;
	tcm->primary_timestamp_enabled = 1;

	platform_set_drvdata(pdev, tcm);

	device_init_wakeup(&pdev->dev, 1);
	init_completion(&tcm->report_complete);
	
	retval = tcm->dev_connect(tcm);
	if (retval < 0) {
		mutex_destroy(&tcm->mutex);
		goto err_connect;
	}

	tcm->speedup_resume_wq = create_singlethread_workqueue("sp_resume0");
	INIT_WORK(&tcm->speed_up_work, syna_speedup_resume);

	syna_tcm_enable_predict_reading(tcm->tcm_dev, true);

	tcm->probe_done = 1;

	return 0;
	tcm->dev_disconnect(tcm);
err_manufacture_info:
err_connect:
	syna_tcm_buf_release(&tcm->event_data);
	mutex_destroy(&tcm->mutex);
err_allocate_cdev:
	kfree(tcm);

	return retval;
}

static void syna_tcm_del_message_wrap(struct tcm_message_data_blob *tcm_msg)
{
	syna_tcm_buf_release(&tcm_msg->temp);
	syna_tcm_buf_release(&tcm_msg->out);
	syna_tcm_buf_release(&tcm_msg->in);
}

void syna_tcm_remove_device(struct tcm_dev *tcm_dev)
{
	if (!tcm_dev)
		return;

	syna_tcm_del_message_wrap(&tcm_dev->msg_data);

	syna_tcm_buf_release(&tcm_dev->touch_config);
	syna_tcm_buf_release(&tcm_dev->external_buf);
	syna_tcm_buf_release(&tcm_dev->report_buf);
	syna_tcm_buf_release(&tcm_dev->resp_buf);

	tcm_dev->hw_if = NULL;

	kfree(tcm_dev);
}

static void syna_dev_remove(struct platform_device *pdev)
{
	struct syna_tcm *tcm = platform_get_drvdata(pdev);
	msleep(25);
	tcm->driver_current_state = IS_REMOVE;
	mutex_lock(&tcm->mutex);
    tcm->dev_disconnect(tcm);
	mutex_unlock(&tcm->mutex);
	syna_tcm_buf_release(&tcm->event_data);
	syna_tcm_remove_device(tcm->tcm_dev);
	kfree(tcm);
}

static void syna_dev_shutdown(struct platform_device *pdev)
{
	syna_dev_remove(pdev);
}

static int syna_spi_suspend(struct device *dev) {
	struct syna_tcm *tcm = dev_get_drvdata(dev);
	struct syna_hw_attn_data *attn;
	if (!tcm || !tcm->hw_if) {
		return 0;
	}
	attn = &tcm->hw_if->bdata_attn;
	if (attn->irq_id == 0)
		return 0;
	tcm->bus_ready = false;
	if (tcm->hw_if->ops_enable_irq)
		tcm->hw_if->ops_enable_irq(tcm->hw_if, false);
	return 0;
}

static int syna_spi_resume(struct device *dev) {
	struct syna_tcm *tcm = dev_get_drvdata(dev);
	struct syna_hw_attn_data *attn;

	if (!tcm || !tcm->hw_if) {
		return 0;
	}
	attn = &tcm->hw_if->bdata_attn;
	if (attn->irq_id == 0)
		return 0;
	if (tcm->hw_if->ops_enable_irq)
		tcm->hw_if->ops_enable_irq(tcm->hw_if, true);
	tcm->bus_ready = true;
	return 0;
}

static const struct dev_pm_ops syna_dev_pm_ops = {
	.suspend = syna_spi_suspend,
	.resume = syna_spi_resume,
};

static struct platform_driver syna_dev_driver = {
	.driver = {
		.name = PLATFORM_DRIVER_NAME,
		.owner = THIS_MODULE,
		.pm = &syna_dev_pm_ops,
	},
	.probe = syna_dev_probe,
	.remove = syna_dev_remove,
	.shutdown = syna_dev_shutdown,
};

static int __init syna_dev_module_init(void) {
	int retval;
	retval = syna_hw_interface_init();
	if (retval < 0)
		return retval;
	return platform_driver_register(&syna_dev_driver);
}

static void __exit syna_dev_module_exit(void) {
	platform_driver_unregister(&syna_dev_driver);
	syna_hw_interface_exit();
}

module_init(syna_dev_module_init);
module_exit(syna_dev_module_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics TCM Touch Driver");
MODULE_LICENSE("GPL v2");

