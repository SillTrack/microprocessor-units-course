# Практическое занятие №3

## Цель

Целями практического занятия являются:

* знакомство с языком программирования C
* изучения модуля АЦП
* изучение алгоритма антидребезга кнопок

## Режим _Учебного стенда_ и функциональная схема

Для выполнения задач переключатели _S1_ и _S2_ должны быть установлены в положение _0_ на учебном стенде.
При этом должен загореться светодиод LED1 _While LED_, обозначающий режим стенда номер _0_.

![ ](../../../../img/sch_1.png)

**Обратите внимание, что выводы _PA13_ и _PA4_ после сброса используются как линии интерфейса отладки _SWD_.**
Вывод _PA13_ после сброса находится в режиме альтернативной функции _SWDIO_ с включенной подтяжкой к питанию.
Вывод _PA14_ - в режиме альтернативной функции _SWCLK_ с включенной подтяжкой к земле.
Альтернативные функции и подтяжки можно отключить в регистрах управления порта и
задействовать как линии порта _PA13_ и _PA14_, но при этом не будет доступен режим отладки,
а только режим программирования.

## Описание задач

На практическом занятие решаются две задачи:

* `gpio_debouncing` - _Антидребезг кнопки_
    Программа подсчета количества нажатий кнопки. Нажатия кнопки реализовано с защитой от дребезга
    контактов.
* `adc_poll` - _Чтение АЦП программным опросом_
    Программа измерения состояния потенциометров _POT1_ и _POT2_ с помощью АЦП в режиме опроса.
