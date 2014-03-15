#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x41086e, "module_layout" },
	{ 0xe2d5255a, "strcmp" },
	{ 0xf2a644fb, "copy_from_user" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x61651be, "strcat" },
	{ 0xe914e41e, "strcpy" },
	{ 0xd35ce8b4, "pid_vnr" },
	{ 0x9c499607, "get_task_pid" },
	{ 0x884f145, "kmem_cache_alloc_trace" },
	{ 0xc288f8ce, "malloc_sizes" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0xd0d8621b, "strlen" },
	{ 0x7d0bea0b, "per_cpu__current_task" },
	{ 0x71dd4137, "debugfs_create_file" },
	{ 0xb72397d5, "printk" },
	{ 0xe64be54c, "debugfs_create_dir" },
	{ 0x37a0cba, "kfree" },
	{ 0xb16161a4, "debugfs_remove" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "ADE2245FE7D679E68A0BC99");

static const struct rheldata _rheldata __used
__attribute__((section(".rheldata"))) = {
	.rhel_major = 6,
	.rhel_minor = 5,
};
