/* --------------------------------------------------
 *                      Magnitometer
 */
static vec16_t bias = { 766, 766, 713 };

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
    
    wend();
    
    m.x = static_cast<int32_t>(m.x) * 766 / bias.x;
    m.y = static_cast<int32_t>(m.y) * 766 / bias.y;
    m.z = static_cast<int32_t>(m.z) * 766 / bias.z;
    
    return m;
}
