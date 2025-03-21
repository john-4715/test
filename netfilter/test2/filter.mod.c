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
	{ 0xbbea7e99, "nf_unregister_net_hook" },
	{ 0x62a38e34, "nf_register_net_hook" },
	{ 0xc5850110, "printk" },
	{ 0x9d92fb56, "dev_queue_xmit" },
	{ 0xf83d29ed, "skb_push" },
	{ 0xe113bbbc, "csum_partial" },
	{ 0xa3bfbf9e, "dev_get_by_name" },
	{ 0x45a04fed, "skb_pull" },
	{ 0x1b6314fd, "in_aton" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "50BA45D9730FD54560CA6DB");
