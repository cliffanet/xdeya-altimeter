/* --------------------------------------------------
 *                      Accel/Gyro
 */
#define MPU6050_ADDR 0x68 ///< The correct MPU6050_WHO_AM_I value

typedef enum {
    MPU6050_REG_SLFTEST_X   = 0x0D, // Self test factory calibrated values register
    MPU6050_REG_SLFTEST_Y   = 0x0E, // Self test factory calibrated values register
    MPU6050_REG_SLFTEST_Z   = 0x0F, // Self test factory calibrated values register
    MPU6050_REG_SLFTEST_A   = 0x10, // Self test factory calibrated values register
    MPU6050_REG_SMPLRT_DIV  = 0x19, // sample rate divisor register
    MPU6050_REG_CFG         = 0x1A, // General configuration register
    MPU6050_REG_GYRO_CFG    = 0x1B, // Gyro specfic configuration register
    MPU6050_REG_ACCEL_CFG   = 0x1C, // Accelerometer specific configration register
    MPU6050_REG_ACCEL_OUT   = 0x3B, // base address for sensor data reads
    MPU6050_REG_INT_PIN     = 0x37, // Interrupt pin configuration register
    MPU6050_REG_TEMP_H      = 0x41, // Temperature data high byte register
    MPU6050_REG_TEMP_L      = 0x42, // Temperature data low byte register
    MPU6050_REG_USER_CTRL   = 0x6A, // FIFO and I2C Master control register
    MPU6050_REG_PWR_MGMT_1  = 0x6B, // Primary power/sleep control register
    MPU6050_REG_PWR_MGMT_2  = 0x6C, // Secondary power/sleep control register
    MPU6050_REG_SIGPATH_RST = 0x68, // Signal path reset register
    MPU6050_REG_WHO_AM_I    = 0x75, // Divice ID register
} mpu6050_reg_t;

typedef enum {
    MPU6050_FILT_260    = 0x00,     // 260 Hz, Docs imply this disables the filter
    MPU6050_FILT_184    = 0x01,     // 184 Hz
    MPU6050_FILT_94     = 0x02,     // 94 Hz
    MPU6050_FILT_44     = 0x03,     // 44 Hz
    MPU6050_FILT_21     = 0x04,     // 21 Hz
    MPU6050_FILT_10     = 0x05,     // 10 Hz
    MPU6050_FILT_5      = 0x06,     // 5 Hz
} mpu6050_filter_t;

typedef enum {
    MPU6050_GYR_SLFTEST_X   = 0x20,     // selftest gyro x
    MPU6050_GYR_SLFTEST_Y   = 0x40,
    MPU6050_GYR_SLFTEST_Z   = 0x80,
} mpu6050_gyro_cfg_t;

typedef enum {
    MPU6050_GYR_RNG_250   = 0x00,   // +/- 250 deg/s (default value)
    MPU6050_GYR_RNG_500   = 0x08,   // +/- 500 deg/s
    MPU6050_GYR_RNG_1000  = 0x10,   // +/- 1000 deg/s
    MPU6050_GYR_RNG_2000  = 0x18,   // +/- 2000 deg/s
} mpu6050_gyro_range_t;

typedef enum {
    MPU6050_ACC_SLFTEST_X   = 0x20,     // selftest accel x
    MPU6050_ACC_SLFTEST_Y   = 0x40,
    MPU6050_ACC_SLFTEST_Z   = 0x80,
} mpu6050_accel_cfg_t;

typedef enum {
    MPU6050_ACC_RNG_2   = 0x00,     // +/- 2g (default value)
    MPU6050_ACC_RNG_4   = 0x08,     // +/- 4g
    MPU6050_ACC_RNG_8   = 0x10,     // +/- 8g
    MPU6050_ACC_RNG_16  = 0x18,     // +/- 16g
} mpu6050_accel_range_t;

typedef enum {
    MPU6050_PWR1_CLK_INTERNAL   = 0x00,     // Internal 8MHz oscillator
    MPU6050_PWR1_CLK_PLL_GYROX  = 0x01,     // PLL with X axis gyroscope reference
    MPU6050_PWR1_CLK_PLL_GYROY  = 0x02,     // PLL with Y axis gyroscope reference
    MPU6050_PWR1_CLK_PLL_GYROZ  = 0x03,     // PLL with Z axis gyroscope reference
    MPU6050_PWR1_CLK_EXT_32     = 0x04,     // PLL with external 32.768kHz reference
    MPU6050_PWR1_CLK_EXT_19     = 0x05,     // PLL with external 19.2MHz reference
    MPU6050_PWR1_CLK_STOP       = 0x07,     // Stops the clock and keeps the timing generator in reset
    MPU6050_PWR1_TEMP_DIS       = 0x08,     // this bit disables the temperature sensor.
    MPU6050_PWR1_CYCLE          = 0x20,     // When this bit is set to 1 and SLEEP is disabled, the MPU-60X0 will cycle
                                            // between sleep mode and waking up to take a single sample of data from
                                            // active sensors at a rate determined by LP_WAKE_CTRL (register 108).
    MPU6050_PWR1_SLEEP          = 0x40,     // this bit puts the MPU-60X0 into sleep mode
    MPU6050_PWR1_DEV_RST        = 0x80,     // this bit resets all internal registers to their default values.
} mpu6050_pwr_mgmt1_t;

static bool accInit() {
    uint8_t id = wread8(MPU6050_ADDR, MPU6050_REG_WHO_AM_I);
    if (id != 0x68)
        return false;
    
    // низкочастотный фильтр
    wwrite8(MPU6050_ADDR, MPU6050_REG_CFG, MPU6050_FILT_260);
    
    // gyro range
    wwrite8(MPU6050_ADDR, MPU6050_REG_GYRO_CFG, MPU6050_GYR_RNG_500);
    
    // accel range
    wwrite8(MPU6050_ADDR, MPU6050_REG_ACCEL_CFG, MPU6050_ACC_RNG_2);
    
    // power (disable sleep, set clock source)
    wwrite8(MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR1_CLK_PLL_GYROX);
    
    return true;
}

static void accStop() {
    wwrite8(MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, MPU6050_PWR1_CLK_STOP | MPU6050_PWR1_SLEEP | MPU6050_PWR1_DEV_RST);
}

static vec16_t accRead() {
    if (!wreq(MPU6050_ADDR, MPU6050_REG_ACCEL_OUT, 6))
        return { 0 };
    
    vec16_t a;
    
    a.x = Wire.read();
    a.x = (a.x << 8) | Wire.read();
    //a.x = a.x * -1;
    a.y = Wire.read();
    a.y = (a.y << 8) | Wire.read();
    //a.y = a.y * -1;
    a.z = Wire.read();
    a.z = (a.z << 8) | Wire.read();
    
    wend();
    
    a.x = static_cast<int32_t>(a.x) * 766 / bias.x;
    a.y = static_cast<int32_t>(a.y) * 766 / bias.y;
    a.z = static_cast<int32_t>(a.z) * 766 / bias.z;
    
    return a;
}
