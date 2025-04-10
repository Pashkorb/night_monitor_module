#include <linux/module.h>   
#include <linux/kernel.h>   
#include <linux/time.h>     
#include <linux/sched.h>    
#include <linux/kobject.h>  
#include <linux/sysfs.h>    
#include <linux/proc_fs.h> 


static int interval=600000;
static int start_hour=2;
static int finish_hour=8;
static struct timer_list night_timer;  
static struct kobject *night_kobj;    
static char alert_message[256]="";       
static struct proc_dir_entry *proc_entry;
static unsigned long trigger_count = 0;
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);

module_param(interval, int);
module_param(start_hour, int);
module_param(finish_hour, int);

MODULE_PARM_DESC(interval, "Интервал проверки в миллисекундах (по умолчанию 600000)");
MODULE_PARM_DESC(start_hour, "Час начала мониторинга (0-23, по умолчанию 2)");
MODULE_PARM_DESC(finish_hour, "Час окончания мониторинга (0-23, по умолчанию 8)");

static ssize_t message_show(struct kobject *kobj, 
                          struct kobj_attribute *attr, 
                          char *buf) 
{
    return sprintf(buf, "%s\n", alert_message);
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

static struct attribute *night_attrs[] = {
    &message_attr.attr,  
    &start_hour_attr.attr,
    &interval_attr.attr,
    &finish_hour_attr.attr,
    NULL,                
};

static struct kobj_attribute interval_attr=__ATTR(interval,0664, interval_show,interval_store);
static struct kobj_attribute finish_hour_attr=__ATTR(finish_hour,0664, finish_hour_show,finish_hour_store);
static struct kobj_attribute start_hour_attr=__ATTR(start_hour,0664, start_hour_show,start_hour_store);
static struct kobj_attribute message_attr = __ATTR_RO(message);

ssize_t interval_show (struct kobject *kobj, struct kobj_attribute *attr,
			char *buf){
	return sprintf(buf, "%d\n" ,interval);
}

ssize_t interval_store (struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count){
        int new_interval=0;
	if(!kstrtoint(buf, 0,&new_interval)){
		return -EINVAL;	
	}
	if (new_interval <= 0) {
        	printk(KERN_ERR "⚠️ Интервал должен быть положительным числом!\n");
        	return -EINVAL;
    	}
	interval=new_interval;
	return count;
}

ssize_t start_hour_show (struct kobject *kobj, struct kobj_attribute *attr,
			char *buf){
	return sprintf(buf, "%d\n" ,start_hour);
}

ssize_t start_hour_store (struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count){
        int new_start_hour=0;
	if(!kstrtoint(buf, 0,&new_start_hour)){
		return -EINVAL;	
	}
	if (new_start_hour < 0) {
        printk(KERN_ERR "⚠️ Начальный час должен быть в пределах 0-23!\n");
        return -EINVAL;
    	}
	start_hour=new_start_hour;
	return count;
}

ssize_t finish_hour_show (struct kobject *kobj, struct kobj_attribute *attr,
			char *buf){
	return sprintf(buf, "%d\n" ,finish_hour);
}

ssize_t finish_hour_store (struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t count){
        int new_finish_hour=0;
	if(!kstrtoint(buf, 0,&new_finish_hour)){
		return -EINVAL;	
	}
	if (new_finish_hour < 0) {
        printk(KERN_ERR "⚠️ Конечный час должен быть в пределах 0-23!\n");
        return -EINVAL;
    	}
	finish_hour=new_finish_hour;
	return count;
}


static struct attribute_group night_attr_group = {
    .attrs = night_attrs,  
};

static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char message[256];
    int len;
    
    len = snprintf(message, sizeof(message),
           "Ночной монитор (v1.0)\n"
           "Статус: активен\n"
           "Диапазон: %02d:00-%02d:00\n"
           "Интервал: %d сек\n"
           "Срабатываний: %lu\n",
           start_hour, finish_hour, interval/1000, trigger_count);
    
    return simple_read_from_buffer(buf, count, ppos, message, len);
}

void night_monitor_callback(struct timer_list *timer) 
{
    struct timespec64 now;  
    int hour;               
    
    ktime_get_real_ts64(&now);
    
    hour = (now.tv_sec % 86400) / 3600;
    
    if (hour >= start_hour && hour < finish_hour) {
        trigger_count++;

        printk(KERN_WARNING "🔔 night monitor active - %d:00!\n", hour);
        
        snprintf(alert_message, 
                sizeof(alert_message), 
                "Возможно вам, пора спать. Сейчас %d:00!", 
                hour);
    }
    
    mod_timer(&night_timer, jiffies + msecs_to_jiffies(interval));
}


static int check_module_params(void)
{
    if (interval <= 0) {
        printk(KERN_ERR "⚠️ Интервал должен быть положительным числом!\n");
        return -EINVAL;
    }
    if (start_hour < 0 || start_hour > 23) {
        printk(KERN_ERR "⚠️ Начальный час должен быть в пределах 0-23!\n");
        return -EINVAL;
    }
    if (finish_hour < 0 || finish_hour > 23) {
        printk(KERN_ERR "⚠️ Конечный час должен быть в пределах 0-23!\n");
        return -EINVAL;
    }
    if (start_hour >= finish_hour) {
        printk(KERN_ERR "⚠️ Начальный час должен быть меньше конечного!\n");
        return -EINVAL;
    }
    return 0;
}

static int __init night_monitor_init(void) 
{
    int ret = 0;
    ret = check_module_params();
    if (ret)
        return ret;

    proc_entry = proc_create("night_monitor", 0444, NULL, &proc_fops);
    if (!proc_entry) {
    	printk(KERN_ERR "Не удалось создать /proc/night_monitor\n");
        return -ENOMEM;
    }

    night_kobj = kobject_create_and_add("night_monitor", kernel_kobj);
    if (!night_kobj) {
        printk(KERN_ERR "⚠️ Не удалось создать kobject!\n");
        return -ENOMEM;
    }
    
    ret = sysfs_create_group(night_kobj, &night_attr_group);
    if (ret) {
        kobject_put(night_kobj);
        printk(KERN_ERR "⚠️ Не удалось создать sysfs-группу!\n");
        return ret;
    }
    
    timer_setup(&night_timer, night_monitor_callback, 0);
    mod_timer(&night_timer, jiffies + msecs_to_jiffies(1000)); 
    
    printk(KERN_INFO "⚠️ Ночной монитор активирован (период %d-%d, интервал %d мс)!\n",
       start_hour, finish_hour, interval);
    return 0;
}

static void __exit night_monitor_exit(void) 
{
    if (proc_entry) proc_remove(proc_entry);

    del_timer(&night_timer);
    
    sysfs_remove_group(night_kobj, &night_attr_group);
    
    kobject_put(night_kobj);

    if (proc_entry) proc_remove(proc_entry);
    
    printk(KERN_INFO "⚠️ Ночной монитор выключен.\n");
}

module_init(night_monitor_init);  
module_exit(night_monitor_exit);  

MODULE_LICENSE("GPL");          
MODULE_AUTHOR("Pavel Korban");      
MODULE_DESCRIPTION("Ночной монитор для ядра Linux"); 