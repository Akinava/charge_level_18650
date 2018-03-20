* data   20.03.2018
* autor  Andrey Chernov
* mail   akinava@gmail.com
* donate btc: 14tAPpwzrfZqBeFVvfBZHiBdByYhsoFofn


* Что это?

Вольтметр для батареи 2.5 - 5.0v.
Уровень заряда в процентах на промежутке от напряжения PERCENT_0 до PERCENT_100.
Сигнал разряда батареи если оно ниже значения LOW_LEVEL процентов.

На дисплей выводится информация о напряжении, процент заряда, а так же информация о том что подключено зарядное устройство.

* Сборка проекта

1. Подключить микроконтроллер к usbasp по spi.
2. Установить avr-gcc и avrdude.
3. Запустить скрипт ./make.sh

* Калибровка

Так как постоянные резисторы обладают не постоянством устройство необходимо откалибровать по отношению к R0 и R1.

1. Раскомментировать блок "calibrate mode" и закомментировать от строки "main mode start" до "main mode end".
2. Собрать и залить проект на контроллер.
3. Собрать схему и подключить вольтметр к (GND) и (VCC), на мониторе должно начать отображаться значения ADC.
4. Подать напряжение ~5v вполне подойдет и напряжение от USB порта. Записать значение напряжение в переменную VOLTS_VALUE_1 и значение с дисплея в переменную ADC_VALUE_1. Подать напряжение ~3v для этого подойдут две батареи AA или AAA, записать значение напряжения в VOLTS_VALUE_2 а значение с дисплея в переменную в ADC_VALUE_2.
5. Вернуть блоки "calibrate mode" и "main mode" в первоначальное состояние.
6. Программа готова для сборки и использования.
