/*------------------------------------------------------------
Suppose your module name is [module name], and failed in the kernel:
1.  cat /proc/kallsyms | grep [module name] | grep __this_module
2.  It will returns like:
	863f8a94 d __this_module [module name]
	Then use the addr. to replaced the one in following codes.
3.  compile to get the module force_rmmod.ko.
4.  insmod force_rmmod.ko 
5.  then you can  rmmod [module name]

BUG:

error: Driver 'xxxxx' is already registered, aborting...
------------------------------------------------------------*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <asm-generic/local.h>
#include <linux/cpumask.h>

void force(void)
{
}

static int __init force_rmmod_init(void)
{
	struct module *mod=(struct module*)0x863f8a94;
	int i,k;
//	int o=0;
	mod->state=MODULE_STATE_LIVE;
	mod->exit=force;
	k=module_refcount(mod);
	for(i=0;i<k;i++)module_put(mod);  //--clear ref count
/*
	for(i=0;i<NR_CPUS;i++){
		mod->ref[i].count=*(local_t *)&o;
	}
*/
	return 0;
}

static void __exit force_rmmod_exit(void)
{

}



module_init(force_rmmod_init);
module_exit(force_rmmod_exit);
MODULE_LICENSE("GPL");
