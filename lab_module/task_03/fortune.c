#include <linux/init.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

#define COOKIE_POT_SIZE PAGE_SIZE
#define FILENAME "fortuneFile"
#define DIRNAME "fortuneDir"
#define SYMLINK "fortuneLink"
#define FILEPATH DIRNAME "/" FILENAME

static struct proc_dir_entry *fortuneFile;
static struct proc_dir_entry *fortuneDir;
static struct proc_dir_entry *fortuneLink;

static char *cookiePot = NULL;
char tmpBuf[COOKIE_POT_SIZE];

static int readInd = 0;
static int writeInd = 0;

static void freeMemory(void)
{
    if (fortuneLink != NULL)
        remove_proc_entry(SYMLINK, NULL);

    if (fortuneFile != NULL)
        remove_proc_entry(FILENAME, fortuneDir);

    if (fortuneDir != NULL)
        remove_proc_entry(DIRNAME, NULL);

    vfree(cookiePot);
}

static int fortuneOpen(struct inode *spInode, struct file *spFile)
{
    printk(KERN_INFO "+ open");
    return 0;
}

static int fortuneRelease(struct inode *spInode, struct file *spFile)
{
    printk(KERN_INFO "+ release");
    return 0;
}

static ssize_t fortuneWrite(struct file *file, const char __user *buf,
                            size_t len, loff_t *fPos)
{
    if (len > COOKIE_POT_SIZE - writeInd + 1)
    {
        printk(KERN_ERR "+ write: buffer overflow (requested %zu, available %d)\n",
               len, COOKIE_POT_SIZE - writeInd);
        return -ENOSPC;
    }

    if (copy_from_user(&cookiePot[writeInd], buf, len) != 0)
    {
        printk(KERN_ERR "+ write: copy_from_user error\n");
        return -EFAULT;
    }

    writeInd += len;
    cookiePot[writeInd - 1] = '\0';

    printk(KERN_INFO "+ write: %s",
           &cookiePot[writeInd - len]);

    return len;
}

static ssize_t fortuneRead(struct file *file, char __user *buf,
                           size_t len, loff_t *fPos)
{
    int readLen;

    if ((*fPos > 0) || (writeInd == 0))
        return 0;

    if (readInd >= writeInd)
    {
        printk(KERN_INFO "+ read: readInd reset to 0 (was %d)\n", readInd);
        readInd = 0;
    }

    readLen = snprintf(tmpBuf, COOKIE_POT_SIZE, "%s\n", &cookiePot[readInd]);

    printk(KERN_INFO "+ read: %s\n", tmpBuf);

    if (copy_to_user(buf, tmpBuf, readLen) != 0)
    {
        printk(KERN_ERR "+ read: copy_to_user error\n");
        return -EFAULT;
    }

    readInd += readLen;
    *fPos += readLen;

    return readLen;
}

static const struct proc_ops fops =
    {
        .proc_open = fortuneOpen,
        .proc_read = fortuneRead,
        .proc_write = fortuneWrite,
        .proc_release = fortuneRelease};

static int __init md_init(void)
{
    printk(KERN_INFO "+ init\n");

    if ((cookiePot = vmalloc(COOKIE_POT_SIZE)) == NULL)
    {
        printk(KERN_ERR "+ memory error");
        return -ENOMEM;
    }

    memset(cookiePot, 0, COOKIE_POT_SIZE);

    if ((fortuneDir = proc_mkdir(DIRNAME, NULL)) == NULL)
    {
        printk(KERN_ERR "+ create dir err");
        freeMemory();

        return -ENOMEM;
    }

    if ((fortuneFile = proc_create(FILENAME, 0666, fortuneDir, &fops)) == NULL)
    {
        printk(KERN_ERR "+ create file err");
        freeMemory();

        return -ENOMEM;
    }

    if ((fortuneLink = proc_symlink(SYMLINK, NULL, FILEPATH)) == NULL)
    {
        printk(KERN_ERR "+ create link err");
        freeMemory();

        return -ENOMEM;
    }

    readInd = 0;
    writeInd = 0;

    printk(KERN_INFO "+ loaded");

    return 0;
}

static void __exit md_exit(void)
{
    printk(KERN_INFO "+ exit");
    freeMemory();
}

module_init(md_init);
module_exit(md_exit);