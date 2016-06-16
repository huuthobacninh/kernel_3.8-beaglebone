/*
 * LEDs driver for GPIOs
 *
 * Copyright (C) 2007 8D Technologies inc.
 * Raphael Assenat <raph@8d.com>
 * Copyright (C) 2008 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>


struct thonh_leds {
	int gpios_id;
	int value;
	struct hrtimer hrtimer;
	
};
//struct thonh_leds *my_leds;

static struct of_device_id thonh_leds_gpio_of_match[] = {
	{ .compatible = "thonh_leds_gpio", },
	{ },
};
MODULE_DEVICE_TABLE(of, thonh_leds_gpio_of_match);

static enum hrtimer_restart hrtimer_callback( struct hrtimer *timer){
	struct thonh_leds *leds = container_of(timer, struct thonh_leds, hrtimer);

	if(leds->value == 1)
		leds->value = 0;
	else
		leds->value = 1;
	gpio_set_value(leds->gpios_id, leds->value);
	
	hrtimer_start( &leds->hrtimer, ktime_set(0, 800000000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}


static int thonh_leds_gpio_probe(struct platform_device *pdev)
{
//	pr_err("%s:ThoNH--------------- probe \n", __func__);
	int node_count;
	const char *state;
	struct thonh_leds *temp, *my_leds;
	struct device_node *child, *np = pdev->dev.of_node;

	node_count = of_get_child_count(np);
	my_leds = kmalloc(node_count*sizeof(struct thonh_leds), GFP_KERNEL);
	temp = &my_leds[0];
	for_each_child_of_node(np, child){
		temp->gpios_id = of_get_named_gpio(child, "gpios", 0);
		state = of_get_property(child, "default-state", NULL);
		if(!strcmp(state, "on"))
			temp->value = 1;
		else
			temp->value = 0;
		gpio_direction_output(temp->gpios_id, temp->value);
		temp++;
		
	}
	hrtimer_init( &my_leds->hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	my_leds->hrtimer.function = &hrtimer_callback;
	hrtimer_start( &my_leds->hrtimer, ktime_set(0, 800000000), HRTIMER_MODE_REL);
	
		
/*	
	gpio_test = of_get_named_gpio(pdev->dev.of_node, "gpios", 0);
	gpio_direction_output(gpio_test, 1);
	gpio_set_value(gpio_test, 1);
*/
	return 0;
}


static struct platform_driver thonh_leds_gpio_device_driver = {
	.probe		= thonh_leds_gpio_probe,
	.driver		= {
		.name	= "thonh_leds_gpio",
		.owner	= THIS_MODULE,
		.of_match_table = thonh_leds_gpio_of_match,
	}
};

static int __init thonh_leds_gpio_init(void)
{
	return platform_driver_register(&thonh_leds_gpio_device_driver);
	
}

static void __exit thonh_leds_gpio_exit(void)
{
	platform_driver_unregister(&thonh_leds_gpio_device_driver);
}

module_init(thonh_leds_gpio_init);
module_exit(thonh_leds_gpio_exit);


MODULE_AUTHOR("Raphael Assenat <raph@8d.com>, Trent Piepho <tpiepho@freescale.com>");
MODULE_DESCRIPTION("ThoNH GPIO LEDS CONFIG");
MODULE_LICENSE("GPL");
