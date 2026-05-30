#ifndef __CONFIG_H
#define __CONFIG_H

/* ===== Механічні константи ===== */
// Двигун: 86HBP113AL4 (NEMA34, 8.5 Нм, 200 кроків/оберт)
// Редуктор: PX86 планетарний 1:10
// Гвинт: T20×4 (крок 4 мм/оберт)
// Драйвер: DM556, мікрокрок 1/8 (1600 кроків/оберт)
#define SCREW_PITCH_MM      4.0f    // крок гвинта T20x4
#define GEAR_RATIO          10.0f   // PX86 планетарний редуктор 1:10
#define MICROSTEP           8       // мікрокрок DM556 (SW5=ON SW6=OFF SW7=ON SW8=ON)
#define MOTOR_STEPS_REV     200     // кроків/оберт (1.8°)

// Кроків на 1 мм = (200 * 8 * 10) / 4 = 4000
#define STEPS_PER_MM        4000.0f

/* ===== Швидкості (кроків/с) =====
 * TIM7 = 1ms → period = 1000/speed → min period = 1ms → max 500 full steps/s
 * Реальна швидкість: speed_actual = 1000 / (2 * period_ms) full steps/s
 *   SPEED_FAST: period=1ms → 500 steps/s → 0.125 mm/s ≈ 7.5 mm/min
 *   SPEED_SLOW: period=2ms → 250 steps/s → 0.063 mm/s ≈ 3.75 mm/min
 *   SPEED_ENC:  period=5ms → 100 steps/s → 0.025 mm/s ≈ 1.5 mm/min
 */
#define SPEED_FAST          1000    // джойстик, грубий рух (period=1ms)
#define SPEED_SLOW          400     // при наближенні до цілі — 80% зусилля (period=2ms)
#define SPEED_ENC           200     // енкодер, тонке підналаштування (period=5ms)
#define ACCEL_STEPS         2000    // кроків/с² (не використовується напряму, залишено для документації)

/* ===== Зусилля ===== */
// Датчик: LCF-6-V 2T (2000 кг = ~19.6 кН)
// Робочий діапазон: 7–16 кН (PDE TDI 300/400)
#define KN_TO_KG            101.97f              // 1 кН = 101.97 кг
#define FORCE_MAX_KG        2040.0f              // 20 кН — ліміт датчика
#define FORCE_DEFAULT_KN    8.75f               // T300 Nozzle midpoint
#define FORCE_DEFAULT_KG    (FORCE_DEFAULT_KN * KN_TO_KG)  // ≈892 кг
#define FORCE_STEP_KN       0.1f                // крок енкодера в кН
#define FORCE_STEP_KG       (FORCE_STEP_KN * KN_TO_KG)     // ≈10.2 кг
#define SLOWDOWN_THRESHOLD  0.80f               // 80% → перехід на SPEED_SLOW
#define OVERLOAD_FACTOR     1.10f               // 110% → аварійна зупинка

/* ===== EEPROM (емуляція у Flash) ===== */
#define EEPROM_CALIB_FACTOR   0x00  // float, 4 байти — калібрувальний коефіцієнт HX711
#define EEPROM_CALIB_OFFSET   0x04  // int32, 4 байти — нульове зміщення (tare)
#define EEPROM_TARGET_FORCE   0x08  // float, 4 байти — останнє задане зусилля
#define EEPROM_MAGIC          0x0C  // uint8 = 0xAB — маркер валідності даних
#define EEPROM_MAGIC_VALUE    0xAC  // 0xAC = v2: додано EEPROM_ANGLE_TARGET

/* ===== HX711 налаштування ===== */
#define HX711_GAIN_128      1       // Channel A, gain 128 (за замовчуванням)
#define HX711_TIMEOUT_MS    500     // таймаут очікування готовності

/* ===== LCD2004 I2C адреса ===== */
#define LCD_I2C_ADDR        (0x27 << 1)  // спробувати 0x3F << 1 якщо не працює

/* ===== Антидребезг ===== */
#define DEBOUNCE_MS         20      // мс для кнопок/джойстика

/* ===== Тонка підстройка (ENC hold mode) ===== */
#define FINE_TIMEOUT_MS     300     // мс без тіків енкодера → зупин мотора

/* ===== Кутовий енкодер (PandAuto P3022-V1-CW360) ===== */
// TIM3 у режимі Encoder Interface (4x), PC6=CH1 (A), PC7=CH2 (B)
#define ANGLE_ENC_PPR       360                              // імпульсів/оберт
#define ANGLE_STEPS_REV     (ANGLE_ENC_PPR * 4)             // 1440 кроків/об (4x)
#define ANGLE_DEG_PER_STEP  (360.0f / ANGLE_STEPS_REV)      // 0.25°/крок
#define ANGLE_DEFAULT_DEG   90.0f                            // типовий кут VW PD
#define ANGLE_STEP_DEG      5.0f                             // крок зміни у налаштуваннях
#define ANGLE_MAX_DEG       720.0f                           // макс. 2 оберти

/* ===== EEPROM — кутовий енкодер ===== */
#define EEPROM_ANGLE_TARGET   0x10  // float, 4 байти — цільовий кут (градуси)
// 0x14–0xFF зарезервовано

#endif /* __CONFIG_H */
