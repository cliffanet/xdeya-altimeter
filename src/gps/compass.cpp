
#include "compass.h"
#include "../log.h"
#include "../clock.h"

#include <Wire.h>

static compass_t _cmp = { 0 };
static vec16_t bias = { 766, 766, 713 };

#define FILTSZ 7
static double filt_sin[FILTSZ] = { 0 };
static double filt_cos[FILTSZ] = { 0 };
static uint8_t filt_cur=0;


/* --------------------------------------------------
 *                      vector func
 */
template <typename Ta, typename Tb>
vec32_t vec_cross(const vec_t<Ta> &a1, const vec_t<Tb> &b) {
    vec32_t a = {
        static_cast<int32_t>(a1.x),
        static_cast<int32_t>(a1.y),
        static_cast<int32_t>(a1.z)
    };
    return {
        (a.y * b.z) - (a.z * b.y),
        (a.z * b.x) - (a.x * b.z),
        (a.x * b.y) - (a.y * b.x)
    };
}

template <typename Ta, typename Tb> float vec_sp(const vec_t<Ta> &a, const vec_t<Tb> &b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

template <typename T> float vec_len(const vec_t<T> &a) {
    return sqrt(vec_sp(a, a));
}

template <typename T> void vec_norm(vec_t<T> &a) {
    float len = vec_len(a);
    a.x /= len;
    a.y /= len;
    a.z /= len;
}

static vecf_t vec16f(const vec16_t &a) {
    return {
        static_cast<float>(a.x),
        static_cast<float>(a.y),
        static_cast<float>(a.z)
    };
}

/* --------------------------------------------------
 *                      wire
 */
static void wwrite8(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}
static uint8_t wread8(uint8_t addr, uint8_t reg) {
    uint8_t val;
    
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
    
    Wire.requestFrom(addr, (uint8_t)1);
    val = Wire.read();
    Wire.endTransmission();

    return val;
}
static bool wwait(uint8_t count, uint64_t us = 100000) {
    uint64_t u = utick();
    while ((Wire.available() < count) && (utm_diff(u) < 100000)) ;
    if (Wire.available() >= count)
        return true;
    CONSOLE("wire read timeout");
    return false;
}
static bool wreq(uint8_t addr, uint8_t reg, uint8_t count, uint64_t us = 100000) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(addr, count);

    // Wait around until enough data is available
    return wwait(count, us);
}
static bool wreq(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t count, uint64_t us = 100000) {
    if (!wreq(addr, reg, count, us))
        return false;
    while (count > 0) {
        *data = Wire.read();
        data++;
        count--;
    }
    
    return true;
}


/* --------------------------------------------------
 *                      Magnitometer
 */
#define HMC5883_ADDR 0x1E // 0011110x

typedef enum {
  HMC5883_REG_CFG_A     = 0x00,
  HMC5883_REG_CFG_B     = 0x01,
  HMC5883_REG_MODE      = 0x02,
  HMC5883_REG_OUT_X_H   = 0x03,
  HMC5883_REG_OUT_X_L   = 0x04,
  HMC5883_REG_OUT_Z_H   = 0x05,
  HMC5883_REG_OUT_Z_L   = 0x06,
  HMC5883_REG_OUT_Y_H   = 0x07,
  HMC5883_REG_OUT_Y_L   = 0x08,
  HMC5883_REG_STATUS    = 0x09,
  HMC5883_REG_ID_A      = 0x0A,
  HMC5883_REG_ID_D      = 0x0B,
  HMC5883_REG_ID_C      = 0x0C,
} hmc5883_reg_t;

typedef enum {
  HMC5883_AVG_1         = 0x00,
  HMC5883_AVG_2         = 0x20,
  HMC5883_AVG_4         = 0x40,
  HMC5883_AVG_8         = 0x60,
} hmc5883_avg_t;

typedef enum {
  HMC5883_RATE_00_75    = 0x00,
  HMC5883_RATE_01_50    = 0x04,
  HMC5883_RATE_03_00    = 0x08,
  HMC5883_RATE_07_50    = 0x0C,
  HMC5883_RATE_15       = 0x10,
  HMC5883_RATE_30       = 0x14,
  HMC5883_RATE_75       = 0x18,
} hmc5883_rate_t;

typedef enum {
  HMC5883_MEAS_NORM     = 0x00,
  HMC5883_MEAS_BIAS_POS = 0x01,
  HMC5883_MEAS_BIAS_NEG = 0x02,
} hmc5883_meas_t;

typedef enum {
  HMC5883_MODE_CONTINUOUS   = 0x00,
  HMC5883_MODE_SINGLE       = 0x01,
  HMC5883_MODE_IDLE         = 0x02,
} hmc5883_mode_t;

typedef enum {
  HMC5883_GAIN_0_9 = 0x00, // +/- 0.88
  HMC5883_GAIN_1_3 = 0x20, // +/- 1.3
  HMC5883_GAIN_1_9 = 0x40, // +/- 1.9
  HMC5883_GAIN_2_5 = 0x60, // +/- 2.5
  HMC5883_GAIN_4_0 = 0x80, // +/- 4.0
  HMC5883_GAIN_4_7 = 0xA0, // +/- 4.7
  HMC5883_GAIN_5_6 = 0xC0, // +/- 5.6
  HMC5883_GAIN_8_1 = 0xE0  // +/- 8.1
} hmc5883_gain_t;

static vec16_t magRead();
static bool magInit() {
    // Проверяем наличие чипа
    uint8_t id[3];
    if (!wreq(HMC5883_ADDR, HMC5883_REG_ID_A, id, 3))
        return false;
    CONSOLE("mag id: 0x%02x, 0x%02x, 0x%02x", id[0], id[1], id[2]);
    if ((id[0] != 'H') || (id[1] != '4') || (id[2] != '3'))
        return false;
    
    //Самодиагностика
    wwrite8(HMC5883_ADDR, HMC5883_REG_MODE,  HMC5883_MODE_IDLE);
    wwrite8(HMC5883_ADDR, HMC5883_REG_CFG_B, HMC5883_GAIN_2_5);
    wwrite8(HMC5883_ADDR, HMC5883_REG_CFG_A, HMC5883_AVG_8 | HMC5883_RATE_15 | HMC5883_MEAS_BIAS_POS);
    wwrite8(HMC5883_ADDR, HMC5883_REG_MODE, HMC5883_MODE_CONTINUOUS);
    delay(80);
    bias = magRead();
    CONSOLE("mag bias positive: %d, %d, %d", bias.x, bias.y, bias.z);
    
    // Возвращаемся в нормальный режим
    wwrite8(HMC5883_ADDR, HMC5883_REG_CFG_A, HMC5883_AVG_8 | HMC5883_RATE_15 | HMC5883_MEAS_NORM);
    // Enable the magnetometer
    wwrite8(HMC5883_ADDR, HMC5883_REG_MODE, HMC5883_MODE_CONTINUOUS);

    // Set the gain to a known level
    wwrite8(HMC5883_ADDR, HMC5883_REG_CFG_B, HMC5883_GAIN_1_3);
    
    return true;
}

static void magStop() {
    wwrite8(HMC5883_ADDR, HMC5883_REG_MODE, HMC5883_MODE_IDLE);
}

static vec16_t magRead() {
    if (!wreq(HMC5883_ADDR, HMC5883_REG_OUT_X_H, 6))
        return { 0 };
    
    vec16_t m;
    
    m.x = Wire.read();
    m.x = (m.x << 8) | Wire.read();
    m.z = Wire.read();
    m.z = (m.z << 8) | Wire.read();
    m.y = Wire.read();
    m.y = (m.y << 8) | Wire.read();
    
    return m;
}


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
    
    return a;
}


/* --------------------------------------------------
 *                      Compass Processing
 */
const compass_t &compass() { return _cmp; }

void compInit() {
    _cmp.ok = 0;
    if (magInit())
        _cmp.ok |= 1;
    else
        CONSOLE("compas fail");
    
    if (accInit())
        _cmp.ok |= 2;
    else
        CONSOLE("accel/gyro fail");
    
    if (_cmp.ok == 3)
        CONSOLE("compas & accel/gyro init ok");
}

void compStop() {
    if (_cmp.ok & 1)
        magStop();
    if (_cmp.ok & 2)
        accStop();
    _cmp = { 0 };
}

void compProcess() {
    if (_cmp.ok & 1) {
        vec16_t v = magRead(); 
        _cmp.mag.x = static_cast<int32_t>(v.x) * 766 / bias.x;
        _cmp.mag.y = static_cast<int32_t>(v.y) * 766 / bias.y;
        _cmp.mag.z = static_cast<int32_t>(v.z) * 766 / bias.z;
    }
    
    if (_cmp.ok & 2)
        _cmp.acc = accRead();
    
    // heading
    if ((_cmp.ok & 3) == 3) {
        vec32_t E = vec_cross(_cmp.mag, _cmp.acc);
        _cmp.e = E;
        //vec32_t N = vec_cross(_cmp.acc, E);
        
        // В математике 0 градусов - это направление оси X,
        // А нам нужны навигационные градусы (по оси Y),
        // Поэтому нам фактически нужен даже не вектор N,
        // а как раз подходит вектор E - его математические
        // градусы равны навигационным градусам вектора N
        _cmp.head = atan2(static_cast<double>(E.y), E.x);

        filt_sin[filt_cur] = sin(_cmp.head);
        filt_cos[filt_cur] = cos(_cmp.head);
        filt_cur ++;
        filt_cur %= FILTSZ;
        double ssin = 0, scos = 0;
        for (auto &f : filt_sin) ssin += f;
        for (auto &f : filt_cos) scos += f;
        _cmp.head = atan2(ssin, scos);
        
        while (_cmp.head < 0)
            _cmp.head += 2*PI;
        while (_cmp.head > 2*PI)
            _cmp.head -= 2*PI;
    }
}

