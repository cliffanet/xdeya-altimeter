/* --------------------------------------------------
 *                      LSM303 - Accel
 */
#define LSM303_ACC_ADDR 0x18

typedef enum {
    // magnetometer register
    LSM303_REG_MAG_CRA_REG_M    = 0x00,     // configure the device for setting the data output rate and measurement configuration
    LSM303_REG_MAG_CRB_REG_M    = 0x01,     // for setting the device gain
    LSM303_REG_MAG_MR_REG_M     = 0x02,     // select the operating mode of the device
    LSM303_REG_MAG_OUT          = 0x03,     // x/y/z axis magnetometer data

    // accel register
    LSM303_REG_ACC_CTRL_REG1_A  = 0x20,     // 
    LSM303_REG_ACC_CTRL_REG4_A  = 0x23,     // 
    
    LSM303_REG_ACC_OUT          = 0x28,     // x/y/z axis acceleration data
    
    LSM303_REG_WHO_AM_I         = 0x0F,     // Divice ID register (в DLH-версии чипа - отсутствует в datasheet, но при этом выдаёт какие-то данные)
} lsm303_reg_t;

typedef enum {
    LSM303_ACC_CTRL_BDU_SINGLE  = 0x80,     // Block data update. (0: continuos update; 1: output registers not updated between MSB and LSB reading)
    LSM303_ACC_CTRL_BLE_MSB     = 0x40,     // Big/little endian data selection. (0: data LSB @ lower address; 1: data MSB @ lower address)
    LSM303_ACC_CTRL_ST_MINUS    = 0x08,     // Self-test sign. (0: self-test plus; 1 self-test minus)
    LSM303_ACC_CTRL_ST_ENABLE   = 0x02,     // Self-test enable
} lsm303_ctrl_reg4a_t;

typedef enum {                              // Power mode and low-power output data rate configurations
    LSM303_ACC_PM_DOWN          = 0x00,     // Power-down
    LSM303_ACC_PM_NORMAL        = 0x20,     // Normal mode
    LSM303_ACC_PM_0_5_HZ        = 0x40,     // 0.5 Hz data rate
    LSM303_ACC_PM_1_HZ          = 0x60,     // 1 Hz data rate
    LSM303_ACC_PM_2_HZ          = 0x80,     // 2 Hz data rate
    LSM303_ACC_PM_5_HZ          = 0xA0,     // 5 Hz data rate
    LSM303_ACC_PM_10_HZ         = 0xC0,     // 10 Hz data rate
} lsm303_powermode_t;

typedef enum {                              // Normal-mode output data rate configurations and low-pass cut-off frequencies
    LSM303_ACC_DR_50            = 0x00,     // 0DR = 50 Hz, Low-pass = 37Hz
    LSM303_ACC_DR_100           = 0x08,     // 0DR = 100 Hz, Low-pass = 74Hz
    LSM303_ACC_DR_400           = 0x10,     // 0DR = 4000 Hz, Low-pass = 292Hz
    LSM303_ACC_DR_1000          = 0x18,     // 0DR = 1000 Hz, Low-pass = 780Hz
} lsm303_acc_datarate_t;

typedef enum {
    LSM303_ACC_AZIS_EN_X        = 0x01,     // X axis enabled
    LSM303_ACC_AZIS_EN_Y        = 0x02,     // Y axis enabled
    LSM303_ACC_AZIS_EN_Z        = 0x04,     // Z axis enabled
} lsm303_acc_axisenable_t;

typedef enum {                              // Full-scale selection.
    LSM303_ACC_FS_2G        = 0x00,
    LSM303_ACC_FS_4G        = 0x10,
    LSM303_ACC_FS_8G        = 0x30,
} lsm303_acc_fs_t;

static bool accInit() {
    //uint8_t id = wread8(LSM303_ACC_ADDR, LSM303_REG_WHO_AM_I);
    //CONSOLE("LSM303_REG_WHO_AM_I: 0x%02x", id) ;
    
    /*
    for (uint8_t addr = 1; addr < 128; addr ++) {
        Wire.beginTransmission(addr);
        auto ret = Wire.endTransmission();
        if (ret == 2)
            continue;
        CONSOLE("addr: 0x%02x, ret: %d", addr, ret);
    }
    */
    
    // У этого чипа нет WHO_AM_I регистра, поэтому валидность подключения к нему
    // будем проверять по факту успешной транзакции
    
    // +/- 2 g full scale
    if (!wwrite8(LSM303_ACC_ADDR, LSM303_REG_ACC_CTRL_REG4_A, LSM303_ACC_FS_2G))
        return false;
    
    // powermode / data rate
    if (!wwrite8(LSM303_ACC_ADDR, LSM303_REG_ACC_CTRL_REG1_A, LSM303_ACC_PM_NORMAL | LSM303_ACC_DR_50 | LSM303_ACC_AZIS_EN_X | LSM303_ACC_AZIS_EN_Y | LSM303_ACC_AZIS_EN_Z))
        return false;
    
    return true;
}

static void accStop() {
    wwrite8(LSM303_ACC_ADDR, LSM303_REG_ACC_CTRL_REG1_A, LSM303_ACC_PM_DOWN);
}

static vec16_t accRead() {
    // Для чтения сразу нескольких байт (из нескольких регистров),
    // этот чип при работе с акселерометром требует установки старшего бита в 1.
    // Выдержка из datasheet (раздел 7.1.2 Linear acceleration digital interface):
    // In order to read multiple bytes, it is necessary to assert
    // the most significant bit of the subaddress field.
    // In other words, SUB(7) must be equal to 1 while SUB(6-0) represents the
    // address of the first register to be read.
    if (!wreq(LSM303_ACC_ADDR, LSM303_REG_ACC_OUT | (1 << 7), 6))
        return { 0 };
    
    vec16_t a;
    
    a.x = Wire.read() | (static_cast<int16_t>(Wire.read()) << 8);
    //a.x = a.x * -1;
    a.y = Wire.read() | (static_cast<int16_t>(Wire.read()) << 8);
    a.y = a.y * -1;
    a.z = Wire.read() | (static_cast<int16_t>(Wire.read()) << 8);
    a.z = a.z * -1;
    
    wend();
    
    return a;
}

/* --------------------------------------------------
 *                      LSM303 - Magnitometer
 */
#define LSM303_MAG_ADDR 0x1e

typedef enum {                              // Data output rate bits
    LSM303_MAG_DR_0_75_HZ       = 0x00,
    LSM303_MAG_DR_1_5_HZ        = 0x04,
    LSM303_MAG_DR_3_HZ          = 0x08,
    LSM303_MAG_DR_7_5_HZ        = 0x0c,
    LSM303_MAG_DR_15_HZ         = 0x10,
    LSM303_MAG_DR_30_HZ         = 0x14,
    LSM303_MAG_DR_75_HZ         = 0x18,
} lsm303_mag_datarate_t;

typedef enum {                              // Magnetic sensor Measurement configuration 
    LSM303_MAG_MEAS_NORMAL      = 0x00,     // Normal measurement configuration (default). In normal measurement
                                            // configuration the device follows normal measurement flow.
    LSM303_MAG_MEAS_POSBIAS     = 0x01,     // Positive bias configuration
    LSM303_MAG_MEAS_NEGPIAS     = 0x02,     // Negative bias configuration
} lsm303_mag_meas_t;

typedef enum {                              // Gain setting
    LSM303_MAG_GAIN_OFF      = 0x00,
    LSM303_MAG_GAIN_1_3      = 0x20,
    LSM303_MAG_GAIN_1_9      = 0x40,
    LSM303_MAG_GAIN_2_5      = 0x60,
    LSM303_MAG_GAIN_4_0      = 0x80,
    LSM303_MAG_GAIN_4_7      = 0xA0,
    LSM303_MAG_GAIN_5_6      = 0xC0,
    LSM303_MAG_GAIN_8_1      = 0xE0,
} lsm303_mag_gain_t;

typedef enum {                              // Magnetic sensor Measurement configuration 
    LSM303_MAG_MODE_CONTINUOUS  = 0x00,     // Continuous-conversion mode
    LSM303_MAG_MODE_SINGLE      = 0x01,     // Single-conversion mode: the device performs a single measurement
    LSM303_MAG_MODE_SLEEP       = 0x03,     // Sleep mode. Device is placed in sleep mode
} lsm303_mag_mode_t;

static bool magInit() {
    // У этого чипа нет WHO_AM_I регистра, поэтому валидность подключения к нему
    // будем проверять по факту успешной транзакции
    
    // data rate
    if (!wwrite8(LSM303_MAG_ADDR, LSM303_REG_MAG_CRA_REG_M, LSM303_MAG_DR_7_5_HZ))
        return false;
    
    // full scale
    if (!wwrite8(LSM303_MAG_ADDR, LSM303_REG_MAG_CRB_REG_M, LSM303_MAG_GAIN_1_3))
        return false;
    
    // continuous-conversion mode
    if (!wwrite8(LSM303_MAG_ADDR, LSM303_REG_MAG_MR_REG_M, LSM303_MAG_MODE_CONTINUOUS))
        return false;
    
    return true;
}

static void magStop() {
    wwrite8(LSM303_MAG_ADDR, LSM303_REG_MAG_MR_REG_M, LSM303_MAG_MODE_SLEEP);
}

static vec16_t magRead() {
    if (!wreq(LSM303_MAG_ADDR, LSM303_REG_MAG_OUT, 6))
        return { 0 };
    
    vec16_t m;
    
    // x-range = -615 .. 320
    // y-range = -400 .. 520
    // z-range = -350 .. 520
    
    m.x = (static_cast<int16_t>(Wire.read()) << 8) | Wire.read();
#if HWVER >= 5
    m.x = m.x * -1;
#endif
    m.y = (static_cast<int16_t>(Wire.read()) << 8) | Wire.read();
#if HWVER < 5
    m.y = m.y * -1;
#endif
    m.z = (static_cast<int16_t>(Wire.read()) << 8) | Wire.read();
    m.z = m.z * -1;
    
    wend();
    
    return m;
}
