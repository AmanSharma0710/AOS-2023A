/*
PART B: Assignment 1
------------------------------------------
Aman Sharma 	: 20CS30063
Ashish Rekhani 	: 20CS10010
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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aman Sharma | Ashish Rekhani");
MODULE_DESCRIPTION("Creates /proc entry with deque functionality");

#define DEVICE_NAME "partb_1_20CS30063_20CS10010"
#define current get_current()

static DEFINE_MUTEX(deque_mutex);	// Mutex on the structure that holds the deque

struct _deque{
	int32_t *arr;
	int8_t front;
	int8_t rear;
	int8_t capacity;
	int8_t left_uninit;
};
typedef struct _deque deque;

/* Deque methods */
static deque* CreateDeque(int8_t capacity);
static void DestroyDeque(deque* d);
static int32_t isFull(deque *d);
static int32_t isEmpty(deque *d);
static int32_t push_front(deque *d, int32_t key);
static int32_t pop_front(deque *d, int32_t *key);
static int32_t push_back(deque *d, int32_t key);
static int32_t pop_back(deque *d, int32_t *key);
static int32_t push(deque *d, int32_t key);
static int32_t pop(deque *d, int32_t *key);


// We use a rb-tree to store the process pid and its corresponding deque
struct _deque_node{
	struct rb_node node;
	int32_t key;
	deque *value;
};
typedef struct _deque_node deque_node;

/* RB-Tree Methods */
static deque_node* search(struct rb_root *root, int32_t key);
static void insert_node(struct rb_root *root, deque_node *node);
static void delete_node(struct rb_root *root, int32_t key);
static void destroy_rbtree(struct rb_root *root);

/* Defining methods for the proc file */
static int     my_open(struct inode *, struct file *);
static int     my_close(struct inode *, struct file *);
static ssize_t my_read(struct file *, char *, size_t, loff_t *);
static ssize_t my_write(struct file *, const char *, size_t, loff_t *);


/* Declare other needed global variables */
static struct rb_root root = RB_ROOT;
static struct rb_node *node = NULL;
static struct rb_node **new = NULL;
static struct rb_node *parent = NULL;
static deque_node* data = NULL;


/* Define the proc operations and initialize the proc entry */
static struct proc_ops file_ops =
{
	.proc_open = my_open,		
	.proc_release = my_close,
	.proc_read = my_read,
	.proc_write = my_write,
};

static deque* CreateDeque(int8_t capacity) {
	deque *d = (deque *) kmalloc(sizeof(deque), GFP_KERNEL);
	if (d == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error in allocating deque!", current->pid);
		return NULL;
	}
	if(capacity <= 0 || capacity > 100) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong size of deque %d!!\n", current->pid, capacity);
		d->left_uninit = 1;
		d->arr = NULL;
		d->capacity = 0;
		d->front = -1;
		d->rear = -1;
		return d;
	}
	d->capacity = capacity;
	d->front = -1;
	d->rear = 0;
	d->left_uninit = 0;
	d->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_KERNEL); //size in bytes
	if (d->arr == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error while allocating deque->arr!", current->pid);
		kfree(d);
		return NULL;
	}
	return d;
}

static void DestroyDeque(deque* d) {
	if (d == NULL)
		return; // deque is not allocated
	printk(KERN_INFO DEVICE_NAME ": PID %d, %ld bytes of deque->arr Space freed.\n", current->pid, sizeof(d->arr));
	if(d->arr != NULL)
		kfree(d->arr);
	kfree(d);
	return;
}

static int32_t isFull(deque *d) {
	return ((d->front == 0 && d->rear == d->capacity - 1) || d->front == d->rear + 1);
}

static int32_t isEmpty(deque *d) {
	return (d->front == -1);
}

static int32_t push_front(deque *d, int32_t key) {
	if (isFull(d)) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque is full!!\n", current->pid);
		return -EACCES;
	}
	if (d->front == -1) {
		d->front = 0;
		d->rear = 0;
	}
	else if (d->front == 0)
		d->front = d->capacity - 1;
	else
		d->front = d->front - 1;
	d->arr[d->front] = key;
	return 0;
}

static int32_t pop_front(deque *d, int32_t *key) {
	if (isEmpty(d)) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque is empty!!\n", current->pid);
		return -EACCES;
	}
	*key = d->arr[d->front];
	if (d->front == d->rear) {
		d->front = -1;
		d->rear = -1;
	}
	else if (d->front == d->capacity - 1)
		d->front = 0;
	else
		d->front = d->front + 1;
	return 0;
}

static int32_t push_back(deque *d, int32_t key) {
	if (isFull(d)) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque is full!!\n", current->pid);
		return -EACCES;
	}
	if (d->front == -1) {
		d->front = 0;
		d->rear = 0;
	}
	else if (d->rear == d->capacity - 1)
		d->rear = 0;
	else
		d->rear = d->rear + 1;
	d->arr[d->rear] = key;
	return 0;
}

static int32_t pop_back(deque *d, int32_t *key) {
	if (isEmpty(d)) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque is empty!!\n", current->pid);
		return -EACCES;
	}
	*key = d->arr[d->rear];
	if (d->front == d->rear) {
		d->front = -1;
		d->rear = -1;
	}
	else if (d->rear == 0)
		d->rear = d->capacity - 1;
	else
		d->rear = d->rear - 1;
	return 0;
}

static int32_t push(deque *d, int32_t key) {
	if (d->left_uninit) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque is not initialized!!\n", current->pid);
		return -EACCES;
	}
	if(key%2==0){
		return push_back(d, key);
	}
	else{
		return push_front(d, key);
	}
	return 0;
}

static int32_t pop(deque *d, int32_t *key) {
	if (d->left_uninit) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque is not initialized!!\n", current->pid);
		return -EACCES;
	}
	return pop_front(d, key);
}


static deque_node* search(struct rb_root *root, int32_t key) {
	node = root->rb_node;
	while(node){
		data = rb_entry(node, deque_node, node);
		if(key < data->key)
			node = node->rb_left;
		else if(key > data->key)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static void insert_node(struct rb_root *root, deque_node *node) {
	new = &(root->rb_node);
	parent = NULL;
	while(*new){
		data = rb_entry(*new, deque_node, node);
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

static void delete_node(struct rb_root *root, int32_t key) {
	node = root->rb_node;
	while(node){
		data = rb_entry(node, deque_node, node);
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

static void destroy_rbtree(struct rb_root *root) {
	node = NULL;
	while((node = rb_first(root)) != NULL){
		data = rb_entry(node, deque_node, node);
		DestroyDeque(data->value);
		rb_erase(node, root);
		kfree(data);
	}
}


static ssize_t my_write(struct file *file, const char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;
	/* The following variables need to be declared locally otherwise multiple processes will pollute the data */
	deque_node *data_local = NULL;
	char buffer[4];

	mutex_lock(&deque_mutex);
	data_local = search(&root, current->pid);
	mutex_unlock(&deque_mutex);

	if(data_local == NULL){
		printk(KERN_ALERT DEVICE_NAME ": PID %d Process not found in RBTree", current->pid);
		return -EACCES;
	}

	if(data_local->value == NULL){
		// Implies that the process has not initialized the deque
		if(count!=1){
			// If the process has not initialized the deque, it can only send 1 byte
			printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong data sent. %ld bytes", current->pid, count);
			// Initialize the deque with invalid values so that the process can't use it
			data_local->value = CreateDeque(-1);
			return -EINVAL;
		}
		// Use the first byte to store the deque size
		if(copy_from_user(buffer, buf, 1)){
			printk(KERN_ALERT DEVICE_NAME ": PID %d Error in copying data from user", current->pid);
			return -EINVAL;
		}
		// Initialize the deque
		data_local->value = CreateDeque(buffer[0]);
		if(data_local->value->left_uninit){
			// If the deque is not initialized, return error
			return -EINVAL;
		}
		return 1;
	}
	// Implies that the process has already initialized the deque
	if(count!=4){
		// If the process has already initialized the deque, it can only send 4 bytes
		printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong data sent. %ld bytes", current->pid, count);
		return -EINVAL;
	}
	if(data_local->value->left_uninit){
		// If the deque is not initialized, return error
		return -EINVAL;
	}
	// Use the first 4 bytes to store the element to be pushed
	if(copy_from_user(buffer, buf, 4)){
		printk(KERN_ALERT DEVICE_NAME ": PID %d Error in copying data from user", current->pid);
		return -EINVAL;
	}
	printk(KERN_INFO DEVICE_NAME ": PID %d pushing %d to deque", current->pid, *(int32_t*)buffer);
	// Push the element to the deque
	if(push(data_local->value, *(int32_t*)buffer) != 0){
		// If the deque is full, return error
		return -EINVAL;
	}
	return 4;
}

static ssize_t my_read(struct file *file, char* buf, size_t count, loff_t* pos) {
	if(!buf || !count)
		return -EINVAL;
	deque_node *data_local = NULL;
	char buffer[4];
	int32_t val = 0;

	mutex_lock(&deque_mutex);
	data_local = search(&root, current->pid);
	mutex_unlock(&deque_mutex);
	
	if(data_local == NULL){
		printk(KERN_ALERT DEVICE_NAME ": PID %d Process not found in RBTree", current->pid);
		return -EACCES;
	}
	if(data_local->value == NULL){
		// Implies that the process has not initialized the deque
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque not initialized", current->pid);
		return -EACCES;
	}
	if(data_local->value->left_uninit){
		// Implies that the process has not initialized the deque
		printk(KERN_ALERT DEVICE_NAME ": PID %d Deque not initialized", current->pid);
		return -EACCES;
	}
	// Implies that the process has already initialized the deque
	if(count<4){
		// If a process is reading from the deque, it can only read 4 bytes thus needs buffer of size 4 atleast
		printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong data sent. %ld bytes", current->pid, count);
		return -EINVAL;
	}
	// Pop the element from the deque
	if(pop(data_local->value, &val) != 0){
		// If the deque is empty, return error
		return -EINVAL;
	}
	// Use the first 4 bytes to store the element to be popped
	if(copy_to_user(buf, &val, 4)){
		printk(KERN_ALERT DEVICE_NAME ": PID %d Error in copying data to user", current->pid);
		return -EINVAL;
	}
	printk(KERN_INFO DEVICE_NAME ": PID %d popping %d from deque", current->pid, val);
	return 4;
}

static int my_open(struct inode *inodep, struct file *filep) {
	deque_node *data_local = NULL;
	mutex_lock(&deque_mutex);
	// If same process has already opened the file
	if(search(&root, current->pid) != NULL){
		printk(KERN_ALERT DEVICE_NAME ": PID %d, Tried to open twice\n", current->pid);
		mutex_unlock(&deque_mutex);
		return -EACCES;
	}
	mutex_unlock(&deque_mutex);

	// Create a new entry for the process
	data_local = kmalloc(sizeof(deque_node), GFP_KERNEL);
	if(data_local == NULL){
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error while allocating deque_node!", current->pid);
		return -ENOMEM;
	}
	data_local->key = current->pid;
	data_local->value = NULL;
	mutex_lock(&deque_mutex);
	printk(DEVICE_NAME ": PID %d !! Adding %d to RBTree !!\n", current->pid, data_local->key);
	insert_node(&root, data_local);
	mutex_unlock(&deque_mutex);
	printk(KERN_INFO DEVICE_NAME ": PID %d Device successfully opened\n", current->pid);
	return 0;
}

static int my_close(struct inode *inodep, struct file *filep) {
	mutex_lock(&deque_mutex);
	// Delete the process entry from the RBTree
	delete_node(&root, current->pid);
	mutex_unlock(&deque_mutex);
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
