/* --------------------------------------------------
 *                      MPU9250 - Accel/Gyro
 */
#define MPU9250_ADDR 0x68 ///< The correct MPU6050_WHO_AM_I value

typedef enum {
    MPU9250_REG_GYR_SLFTST_X    = 0x00, // Gyroscope Self-Test Registers
    MPU9250_REG_GYR_SLFTST_Y    = 0x01,
    MPU9250_REG_GYR_SLFTST_Z    = 0x02,
    MPU9250_REG_ACC_SLFTST_X    = 0x0D, // Accelerometer Self-Test Registers
    MPU9250_REG_ACC_SLFTST_Y    = 0x0E,
    MPU9250_REG_ACC_SLFTST_Z    = 0x0F,
    MPU9250_REG_SMPLRT_DIV      = 0x19, // Sample Rate Divider
    MPU9250_REG_CFG             = 0x1A, // General configuration register
    MPU9250_REG_GYRO_CFG        = 0x1B, // Gyro specfic configuration register
    MPU9250_REG_ACCEL_CFG       = 0x1C, // Accelerometer specific configration register
    MPU9250_REG_ACCEL_CFG2      = 0x1D, // Accelerometer specific configration register

    MPU9250_REG_INT_BYPASS      = 0x37, // INT Pin / Bypass Enable Configuration
    MPU9250_REG_INT_ENABLE      = 0x38, // Interrupt Enable
    MPU9250_REG_ACCEL_OUT       = 0x3B, // base address for sensor data reads
    
    MPU9250_REG_PWR_MGMT_1      = 0x6B, // Primary power/sleep control register
    MPU9250_REG_PWR_MGMT_2      = 0x6C, // Secondary power/sleep control register
    
    MPU9250_REG_WHO_AM_I    = 0x75, // Divice ID register
} mpu9250_reg_t;

typedef enum {
    MPU9250_SMPL_1000HZ     = 0x00, // Sample Rate Divider
    MPU9250_SMPL_500HZ,
    MPU9250_SMPL_333HZ,
    MPU9250_SMPL_250HZ,
    MPU9250_SMPL_200HZ,
    MPU9250_SMPL_167HZ,
    MPU9250_SMPL_143HZ,
    MPU9250_SMPL_125HZ,
} mpu9250_filter_t;

typedef enum {
    MPU9250_DLPF_250HZ      = 0x00, // DLPF_CFG 
    MPU9250_DLPF_184HZ,
    MPU9250_DLPF_92HZ,
    MPU9250_DLPF_41HZ,
    MPU9250_DLPF_20HZ,
    MPU9250_DLPF_10HZ,
    MPU9250_DLPF_5HZ,
    MPU9250_DLPF_3600HZ,
    MPU9250_SYNC_DISABLED   = 0x08, // FSYNC pin data to be sampled
    MPU9250_SYNC_TEMP,
    MPU9250_SYNC_GYRO_X,
    MPU9250_SYNC_GYRO_Y,
    MPU9250_SYNC_GYRO_Z,
    MPU9250_SYNC_ACCEL_X,
    MPU9250_SYNC_ACCEL_Y,
    MPU9250_SYNC_ACCEL_Z,
    MPU9250_FIFO_BLOCK      = 0x40, // When set to ‘1’, when the fifo is full, additional writes will not be written to fifo.
                                    // When set to ‘0’, when the fifo is full, additional writes will be written to the fifo,
                                    // replacing the oldest data.
} mpu9250_cfg_t;

typedef enum {
    MPU9250_GYR_FCH_b_11    = 0x00,     // inverted Fchoice 11
    MPU9250_GYR_FCH_b_x0    = 0x01,     // inverted Fchoice x0
    MPU9250_GYR_FCH_b_01    = 0x02,     // inverted Fchoice 01
    MPU9250_GYR_SLFTST_Z    = 0x20,     // selftest gyro x
    MPU9250_GYR_SLFTST_Y    = 0x40,
    MPU9250_GYR_SLFTST_X    = 0x80,
} mpu9250_gyro_cfg_t;

typedef enum {
    MPU9250_GYR_FS_250DPS   = 0x00, // Gyro Full Scale +250 deg/s
    MPU9250_GYR_FS_500DPS   = 0x08,
    MPU9250_GYR_FS_1000DPS  = 0x10,
    MPU9250_GYR_FS_2000DPS  = 0x18,
} mpu6050_gyro_fs_t;

typedef enum {
    MPU9250_ACC_SLFTEST_Z   = 0x20,     // selftest accel x
    MPU9250_ACC_SLFTEST_Y   = 0x40,
    MPU9250_ACC_SLFTEST_X   = 0x80,
} mpu6050_accel_cfg_t;

typedef enum {
    MPU9250_ACC_RNG_2   = 0x00,     // +/- 2g (default value)
    MPU6050_ACC_RNG_4   = 0x08,     // +/- 4g
    MPU9250_ACC_RNG_8   = 0x10,     // +/- 8g
    MPU9250_ACC_RNG_16  = 0x18,     // +/- 16g
} mpu6050_accel_range_t;

typedef enum {
    MPU9250_ACC_DLPF_460HZ_0= 0x00,     // accel DLPF
    MPU9250_ACC_DLPF_184HZ  = 0x01,
    MPU9250_ACC_DLPF_92HZ   = 0x02,
    MPU9250_ACC_DLPF_41HZ   = 0x03,
    MPU9250_ACC_DLPF_20HZ   = 0x04,
    MPU9250_ACC_DLPF_10HZ   = 0x05,
    MPU9250_ACC_DLPF_5HZ    = 0x06,
    MPU9250_ACC_DLPF_460HZ_1= 0x07,
    MPU9250_ACC_FCH_b_0     = 0x08,     // inverted Fchoice 0
} mpu6050_accel_cfg2_t;

typedef enum {
    MPU9250_PWR1_CLK_INTERNAL   = 0x00,     // Internal 20MHz oscillator
    MPU9250_PWR1_CLK_AUTO       = 0x01,     // Auto selects the best available clock source – PLL if ready, else
                                            // use the Internal oscillator
    MPU9250_PWR1_CLK_STOP       = 0x07,     // Stops the clock and keeps timing generator in reset
    MPU9250_PWR1_PD_PTAT        = 0x08,     // Power down internal PTAT voltage generator and PTAT ADC
    MPU9250_PWR1_GYRO_STANDBY   = 0x10,     // When set, the gyro drive and pll circuitry are enabled, but the sense paths
                                            // are disabled. This is a low power mode that allows quick enabling of the
                                            // gyros.
    MPU9250_PWR1_CYCLE          = 0x20,     // When this bit is set to 1 and SLEEP is disabled, the MPU-60X0 will cycle
                                            // between sleep mode and waking up to take a single sample of data from
                                            // active sensors at a rate determined by LP_WAKE_CTRL (register 108).
    MPU9250_PWR1_SLEEP          = 0x40,     // this bit puts the MPU-60X0 into sleep mode
    MPU9250_PWR1_DEV_RST        = 0x80,     // this bit resets all internal registers to their default values.
} mpu9250_pwr_mgmt1_t;

typedef enum {
    MPU9250_BYPASS_EN           = 0x02,     // When asserted, the i2c_master interface pins(ES_CL and ES_DA) will go
                                            // into ‘bypass mode’ when the i2c master interface is disabled. The pins
                                            // will float high due to the internal pull-up if not enabled and the i2c master
                                            // interface is disabled.
    MPU9250_FSYNC_INT_MODE_EN   = 0x04,     // 1 – This enables the FSYNC pin to be used as an interrupt. A transition
                                            // to the active level described by the ACTL_FSYNC bit will cause an
                                            // interrupt. The status of the interrupt is read in the I2C Master Status
                                            // register PASS_THROUGH bit.
                                            // 0 – This disables the FSYNC pin from causing an interrupt.
    MPU9250_ACTL_FSYNC          = 0x08,     // 1 – The logic level for the FSYNC pin as an interrupt is active low.
                                            // 0 – The logic level for the FSYNC pin as an interrupt is active high.
    MPU9250_INT_ANYRD_2CLEAR    = 0x10,     // 1 – Interrupt status is cleared if any read operation is performed.
                                            // 0 – Interrupt status is cleared only by reading INT_STATUS register
    MPU9250_LATCH_INT_EN        = 0x20,     // 1 – INT pin level held until interrupt status is cleared
                                            // 0 – INT pin indicates interrupt pulse’s is width 50us.
    MPU9250_INTPIN_OPEN         = 0x40,     // 1 – INT pin is configured as open drain.
                                            // 0 – INT pin is configured as push-pull.
    MPU9250_INTPIN_ACTL         = 0x80,     // 1 – The logic level for INT pin is active low.
                                            // 0 – The logic level for INT pin is active high.
} mpu9250_int_bypass_t;

static bool accInit() {
    uint8_t id = wread8(MPU9250_ADDR, MPU9250_REG_WHO_AM_I);
    CONSOLE("MPU9250_REG_WHO_AM_I: 0x%02x", id) ;
    if ((id != 0x68) && (id != 0x71) && (id != 0x73) && (id != 0x75))
        return false;
    

    wwrite8(MPU9250_ADDR, MPU9250_REG_PWR_MGMT_1, MPU9250_PWR1_DEV_RST);
    delay(100);
    wwrite8(MPU9250_ADDR, MPU9250_REG_PWR_MGMT_1, 0);
    delay(100);
    
    // power (disable sleep, set clock source)
    wwrite8(MPU9250_ADDR, MPU9250_REG_PWR_MGMT_1, MPU9250_PWR1_CLK_AUTO);
    
    // DLPF фильтр
    wwrite8(MPU9250_ADDR, MPU9250_REG_CFG, MPU9250_DLPF_41HZ);
    
    // sample rate divider
    wwrite8(MPU9250_ADDR, MPU9250_REG_SMPLRT_DIV, MPU9250_SMPL_200HZ);

    // gyro cfg
    wwrite8(MPU9250_ADDR, MPU9250_REG_GYRO_CFG, MPU9250_GYR_FCH_b_11 | MPU9250_GYR_FS_500DPS );
    
    // accel range
    wwrite8(MPU9250_ADDR, MPU9250_REG_ACCEL_CFG, MPU9250_ACC_RNG_2);
    wwrite8(MPU9250_ADDR, MPU9250_REG_ACCEL_CFG2, MPU9250_ACC_DLPF_41HZ);
    
    // i2c bypass enable
    wwrite8(MPU9250_ADDR, MPU9250_REG_INT_BYPASS, MPU9250_BYPASS_EN);
    delay(200);
    
    return false;
}

static void accStop() {
    wwrite8(MPU9250_ADDR, MPU9250_REG_PWR_MGMT_1, MPU9250_PWR1_CLK_STOP | MPU9250_PWR1_SLEEP | MPU9250_PWR1_DEV_RST);
}

static vec16_t accRead() {
    if (!wreq(MPU9250_ADDR, MPU9250_REG_ACCEL_OUT, 6))
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
    
    return a;
}

/* --------------------------------------------------
 *                      AK8963 - Magnitometer
 */
static vec8_t adj;

#define AK8963_ADDR 0x0C

typedef enum {
    AK8963_REG_WHO_AM_I     = 0x00,     // WIA: Device ID
    AK8963_REG_ST1          = 0x02,     // Status 1
    AK8963_REG_OUT          = 0x03,     // HXL to HZH: Measurement Data
    AK8963_REG_CNTL1        = 0x0A,     // Control 1
    AK8963_REG_FUSEROM_ADJ  = 0x10,     // Fuse ROM sensitivity adjustment value (x,y,z axis)
} ak8963_reg_t;

typedef enum {
    AK8963_ST1_DRDY         = 0x01,     // Data Ready
    //AK8963_ST1_DOR          = 0x,     // Data Overrun
} ak8963_status1_t;

typedef enum {
    AK8963_MODE_PWRDOWN         = 0x00,     // Power-down mode
    AK8963_MODE_SINGLE          = 0x01,     // Single measurement mode
    AK8963_MODE_CONTINUOUS_1    = 0x02,     // Continuous measurement mode 1 (8Hz)
    AK8963_MODE_EXT_TRIGGER     = 0x04,     // External trigger measurement mode
    AK8963_MODE_CONTINUOUS_2    = 0x06,     // Continuous measurement mode 2 (100Hz)
    AK8963_MODE_SELF_TEST       = 0x08,     // Self-test mode
    AK8963_MODE_FUSE_ROM        = 0x0F,     // Fuse ROM access mode
    
    AK8963_OUT_16BIT            = 0x10,     // "1": 16-bit output ("0": 14-bit output)
} ak8963_cntl1_t;

static bool magInit() {
    // Проверяем наличие чипа
    uint8_t id = wread8(AK8963_ADDR, AK8963_REG_WHO_AM_I);
    CONSOLE("AK8963_REG_WHO_AM_I: 0x%02x", id) ;
    if (id != 0x48)
        return false;
    
    //Самодиагностика
    wwrite8(AK8963_ADDR, AK8963_REG_CNTL1, AK8963_MODE_PWRDOWN);
    delay(10);
    wwrite8(AK8963_ADDR, AK8963_REG_CNTL1, AK8963_MODE_FUSE_ROM);
    delay(10);
    if (!wreq(AK8963_ADDR, AK8963_REG_FUSEROM_ADJ, 3))
        return false;
    adj.x = Wire.read();
    adj.y = Wire.read();
    adj.z = Wire.read();
    wend();
    CONSOLE("mag adj: %d, %d, %d", adj.x, adj.y, adj.z);
    
    // Возвращаемся в нормальный режим
    wwrite8(AK8963_ADDR, AK8963_REG_CNTL1, AK8963_MODE_PWRDOWN);
    delay(10);
    wwrite8(AK8963_ADDR, AK8963_REG_CNTL1, AK8963_MODE_CONTINUOUS_2 | AK8963_OUT_16BIT);

    delay(10);
    uint8_t st1 = wread8(AK8963_ADDR, AK8963_REG_ST1);
    CONSOLE("Mag status1: 0x%02x", st1) ;
    
    return true;
}

static void magStop() {
    wwrite8(AK8963_ADDR, AK8963_REG_CNTL1, AK8963_MODE_PWRDOWN);
}

static vec16_t magRead() {
    if (!wreq(AK8963_ADDR, AK8963_REG_OUT, 6))
        return { 0 };
    
    vec16_t m;
    
    m.x = Wire.read();
    m.x = (m.x << 8) | Wire.read();
    m.y = Wire.read();
    m.y = (m.z << 8) | Wire.read();
    m.z = Wire.read();
    m.z = (m.y << 8) | Wire.read();
    
    wend();
    
    m.x = (static_cast<int32_t>(m.x) * adj.x + static_cast<int32_t>(m.x)) / 256;
    m.y = (static_cast<int32_t>(m.y) * adj.y + static_cast<int32_t>(m.y)) / 256;
    m.z = (static_cast<int32_t>(m.z) * adj.z + static_cast<int32_t>(m.z)) / 256;
    
    return m;
}
