#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");

#define MAGIC_NUMBER 0xB16B00B5
#define SLAB_NAME "myvfs_cache"

struct myvfs_inode
{
    int i_mode;
    unsigned long i_ino;
};

static void myvfs_put_super(struct super_block *sb)
{
    printk(KERN_INFO "+ myvfs: myfs_put_super\n");
}

static int myvfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
    printk(KERN_INFO "+ myvfs: call myvfs_statfs\n");
    return simple_statfs(dentry, buf);
}

int myvfs_delete_inode(struct inode *inode)
{
    printk(KERN_INFO "+ myvfs: call myvfs_delete_inode\n");
    return generic_delete_inode(inode);
}

static struct super_operations const myvfs_super_operations = {
    .put_super = myvfs_put_super,
    .statfs = myvfs_statfs,
    .drop_inode = myvfs_delete_inode
};

static struct inode *my_vfs_make_inode(struct super_block *sb, int mode)
{
    struct inode *ret = new_inode(sb);

    if (ret)
    {
        inode_init_owner(&nop_mnt_idmap, ret, NULL, mode);
        ret->i_size = PAGE_SIZE;
        struct timespec64 now = current_time(ret);
        inode_set_atime_to_ts(ret, now);
        inode_set_mtime_to_ts(ret, now);
        inode_set_ctime_to_ts(ret, now);
        ret->i_private = &myvfs_inode;
        ret->i_ino = 1;
    }

    return ret;
}

static int myvfs_fill_super(struct super_block *sb, void *data, int silent)
{
    printk(KERN_INFO "+ myvfs: call myvfs_fill_super\n");
    struct inode *root_inode;

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = MAGIC_NUMBER;
    sb->s_op = &myvfs_super_operations;
    sb->s_time_gran = 1;

    root_inode = my_vfs_make_inode(sb, S_IFDIR | 0755);
    if (!root_inode)
    {
        printk(KERN_ERR "+ myvfs: my_vfs_make_inode error\n");
        return -ENOMEM;
    }

    root_inode->i_op = &simple_dir_inode_operations;
    root_inode->i_fop = &simple_dir_operations;

    set_nlink(root_inode, 2);
    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root)
    {
        printk(KERN_ERR "+ myvfs: ERR: d_make_root\n");
        iput(root_inode);
        return -ENOMEM;
    }

    return 0;
}

static struct dentry *myvfs_mount(struct file_system_type *type, int flags, const char *dev, void *data)
{
    struct dentry *const entry = mount_nodev(type, flags, data, myvfs_fill_super);
    if (IS_ERR(entry))
        printk(KERN_ERR "+ myvfs: mounting failed\n");
    else
        printk(KERN_INFO "+ myvfs: mounted\n");
    return entry;
}

static void myvfs_kill_super(struct super_block *sb)
{
    printk(KERN_INFO "+ myvfs: call myvfs_kill_super\n");
    kill_anon_super(sb);
}

static struct file_system_type myvfs_type = {
    .owner = THIS_MODULE,
    .name = "myvfs",
    .mount = myvfs_mount,
    .kill_sb = myvfs_kill_super,
    .fs_flags = FS_USERNS_MOUNT};

static struct kmem_cache *cache = NULL;
static void **cache_mem = NULL;
static int cached_count = 128;
module_param(cached_count, int, 0);

void f_init(void *p)
{
    *(int *)p = (int)p;
}

static int __init myvfs_init(void)
{
    int ret = register_filesystem(&myvfs_type);
    if (ret)
    {
        printk(KERN_ERR "+ myvfs: can't register_filesystem\n");
        return ret;
    }
    cache_mem = kmalloc(sizeof(void *) * cached_count, GFP_KERNEL);
    if (!cache_mem)
    {
        printk(KERN_ERR "+ myvfs: can't kmalloc\n");
        unregister_filesystem(&myvfs_type);
        return -ENOMEM;
    }
    cache = kmem_cache_create(SLAB_NAME, sizeof(struct myvfs_inode), 0, SLAB_HWCACHE_ALIGN, f_init);
    if (!cache)
    {
        printk(KERN_ERR "+ myvfs: can't kmem_cache_create\n");
        kfree(cache_mem);
        unregister_filesystem(&myvfs_type);
        return -ENOMEM;
    }
    for (int i = 0; i < cached_count; i++)
    {
        if (NULL == (cache_mem[i] = kmem_cache_alloc(cache, GFP_KERNEL)))
        {
            printk(KERN_ERR "+ myvfs: can't kmem_cache_alloc\n");
            for (int j = 0; j < i; j++)
                kmem_cache_free(cache, cache_mem[j]);
            kmem_cache_destroy(cache);
            kfree(cache_mem);
            unregister_filesystem(&myvfs_type);
            return -ENOMEM;
        }
    }
    printk(KERN_INFO "+ myvfs: alloc %d objects into slab: %s\n", cached_count, SLAB_NAME);
    printk(KERN_INFO "+ myvfs: object size %ld bytes, full size %ld bytes\n",
           sizeof(struct myvfs_inode),
           sizeof(struct myvfs_inode *) * cached_count);
    printk(KERN_INFO "+ myvfs: module loaded\n");
    return 0;
}

static void __exit myvfs_exit(void)
{
    int ret = unregister_filesystem(&myvfs_type);
    if (ret)
        printk(KERN_ERR "+ myvfs: can't unregister_filesystem\n");
    for (int i = 0; i < cached_count; i++)
    {
        if (cache_mem[i])
            kmem_cache_free(cache, cache_mem[i]);
    }
    kfree(cache_mem);
    kmem_cache_destroy(cache);
    printk(KERN_INFO "+ myvfs: module unloaded\n");
}

module_init(myvfs_init);
module_exit(myvfs_exit);