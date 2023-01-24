# CP2102 - Проблемы с прошивкой

Устройство при подключении к USB определяется как "Verifone USB to Printer".

Проблема с прошивкой самого чипа CP2102.

## Инструкция по исправлению

1. В MacOS:

    ```
    system_profiler SPUSBDataType
    ```

    Ищем `Product ID` и `Vendor ID`.

2. На сайте https://www.silabs.com/support ищем:

    ```
    CP210x_Universal_Windows_Driver
    ```

    Устанавливаем.

3. Подставляем PID и VID, генерируем драйвер, либо используем стандартный универсальный драйвер.

4. Там же https://www.silabs.com/support:

    ```
    CP21xxCustomizationUtility
    ```

    Должно появиться устройство. Если устройство не видно, надо сгенерировать драйвер ещё раз.

5. Меняем:
    
    ```
    Vendor ID: 0x10c4  (Silicon Laboratories, Inc.)
    Product ID: 0xea60
    Power: 32
    Power Mode: 00 - Bus powered
    Realese Version: 0100
    Product Description: CP2102 USB to UART Bridge Controller
    Serial: 0001
    Lock Device: 00 - Device is unlocked
    ```

6. Жмём `Program Device`

