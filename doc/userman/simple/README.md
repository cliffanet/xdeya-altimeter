# Знакомство

Вы никогда не задавались вопросом:

> И `хде йа` нахожусь?!

Или может в 2 часа ночи интересовались:

> Как пройти в библиотеку?

**Привет!**

Я - `высотомер` и `GPS/GLONASS` в одном приборе.

Я `**НЕ**` являюсь навигатором. Только возвращалкой к заранее заданной точке. Покажу направление и расстояние до точки по прямой.

Пока я ещё как эксперементальная разработка. Жить этому проекту или нет, выберете Вы сами.

А по карте маршрут вам проложит Яндекс Навигатор, но не я!


## Функции устройства

* Классические для высотомера: отображение высоты, логбук и т.д.
* Приём сигнала навигации (GPS + GLONASS)
* Отображение направления и расстояния до заранее сохранённой точки (возвращалка)
* Отображение вертикальной (по барометру) и горизонтальной (по навигации) скоростей
* В логбуке сохраняются показания высотомера и координаты на местности
* Запись трека со всеми имеющимися параметрами барометра и GPS/GLONASS-приёмника
* Синхронизация по WiFi (по запросу пользователя): отправка логбука, треков, приём некоторых настроек с сервера


# Диаграмма управления

![](../img/01.ctrl.diag.png)

# Органы управления

Переключение между страницами на главном экране - нажатие на `среднюю кнопку`.

Кнопки `Вверх` и `Вниз` работают только по `длинному нажатию`, их функциями можно управлять в меню `Параметры` -> `Функции кнопок`.


##  Экран: Навигация + Высотомер

![](../img/02.alt.navi.jpg)

1. Индикатор заряда батареи.
2. Относительное отображение расположение точки.
3. Текущее время (синхронизируется по GPS/GLONASS-сигналу).
4. Высота по барометру (метры).
5. Временное отображение режима высотомера (по текущей высоте, скорости и вертикальному направлению движения).
6. Вертикальная скорость (м/с).
7. Расстояние до выбранной точки (метры или километры).
8. Горизонтальная скорость.
9. Режим работы Navi приёмника (во включенном состоянии отображает число найденных спутников).
10. Условное обозначение "Севера".
11. Центр карты: тут мы, при движении рисуется стрелка - чем быстрее, тем длиннее.


##  Экран: Навигация

![](../img/02.navi.jpg)

1. Высота по барометру (метры).
2. Условное обозначение "Севера".
3. Расстояние до выбранной точки (метры или километры).
4. Режим работы Navi приёмника (во включенном состоянии отображает число найденных спутников).
5. Относительное отображение расположение точки.
6. Направление до точки (в градусах).
7. Направление движения (в градусах).


## Экран: Высотомер

![](../img/02.altimeter.jpg)

1. Индикатор заряда батареи.
2. Текущее время (синхронизируется по GPS/GLONASS-сигналу).
3. Высота по барометру (километры).
4. Вертикальная скорость (км/ч).


# Как вбить конечную точку

Меню: `Navi-точки` - выбираем из трёх слотов хранения координат и сохраняем текущее местоположение.

![](../img/pntset.jpg)


# Как заряжать

В это отверстие вставляем USB-шнурок одним концом, а другим в зарядку или ваш домашний компьютер:

![](../img/01.charge1.jpg "USB")
![](../img/01.charge2.jpg "USB")


# Крепление

* На руку

![](../img/01.mount1.jpg "Крепление на руку")

* На ногу

![](../img/01.mount2.jpg "Крепление на ногу")


# Подсветка

Меню: `Параметры` -> `Дисплей` -> `Подсветка`

Для ночных прыжочков:

![](../img/01.light1.jpg "Подсветка")
![](../img/01.light2.jpg "Подсветка")
