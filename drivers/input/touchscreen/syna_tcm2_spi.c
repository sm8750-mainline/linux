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

#include <linux/spi/spi.h>
#include "syna_tcm2.h"
#include <linux/pinctrl/consumer.h>

#define SPI_MODULE_NAME "synaptics_tcm_spi"

static unsigned char *rx_buf;
static unsigned char *tx_buf;

static unsigned int buf_size;

static struct spi_transfer *xfer;

struct platform_device *syna_spi_device;

static struct device *syna_request_managed_device(void) {
	if (!syna_spi_device)
		return NULL;

	return syna_spi_device->dev.parent;
}

static void syna_spi_hw_reset(struct syna_hw_interface *hw_if) {
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;

	if (rst->reset_gpio >= 0) {
		syna_pal_mutex_lock(&rst->reset_en_mutex);
		gpio_set_value(rst->reset_gpio, rst->reset_on_state);
		msleep(rst->reset_active_ms);
		gpio_set_value(rst->reset_gpio, !rst->reset_on_state);
		msleep(rst->reset_delay_ms);
		syna_pal_mutex_unlock(&rst->reset_en_mutex);
	}
}

static int syna_spi_request_gpio(int gpio, bool config, int dir, int state, char *label) {
	int retval;

	struct device *dev = syna_request_managed_device();

	if (!dev)
		return -ENODEV;

	if (gpio < 0)
		return -EINVAL;

	if (config) {
		retval = snprintf(label, 16, "tcm_gpio_%d\n", gpio);
		if (retval < 0)
			return retval;
		
		retval = gpio_request(gpio, label);
		if (retval < 0)
			return retval;

		if (dir == 0)
			retval = gpio_direction_input(gpio);
		else
			retval = gpio_direction_output(gpio, state);

		if (retval < 0)
			return retval;
	} else {
		gpio_free(gpio);
	}

	return 0;
}

static void syna_spi_release_gpio(struct syna_hw_interface *hw_if)
{
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;

	if (rst->reset_gpio >= 0)
		syna_spi_request_gpio(rst->reset_gpio, false, 0, 0, NULL);
	if (attn->irq_gpio >= 0)
		syna_spi_request_gpio(attn->irq_gpio, false, 0, 0, NULL);
}

static int syna_spi_active_pinctrl(struct syna_hw_interface *hw_if)
{
	int retval = 0;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;

	if (bus->pin_spi_mode_active != NULL) {
		retval = pinctrl_select_state(bus->pinctrl, bus->pin_spi_mode_active);
		if (retval < 0)
			goto err_active_pinctrl;
	}

err_active_pinctrl:
	return retval;
}

static int syna_spi_config_gpio(struct syna_hw_interface *hw_if) {
	int retval;
	static char str_irq_gpio[32] = {0};
	static char str_rst_gpio[32] = {0};
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;

	if (attn->irq_gpio >= 0) {
		retval = syna_spi_request_gpio(attn->irq_gpio, true, 0, 0, str_irq_gpio);
		if (retval < 0)
			goto err_set_gpio_irq;
	}

	if (rst->reset_gpio >= 0) {
		retval = syna_spi_request_gpio(rst->reset_gpio, true, 1, !rst->reset_on_state, str_rst_gpio);
		if (retval < 0)
			goto err_set_gpio_reset;
	}

	return 0;

err_set_gpio_reset:
	if (attn->irq_gpio >= 0)
		syna_spi_request_gpio(attn->irq_gpio, false, 0, 0, NULL);
err_set_gpio_irq:
	return retval;
}

static int syna_spi_enable_pwr_gpio(struct syna_hw_interface *hw_if, bool en) {
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;
	int state = (en) ? pwr->power_on_state : !pwr->power_on_state;
	int ret = 0;

	if (pwr->avdd_gpio >= 0) {
		ret = gpio_direction_output(pwr->avdd_gpio, state);
		if (ret)
			return ret;
	}

	if (pwr->vdd_gpio >= 0) {
		ret = gpio_direction_output(pwr->vdd_gpio, state);
		if (ret)
			return ret;
	}

	return ret;
}

static int syna_spi_enable_regulator(struct syna_hw_interface *hw_if, bool en) {
	int retval;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;
	struct regulator *vdd_reg = pwr->vdd_reg_dev;

	if (!en) {
		retval = 0;
		goto disable_pwr_reg;
	}

	if (vdd_reg) {
		retval = regulator_enable(vdd_reg);
		if (retval < 0)
			goto exit;
	}

	return 0;

disable_pwr_reg:
	if (vdd_reg)
		regulator_disable(vdd_reg);

exit:
	return retval;
}

static int syna_spi_power_on(struct syna_hw_interface *hw_if, bool en) {
	int retval;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	retval = syna_spi_enable_pwr_gpio(hw_if, en);
	if (retval < 0)
		return retval;

	retval = syna_spi_enable_regulator(hw_if, en);
	if (retval < 0)
		return retval;

	msleep(pwr->power_on_delay_ms);

	return 0;
}

static int syna_spi_get_regulator(struct syna_hw_interface *hw_if, bool get) {
	int retval;
	struct device *dev = syna_spi_device->dev.parent;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	if (!get) {
		retval = 0;
		goto regulator_put;
	}

	pwr->vdd_reg_dev = regulator_get(dev, "vdd");
	if (IS_ERR((struct regulator *)pwr->vdd_reg_dev)) {
		retval = PTR_ERR((struct regulator *)pwr->vdd_reg_dev);
		goto exit;
	} else {
		if (regulator_count_voltages(pwr->vdd_reg_dev) > 0) {
			retval = regulator_set_voltage(pwr->vdd_reg_dev, 1800000, 1800000);
			if (retval)
				goto exit;

			retval = regulator_set_load(pwr->vdd_reg_dev, 200000);
			if (retval < 0)
				goto exit;
		}
	}

	return 0;

regulator_put:
	if (pwr->vdd_reg_dev) {
		regulator_put(pwr->vdd_reg_dev);
		pwr->vdd_reg_dev = NULL;
	}
exit:
	return retval;
}

static int syna_spi_config_psu(struct syna_hw_interface *hw_if)
{
	int retval;
	static char str_avdd_gpio[32] = {0};
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	if (pwr->psu == PSU_GPIO) {
		if (pwr->avdd_gpio >= 0) {
			retval = syna_spi_request_gpio(pwr->avdd_gpio, true, 1, !pwr->power_on_state, str_avdd_gpio);
			if (retval < 0) {
				syna_spi_request_gpio(pwr->vdd_gpio, false, 0, 0, NULL);
				return retval;
			}
		}
	} else {
		retval = syna_spi_get_regulator(hw_if, true);
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int syna_spi_release_psu(struct syna_hw_interface *hw_if)
{
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;

	if (pwr->psu == PSU_GPIO) {
		syna_spi_request_gpio(pwr->avdd_gpio, false, 0, 0, NULL);
	} else {
		syna_spi_get_regulator(hw_if, false);
	}

	return 0;
}

static int syna_spi_enable_irq(struct syna_hw_interface *hw_if, bool en) {
	int retval = 0;
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;

	if (attn->irq_id == 0)
		return 0;

	syna_pal_mutex_lock(&attn->irq_en_mutex);

	if (en) {
		if (attn->irq_enabled) {
			retval = 0;
			goto exit;
		}

		enable_irq(attn->irq_id);
		attn->irq_enabled = true;
	}
	else {
		if (!attn->irq_enabled) {
			retval = 0;
			goto exit;
		}

		disable_irq_nosync(attn->irq_id);
		attn->irq_enabled = false;
	}

exit:
	syna_pal_mutex_unlock(&attn->irq_en_mutex);

	return retval;
}

static int syna_spi_parse_dt(struct syna_hw_interface *hw_if, struct device *dev) {
	int retval;
	struct property *prop;
	struct device_node *np = dev->of_node;
	struct syna_hw_attn_data *attn = &hw_if->bdata_attn;
	struct syna_hw_pwr_data *pwr = &hw_if->bdata_pwr;
	struct syna_hw_rst_data *rst = &hw_if->bdata_rst;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;

	prop = of_find_property(np, "synaptics,irq-gpio", NULL);
	if (prop && prop->length) {
		attn->irq_gpio = of_get_named_gpio(np, "synaptics,irq-gpio", 0);
	} else {
		attn->irq_gpio = -1;
	}

	attn->irq_on_state = 0;
	pwr->psu = (int)PSU_REGULATOR;

	prop = of_find_property(np, "synaptics,avdd-gpio", NULL);
	if (prop && prop->length) {
		pwr->avdd_gpio = of_get_named_gpio(np, "synaptics,avdd-gpio", 0);
	} else {
		pwr->avdd_gpio = -1;
	}
	
	pwr->power_on_state = 1;
	pwr->power_on_delay_ms = 200;

	prop = of_find_property(np, "synaptics,reset-gpio", NULL);
	if (prop && prop->length) {
		rst->reset_gpio = of_get_named_gpio(np, "synaptics,reset-gpio", 0);
	} else {
		rst->reset_gpio = -1;
	}

	rst->reset_on_state = 0;
	rst->reset_active_ms = 10;
	rst->reset_delay_ms = 80;
	
	bus->spi_byte_delay_us = 0;
	bus->spi_block_delay_us = 0;
	bus->spi_mode = 0;
	bus->switch_gpio = -1;
	bus->switch_state = 1;

	bus->pinctrl = devm_pinctrl_get(dev);

	if (!IS_ERR_OR_NULL(bus->pinctrl)) {
		bus->pin_spi_mode_active = pinctrl_lookup_state(bus->pinctrl, "ts_spi_active");
		if (IS_ERR_OR_NULL(bus->pin_spi_mode_active)) {
			retval = PTR_ERR(bus->pin_spi_mode_active);
			bus->pin_spi_mode_active = NULL;
		}

		bus->pin_spi_mode_suspend = pinctrl_lookup_state(bus->pinctrl, "ts_spi_suspend");
		if (IS_ERR_OR_NULL(bus->pin_spi_mode_suspend)) {
			retval = PTR_ERR(bus->pin_spi_mode_suspend);
			bus->pin_spi_mode_suspend = NULL;
		}
	}

	return 0;
}

static int syna_spi_alloc_mem(unsigned int count, unsigned int size) {
	static unsigned int xfer_count;

	if (count > xfer_count) {
		kfree(xfer);
		xfer = syna_pal_mem_alloc(count, sizeof(*xfer));
		if (!xfer) {
			xfer_count = 0;
			return -ENOMEM;
		}
		xfer_count = count;
	} else {
		memset(xfer, 0, count * sizeof(*xfer));
	}

	if (size > buf_size) {
		if (rx_buf) {
			kfree(rx_buf);
			rx_buf = NULL;
		}
		if (tx_buf) {
			kfree(tx_buf);
			tx_buf = NULL;
		}

		rx_buf = syna_pal_mem_alloc(size, sizeof(unsigned char));
		if (!rx_buf) {
			buf_size = 0;
			return -ENOMEM;
		}
		tx_buf = syna_pal_mem_alloc(size, sizeof(unsigned char));
		if (!tx_buf) {
			buf_size = 0;
			return -ENOMEM;
		}

		buf_size = size;
	}

	return 0;
}

static int syna_spi_read(struct syna_hw_interface *hw_if, unsigned char *rd_data, unsigned int rd_len) {
	int retval;
	unsigned int idx;
	struct spi_message msg;
	struct spi_device *spi = hw_if->pdev;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;

	if (!spi)
		return -ENXIO;

	syna_pal_mutex_lock(&bus->io_mutex);

	spi_message_init(&msg);

	if (bus->spi_byte_delay_us == 0)
		retval = syna_spi_alloc_mem(1, rd_len);
	else
		retval = syna_spi_alloc_mem(rd_len, rd_len);
	if (retval < 0)
		goto exit;

	if (bus->spi_byte_delay_us == 0) {
		memset(tx_buf, 0xff, rd_len);
		xfer[0].len = rd_len;
		xfer[0].tx_buf = tx_buf;
		xfer[0].rx_buf = rx_buf;
		spi_message_add_tail(&xfer[0], &msg);
	} else {
		tx_buf[0] = 0xff;
		for (idx = 0; idx < rd_len; idx++) {
			xfer[idx].len = 1;
			xfer[idx].tx_buf = tx_buf;
			xfer[idx].rx_buf = &rx_buf[idx];
			spi_message_add_tail(&xfer[idx], &msg);
		}
	}

	retval = spi_sync(spi, &msg);
	if (retval != 0)
		goto exit;
	retval = syna_pal_mem_cpy(rd_data, rd_len, rx_buf, rd_len, rd_len);
	if (retval < 0)
		goto exit;

	retval = rd_len;

exit:
	syna_pal_mutex_unlock(&bus->io_mutex);

	return retval;
}

static int syna_spi_write(struct syna_hw_interface *hw_if, unsigned char *wr_data, unsigned int wr_len) {
	int retval;
	unsigned int idx;
	struct spi_message msg;
	struct spi_device *spi = hw_if->pdev;
	struct syna_hw_bus_data *bus = &hw_if->bdata_io;

	if (!spi)
		return -ENXIO;

	syna_pal_mutex_lock(&bus->io_mutex);

	spi_message_init(&msg);

	if (bus->spi_byte_delay_us == 0)
		retval = syna_spi_alloc_mem(1, wr_len);
	else
		retval = syna_spi_alloc_mem(wr_len, wr_len);
	if (retval < 0)
		goto exit;

	retval = syna_pal_mem_cpy(tx_buf, wr_len, wr_data, wr_len, wr_len);
	if (retval < 0)
		goto exit;

	if (bus->spi_byte_delay_us == 0) {
		xfer[0].len = wr_len;
		xfer[0].tx_buf = tx_buf;
		spi_message_add_tail(&xfer[0], &msg);
	} else {
		for (idx = 0; idx < wr_len; idx++) {
			xfer[idx].len = 1;
			xfer[idx].tx_buf = &tx_buf[idx];
			spi_message_add_tail(&xfer[idx], &msg);
		}
	}

	retval = spi_sync(spi, &msg);
	if (retval != 0)
		goto exit;

	retval = wr_len;

exit:
	syna_pal_mutex_unlock(&bus->io_mutex);

	return retval;
}

static struct syna_hw_interface syna_spi_hw_if = {
	.bdata_io = {
		.type = BUS_TYPE_SPI,
		.rd_chunk_size = RD_CHUNK_SIZE,
		.wr_chunk_size = WR_CHUNK_SIZE,
	},
	.bdata_attn = {
		.irq_enabled = false,
		.irq_on_state = 0,
	},
	.bdata_rst = {
		.reset_on_state = 0,
		.reset_delay_ms = 80,
		.reset_active_ms = 10,
	},
	.bdata_pwr = {
		.power_on_state = 1,
		.power_on_delay_ms = 200,
	},
	.ops_power_on = syna_spi_power_on,
	.ops_hw_reset = syna_spi_hw_reset,
	.ops_read_data = syna_spi_read,
	.ops_write_data = syna_spi_write,
	.ops_enable_irq = syna_spi_enable_irq,
};

static int syna_spi_probe(struct spi_device *spi)
{
	int retval;
	struct syna_hw_attn_data *attn = &syna_spi_hw_if.bdata_attn;
	struct syna_hw_bus_data *bus = &syna_spi_hw_if.bdata_io;
	struct syna_hw_rst_data *rst = &syna_spi_hw_if.bdata_rst;

	syna_spi_device = platform_device_alloc(PLATFORM_DRIVER_NAME, 0);
	if (!syna_spi_device)
		return _ENODEV;

	syna_spi_parse_dt(&syna_spi_hw_if, &spi->dev);

	mutex_init((struct mutex *)&attn->irq_en_mutex);
	mutex_init((struct mutex *)&bus->io_mutex);
	mutex_init((struct mutex *)&rst->reset_en_mutex);

	spi->mode = SPI_MODE_0;

	syna_spi_hw_if.pdev = spi;

	syna_spi_device->dev.parent = &spi->dev;
	syna_spi_device->dev.platform_data = &syna_spi_hw_if;

	spi->bits_per_word = 8;

	retval = spi_setup(spi);
	if (retval < 0)
		return retval;

	retval = syna_spi_config_psu(&syna_spi_hw_if);
	if (retval < 0)
		return retval;

	retval = syna_spi_config_gpio(&syna_spi_hw_if);
	if (retval < 0)
		return retval;

	retval = syna_spi_active_pinctrl(&syna_spi_hw_if);
	if (retval < 0)
		return retval;

	retval = platform_device_add(syna_spi_device);
	if (retval < 0)
		return retval;

	return 0;
}

static void syna_spi_remove(struct spi_device *spi)
{
	syna_spi_release_gpio(&syna_spi_hw_if);
	syna_spi_release_psu(&syna_spi_hw_if);
	syna_spi_device->dev.platform_data = NULL;
	platform_device_unregister(syna_spi_device);
}
static const struct spi_device_id syna_spi_id_table[] = {
	{SPI_MODULE_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(spi, syna_spi_id_table);

static const struct of_device_id syna_spi_of_match_table[] = {
	{
		.compatible = "synaptics,tcm-spi-hbp",
	},
	{},
};
MODULE_DEVICE_TABLE(of, syna_spi_of_match_table);


static struct spi_driver syna_spi_driver = {
	.driver = {
		.name = SPI_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = syna_spi_of_match_table,
	},
	.probe = syna_spi_probe,
	.remove = syna_spi_remove,
	.id_table = syna_spi_id_table,
};

int syna_hw_interface_init(void)
{
	return spi_register_driver(&syna_spi_driver);
}

void syna_hw_interface_exit(void)
{
	if (rx_buf) {
		kfree(rx_buf);
		rx_buf = NULL;
	}

	if (tx_buf) {
		kfree(tx_buf);
		tx_buf = NULL;
	}

	if (xfer) {
		kfree(xfer);
		xfer = NULL;
	}

	spi_unregister_driver(&syna_spi_driver);
}

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics TouchCom SPI Bus Module");
MODULE_LICENSE("GPL v2");

