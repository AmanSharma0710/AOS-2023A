/*
PART B: Assignment 1
------------------------------------------
Aman Sharma 	: 20CS30063
Ashish Rekhani 	: 20CS1010
------------------------------------------
Kernel Version used : 5.10.191
System : Ubuntu 20.04 LTS
*/


#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/rbtree.h>
#include <linux/random.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aman Sharma | Ashish Rekhani");
MODULE_DESCRIPTION("Creates /proc entry with deque functionality");

#define DEVICE_NAME "partb_1_20CS30063_20CS10010"
#define current get_current()

static DEFINE_MUTEX(deque_mutex);	// Mutex on the structure that holds the deque


// We use a rb-tree to store the process pid and its corresponding heap
struct _deque_node{
    struct rb_node node;
	int32_t key;
	int32_t value;
};
typedef struct _deque_node deque_node;

/* RB-Tree Methods */
static struct rb_root root = RB_ROOT;
// static struct rb_node *node = NULL;
static deque_node* search(struct rb_root *root, int32_t key);
static void insert_node(struct rb_root *root, deque_node *node);
static void delete_node(struct rb_root *root, int32_t key);
static void print_tree(struct rb_root *root);
static void destroy_rbtree(struct rb_root *root);

/* Defining methods for the proc file */
static int     my_open(struct inode *, struct file *);
static int     my_close(struct inode *, struct file *);
static ssize_t my_read(struct file *, char *, size_t, loff_t *);
static ssize_t my_write(struct file *, const char *, size_t, loff_t *);


//Declare other needed global variables

static struct proc_ops file_ops =
{
	.proc_open = my_open,		
	.proc_release = my_close,
	.proc_read = my_read,
	.proc_write = my_write,
};

static deque_node* search(struct rb_root *root, int32_t key){
    struct rb_node *node = root->rb_node;
    while(node){
        deque_node* data = rb_entry(node, deque_node, node);
        if(key < data->key)
            node = node->rb_left;
        else if(key > data->key)
            node = node->rb_right;
        else
            return data;
    }
    return NULL;
}

static void insert_node(struct rb_root *root, deque_node *node){
    struct rb_node **new = &(root->rb_node), *parent = NULL;
    while(*new){
        deque_node* data = rb_entry(*new, deque_node, node);
        parent = *new;
        if(node->key < data->key)
            new = &((*new)->rb_left);
        else if(node->key > data->key)
            new = &((*new)->rb_right);
        else
            return;
    }
    rb_link_node(&node->node, parent, new);
    rb_insert_color(&node->node, root);
}

static void delete_node(struct rb_root *root, int32_t key){
    struct rb_node *node = root->rb_node;
    while(node){
        deque_node* data = rb_entry(node, deque_node, node);
        if(key < data->key)
            node = node->rb_left;
        else if(key > data->key)
            node = node->rb_right;
        else{
            rb_erase(node, root);
            kfree(data);
            return;
        }
    }
}

static void print_tree(struct rb_root *root){
    printk(KERN_INFO "Printing the rbtree\n");
    struct rb_node *node;
    for(node = rb_first(root); node; node = rb_next(node)){
        deque_node* data = rb_entry(node, deque_node, node);
        printk(KERN_INFO "Key: %d, Value: %d\n", data->key, data->value);
    }
}

static void destroy_rbtree(struct rb_root *root){
    struct rb_node *node = NULL;
    while((node = rb_first(root)) != NULL){
        rb_erase(node, root);
        kfree(rb_entry(node, deque_node, node));
    }
}


static ssize_t my_write(struct file *file, const char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;

	//Print the data received from the user
    printk(KERN_INFO DEVICE_NAME ": PID %d Received %ld characters from the user\n", current->pid, count);
    printk(KERN_INFO DEVICE_NAME ": PID %d Received %s from the user\n", current->pid, buf);
    //also print offset
    printk(KERN_INFO DEVICE_NAME ": PID %d Offset is %lld\n", current->pid, *pos);
    int i, j;
    get_random_bytes(&i, sizeof(i));
    get_random_bytes(&j, sizeof(j));
    i %= 10;
    j %= 10;
    if(search(&root, i) != NULL){
    	printk(KERN_INFO DEVICE_NAME ": PID %d Key %d already exists in the rbtree\n", current->pid, i);
    	return count;
    }
    printk(KERN_INFO DEVICE_NAME ": PID %d Adding %d and %d to the rbtree\n", current->pid, i, j);
    deque_node* node = kmalloc(sizeof(deque_node), GFP_KERNEL);
    node->key = i;
    node->value = j;
    insert_node(&root, node);
    print_tree(&root);
    return count;
}


static ssize_t my_read(struct file *file, char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;

	//Print the data received from the user
    printk(KERN_INFO DEVICE_NAME ": PID %d Received %ld characters from the user\n", current->pid, count);
    printk(KERN_INFO DEVICE_NAME ": PID %d Received %s from the user\n", current->pid, buf);
    //also print offset
    printk(KERN_INFO DEVICE_NAME ": PID %d Offset is %lld\n", current->pid, *pos);
    int i, j;
    get_random_bytes(&i, sizeof(i));
    get_random_bytes(&j, sizeof(j));
    i %= 10;
    j %= 10;
    //if node with key i exists, then delete it
    printk(KERN_INFO DEVICE_NAME ": PID %d Deleting %d from the rbtree\n", current->pid, i);
    delete_node(&root, i);
    print_tree(&root);
    return count;
}


static int my_open(struct inode *inodep, struct file *filep) {
	printk(KERN_INFO DEVICE_NAME ": PID %d Device successfully opened\n", current->pid);
	return 0;
}


static int my_close(struct inode *inodep, struct file *filep) {
	printk(KERN_INFO DEVICE_NAME ": PID %d Device successfully closed\n", current->pid);
	return 0;
}


static int hello_init(void) {
	struct proc_dir_entry *entry = proc_create(DEVICE_NAME, 0777, NULL, &file_ops);
	if (!entry)
		return -ENOENT;
	printk(KERN_ALERT DEVICE_NAME ": Initialising the LKM!\n");
	mutex_init(&deque_mutex);
	return 0;
}


static void hello_exit(void) {
	mutex_destroy(&deque_mutex);
	destroy_rbtree(&root);
	remove_proc_entry(DEVICE_NAME, NULL);

	printk(KERN_ALERT DEVICE_NAME ": Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);
