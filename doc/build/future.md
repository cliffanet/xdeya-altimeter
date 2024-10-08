# Хде-Йа - Планы на ближайшее будушее

## Прошивка

* Переработка доступа к flash-памяти
    
    * исправление подвисаний при записи трека
    
    * подготовка к работе с sd-картой для записи треков и логбука прыжков

* Переработка сетевого стека:
    
    * работа с bluetouth
    
    * подготовка к работе с мобильным приложением

* Переработка отображения навигации
    
    * поддержка аппаратного компаса
    
    * изменение подхода к отображению информации

* Отказ от чипа часов

    Будем использовать внутренние часы чипа навигации.
    
    Использование дополнительного чипа нецелесообразно - только зря занимает место на плате и кушает аккумулятор.

## Аппаратная часть

* Внешняя SD-карта

    Уже давно напрашивается, и в новых аппаратных версиях она точно будет.

* Аппаратный компас

    Пока что я ещё не встретил достуюную аппаратную реализацию, но вариант `более-менее` уже протестирован.

* Внешняя GPS/GLONASS-антенна

    Это будет альтернативой попытке вынести антенну за площадь экрана.

Ближайшая аппаратная версия: [`v.0.5`](../models/03.v.0.5.md)

## Более далёкие планы

* Сменить чип на более энергоэффективный

    Это потянет за собой необходимость прорабатывать уже протестированное и отлаженное. Например WiFi. Всё это муторно и долго, поэтому требует мотивации. В первую очередь мотивация подкрепляется заинтересованностью общественности в данном проекте.

* Мобильное приложение и синхронизация по bluetouth

    Тут ещё больше проблем, чем при смене чипа. Идея очень нужная в реализации, но это всё не быстро.
    
    Но при этом не будет требоваться обновления аппаратной модификации на новую. Текущие версии устройств с новыми прошивками гипотетически заработают без проблем.

* Помощь в программах обучения (пилотирование и т.п.)

    На данный момент это уже возможно, если в прыжке использовать устройство как запись трека, а на замле производить разбор. На практике в таком амплуа оно ещё не проверялось и не отлаживалось.
    
    В какой-то будущей перспективе можно снабжать устройство звуковой сигнализацией. Например, по аналогии с FlySight.
