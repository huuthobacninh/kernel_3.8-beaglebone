/* xfrm_user.c: User interface to configure xfrm engine.
 *
 * Copyright (C) 2002 David S. Miller (davem@redhat.com)
 *
 * Changes:
 *	Mitsuru KANDA @USAGI
 * 	Kazunori MIYAZAWA @USAGI
 * 	Kunihiro Ishiguro <kunihiro@ipinfusion.com>
 * 		IPv6 support
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>


static int __init thonh_module_init(void)
{
	printk(KERN_EMERG "%s:ThoNH-------------------init function \n", __func__);

	return(0);
}

static void __exit thonh_module_exit(void)
{
	printk(KERN_EMERG "%s:ThoNH-------------------exit function \n", __func__);
}

module_init(thonh_module_init);
module_exit(thonh_module_exit);
MODULE_LICENSE("GPL");

