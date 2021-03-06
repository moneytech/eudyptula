#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");

#define MYID "832a404c8ce0"
#define MYIDLEN 12

static struct dentry *eudyptula_debugfs_root;
static char foo_buf[4096];
static char foo_pos;
static spinlock_t foo_lock;


static ssize_t eudyptula_foo_read(struct file *file, char __user *buf,
				  size_t count, loff_t *ppos)
{
	ssize_t ret;

	spin_lock(&foo_lock);
	ret = simple_read_from_buffer(buf, count, ppos, foo_buf, foo_pos);
	spin_unlock(&foo_lock);
	return ret;
}

static ssize_t eudyptula_foo_write(struct file *file, const char __user *buf,
				   size_t count, loff_t *ppos)
{
	ssize_t ret;

	/* TODO: LOCKING */
	if (count > 4096 - foo_pos)
		count = 4096 - foo_pos;

	spin_lock(&foo_lock);
	ret = simple_write_to_buffer(foo_buf, 4096 - foo_pos,
				     ppos, buf, count);
	if (ret < 0)
		goto failed;
	if (ret < count) {
		ret = -EINVAL;
		goto failed;
	}
	foo_pos = count;
	spin_unlock(&foo_lock);
	return count;
failed:
	spin_unlock(&foo_lock);
	return ret;
}

static const struct file_operations eudyptula_foo_fops = {
	.owner = THIS_MODULE,
	.read = eudyptula_foo_read,
	.write = eudyptula_foo_write,
};

static ssize_t eudyptula_id_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(buf, count, ppos, MYID, 12);
}

static ssize_t eudyptula_id_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	char kbuf[20] = {0,};
	ssize_t ret;

	ret = simple_write_to_buffer(kbuf, 19, ppos, buf, count);
	if (ret < 0)
		return ret;
	if (ret < MYIDLEN || strncmp(kbuf, MYID, ret))
		return -EINVAL;
	return count;
}

static const struct file_operations eudyptula_id_fops = {
	.owner = THIS_MODULE,
	.read = eudyptula_id_read,
	.write = eudyptula_id_write,
};

static int __init hello_init(void)
{
	struct dentry *debugfs_id, *debugfs_jiffies, *debugfs_foo;

	eudyptula_debugfs_root = debugfs_create_dir("eudyptula", NULL);
	if (!eudyptula_debugfs_root
	    || eudyptula_debugfs_root == ERR_PTR(-ENODEV)) {
		pr_err("failed to create debugfs dir: eudyptula\n");
		return 0;
	}

	debugfs_id = debugfs_create_file_size("id",
					      S_IRUSR | S_IWUSR |
					      S_IRGRP | S_IWGRP |
					      S_IROTH | S_IWOTH,
					      eudyptula_debugfs_root,
					      NULL,
					      &eudyptula_id_fops,
					      strlen(MYID));
	if (!debugfs_id
	    || debugfs_id == ERR_PTR(-ENODEV)) {
		pr_err("failed to create debugfs file: id\n");
		return 0;
	}

	debugfs_jiffies = debugfs_create_u64("jiffies",
					     S_IRUSR |
					     S_IRGRP | S_IROTH,
					     eudyptula_debugfs_root,
					     &jiffies_64);
	if (!debugfs_jiffies
	    || debugfs_jiffies == ERR_PTR(-ENODEV)) {
		pr_err("failed to create debugfs file: jiffies\n");
		return 0;
	}

	spin_lock_init(&foo_lock);
	debugfs_foo = debugfs_create_file_size("foo",
					       S_IRUSR | S_IWUSR |
					       S_IRGRP |
					       S_IROTH,
					       eudyptula_debugfs_root,
					       NULL,
					       &eudyptula_foo_fops,
					       4096);
	if (!debugfs_foo
	    || debugfs_foo == ERR_PTR(-ENODEV)) {
		pr_err("failed to create debugfs file: foo\n");
		return 0;
	}
	return 0;
}

static void __exit hello_exit(void)
{
	debugfs_remove_recursive(eudyptula_debugfs_root);
}

module_init(hello_init);
module_exit(hello_exit);
