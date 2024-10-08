Руководство по старту работы с бибиотекой libble для доступа к устройствам Bluetooth Low-Energy
===

Шаг 1. Приобрести Bluetooth адаптер, полноценно поддерживающий спецификацию Bluetooth 4.0
(например Asus BT400, другие варианты можно найти в сети)

Шаг 2. Проверить работу адаптера с LE-устройствами

Установить утилиты для работы с bluetooth
```
$ sudo apt-get install bluetooth
```

- команда hciconfig должна показать, что адаптер определился и запущен
```
borealis@atom:~$ hciconfig
hci0:	Type: BR/EDR  Bus: USB
	BD Address: 5C:F3:70:63:EE:2E  ACL MTU: 1021:8  SCO MTU: 64:1
	UP RUNNING PSCAN ISCAN 
	RX bytes:1936 acl:6 sco:0 events:86 errors:0
	TX bytes:1274 acl:6 sco:0 commands:65 errors:0
```

- команду hcitool можно использовать для поиска активных LE-устройств
Следующим образом можно запустить сканирование. LE Scan покажет найденные
LE-устройства (они должны быть в сосотоянии Advertizing, чтобы адаптер их нашел)

```
borealis@atom:~$ sudo hcitool lescan
LE Scan ...
84:DD:20:C5:70:43 (unknown)
84:DD:20:C5:70:43 VE Control Sensor
^C
```

- команду gatttool можно использовать, чтобы подключиться к LE-устройству,
считать/записать значения характеристик и слушать уведомления (notifications)
```
borealis@atom:~$ gatttool -b 84:DD:20:C5:70:43 -I
[   ][84:DD:20:C5:70:43][LE]> connect 
[CON][84:DD:20:C5:70:43][LE]> char-read-hnd 0048
[CON][84:DD:20:C5:70:43][LE]> 
Characteristic value/descriptor: 00 00 
[CON][84:DD:20:C5:70:43][LE]> char-write-req 0048 0100
[CON][84:DD:20:C5:70:43][LE]> Characteristic value was written successfully

[CON][84:DD:20:C5:70:43][LE]> 
Notification handle = 0x0047 value: 01 
[CON][84:DD:20:C5:70:43][LE]> 
Notification handle = 0x0047 value: 00 
[CON][84:DD:20:C5:70:43][LE]> 
Notification handle = 0x0047 value: 01 
[CON][84:DD:20:C5:70:43][LE]> 
Notification handle = 0x0047 value: 00 
[CON][84:DD:20:C5:70:43][LE]> quit
```

Шаг 3. Компиляция и использование библиотеки libble

Для сборки библиотеки необходимо установить некоторые пакеты

Для Ubuntu:
```
$ sudo apt-get install libglib-2.0-dev libbluetooth3 libbluetooth-dev
```

Далее, перейти в каталог libble, где лежит Makefile и выполнить:
```
$ make
```
Если сборка завершится успешно, то появится библиотека libble.so и несколько тестовых программ.
Сборка настроена таким образом, чтобы тестовые программы искали необходимые библиотеки не только по
системным путям, но и в текущем каталоге. Поэтому устанавливать libble.so в систему не обязательно.

Исходники тестовых программ находятся в каталоге src/
Документация по API libble - doc/api.txt

