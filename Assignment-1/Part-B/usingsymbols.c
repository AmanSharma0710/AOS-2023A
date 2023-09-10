#include<linux/init.h>
#include<linux/module.h>

MODULE_LICENSE("GPL");

extern int loadingsymbol_fun(void);

int usingsymbol_entry(void){
    printk(KERN_ALERT "We are inside %s function!\n", __FUNCTION__);
    loadingsymbol_fun();
    return 0;
}

void usingsymbol_exit(void){
    printk(KERN_ALERT "We are inside %s function!\n", __FUNCTION__);
    return;
}

module_init(usingsymbol_entry);
module_exit(usingsymbol_exit);