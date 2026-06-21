#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ME");
MODULE_DESCRIPTION("Print info about task");
MODULE_VERSION("Version 1.0");

static int __init mod_init(void) {
    struct task_struct *task = &init_task;
    
    do {
        printk(KERN_INFO "+ comm - [%s], pid - %d, tgid - %d, parent - %s, ppid - %d\n"
               "  state - %u, on_cpu - %d, flags - 0x%x, sessionid - %d\n"
               "  prio - %d, static_prio - %d, normal_prio - %d, rt_priority - %u\n"
               "  policy - %u, migration_disabled: %u, migration_flags: %u\n"
               "  exit_state - %d, exit_code - %d, exit_signal - %d\n"
               "  last_switch_count - %llu, last_switch_time - %llu\n"
               "  utime - %llu, stime - %llu\n",
               task->comm,
               task->pid,
               task->tgid,
               task->parent->comm,
               task->parent->pid,
               task->__state,
               task->on_cpu,
               task->flags,
               task->sessionid,
               task->prio,
               task->static_prio,
               task->normal_prio,
               task->rt_priority,
               task->policy,
               task->migration_disabled,
               task->migration_flags,
               task->exit_state,
               task->exit_code,
               task->exit_signal,
               (unsigned long long)(task->nvcsw + task->nivcsw),
               (unsigned long long)task->last_switch_time,
               (unsigned long long)task->utime,
               (unsigned long long)task->stime);
        } while ((task = next_task(task)) != &init_task);

    printk(KERN_INFO "+ Current task:\n"
           "+ comm - [%s], pid - %d, tgid - %d, parent - %s, ppid - %d\n"
           "  state - %u, on_cpu - %d, flags - 0x%x, sessionid - %d\n"
           "  prio - %d, static_prio - %d, normal_prio - %d, rt_priority - %u\n"
           "  policy - %u, migration_disabled: %u, migration_flags: 0x%hx\n"
           "  exit_state - %d, exit_code - %d, exit_signal - %d\n"
           "  last_switch_count - %llu, last_switch_time - %llu\n"
           "  utime - %llu, stime - %llu\n",
           current->comm,
           current->pid,
           current->tgid,
           current->parent->comm,
           current->parent->pid,
           current->__state,
           current->on_cpu,
           current->flags,
           current->sessionid,
           current->prio,
           current->static_prio,
           current->normal_prio,
           current->rt_priority,
           current->policy,
           current->migration_disabled,
           current->migration_flags,
           current->exit_state,
           current->exit_code,
           current->exit_signal,
           (unsigned long long)(current->nvcsw + current->nivcsw),
           (unsigned long long)current->last_switch_time,
           (unsigned long long)current->utime,
           (unsigned long long)current->stime);

    return 0;
}

static void __exit mod_exit(void) {
    printk(KERN_INFO "Good bye!\n");
}

module_init(mod_init);
module_exit(mod_exit);