obj-m := night_monitor_module.c

all:
	make -C /lib/modules//$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C/lib/modules/$(shell uname -r)/build M=$(PWD) clean
