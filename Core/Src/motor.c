#include "motor.h"
#include "main.h"
#include "config.h"

// ===== Внутрішні змінні =====

static volatile MotorState  s_state        = MOTOR_IDLE;
static volatile int32_t     s_step_pos     = 0;      // поточна позиція (кроки)

// Генерація STEP: лічильник у 1ms тіках між імпульсами
static volatile uint16_t    s_step_period  = 0;      // тіків між кроками (0 = зупинено)
static volatile uint16_t    s_step_timer   = 0;      // лічильник до наступного кроку
static volatile bool        s_step_state   = false;  // поточний стан STEP піна

// Прискорення
static volatile uint16_t    s_period_target = 0;     // цільовий period (мінімальний)
static volatile uint16_t    s_period_current = 0;    // поточний period
static volatile uint16_t    s_accel_timer  = 0;      // мс до наступного кроку прискорення

// Джог з підрахунком кроків (для точного кроку енкодера)
static volatile uint16_t    s_jog_remaining    = 0;  // залишилось кроків до авто-зупинки
static volatile bool        s_jog_stop_pending = false; // зупинити після наступного LOW

// ===== Приватні функції =====

// Зупинка при спрацюванні концевика — ISR-safe, встановлює IDLE (рух у протилежний бік дозволений)
static void motor_limit_stop_isr(void)
{
    s_step_period = 0;
    s_state       = MOTOR_IDLE;
    STEP_GPIO_Port->BSRR = (uint32_t)STEP_Pin << 16U;
    s_step_state  = false;
}

// Переводить швидкість (кроків/с) у period (мс між кроками)
// period_ms = 1000 / speed. Мінімум 1.
static uint16_t speed_to_period(uint16_t speed_steps_per_sec)
{
    if (speed_steps_per_sec == 0) return 0;
    uint32_t p = 1000U / speed_steps_per_sec;
    return (p < 1) ? 1 : (uint16_t)p;
}

// Встановити напрямок
static void set_direction(bool up)
{
    if (up) {
        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
    }
}

// ===== Публічні функції =====

void motor_init(void)
{
    // Активувати драйвер (ENABLE = LOW)
    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);

    s_state          = MOTOR_IDLE;
    s_step_pos       = 0;
    s_step_period    = 0;
    s_step_timer     = 0;
    s_step_state     = false;
    s_period_target  = 0;
    s_period_current = 0;
}

void motor_clear_error(void)
{
    __disable_irq();
    if (s_state == MOTOR_ERROR) {
        s_state = MOTOR_IDLE;
    }
    __enable_irq();
}

void motor_move_up(uint16_t speed)
{
    if (s_state == MOTOR_ERROR) return;
    // Кінцевик перевіряється в TIM7 ISR кожні 100 мкс — не блокуємо тут
    set_direction(true);
    uint16_t target = speed_to_period(speed);

    __disable_irq();
    s_period_target = target;
    if (s_state != MOTOR_MOVING_UP) {
        // Старт з нуля: починаємо повільно і розганяємось
        uint16_t start_period = speed_to_period(50);
        if (start_period < target) start_period = target;
        s_period_current = start_period;
        s_step_period    = start_period;
        s_step_timer     = start_period;
        s_accel_timer    = 10;
    } else {
        // Вже рухаємось: миттєво застосовуємо нову швидкість
        s_period_current = target;
        s_step_period    = target;
    }
    s_state = MOTOR_MOVING_UP;
    __enable_irq();
}

void motor_move_down(uint16_t speed)
{
    if (s_state == MOTOR_ERROR) return;
    if (motor_is_limit_bot()) return;

    set_direction(false);
    uint16_t target = speed_to_period(speed);

    __disable_irq();
    s_period_target = target;
    if (s_state != MOTOR_MOVING_DOWN) {
        uint16_t start_period = speed_to_period(50);
        if (start_period < target) start_period = target;
        s_period_current = start_period;
        s_step_period    = start_period;
        s_step_timer     = start_period;
        s_accel_timer    = 10;
    } else {
        s_period_current = target;
        s_step_period    = target;
    }
    s_state = MOTOR_MOVING_DOWN;
    __enable_irq();
}

void motor_nudge_up(uint16_t speed)
{
    if (s_state == MOTOR_ERROR) return;
    set_direction(true);
    uint16_t period = speed_to_period(speed);

    __disable_irq();
    s_period_target  = period;
    s_period_current = period;
    s_step_period    = period;
    // Не скидаємо s_step_timer якщо вже рухаємось вгору — уникаємо стрибка
    if (s_state != MOTOR_MOVING_UP) s_step_timer = period;
    s_accel_timer    = 0;
    s_state          = MOTOR_MOVING_UP;
    __enable_irq();
}

void motor_nudge_down(uint16_t speed)
{
    if (s_state == MOTOR_ERROR) return;
    if (motor_is_limit_bot()) return;

    set_direction(false);
    uint16_t period = speed_to_period(speed);

    __disable_irq();
    s_period_target  = period;
    s_period_current = period;
    s_step_period    = period;
    if (s_state != MOTOR_MOVING_DOWN) s_step_timer = period;
    s_accel_timer    = 0;
    s_state          = MOTOR_MOVING_DOWN;
    __enable_irq();
}

void motor_stop(void)
{
    __disable_irq();
    s_step_period = 0;
    s_state       = MOTOR_IDLE;
    // Залишаємо STEP LOW
    HAL_GPIO_WritePin(STEP_GPIO_Port, STEP_Pin, GPIO_PIN_RESET);
    s_step_state = false;
    __enable_irq();
}

void motor_emergency_stop(void)
{
    // Викликається з ISR — без disable_irq, не блокує мотор у стані ERROR
    s_step_period = 0;
    s_state       = MOTOR_IDLE;
    STEP_GPIO_Port->BSRR = (uint32_t)STEP_Pin << 16U;  // STEP = LOW (атомарно)
    s_step_state = false;
}

// Викликати з TIM7 IRQ (кожну 1 мс)
void motor_tim_tick(void)
{
    if (s_step_period == 0) return;

    // Перевірка концевиків у ISR — зупиняє з IDLE (рух у протилежний бік дозволений)
    if (s_state == MOTOR_MOVING_UP && motor_is_limit_top()) {
        motor_limit_stop_isr();
        return;
    }
    if (s_state == MOTOR_MOVING_DOWN && motor_is_limit_bot()) {
        motor_limit_stop_isr();
        return;
    }

    // Прискорення: поступово зменшуємо period до target
    if (s_accel_timer > 0) {
        s_accel_timer--;
    } else {
        s_accel_timer = 5;
        if (s_period_current > s_period_target) {
            s_period_current--;
            s_step_period = s_period_current;
        }
    }

    // Генерація STEP
    if (s_step_timer > 0) {
        s_step_timer--;
    } else {
        s_step_timer = s_step_period;

        if (!s_step_state) {
            // Передній фронт — рахуємо крок
            STEP_GPIO_Port->BSRR = STEP_Pin;  // STEP = HIGH
            s_step_state = true;

            if (s_state == MOTOR_MOVING_UP) {
                s_step_pos++;
            } else if (s_state == MOTOR_MOVING_DOWN) {
                s_step_pos--;
            }

            // Джог: декрементуємо лічильник, зупинка заплановується після LOW
            if (s_jog_remaining > 0) {
                if (--s_jog_remaining == 0) {
                    s_jog_stop_pending = true;
                }
            }
        } else {
            // Задній фронт
            STEP_GPIO_Port->BSRR = (uint32_t)STEP_Pin << 16U;  // STEP = LOW
            s_step_state = false;

            // Зупинка після останнього кроку (LOW завершено)
            if (s_jog_stop_pending) {
                s_jog_stop_pending = false;
                s_step_period = 0;
                s_state       = MOTOR_IDLE;
            }
        }
    }
}

// Викликати з main loop: перевіряє переходи стану
void motor_update(void)
{
    // Якщо в стані ERROR — нічого не робимо, чекаємо команди зовні
    if (s_state == MOTOR_ERROR) return;

    // Перевірка концевиків для зупинки (додатковий захист у main loop)
    if (s_state == MOTOR_MOVING_UP && motor_is_limit_top()) {
        motor_stop();
    }
    if (s_state == MOTOR_MOVING_DOWN && motor_is_limit_bot()) {
        motor_stop();
    }
}

bool motor_is_limit_top(void)
{
    // NC контакт на GND: нормально LOW, спрацювання = NC розімкнувся = HIGH
    return HAL_GPIO_ReadPin(LIMIT_TOP_GPIO_Port, LIMIT_TOP_Pin) == GPIO_PIN_SET;
}

bool motor_is_limit_bot(void)
{
    return HAL_GPIO_ReadPin(LIMIT_BOT_GPIO_Port, LIMIT_BOT_Pin) == GPIO_PIN_SET;
}

bool motor_is_running(void)
{
    return (s_state == MOTOR_MOVING_UP || s_state == MOTOR_MOVING_DOWN);
}

float motor_get_position_mm(void)
{
    int32_t pos;
    __disable_irq();
    pos = s_step_pos;
    __enable_irq();
    return (float)pos / STEPS_PER_MM;
}

void motor_reset_position(void)
{
    __disable_irq();
    s_step_pos = 0;
    __enable_irq();
}

MotorState motor_get_state(void)
{
    return s_state;
}

// Рухатися рівно n_steps кроків, потім авто-зупинка (без розгону)
void motor_jog_steps(bool up, uint16_t n_steps, uint16_t speed)
{
    if (s_state == MOTOR_ERROR) return;
    if (n_steps == 0) return;
    if (!up && motor_is_limit_bot()) return;

    set_direction(up);
    uint16_t period = speed_to_period(speed);

    __disable_irq();
    s_jog_remaining    = n_steps;
    s_jog_stop_pending = false;
    s_period_target    = period;
    s_period_current   = period;
    s_step_period      = period;
    s_step_timer       = period;
    s_accel_timer      = 0;
    s_state = up ? MOTOR_MOVING_UP : MOTOR_MOVING_DOWN;
    __enable_irq();
}
