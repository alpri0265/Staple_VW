# CLAUDE.md — Стапель насос-форсунок VW
> Цей файл містить повний контекст проекту для автономної роботи Claude Code в Cursor.
> Читай його ПОВНІСТЮ перед будь-якими діями.

---

## 1. Що це за проект

Прес-стапель для розбирання/збирання насос-форсунок Volkswagen.
Шаговий двигун через редуктор крутить гвинт T20×4 і створює притискне зусилля.
Зусилля вимірюється тензодатчиком Load Cell + HX711.
Оператор керує джойстиком (грубо) та енкодером (точно).
LCD2004 показує поточне та задане зусилля, меню налаштувань.

---

## 2. Залізо

| Компонент | Деталь |
|---|---|
| МК | STM32F407ZET6 (BLACK_F407ZE) |
| Двигун | Шаговий + драйвер A4988 або DRV8825 |
| Редуктор | Передавальне число = 20 |
| Гвинт | T20×4 (крок 4 мм/оберт) |
| Датчик зусилля | Load Cell + HX711 |
| Дисплей | LCD2004 по I2C |
| Керування | Джойстик 2 положення + енкодер KY-040 + кнопка СТОП |
| Концевики | 2 шт, NC, механічні |
| Тактування | HSE 8 MHz → PLL → 168 MHz |
| IDE | Cursor + PlatformIO, framework=stm32cube |

---

## 3. Піни (з main.h — НЕ ЗМІНЮВАТИ)

```c
// Шаговий двигун
#define STEP_Pin            GPIO_PIN_0   // GPIOE
#define DIR_Pin             GPIO_PIN_1   // GPIOE
#define ENABLE_Pin          GPIO_PIN_2   // GPIOE

// HX711
#define HX_DAT_Pin          GPIO_PIN_0   // GPIOC  Input Pull-up
#define HX_CLK_Pin          GPIO_PIN_1   // GPIOC  Output PP

// Кнопка СТОП
#define STOP_BTN_Pin        GPIO_PIN_0   // GPIOA  EXTI Falling, Pull-up, пріоритет 0

// Енкодер
#define ENC_CLK_Pin         GPIO_PIN_3   // GPIOB  EXTI Falling, Pull-up, пріоритет 1
#define ENC_DT_Pin          GPIO_PIN_1   // GPIOB  Input Pull-up
#define ENC_SW_Pin          GPIO_PIN_2   // GPIOB  Input Pull-up

// Концевики
#define LIMIT_TOP_Pin       GPIO_PIN_0   // GPIOD  Input Pull-up (LOW = спрацював)
#define LIMIT_BOT_Pin       GPIO_PIN_1   // GPIOD  Input Pull-up (LOW = спрацював)

// Джойстик
#define JOY_UP_Pin          GPIO_PIN_2   // GPIOD  Input Pull-up (LOW = натиснуто)
#define JOY_DOWN_Pin        GPIO_PIN_3   // GPIOD  Input Pull-up (LOW = натиснуто)

// I2C1 (LCD2004)
// PB6 = SCL, PB7 = SDA  AF OD 100kHz
```

---

## 4. Таймери та переривання

| Периферія | Налаштування | Призначення |
|---|---|---|
| TIM6 | Системний timebase HAL | HAL_IncTick() — НЕ ЧІПАТИ |
| TIM7 | Prescaler=167, Period=999 → 1ms тік | Генерація STEP імпульсів + опит входів |
| EXTI0 (PA0) | Falling, пріоритет 0 | Аварійна зупинка СТОП |
| EXTI3 (PB3) | Falling, пріоритет 1 | Енкодер CLK |
| I2C1 EV/ER | пріоритет 3 | LCD2004 |

---

## 5. Механічні константи

```c
#define SCREW_PITCH_MM      4.0f    // крок гвинта T20x4
#define GEAR_RATIO          20.0f   // передавальне число редуктора
#define MICROSTEP           8       // мікрокрок драйвера
#define MOTOR_STEPS_REV     200     // кроків/оберт (1.8°)

// Кроків на 1 мм = (200 * 8 * 20) / 4 = 8000
#define STEPS_PER_MM        8000.0f

// Швидкості (кроків/с)
#define SPEED_FAST          4000    // джойстик, грубий рух
#define SPEED_SLOW          800     // при наближенні до цілі (80% зусилля)
#define SPEED_ENC           400     // енкодер, тонке підналаштування
#define ACCEL_STEPS         2000    // кроків/с² (програмне прискорення)

// Зусилля
#define FORCE_MAX_KG        50.0f
#define FORCE_DEFAULT_KG    15.0f
#define FORCE_STEP_KG       0.1f    // крок енкодера
#define SLOWDOWN_THRESHOLD  0.80f   // 80% → перехід на SPEED_SLOW
#define OVERLOAD_FACTOR     1.10f   // 110% → аварійна зупинка
```

---

## 6. Структура файлів проекту

```
Staple_VW/
├── CLAUDE.md               ← цей файл
├── Core/
│   ├── Inc/
│   │   ├── main.h          ← згенеровано CubeMX, НЕ ЗМІНЮВАТИ поза USER CODE
│   │   ├── config.h        ← ← ← СТВОРИТИ: всі константи проекту
│   │   ├── motor.h         ← ← ← СТВОРИТИ
│   │   ├── loadcell.h      ← ← ← СТВОРИТИ
│   │   ├── display.h       ← ← ← СТВОРИТИ
│   │   ├── input.h         ← ← ← СТВОРИТИ
│   │   └── calibration.h   ← ← ← СТВОРИТИ
│   └── Src/
│       ├── main.c          ← згенеровано CubeMX, додавати тільки в USER CODE
│       ├── motor.c         ← ← ← СТВОРИТИ
│       ├── loadcell.c      ← ← ← СТВОРИТИ
│       ├── display.c       ← ← ← СТВОРИТИ
│       ├── input.c         ← ← ← СТВОРИТИ
│       └── calibration.c   ← ← ← СТВОРИТИ
├── Drivers/                ← згенеровано CubeMX, НЕ ЧІПАТИ
└── Staple_VW.ioc           ← CubeMX проект
```

---

## 7. EEPROM (емуляція у Flash STM32)

```c
// Адреси в емульованому EEPROM (або backup registers)
#define EEPROM_CALIB_FACTOR   0x00  // float, 4 байти — калібрувальний коефіцієнт HX711
#define EEPROM_CALIB_OFFSET   0x04  // int32, 4 байти — нульове зміщення (tare)
#define EEPROM_TARGET_FORCE   0x08  // float, 4 байти — останнє задане зусилля
#define EEPROM_MAGIC          0x0C  // uint8 = 0xAB — маркер валідності даних
```

---

## 8. Архітектура програми

### Головний цикл (main.c)
```
setup():
  HAL_Init()
  SystemClock_Config()
  MX_GPIO_Init()
  MX_I2C1_Init()
  MX_TIM7_Init()
  motor_init()
  loadcell_init()
  display_init()
  input_init()
  HAL_TIM_Base_Start_IT(&htim7)  ← запуск 1ms тіку

loop():
  loadcell_update()     ← зчитати зусилля
  input_update()        ← опит джойстика, енкодера, кнопок
  motor_update()        ← логіка руху + захист
  display_update()      ← оновити LCD якщо змінились дані
```

### TIM7 IRQ (1ms тік)
```
HAL_TIM_PeriodElapsedCallback(TIM7):
  motor_tim_tick()      ← генерація STEP імпульсів
  input_debounce_tick() ← антидребезг кнопок
```

### EXTI0 IRQ (СТОП, пріоритет 0)
```
HAL_GPIO_EXTI_Callback(STOP_BTN_Pin):
  motor_emergency_stop()   ← миттєва зупинка
  g_emergency_stop = true  ← прапор для main loop
```

### EXTI3 IRQ (Енкодер CLK, пріоритет 1)
```
HAL_GPIO_EXTI_Callback(ENC_CLK_Pin):
  читаємо ENC_DT → визначаємо напрямок
  оновлюємо g_enc_delta (volatile int8_t)
```

---

## 9. Модулі — детальний опис

### motor.c / motor.h
Керування шаговим двигуном без сторонніх бібліотек (чистий HAL).

**Алгоритм генерації STEP:**
- В TIM7 IRQ (кожну 1ms) декрементуємо лічильник `step_timer`
- Коли `step_timer == 0` → toggle STEP_Pin → перезавантажити `step_timer = step_period`
- `step_period` (мс між кроками) визначає швидкість
- Програмне прискорення: поступово зменшувати `step_period` від `period_start` до `period_target`

**Функції:**
```c
void motor_init(void);
void motor_update(void);          // викликати з main loop
void motor_tim_tick(void);        // викликати з TIM7 IRQ — генерує STEP
void motor_move_up(uint16_t speed);
void motor_move_down(uint16_t speed);
void motor_stop(void);            // м'яка зупинка
void motor_emergency_stop(void);  // миттєва зупинка (з ISR)
bool motor_is_limit_top(void);
bool motor_is_limit_bot(void);
bool motor_is_running(void);
float motor_get_position_mm(void);
void motor_reset_position(void);
MotorState motor_get_state(void);
```

### loadcell.c / loadcell.h
Бітбанг протокол HX711 через GPIO (PC0=DAT, PC1=CLK).

**Функції:**
```c
void    loadcell_init(void);
void    loadcell_update(void);       // викликати з main loop
float   loadcell_get_kg(void);       // повертає зусилля в кг
bool    loadcell_is_ready(void);     // HX711 готовий до зчитування
void    loadcell_tare(void);         // скинути нуль
void    loadcell_set_scale(float s); // встановити калібрувальний коефіцієнт
float   loadcell_get_scale(void);
int32_t loadcell_read_raw(void);     // сире значення для калібровки
```

### display.c / display.h
LCD2004 по I2C (адреса 0x27 або 0x3F).

**Екрани (enum DisplayScreen):**
```c
SCREEN_MAIN        // головний робочий екран
SCREEN_MENU        // головне меню
SCREEN_CALIBRATION // режим калібровки
SCREEN_SETTINGS    // налаштування (макс. зусилля, швидкість)
SCREEN_ERROR       // помилка
```

**Головний екран:**
```
┌────────────────────┐
│ ПРЕС НФ VW         │
│ Зусилля: 12.4 кг   │
│ Задання: 15.0 кг   │
│ [^v] ENC  СТОП     │
└────────────────────┘
```

**Меню (навігація енкодером):**
```
> 1. Робочий режим
  2. Калібровка
  3. Налаштування
  4. Додому (HOME)
```

**Функції:**
```c
void display_init(void);
void display_update(void);           // оновити якщо змінились дані
void display_set_screen(DisplayScreen s);
void display_set_force(float current, float target);
void display_show_error(const char *msg);
void display_menu_next(void);
void display_menu_select(void);
```

### input.c / input.h
Опит джойстика, енкодера, кнопок з антидребезгом.

**Функції:**
```c
void input_init(void);
void input_update(void);          // викликати з main loop
void input_debounce_tick(void);   // викликати з TIM7 IRQ (1ms)

bool input_joy_up(void);          // джойстик вгору (з дребезгом)
bool input_joy_down(void);        // джойстик вниз
bool input_enc_sw_pressed(void);  // кнопка енкодера (одноразово)
int8_t input_enc_get_delta(void); // кроки енкодера з останнього виклику
bool input_stop_pressed(void);    // прапор аварійної зупинки
void input_stop_clear(void);      // скинути прапор після обробки
```

### calibration.c / calibration.h
Покроковий майстер калібровки HX711.

**Кроки калібровки:**
```
CALIB_STEP_IDLE       → старт
CALIB_STEP_TARE       → "Приберіть вантаж" → чекаємо кнопку → tare()
CALIB_STEP_LOAD       → "Поставте еталон" → чекаємо кнопку
CALIB_STEP_INPUT_MASS → енкодером вводимо масу еталону (кг)
CALIB_STEP_CALCULATE  → рахуємо scale = raw / known_mass
CALIB_STEP_SAVE       → зберігаємо в EEPROM (Flash)
CALIB_STEP_VERIFY     → показуємо результат на LCD
CALIB_STEP_DONE       → вихід в меню
```

**Функції:**
```c
void calib_init(void);
void calib_update(void);          // викликати з main loop коли активна
void calib_start(void);           // вхід в режим калібровки
void calib_confirm(void);         // підтвердження поточного кроку (кнопка енкодера)
void calib_adjust(int8_t delta);  // зміна маси енкодером
bool calib_is_active(void);
CalibStep calib_get_step(void);
```

---

## 10. Захисна логіка (Safety)

```
Пріоритет 1 (ISR EXTI0):
  → Кнопка СТОП → motor_emergency_stop() → g_emergency_stop = true

Пріоритет 2 (ISR TIM7, кожну 1ms):
  → Перевірка концевиків під час руху
  → Якщо LIMIT_TOP і рух вгору → зупинка
  → Якщо LIMIT_BOT і рух вниз → зупинка

Пріоритет 3 (main loop):
  → Якщо force >= target * OVERLOAD_FACTOR → motor_emergency_stop()
  → Якщо force >= target * SLOWDOWN_THRESHOLD → знизити швидкість
  → Якщо force >= target → motor_stop()
  → Якщо HX711 не відповідає 500мс → зупинка + помилка на LCD
```

---

## 11. Порядок реалізації (по етапах)

### ✅ Етап 0 — ЗРОБЛЕНО
- CubeMX конфігурація
- Генерація HAL коду
- Перевірка main.h пінів

### 🔲 Етап 1 — config.h
Створити `Core/Inc/config.h` з усіма константами розділу 5 і 7.

### 🔲 Етап 2 — motor.c / motor.h
Повна реалізація згідно розділу 9.
Тест: джойстик → рух → концевики зупиняють.

### 🔲 Етап 3 — loadcell.c / loadcell.h
Бітбанг HX711. Тест: Serial вивід зусилля в кг.

### 🔲 Етап 4 — input.c / input.h
Джойстик + енкодер + СТОП з антидребезгом.

### 🔲 Етап 5 — display.c / display.h
LCD2004 I2C. Головний екран + меню.

### 🔲 Етап 6 — calibration.c / calibration.h
Майстер калібровки + збереження у Flash.

### 🔲 Етап 7 — Інтеграція
Зв'язати всі модулі в main.c. Повне тестування.

### 🔲 Етап 8 — Safety layer
Перевірка всіх аварійних ситуацій.

---

## 12. Правила написання коду

1. **Тільки HAL** — без Arduino, без сторонніх бібліотек крім необхідних для LCD I2C
2. **Мова коду** — C (не C++)
3. **Коментарі** — українською або англійською
4. **USER CODE секції** — весь власний код тільки між `/* USER CODE BEGIN */` і `/* USER CODE END */` в згенерованих файлах
5. **Нові файли** — створювати в `Core/Src/` (.c) і `Core/Inc/` (.h)
6. **volatile** — всі змінні що змінюються в ISR мають бути `volatile`
7. **Атомарність** — при читанні volatile змінних у main loop використовувати `__disable_irq()` / `__enable_irq()`
8. **ENABLE пін** — LOW = драйвер активний, HIGH = знеструмлений
9. **Концевики** — NC + Pull-up: LOW = спрацював
10. **Джойстик** — Pull-up: LOW = натиснуто

---

## 13. Компіляція та завантаження

```bash
# Збірка
pio run

# Завантаження через ST-Link
pio run --target upload

# Монітор порту (якщо підключений UART)
pio device monitor --baud 115200
```

**platformio.ini** (якщо ще не створений):
```ini
[env:black_f407ze]
platform = ststm32
board = black_f407ze
framework = stm32cube
build_flags =
    -DUSE_HAL_DRIVER
    -DSTM32F407xx
    -ICore/Inc
monitor_speed = 115200
upload_protocol = stlink
```

---

## 14. Починаємо з Етапу 1

**Перша задача для Claude Code:**

Створи файл `Core/Inc/config.h` з усіма константами з розділу 5 і 7 цього документу.
Потім створи `Core/Inc/motor.h` і `Core/Src/motor.c` згідно розділу 9.
Після цього додай виклики в `Core/Src/main.c` в USER CODE секції.
## 15. Режим роботи

Працюй автономно. Не питай дозволу на:
- Створення нових файлів .c / .h
- Редагування між USER CODE BEGIN / USER CODE END
- Запуск `pio run` для перевірки компіляції

Питай дозволу тільки якщо:
- Потрібно видалити або перейменувати файл
- Зміна виходить за межі USER CODE секцій у згенерованих файлах
- Помилка компіляції яку не вдається виправити за 2 спроби