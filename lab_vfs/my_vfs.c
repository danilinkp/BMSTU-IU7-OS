#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/fs_context.h>
#include <asm/atomic.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define MYFS_MAGIC_NUMBER 0x13131313
#define SLAB_NAME "myfs_cache"
#define MYFS_SLAB_OBJSIZE 16

static void **cache_mem_area = NULL;
struct kmem_cache *cache     = NULL;

static void *tmp[257] = {0};

static void myfs_put_super(struct super_block *sb)
{
    printk(KERN_INFO "myfs: super block destroyed\n");
    printk("myfs: put_super\n");
}

static struct super_operations const myfs_super_ops = {
    .put_super = myfs_put_super,
    .statfs = simple_statfs,
};

static int myfs_fill_sb(struct super_block *sb, struct fs_context *fc)
{
    printk(KERN_INFO "myfs: call myfs_fill_sb\n");
    struct inode *inode;
    struct dentry *root;

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = MYFS_MAGIC_NUMBER;
    sb->s_op = &myfs_super_ops;
    sb->s_time_gran = 1;

    inode = new_inode(sb);
    printk(KERN_INFO "myfs: new_inode\n");
    if (!inode) {
        printk(KERN_ERR "myfs: new_inode error\n");
        return -ENOMEM;
    }

    inode->i_ino = 1;
    inode->i_mode = S_IFDIR | 0755;
    simple_inode_init_ts(inode);
    printk("myfs: simple_inode_init_ts\n");
    inode->i_op = &simple_dir_inode_operations;
    inode->i_fop = &simple_dir_operations;

    root = d_make_root(inode);
    printk("myfs: d_make_root\n");
    if (!root) {
        printk(KERN_ERR "myfs: d_make_root error\n");
        return -ENOMEM;
    }

    sb->s_root = root;
    printk(KERN_INFO "myfs: VFS root created\n");
    printk(KERN_INFO "myfs: super_block init\n");
    return 0;
}

static int myfs_get_tree(struct fs_context *fc)
{
    printk(KERN_INFO "myfs: get_tree\n");
    int ret;
    ret = get_tree_nodev(fc, myfs_fill_sb);
    if (ret < 0)
        printk(KERN_ERR "myfs: get_tree error: %d\n", ret);
    else
        printk(KERN_INFO "myfs: mount\n");
    return ret;
}

static const struct fs_context_operations myfs_context_ops = {
    .get_tree = myfs_get_tree,
};

static int myfs_init_fs_context(struct fs_context *fc)
{
    printk(KERN_INFO "myfs: init_fs_context\n");
    fc->ops = &myfs_context_ops;
    return 0;
}

static void myfs_kill_super(struct super_block *sb)
{
    printk(KERN_INFO "myfs: kill_super\n");
    kill_anon_super(sb);
    printk(KERN_INFO "myfs: super block destroyed\n");
}

static struct file_system_type myfs_type = {
    .owner            = THIS_MODULE,
    .name             = "myfs",
    .init_fs_context  = myfs_init_fs_context,
    .kill_sb          = myfs_kill_super,
};

static int __init myfs_init(void)
{
    int ret;
    printk(KERN_INFO "myfs: register_filesystem call\n");
    ret = register_filesystem(&myfs_type);
    if (ret)
    {
        printk(KERN_ERR "myfs: register_filesystem error\n");
        return ret;
    }
    printk(KERN_INFO "myfs: register_filesystem\n");

    if ((cache_mem_area = (void **)kmalloc(sizeof(void *), GFP_KERNEL)) == NULL)
    {
        printk(KERN_ERR "myfs: cache_mem_area error\n");
        return -ENOMEM;
    }
    printk("myfs: kmalloc\n");

    if ((cache = kmem_cache_create(SLAB_NAME, MYFS_SLAB_OBJSIZE, 0, SLAB_NO_MERGE, NULL)) == NULL)
    {
        printk(KERN_ERR "myfs: kmem_cache_create error\n");
        kfree(cache_mem_area);
        printk("myfs: kfree\n");
        return -ENOMEM;
    }
    printk("myfs: kmem_cache_create\n");

    for (size_t i = 0; i < 256; ++i) {
        if ((tmp[i] = kmem_cache_alloc(cache, GFP_KERNEL)) == NULL) {
            printk(KERN_ERR "myfs: kmem_cache_alloc error\n");
        }
    }
    printk("myfs: kmem_cache_alloc\n");
    printk(KERN_INFO "myfs: loaded\n");
    return 0;
}

static void __exit myfs_exit(void)
{
    int ret;
    for (size_t i = 0; i < 256; ++i) {
        kmem_cache_free(cache, tmp[i]);
    }
    printk("myfs: kmem_cache_free\n");
    kmem_cache_destroy(cache);
    printk("myfs: kmem_cache_destroy\n");
    kfree(cache_mem_area);
    printk("myfs: kfree\n");
    ret = unregister_filesystem(&myfs_type);
    if (ret)
        printk(KERN_ERR "myfs: unregister_filesystem error\n");
    else
        printk("myfs: unregister_filesystem\n");
    printk(KERN_INFO "myfs: unloaded\n");
}

module_init(myfs_init);
module_exit(myfs_exit);