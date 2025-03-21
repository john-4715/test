#include <linux/build-salt.h>
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(.gnu.linkonce.this_module) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section(__versions) = {
	{ 0xdd8f8694, "module_layout" },
	{ 0x609f1c7e, "synchronize_net" },
	{ 0x6565e06e, "sock_release" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xc5850110, "printk" },
	{ 0xac5bc229, "netlink_unicast" },
	{ 0x45a04fed, "skb_pull" },
	{ 0x30cb0399, "init_net" },
	{ 0x62a38e34, "nf_register_net_hook" },
	{ 0xbbea7e99, "nf_unregister_net_hook" },
	{ 0x71a7b444, "__alloc_skb" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xa1407a51, "__netlink_kernel_create" },
	{ 0x69acdf38, "memcpy" },
	{ 0xaf0ee0b5, "__nlmsg_put" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "E619699DD7D210459758641");
