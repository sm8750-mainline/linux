#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/init.h>
#include <linux/iio/magnetometer/ak09970.h>
#include <linux/input/oplus_tri_key.h>
#include <linux/seq_file.h>

#define DHALL_NUM 1
#define TRI_KEY_TAG                  "[tri_state_key] "
#define TRI_KEY_ERR(fmt, args...)\
	pr_err(TRI_KEY_TAG" %s : "fmt, __func__, ##args)


static struct oplus_dhall_chip *g_chip;

static struct hall_srs ak09970_ranges[] = {
	{"36_04mT", AK09970_HIGH_SENSITIVITY, false},
	{"101_57mT", AK09970_WIDE_RANGE, false},
};

static DEFINE_MUTEX(ak09970_i2c_mutex);

#define MAX_I2C_RETRY_TIME 2
static int ak09970_i2c_read_block(struct oplus_dhall_chip *chip, u8 addr, u8 *data, u8 len)
{
	u8 reg_addr = addr;
	int err = 0, retry = 0;
	struct i2c_client *client = chip->client;
	struct i2c_msg msgs[2] = {{0}, {0}};

	if (!client) {
		return -EINVAL;
	} else if (len > AK09970_I2C_REG_MAX_SIZE) {
		return -EINVAL;
	}
	mutex_lock(&ak09970_i2c_mutex);

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg_addr;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = data;

	for (retry = 0; retry < MAX_I2C_RETRY_TIME; retry++) {
		err = i2c_transfer(client->adapter, msgs, (sizeof(msgs) / sizeof(msgs[0])));

		if (err < 0) {
			msleep(20);
		} else {
			err = 0;
			break;
		}
	}
	if (retry == MAX_I2C_RETRY_TIME) {
		err = -EIO;
	}
	mutex_unlock(&ak09970_i2c_mutex);

	return err;
}

static int ak09970_i2c_write_block(struct oplus_dhall_chip *chip, u8 addr, u8 *data, u8 len)
{
	int err = 0, retry = 0;
	int idx = 0;
	int num = 0;
	char buf[AK09970_I2C_REG_MAX_SIZE] = {0};
	struct i2c_client *client = chip->client;

	if (!client) {
		return -EINVAL;
	} else if (len >= AK09970_I2C_REG_MAX_SIZE) {
		return -EINVAL;
	}

	mutex_lock(&ak09970_i2c_mutex);

	buf[num++] = addr;
	for (idx = 0; idx < len; idx++) {
		buf[num++] = data[idx];
	}

	for (retry = 0; retry < MAX_I2C_RETRY_TIME; retry++) {
		err = i2c_master_send(client, buf, num);

		if (err < 0) {
			msleep(20);
		} else {
			break;
		}
	}

	mutex_unlock(&ak09970_i2c_mutex);
	return err;
}

static int mysqrt(long x)
{
	long temp = 0;
	int res = 0;
	int count = 0;
	if (x == 1 || x == 0)
		return x;
	temp = x / 2;
	while (1) {
		long a = temp;
		count++;
		temp = (temp + x / temp) / 2;
		if (count > MAX_SQRT_TIME) {
			res = temp;
			return res;
		}
		if (((a - temp) < 2) && ((a - temp) > -2)) {
			res = temp;
			return res;
		}
	}
}

static int ak09970_get_data(struct dhall_data_xyz *data)
{
	int err = 0;
	int irqval = 0;
	u8 buf[7] = {0};
	short value_x = 0;
	short value_y = 0;
	short value_z = 0;
	long value_v = 0;
	u8 st = 0;
	if (g_chip== NULL) {
		return -EINVAL;
	}

	msleep(35);

	err = ak09970_i2c_read_block(g_chip, AK09970_REG_ST1_ZYX, buf, sizeof(buf));
	if (err < 0) {
		return err;
	}

	st = buf[0];
	if (buf[0] & 0x01) {
		value_x = (short)((u16)(buf[5] << 8) + buf[6]);
		value_y = (short)((u16)(buf[3] << 8) + buf[4]);
		value_z = (short)((u16)(buf[1] << 8) + buf[2]);
	} else {
		data->hall_x = value_x;
		data->hall_y = value_y;
		data->hall_z = value_z;
		data->st = st;
		return err;
	}
	err = ak09970_i2c_read_block(g_chip, AK09970_REG_ST1_V, buf, sizeof(buf));
	if (err < 0) {
		return err;
	}
	if (buf[0] & 0x01) {
		value_v = (long)(value_x * value_x) +(long)(value_y * value_y) + (long)(value_z * value_z);
	} else {
		value_v = (long)((u32)(buf[1] << 24) +(u32)(buf[2] << 16) + (u32)(buf[3] << 8) + (u32)buf[4]);
	}
	data->hall_x = value_x;
	data->hall_y = value_y;
	data->hall_z = value_z;
	data->hall_v = mysqrt(value_v);
	irqval = gpio_get_value(g_chip->irq_gpio);
	return 0;
}

static void ak09970_dump_reg(struct seq_file *s)
{
	int i = 0, err = 0;
	int k = 0;
	u8 val[20] = {0};
	char *buffer = kzalloc(2048, GFP_KERNEL);
	char _buf[40] = {0};
	u8 reg[][2] = {
		{ 0x10, 1},
		{ 0x11, 3},
		{ 0x12, 3},
		{ 0x13, 5},
		{ 0x14, 3},
		{ 0x15, 5},
		{ 0x16, 5},
		{ 0x17, 7},
		{ 0x18, 5},
		{ 0x19, 2},
		{ 0x1a, 2},
		{ 0x1b, 3},
		{ 0x1c, 2},
		{ 0x1d, 3},
		{ 0x1e, 3},
		{ 0x1f, 4},
		{ 0x20, 2},
		{ 0x21, 1},
		{ 0x22, 4},
		{ 0x23, 4},
		{ 0x24, 4},
		{ 0x25, 4}
	};
	
	if (!buffer) {
		seq_printf(s, "Failed to allocate memory for buffer\n");
		return;
	}

	for (k = 0; k < sizeof(reg) / sizeof(reg[0]); k++) {
		memset(val, 0, sizeof(val));
		if (reg[k][1] <= sizeof(val)) {
			err = ak09970_i2c_read_block(g_chip, reg[k][0], val, reg[k][1]);
			if (err < 0) {
				seq_printf(s, "read reg %d error\n", reg[k][0]);
				goto free_buffer;
			}
			for (i = 0; i < reg[k][1]; i++) {
				snprintf(_buf, sizeof(_buf), "reg 0x%x:0x%x\n", reg[k][0], val[i]);
				if (strlen(buffer) + strlen(_buf) < 2048) {
					strncat(buffer, _buf, strlen(_buf));
				}
			}
		} else {
			seq_printf(s, "reg len error\n");
		}
	}
	seq_printf(s, "%s", buffer);

free_buffer:
	kfree(buffer);
	return;
}

static int ak09970_set_reg(int reg, int val)
{
	u8 data = (u8)val;

	if (g_chip == NULL) {
		return -EINVAL;
	}

	ak09970_i2c_write_block(g_chip, (u8)reg, &data, 1);

	return 0;
}

static bool ak09970_is_power_on(void)
{
	if (g_chip == NULL) {
		return false;
	}

	return g_chip->is_power_on;
}

static int ak09970_set_power(struct oplus_dhall_chip *chip, bool on)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(chip->power_1v8)) {
		return -EINVAL;
	}

	if (on) {
		if (regulator_count_voltages(chip->power_1v8) > 0) {
			ret = regulator_set_voltage(chip->power_1v8, 1800000, 1800000);
			if (ret) {
				return ret;
			}
		}

		ret = regulator_enable(chip->power_1v8);
		if (ret) {
			TRI_KEY_ERR("Failed to enable regulator: %d\n", ret);
			return ret;
		}
		msleep(5);
		chip->is_power_on = true;

	} else {
		regulator_disable(chip->power_1v8);
	}

	return 0;
}

static void ak09970_inttobuff(u8* th, int low, int high)
{
	th[0] = (u8)(((u16)high) >> 8);
	th[1] = (u8)(((u16)high) & 0xFF);
	th[2] = (u8)(((u16)low) >> 8);
	th[3] = (u8)(((u16)low) & 0xFF);
	return;
}


static bool ak09970_update_threshold(int position, short lowthd, short highthd, struct dhall_data_xyz *halldata, int interf)
{
	u8 th[4] = {0};
	u8 sth[4] = {0};
	u8 vth[4] = {0};
	int err = 0;
	u8 data[2] = {0};
	int irqval = 0;
	int second_low = 0;
	int second_high = 0;
	int vlow = 0;
	int vhigh = 0;
	if (g_chip == NULL) {
		return -EINVAL;
	}

	if (position >= 6) {
		return false;
	}
	irqval = gpio_get_value(g_chip->irq_gpio);
	switch (position) {
	case UP_STATE:
		lowthd = halldata->hall_x - YBOP_TOL;
		highthd = halldata->hall_x - YBRP_TOL;
		break;
	case DOWN_STATE:
		lowthd = halldata->hall_x + YBRP_TOL;
		highthd = halldata->hall_x + YBOP_TOL;
		break;
	case MID_STATE:
		break;
	default:
		break;
	}
	ak09970_inttobuff(th, lowthd, highthd);
	switch (position) {
	case UP_STATE:
		data[0] = 0x03;
		data[1] = 0x16;
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_CNTL1, data, 2);
		if (err < 0) {
			return err;
		}
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1, th, 4);
		if (err < 0) {
			return err;
		}
		second_low = halldata->hall_y - 2701;
		second_high = halldata->hall_y - 2700;
		ak09970_inttobuff(sth, second_low, second_high);
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1+1, sth, 4);
		if (err < 0) {
			return err;
		}
		vlow = halldata->hall_v + XBRP_TOL;
		vhigh = halldata->hall_v + XBOP_TOL;
		ak09970_inttobuff(vth, vlow, vhigh);
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1+3, vth, 4);
		if (err < 0) {
			return err;
		}
		err = ak09970_i2c_read_block(g_chip, AK09970_REG_CNTL1, data, 2);
		break;
	case DOWN_STATE:
		data[0] = 0x02;
		data[1] = 0x16;
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_CNTL1, data, 2);
		if (err < 0) {
			return err;
		}
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1, th, 4);
		if (err < 0) {
			return err;
		}
		second_low = halldata->hall_y - YBOP_TOL;
		second_high = halldata->hall_y - YBRP_TOL;
		ak09970_inttobuff(sth, second_low, second_high);
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1+1, sth, 4);
		if (err < 0) {
			return err;
		}
		vlow = halldata->hall_v + XBRP_TOL;
		vhigh = halldata->hall_v + XBOP_TOL;
		ak09970_inttobuff(vth, vlow, vhigh);
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1+3, vth, 4);
		if (err < 0) {
			return err;
		}
		err = ak09970_i2c_read_block(g_chip, AK09970_REG_SWX1, th, 4);
		if (err < 0) {
			return err;
		}

		err = ak09970_i2c_read_block(g_chip, AK09970_REG_CNTL1, data, 2);
		if (err < 0) {
			return err;
		}
		err = ak09970_i2c_read_block(g_chip, AK09970_REG_SWX1+3, vth, 4);
		if (err < 0) {
			return err;
		}
		break;
	case MID_STATE:
		if((halldata->hall_x < 0)&& (halldata->hall_y < 0)) {
			data[0] = 0x00;
			second_low = halldata->hall_x + XBRP_TOL;
			second_high = halldata->hall_x + XBOP_TOL;
			lowthd = halldata->hall_y +YBRP_TOL;
			highthd = halldata->hall_y +YBOP_TOL;
		} else if ((halldata->hall_x < 0) && (halldata->hall_y >= 0)) {
			data[0] = 0x02;
			second_low = halldata->hall_x + XBRP_TOL;
			second_high = halldata->hall_x + XBOP_TOL;
			lowthd = halldata->hall_y - YBOP_TOL;
			highthd = halldata->hall_y - YBRP_TOL;
		} else if ((halldata->hall_x >= 0) && (halldata->hall_y < 0)) {
			data[0] = 0x01;
			second_low = halldata->hall_x - YBOP_TOL;
			second_high = halldata->hall_x - YBRP_TOL;
			lowthd = halldata->hall_y + YBRP_TOL;
			highthd = halldata->hall_y + YBOP_TOL;
		} else {
			data[0] = 0x03;
			second_low = halldata->hall_x - YBOP_TOL;
			second_high = halldata->hall_x - YBRP_TOL;
			lowthd = halldata->hall_y - YBOP_TOL;
			highthd = halldata->hall_y - YBRP_TOL;
		}
		data[1] = 0x16;
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_CNTL1, data, 2);
		if (err < 0) {
			return err;
		}
		ak09970_inttobuff(th, lowthd, highthd);
		err = ak09970_i2c_read_block(g_chip, AK09970_REG_CNTL1, data, 2);
		if (err < 0) {
			return err;
		}
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1 + 1, th, 4);
		if (err < 0) {
			return err;
		}
		ak09970_inttobuff(sth, second_low, second_high);
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1, sth, 4);
		if (err < 0) {
			return err;
		}
		vlow = halldata->hall_v + XBRP_TOL;
		vhigh = halldata->hall_v + XBOP_TOL;
		ak09970_inttobuff(vth, vlow, vhigh);
		err = ak09970_i2c_write_block(g_chip, AK09970_REG_SWX1+3, vth, 4);
		if (err < 0) {
			return err;
		}
		break;
	default:
		break;
	}
	irqval = gpio_get_value(g_chip->irq_gpio);
	if (err < 0) {
		return false;
	} else {
		return true;
	}
}

static irqreturn_t ak09970_irq_handler(int irq, void *dev_id)
{
	struct extcon_dev_data *hall_dev = NULL;

	if (!g_chip) {
		return -EINVAL;
	}
	if (g_chip->ws) {
		__pm_stay_awake(g_chip->ws);
	}
	hall_dev = i2c_get_clientdata(g_chip->client);
	if (hall_dev && hall_dev->bus_ready == false) {
		wait_event_interruptible_timeout(g_chip->wait,
						 hall_dev->bus_ready,
						 msecs_to_jiffies(50));
	}

	if (hall_dev && hall_dev->bus_ready == false) {
		goto exit;
	}

	threeaxis_hall_irq_handler(DHALL_0);

exit:
	if (g_chip->ws) {
		__pm_relax(g_chip->ws);
	}

	return IRQ_HANDLED;
}

static int ak09970_setup_eint(struct oplus_dhall_chip *chip)
{
	int ret = 0;

	if (gpio_is_valid(chip->irq_gpio)) {
		if (chip->id < DHALL_NUM) {
			ret = gpio_request(chip->irq_gpio, "ak09970-irq");
			if (ret) {
				return -EINVAL;
			}

			ret = gpio_direction_input(chip->irq_gpio);

			msleep(50);

			chip->irq = gpio_to_irq(chip->irq_gpio);
			ret = 0;
		} else {
		}
	} else {
		chip->irq = -EINVAL;
		ret = -EINVAL;
	}


	return ret;
}

static int ak09970_set_detection_mode(u8 mode)
{
	int err = 0;
	u8 sdata[4] = {0};
	if (g_chip== NULL) {
		return -EINVAL;
	}
	err = ak09970_i2c_read_block(g_chip, AK09970_REG_SWX1, sdata, 4);
	if (err < 0) {
		return err;
	}
	if (!g_chip->irq_enabled) {
		if (g_chip->irq > 0) {
			err = request_threaded_irq(g_chip->irq, NULL,&ak09970_irq_handler,IRQ_TYPE_LEVEL_LOW | IRQF_ONESHOT,
							"ak09970_0", (void *)g_chip->client);
			if (err < 0) {
				return -EINVAL;
			}
		} else {
			return -EINVAL;
		}
		irq_set_irq_wake(g_chip->irq, 1);

		g_chip->irq_enabled = 1;
	}

	return 0;
}

static int ak09970_enable_irq(bool enable)
{
	if (g_chip == NULL) {
		return -EINVAL;
	}

	if (enable) {
		enable_irq(g_chip->irq);
	} else {
		disable_irq_nosync(g_chip->irq);
	}
	return 0;
}

static int ak09970_clear_irq(void)
{
	if (g_chip == NULL) {
		return -EINVAL;
	}

	return 0;
}

static int ak09970_get_irq_state(void)
{
	if (g_chip == NULL) {
		return -EINVAL;
	}

	return g_chip->irq_enabled;
}

static void ak09970_set_sensitivity(char *value)
{
	int i = 0;
	struct hall_srs *srs = NULL;


	for (i = 0; i < sizeof(ak09970_ranges) / sizeof(struct hall_srs); i++) {
		srs = &ak09970_ranges[i];
		if (!strncmp(srs->name, value, strlen(srs->name))) {
			break;
		} else {
			srs = NULL;
		}
	}

	if (!srs) {
		return;
	}
}

static int ak09970_reset_device(struct oplus_dhall_chip *chip)
{
	int err = 0;
	u8 data = 0;
	u8 rdata = 0;

	data = AK09970_VAL_SRST_RESET;
	err = ak09970_i2c_write_block(chip, AK09970_REG_SRST, &data, 1);
	if (err < 0) {
		return err;
	}
	msleep(5);
	rdata = 0x2a;
	err = ak09970_i2c_write_block(chip, AK09970_REG_CNTL2, &rdata, 1);
	if (err < 0) {
		return err;
	}

	return err;
}

static void ak09970_parse_dts(struct oplus_dhall_chip *chip)
{
	struct device_node *np = NULL;
	int rc = 0;
	uint32_t value;

	np = chip->client->dev.of_node;
	chip->irq_gpio = of_get_named_gpio(np, "dhall,irq-gpio", 0);
	chip->power_1v8 = regulator_get(&chip->client->dev, "vcc");
	chip->pctrl = devm_pinctrl_get(&chip->client->dev);
	chip->irq_state = pinctrl_lookup_state(chip->pctrl, "hall_interrupt_input");
	chip->enable_hidden = of_property_read_bool(np, "hall,bias_support");
	if (chip->enable_hidden) {
		rc = of_property_read_u32(np, "hall,bias-ratio", &value);
		if (rc) {
			chip->bias_ratio = 100;
		} else {
			chip->bias_ratio = value;
		}
	}
}

static struct dhall_operations  ak09970_ops = {
	.enable_irq = ak09970_enable_irq,
	.clear_irq = ak09970_clear_irq,
	.get_irq_state = ak09970_get_irq_state,
	.set_detection_mode = ak09970_set_detection_mode,
	.update_threeaxis_threshold = ak09970_update_threshold,
	.dump_regs = ak09970_dump_reg,
	.set_reg = ak09970_set_reg,
	.is_power_on = ak09970_is_power_on,
	.set_sensitivity = ak09970_set_sensitivity,
	.get_threeaxis_data  = ak09970_get_data,
};

static int ak09970_i2c_probe(struct i2c_client *client)
{
	struct oplus_dhall_chip *chip = NULL;
	struct extcon_dev_data	*hall_dev = NULL;
	int err = 0;


	chip = devm_kzalloc(&client->dev, sizeof(struct oplus_dhall_chip), GFP_KERNEL);
	if (!chip) {
		return -ENOMEM;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		return -EOPNOTSUPP;
	}

	hall_dev = kzalloc(sizeof(struct extcon_dev_data), GFP_KERNEL);
	if (!hall_dev) {
		TRI_KEY_ERR("kernel memory alocation was failed\n");
		return -ENOMEM;
	}

	chip->client = client;
	hall_dev->client = client;
	i2c_set_clientdata(client, hall_dev);
	hall_dev->dev = &client->dev;
	ak09970_parse_dts(chip);

	ak09970_set_power(chip, 1);

	if (!IS_ERR_OR_NULL(chip->pctrl) && !IS_ERR_OR_NULL(chip->irq_state)) {
		pinctrl_select_state(chip->pctrl, chip->irq_state);
	}

	err = ak09970_reset_device(chip);
	if (err < 0) {
		goto fail;
	}

	err = ak09970_setup_eint(chip);
	if (err < 0) {
		goto fail;
	}

	init_waitqueue_head(&chip->wait);
	hall_dev->bus_ready = true;
	chip->ws = wakeup_source_register(&chip->client->dev, dev_name(&chip->client->dev));

	g_chip = chip;
	oplus_register_hall("three_axis_hall", &ak09970_ops, hall_dev);

	err = oplus_hall_register_notifier();
	if (err < 0) {
		goto fail;
	}


	return 0;

fail:
	if (hall_dev) {
		kfree(hall_dev);
		hall_dev = NULL;
	}
	return -ENXIO;
}

static void ak09970_i2c_remove(struct i2c_client *client)
{
	struct extcon_dev_data	*hall_dev = NULL;

	if (g_chip && g_chip->ws) {
		wakeup_source_unregister(g_chip->ws);
	}

	oplus_hall_unregister_notifier();

	hall_dev = i2c_get_clientdata(client);
	if (hall_dev) {
		kfree(hall_dev);
		hall_dev = NULL;
	}
	return;
}

static int ak09970_i2c_suspend(struct device *dev)
{
	struct extcon_dev_data *hall_dev = NULL;

	if (!g_chip)
		return 0;

	if (g_chip->irq == 0)
		return 0;

	hall_dev = i2c_get_clientdata(g_chip->client);
	if (hall_dev) {
		hall_dev->bus_ready = false;
	}

	if (!g_chip->irq_wake) {
		enable_irq_wake(g_chip->irq);
		g_chip->irq_wake = true;
	}

	return 0;
}

static int ak09970_i2c_resume(struct device *dev)
{
	struct extcon_dev_data *hall_dev = NULL;

	if (!g_chip)
		return 0;

	if (g_chip->irq == 0)
		return 0;

	if (g_chip->irq_wake) {
		disable_irq_wake(g_chip->irq);
		g_chip->irq_wake = false;
	}

	hall_dev = i2c_get_clientdata(g_chip->client);
	if (hall_dev) {
		hall_dev->bus_ready = true;
	}

	wake_up_interruptible(&g_chip->wait);

	return 0;
}

static const struct of_device_id ak09970_match[] = {
	{ .compatible = "oplus,dhall-ak09970"},
	{},
};

static const struct i2c_device_id ak09970_id[] = {
	{"dhall-ak09970", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, ak09970_id);

static const struct dev_pm_ops ak09970_pm_ops = {
	.suspend = ak09970_i2c_suspend,
	.resume = ak09970_i2c_resume,
};

static struct i2c_driver ak09970_i2c_driver = {
	.driver = {
		.name	= "dhall-ak09970",
		.of_match_table =  ak09970_match,
		.pm = &ak09970_pm_ops,
	},
	.probe		= ak09970_i2c_probe,
	.remove		= ak09970_i2c_remove,
	.id_table	= ak09970_id,
};

static int __init ak09970_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&ak09970_i2c_driver);
	return 0;
}

static void __exit ak09970_exit(void)
{
	i2c_del_driver(&ak09970_i2c_driver);
}

module_init(ak09970_init);
module_exit(ak09970_exit);

MODULE_DESCRIPTION("AK09970 hallswitch driver");
MODULE_LICENSE("GPL");