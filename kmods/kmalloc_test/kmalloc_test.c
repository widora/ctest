#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

MODULE_LICENSE("GPL");

unsigned char *pagemem;
unsigned char *kmallocmem;
unsigned char *vmallocmem;

/*-----------------------------------------
test result: in MIPS structure 
pagemem and kmallocmem will callocate mem from 0x8000.0000 to 0xA000.0000
vmallocmem will allocate mem starting from 0xC000,0000
---------------------------------------*/


int __init mem_module_init(void)
{
	printk("PAGE_SIZE=%d\n",PAGE_SIZE);

	pagemem=(unsigned char*)__get_free_page(0); // 2^0=1 page
	printk("<1>pagemem addr=%x\n",pagemem);

	kmallocmem=(unsigned char*)kmalloc(100,0); //size:100  flag:0 GFP_NOWAIT 
//allocate continuous physical and virtual address. 
//??Max.32xPAGE_SIZE=32x4k=128k(-16 page description)
//-- Min 32 or 64 bytes depending on system structure
	printk("<1>kmallocmem addr=%x\n",kmallocmem);

	vmallocmem=(unsigned char*)vmalloc(1000000); //size:1000000bytes   
//allocate continuous virtual address. 
//??Max.32xPAGE_SIZE=32x4k=128k(-16 page description)
//-- Min 32 or 64 bytes depending on system structure
	printk("<1>vmallocmem addr=%x\n",vmallocmem);

	return 0;
}


void __exit mem_module_exit(void)
{
	free_page(pagemem);
	kfree(kmallocmem);
	vfree(vmallocmem);
}


module_init(mem_module_init);
module_exit(mem_module_exit);
