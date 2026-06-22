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

/* ===== Таймінг TIM7 / STEP =====
 * TIM7 працює з тікoм 100 us.
 * STEP формується коротким імпульсом тривалістю STEP_PULSE_TICKS тікiв.
 */
#define TIM7_TICK_US        100U
#define STEP_PULSE_TICKS    1U      // 100 us HIGH, чого достатньо для DM556

/* ===== Швидкості (кроків/с) =====
 * При TIM7 = 100 us швидкість відповідає заданій набагато точніше.
 *   SPEED_FAST:     1000 steps/s → 0.25 mm/s ≈ 15 mm/min
 *   SPEED_PRESS:     400 steps/s → 0.10 mm/s ≈ 6  mm/min
 *   SPEED_ENC:       200 steps/s → 0.05 mm/s ≈ 3  mm/min
 *   SPEED_APPROACH: 2000 steps/s → 0.50 mm/s ≈ 30 mm/min
 *   SPEED_APPROACH_SOFT: 800 steps/s → 0.20 mm/s ≈ 12 mm/min
 */
#define SPEED_FAST          1000    // вільний рух / retract
#define SPEED_PRESS         400     // робочий рух після контакту
#define SPEED_ENC           200     // енкодер, тонке підналаштування
#define SPEED_APPROACH      2000    // швидкий підхід до деталі до появи навантаження
#define SPEED_APPROACH_SOFT 800     // м'який підхід перед контактом
#define APPROACH_SOFT_KG    5.0f    // після цього порогу скидаємо швидкість до soft approach
#define APPROACH_CONTACT_KG 10.0f   // поріг контакту: після нього переходимо на SPEED_PRESS
#define HEAVY_START_SPEED   80      // окремий профіль старту для важкої механіки
#define HEAVY_RAMP_MS       20      // раз на N мс зменшуємо/збільшуємо period у heavy profile
#define HEAVY_RAMP_STEP     1       // крок зміни period у heavy profile
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
#define SLOWDOWN_THRESHOLD  0.80f               // 80% → резерв під додаткове уповільнення
#define OVERLOAD_FACTOR     1.10f               // 110% → аварійна зупинка

/* ===== AUTO режим по зусиллю ===== */
#define AUTO_FORCE_TOLERANCE_KG  3.0f    // вікно готовності навколо target
#define AUTO_CRUISE_ENTRY_KG     50.0f   // вище цієї похибки можна ще рухатись безперервно
#define AUTO_BURST_LARGE_ERR_KG  20.0f   // велика похибка → великий burst
#define AUTO_BURST_MED_ERR_KG    8.0f    // середня похибка → середній burst
#define AUTO_BURST_LARGE_STEPS   50U
#define AUTO_BURST_MED_STEPS     20U
#define AUTO_BURST_SMALL_STEPS   5U
#define AUTO_SETTLE_MS           400U    // пауза на стабілізацію після burst
#define AUTO_HOLD_MS             700U    // коротка пауза підтвердження в допуску
#define AUTO_DONE_MS             1500U   // показати DONE перед поверненням в IDLE
#define AUTO_FAST_FILTER_ALPHA   0.50f   // швидкий фільтр сили для AUTO алгоритму

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
