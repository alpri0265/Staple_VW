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
// Датчик: LCF-6-V 2T (2000 кг), практичне обмеження для НФ
#define FORCE_MAX_KG        500.0f
#define FORCE_DEFAULT_KG    15.0f
#define FORCE_STEP_KG       0.1f    // крок енкодера
#define SLOWDOWN_THRESHOLD  0.80f   // 80% → перехід на SPEED_SLOW
#define OVERLOAD_FACTOR     1.10f   // 110% → аварійна зупинка

/* ===== EEPROM (емуляція у Flash) ===== */
#define EEPROM_CALIB_FACTOR   0x00  // float, 4 байти — калібрувальний коефіцієнт HX711
#define EEPROM_CALIB_OFFSET   0x04  // int32, 4 байти — нульове зміщення (tare)
#define EEPROM_TARGET_FORCE   0x08  // float, 4 байти — останнє задане зусилля
#define EEPROM_MAGIC          0x0C  // uint8 = 0xAB — маркер валідності даних
#define EEPROM_MAGIC_VALUE    0xAB

/* ===== HX711 налаштування ===== */
#define HX711_GAIN_128      1       // Channel A, gain 128 (за замовчуванням)
#define HX711_TIMEOUT_MS    500     // таймаут очікування готовності

/* ===== LCD2004 I2C адреса ===== */
#define LCD_I2C_ADDR        (0x27 << 1)  // спробувати 0x3F << 1 якщо не працює

/* ===== Антидребезг ===== */
#define DEBOUNCE_MS         20      // мс для кнопок/джойстика

/* ===== Тонка підстройка (ENC hold mode) ===== */
#define FINE_TIMEOUT_MS     300     // мс без тіків енкодера → зупин мотора

#endif /* __CONFIG_H */
