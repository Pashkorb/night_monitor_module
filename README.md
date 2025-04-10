# Linux Kernel Night Monitor  

Мониторит ночное время (по умолчанию 2:00–8:00) и отправляет уведомления.  

## **Функционал**  
- Таймер с настраиваемым интервалом (`interval`)  
- Динамический диапазон часов (`start_hour`, `finish_hour`)  
- Интерфейсы:  
  - **`/proc/night_monitor`** — статус и статистика  
  - **`/sys/kernel/night_monitor/`** — управление параметрами  
  - **Демон** (`night_monitor_daemon`) — уведомления через `notify-send`  

## **Установка:**  
```sh
make  
sudo insmod night_monitor_module.ko  
sudo ./night_monitor_daemon &
```
## **Настройка:**  
```sh
sudo insmod night_monitor_module.ko interval=300000 start_hour=1 finish_hour=7  
```
## **Через sysfs:**  
```sh
echo 500000 | sudo tee /sys/kernel/night_monitor/interval  
```

## **Через /proc:*  
```sh
cat /proc/night_monitor  
```
