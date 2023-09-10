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
static deque* DestroyDeque(deque* d);
static int32_t push(deque *d, int32_t key);
static int32_t pop(deque *d);


// We use a rb-tree to store the process pid and its corresponding heap
struct _deque_node{
	int32_t key;
	struct rb_node node;
};
typedef struct _deque_node deque_node;

/* RB-Tree Methods */
static struct rb_root root = RB_ROOT;
static struct rb_node *node = NULL;
static deque_node* search(struct rb_root *root, int32_t key);
static void insert_node(struct rb_root *root, deque_node *node);
static void delete_node(struct rb_root *root, int32_t key);
static void DestroyRBTree(struct rb_root *root);

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

/* Add a new process with pid to the linked list */
static void key_add(struct h_struct* entry) {
	entry->next = htable->next;
	htable->next = entry;
}

/* Return a process with a specific pic */
static struct h_struct* get_entry_from_key(int key) {
	struct h_struct* temp = htable->next;
	while (temp != NULL) {
		if (temp->key == key) {
			return temp;
		}
		temp = temp->next;
	}
	return NULL;
}

/* Deletes a process entry */
static void key_del(int key) {
	struct h_struct *prev, *temp;
	prev = temp = htable;
	temp = temp->next;

	while (temp != NULL) {
		if (temp->key == key) {
			prev->next = temp->next;
			temp->global_heap = DestroyHeap(temp->global_heap);
			temp->next = NULL;
			printk("PID %d <key_del> Kfree key = %d", current->pid, temp->key);
			kfree(temp);
			break;
		}
		prev = temp;
		temp = temp->next;
	}

}

/* Prints all the process pid */
static void print_key(void) {
	struct h_struct *temp;
	temp = htable->next;
	while (temp != NULL) {
		printk("<print_key> key = %d", temp->key);
		temp = temp->next;
	}
}

/* Destroy Linked List of porcesses */
static void DestroyHashTable(void) {
	struct h_struct *temp, *temp2;
	temp = htable->next;
	htable->next = NULL;
	while (temp != NULL) {
		temp2 = temp;
		printk("<DestroyHashTable> Kfree key = %d", temp->key);
		temp = temp->next;
		kfree(temp2);
	}
	kfree(htable);
}

/* Create Heap */
static Heap *CreateHeap(int32_t capacity, char heap_type) {
	Heap *h = (Heap * ) kmalloc(sizeof(Heap), GFP_KERNEL);

	//check if memory allocation is fails
	if (h == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error in allocating heap!", current->pid);
		return NULL;
	}
	h->heap_type = ((heap_type == MIN_HEAP) ? 0 : 1);
	h->count = 0;
	h->capacity = capacity;
	h->arr = (int32_t *) kmalloc_array(capacity, sizeof(int32_t), GFP_KERNEL); //size in bytes

	//check if allocation succeed
	if ( h->arr == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Memory Error while allocating heap->arr!", current->pid);
		return NULL;
	}
	return h;
}

/* Destroy Heap */
static Heap* DestroyHeap(Heap* heap) {
	if (heap == NULL)
		return NULL; // heap is not allocated
	printk(KERN_INFO DEVICE_NAME ": PID %d, %ld bytes of heap->arr Space freed.\n", current->pid, sizeof(heap->arr));
	kfree_const(heap->arr);
	kfree_const(heap);
	return NULL;
}

/* Insert a number into heap */
static int32_t insert(Heap *h, int32_t key) {
	if ( h->count < h->capacity) {
		h->arr[h->count] = key;
		heapify_bottom_top(h, h->count);
		h->count++;
		h->last_inserted = key;
	}
	else {
		return -EACCES;  // Number of elements exceeded the capacity
	}
	return 0;
}

/* Heapify while inserting */
static void heapify_bottom_top(Heap *h, int32_t index) {
	int32_t temp;
	int32_t parent_node = (index - 1) / 2;

	if (h->heap_type == 0) {	// Min Heap
		if (h->arr[parent_node] > h->arr[index]) {
			//swap and recursive call
			temp = h->arr[parent_node];
			h->arr[parent_node] = h->arr[index];
			h->arr[index] = temp;
			heapify_bottom_top(h, parent_node);
		}
	}
	else {	// Max Heap
		if (h->arr[parent_node] < h->arr[index]) {
			//swap and recursive call
			temp = h->arr[parent_node];
			h->arr[parent_node] = h->arr[index];
			h->arr[index] = temp;
			heapify_bottom_top(h, parent_node);
		}
	}
}

/* Heapify while deleting */
static void heapify_top_bottom(Heap *h, int32_t parent_node) {
	int32_t left = parent_node * 2 + 1;
	int32_t right = parent_node * 2 + 2;
	int32_t temp;

	if (left >= h->count || left < 0)
		left = -1;
	if (right >= h->count || right < 0)
		right = -1;

	if (h->heap_type == 0) {	// Min heap
		int32_t min;
		if (left != -1 && h->arr[left] < h->arr[parent_node])
			min = left;
		else
			min = parent_node;
		if (right != -1 && h->arr[right] < h->arr[min])
			min = right;

		if (min != parent_node) {
			temp = h->arr[min];
			h->arr[min] = h->arr[parent_node];
			h->arr[parent_node] = temp;

			// recursive  call
			heapify_top_bottom(h, min);
		}
	}
	else {	// Max heap
		int32_t max;
		if (left != -1 && h->arr[left] > h->arr[parent_node])
			max = left;
		else
			max = parent_node;
		if (right != -1 && h->arr[right] > h->arr[max])
			max = right;

		if (max != parent_node) {
			temp = h->arr[max];
			h->arr[max] = h->arr[parent_node];
			h->arr[parent_node] = temp;

			// recursive  call
			heapify_top_bottom(h, max);
		}
	}
}

/* Extract the top node of a heap */
static int32_t PopMin(Heap *h) {
	int32_t pop;
	if (h->count == 0) {
		return -INF;
	}
	// replace first node by last and delete last
	pop = h->arr[0];
	h->arr[0] = h->arr[h->count - 1];
	h->count--;
	heapify_top_bottom(h, 0);
	return pop;
}


static ssize_t dev_write(struct file *file, const char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;
	if (copy_from_user(buffer, buf, count < 256 ? count : 256))
		return -ENOBUFS;

	// Get the process corresponing heap
	entry = get_entry_from_key(current->pid);
	if (entry == NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d RAISED ERROR in dev_write entry is non-existent", current->pid);
		return -EACCES;
	}
	// Args Set = 1 if the heap is initialized(not NULL)
	args_set = (entry->global_heap) ? 1 : 0;
	buffer_len = count < 256 ? count : 256;

	// If heap is initialized
	if (args_set) {
		if (count != 4) {
			printk(KERN_ALERT DEVICE_NAME ": PID %d WRONG DATA SENT. %d bytes", current->pid, buffer_len);
			return -EINVAL;
		}
		// Check for unexpected type
		char arr[4];
		memset(arr, 0, 4 * sizeof(char));
		memcpy(arr, buf, count * sizeof(char));
		memcpy(&num, arr, sizeof(num));
		printk(DEVICE_NAME ": PID %d writing %d to heap\n", current->pid, num);

		ret = insert(entry->global_heap, num);
		if (ret < 0) { // Heap is filled to capacity
			return -EACCES;
		}
		return sizeof(num);
	}

	 // Any other call before the heap has been initialized
	if (buffer_len != 2) {
		return -EACCES;
	}

	// Initlize Heap
	heap_type = buf[0];
	heap_size = buf[1];
	printk(DEVICE_NAME ": PID %d HEAP TYPE: %d", current->pid, heap_type);
	printk(DEVICE_NAME ": PID %d HEAP SIZE: %d", current->pid, heap_size);
	printk(DEVICE_NAME ": PID %d RECIEVED:  %ld bytes", current->pid, count);

	if (heap_type != MIN_HEAP && heap_type != MAX_HEAP) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong Heap type sent!! %c\n", current->pid, heap_type);
		return -EINVAL;
	}

	if (heap_size <= 0 || heap_size > 100) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Wrong size of heap %d!!\n", current->pid, heap_size);
		return -EINVAL;
	}

	entry->global_heap = DestroyHeap(entry->global_heap);	// destroy any existing heap before creating a new one
	entry->global_heap = CreateHeap(heap_size, heap_type);	// allocating space for new heap

	return buffer_len;
}


static ssize_t dev_read(struct file *file, char* buf, size_t count, loff_t* pos) {
	if (!buf || !count)
		return -EINVAL;

	// Get the process corresponing heap
	entry = get_entry_from_key(current->pid);
	if (entry == NULL) {
		printk(KERN_ALERT DEVICE_NAME "PID %d RAISED ERROR in dev_read entry is non-existent", current->pid);
		return -EACCES;
	}
	// Args Set = 1 if the heap is initialized(not NULL)
	args_set = (entry->global_heap) ? 1 : 0;

	 // If heap is not initialized
	if (args_set == 0) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d Heap not initialized", current->pid);
		return -EACCES;
	}

	// Extract the topmost node of heap
	topnode = PopMin(entry->global_heap);
	printk(DEVICE_NAME ": PID %d asking for %ld bytes\n", current->pid, count);
	retval = copy_to_user(buf, (int32_t*)&topnode, count < sizeof(topnode) ? count : sizeof(topnode));

	if (retval == 0 && topnode != -INF) {    // success!
		printk(KERN_INFO DEVICE_NAME ": PID %d Sent %ld chars with value %d to the user\n", current->pid, sizeof(topnode), topnode);
		return sizeof(topnode);
	}
	else {
		printk(KERN_INFO DEVICE_NAME ": PID %d Failed to send retval : %d, topnode is %d\n", current->pid, retval, topnode);
		return -EACCES;      // Failed -- return a bad address message (i.e. -14)
	}
}


static int dev_open(struct inode *inodep, struct file *filep) {
	// If same process has already opened the file
	if (get_entry_from_key(current->pid) != NULL) {
		printk(KERN_ALERT DEVICE_NAME ": PID %d, Tried to open twice\n", current->pid);
		return -EACCES;
	}

	// Create a new entry to the process linked list
	entry = kmalloc(sizeof(struct h_struct), GFP_KERNEL);
	*entry = (struct h_struct) {current->pid, NULL, NULL};
	if (!mutex_trylock(&deque_mutex)) {
		printk(KERN_ALERT DEVICE_NAME "PID %d Device is in Use <dev_open>", current->pid);
		return -EBUSY;
	}
	printk(DEVICE_NAME ": PID %d !!!! Adding %d to HashTable\n", current->pid, entry->key);
	key_add(entry);

	numberOpens++;
	printk(KERN_INFO DEVICE_NAME ": PID %d Device has been opened %d time(s)\n", current->pid, numberOpens);
	print_key();
	mutex_unlock(&deque_mutex);
	return 0;
}


static int dev_release(struct inode *inodep, struct file *filep) {
	if (!mutex_trylock(&deque_mutex)) {
		printk(KERN_ALERT DEVICE_NAME "PID %d Device is in Use <dev_release>.", current->pid);
		return -EBUSY;
	}
	// Delete the process entry from the process linked list
	key_del(current->pid);
	printk(KERN_INFO DEVICE_NAME ": PID %d Device successfully closed\n", current->pid);
	print_key();
	mutex_unlock(&deque_mutex);
	return 0;
}


static int hello_init(void) {
	struct proc_dir_entry *entry = proc_create(DEVICE_NAME, 0, NULL, &file_ops);
	if (!entry)
		return -ENOENT;
	printk(KERN_ALERT DEVICE_NAME ": Initialising the LKM!\n");
	mutex_init(&deque_mutex);
	return 0;
}


static void hello_exit(void) {
	mutex_destroy(&deque_mutex);
	DestroyRBTree(&root);
	remove_proc_entry(DEVICE_NAME, NULL);

	printk(KERN_ALERT DEVICE_NAME ": Goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);