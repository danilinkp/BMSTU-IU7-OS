#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/string.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");

#define IRQ_NUM 1
#define PROC_NAME "irq_handler_workqueue"
#define BLOCK_MS 10

static char *ascii[84] = {
	" ", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "+", "Backspace",
	"Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "Enter", "Ctrl",
	"a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "\"", "'", "Shift (left)", "|",
	"z", "x", "c", "v", "b", "n", "m", "<", ">", "?", "Shift (right)",
	"*", "Alt", "Space", "CapsLock",
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
	"NumLock", "ScrollLock", "Home", "Up", "Page-Up", "-", "Left",
	" ", "Right", "+", "End", "Down", "Page-Down", "Insert", "Delete"
};

struct my_work_struct {
	struct work_struct work;
	ktime_t start;
	int code;
};

static struct workqueue_struct *wq;
static struct my_work_struct *work1;
static struct my_work_struct *work2;
static char last_key_name[20];
static int last_code = -1;
static s64 fast_ns;
static s64 slow_ns;

static void work1_fn(struct work_struct *work)
{
	struct my_work_struct *iw = container_of(work, struct my_work_struct, work);
	int press_code = iw->code & 0x7F;

	last_code = iw->code;
	strncpy(last_key_name, ascii[press_code], sizeof(last_key_name) - 1);
	last_key_name[sizeof(last_key_name) - 1] = '\0';
	fast_ns = ktime_to_ns(ktime_sub(ktime_get(), iw->start));

	printk(KERN_INFO "irq_handler_workqueue: fast key=%s code=%#x exec_ns=%lld\n", last_key_name, iw->code, fast_ns);
}

static void work2_fn(struct work_struct *work)
{
	struct my_work_struct *iw = container_of(work, struct my_work_struct, work);
	int press_code = iw->code & 0x7F;

	msleep(BLOCK_MS);

	last_code = iw->code;
	strncpy(last_key_name, ascii[press_code], sizeof(last_key_name) - 1);
	last_key_name[sizeof(last_key_name) - 1] = '\0';
	slow_ns = ktime_to_ns(ktime_sub(ktime_get(), iw->start));

	printk(KERN_INFO "irq_handler_workqueue: slow key=%s code=%#x exec_ns=%lld\n", last_key_name, iw->code, slow_ns);
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
	int code = inb(0x60);
	int is_release = (code & 0x80) ? 1 : 0;
	int press_code = code & 0x7F;

	if (irq == IRQ_NUM) {
		if (is_release && press_code < 84 && press_code != 28) {
			work1->start = ktime_get();
			work1->code = code;
			work2->start = ktime_get();
			work2->code = code;

			queue_work(wq, &work1->work);
			queue_work(wq, &work2->work);
			return IRQ_HANDLED;
		}
	}
	return IRQ_NONE;
}

static int proc_show(struct seq_file *m, void *v)
{
	if (last_code != -1)
		seq_printf(m, "key=%s code=%#x\n", last_key_name, last_code);
	seq_printf(m, "fast_ns=%lld\n", fast_ns);
	seq_printf(m, "slow_ns=%lld\n", slow_ns);
	seq_printf(m, "block_ms=%d\n", BLOCK_MS);
	return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_fops = {
	.proc_open = proc_open,
	.proc_read = seq_read,
	.proc_release = single_release,
};

static int __init my_init(void)
{
	int ret;

	wq = alloc_workqueue("irq_handler_wq", WQ_MEM_RECLAIM, 1);
	if (!wq)
		return -ENOMEM;

	work1 = kmalloc(sizeof(*work1), GFP_KERNEL);
	if (!work1) {
		destroy_workqueue(wq);
		return -ENOMEM;
	}

	work2 = kmalloc(sizeof(*work2), GFP_KERNEL);
	if (!work2) {
		destroy_workqueue(wq);
		kfree(work1);
		return -ENOMEM;
	}

	INIT_WORK(&work1->work, work1_fn);
	INIT_WORK(&work2->work, work2_fn);

	ret = request_irq(IRQ_NUM, irq_handler, IRQF_SHARED, "irq_handler_workqueue", irq_handler);
	if (ret) {
		kfree(work1);
		kfree(work2);
		destroy_workqueue(wq);
		return ret;
	}

	if (!proc_create(PROC_NAME, 0444, NULL, &proc_fops)) {
		free_irq(IRQ_NUM, irq_handler);
		kfree(work1);
		kfree(work2);
		destroy_workqueue(wq);
		return -ENOMEM;
	}

	printk(KERN_INFO "irq_handler_workqueue: loaded\n");
	return 0;
}

static void __exit my_exit(void)
{
	remove_proc_entry(PROC_NAME, NULL);
	synchronize_irq(IRQ_NUM);
	free_irq(IRQ_NUM, irq_handler);
	flush_workqueue(wq);
	destroy_workqueue(wq);
	kfree(work1);
	kfree(work2);
	printk(KERN_INFO "irq_handler_workqueue: unloaded\n");
}

module_init(my_init);
module_exit(my_exit);