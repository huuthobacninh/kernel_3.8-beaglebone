
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>


static struct of_device_id thonh_button_gpio_of_match[] = {
	{ .compatible = "thonh_button_gpio", },
	{ },
};

MODULE_DEVICE_TABLE(of, thonh_button_gpio_of_match);


static irqreturn_t button_gpio_irq(int irq, void *dev_id)
{
	pr_err("%s:ThoNH--------------- press pushbutton \n", __func__);
	
	return IRQ_HANDLED;
}

static int thonh_button_gpio_probe(struct platform_device *pdev)
{
	int test_irq, gpio_test, err;
			
	gpio_test = of_get_named_gpio(pdev->dev.of_node, "gpios", 0);
	gpio_request(gpio_test, "thonh-button");
	gpio_direction_input(gpio_test);
	gpio_set_debounce(gpio_test, 100);
	test_irq = gpio_to_irq(gpio_test);
	err = request_irq(test_irq, button_gpio_irq, IRQF_TRIGGER_FALLING, "thonh_isr", NULL);
	
	return 0;
}


static struct platform_driver thonh_button_gpio_device_driver = {
	.probe		= thonh_button_gpio_probe,
	.driver		= {
		.name	= "thonh_button_gpio",
		.owner	= THIS_MODULE,
		.of_match_table = thonh_button_gpio_of_match,
	}
};

static int __init thonh_button_gpio_init(void)
{
	return platform_driver_register(&thonh_button_gpio_device_driver);
	
}

static void __exit thonh_button_gpio_exit(void)
{
	platform_driver_unregister(&thonh_button_gpio_device_driver);
}

module_init(thonh_button_gpio_init);
module_exit(thonh_button_gpio_exit);


MODULE_AUTHOR("Raphael Assenat <raph@8d.com>, Trent Piepho <tpiepho@freescale.com>");
MODULE_DESCRIPTION("ThoNH GPIO BUTTON CONFIG");
MODULE_LICENSE("GPL");
