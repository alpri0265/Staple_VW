#include "input.h"
#include "main.h"
#include "config.h"

// ===== Антидребезг =====

typedef struct {
    uint8_t  counter;    // лічильник мс утримання
    bool     state;      // поточний підтверджений стан (true = натиснуто)
    bool     prev;       // попередній підтверджений стан
} DebounceBtn_t;

static DebounceBtn_t s_joy_up;
static DebounceBtn_t s_joy_down;
static DebounceBtn_t s_enc_sw;

// Енкодер
static volatile int8_t s_enc_delta  = 0;   // накопичені кроки
static volatile bool   s_stop_flag  = false;
static bool            s_clk_prev   = true; // попередній стан CLK (HIGH = спокій)

// ===== Приватні функції =====

static void debounce_tick(DebounceBtn_t *btn, bool raw_pressed)
{
    if (raw_pressed) {
        if (btn->counter < DEBOUNCE_MS) {
            btn->counter++;
        }
    } else {
        btn->counter = 0;
    }
    btn->prev  = btn->state;
    btn->state = (btn->counter >= DEBOUNCE_MS);
}

// ===== Публічні функції =====

void input_init(void)
{
    s_joy_up  = (DebounceBtn_t){0};
    s_joy_down = (DebounceBtn_t){0};
    s_enc_sw  = (DebounceBtn_t){0};
    s_enc_delta = 0;
    s_stop_flag = false;
}

void input_debounce_tick(void)
{
    // LOW = натиснуто (pull-up + active-low)
    debounce_tick(&s_joy_up,   HAL_GPIO_ReadPin(JOY_UP_GPIO_Port,   JOY_UP_Pin)   == GPIO_PIN_RESET);
    debounce_tick(&s_joy_down, HAL_GPIO_ReadPin(JOY_DOWN_GPIO_Port, JOY_DOWN_Pin) == GPIO_PIN_RESET);
    debounce_tick(&s_enc_sw,   HAL_GPIO_ReadPin(ENC_SW_GPIO_Port,   ENC_SW_Pin)   == GPIO_PIN_RESET);
}

// Викликається з EXTI3 ISR на falling edge ENC_CLK
void input_enc_isr(void)
{
    // KY-040: якщо DT=HIGH при CLK=FALLING → за годинниковою стрілкою (+1)
    //         якщо DT=LOW  при CLK=FALLING → проти годинникової стрілки (-1)
    if (HAL_GPIO_ReadPin(ENC_DT_GPIO_Port, ENC_DT_Pin) == GPIO_PIN_SET) {
        s_enc_delta++;
    } else {
        s_enc_delta--;
    }
}

void input_update(void)
{
    // Encoder polling: детектуємо falling edge CLK в main loop
    bool clk = (HAL_GPIO_ReadPin(ENC_CLK_GPIO_Port, ENC_CLK_Pin) == GPIO_PIN_SET);
    if (!clk && s_clk_prev) {
        if (HAL_GPIO_ReadPin(ENC_DT_GPIO_Port, ENC_DT_Pin) == GPIO_PIN_SET) {
            s_enc_delta++;
        } else {
            s_enc_delta--;
        }
    }
    s_clk_prev = clk;
}

bool input_joy_up(void)
{
    return s_joy_up.state;
}

bool input_joy_down(void)
{
    return s_joy_down.state;
}

bool input_enc_sw_pressed(void)
{
    return (s_enc_sw.state && !s_enc_sw.prev);
}

bool input_enc_sw_held(void)
{
    return s_enc_sw.state;
}

bool input_enc_sw_released(void)
{
    return (!s_enc_sw.state && s_enc_sw.prev);
}

int8_t input_enc_get_delta(void)
{
    int8_t delta;
    __disable_irq();
    delta = s_enc_delta;
    s_enc_delta = 0;
    __enable_irq();
    return delta;
}

bool input_boost_held(void)
{
    return HAL_GPIO_ReadPin(BOOST_BTN_GPIO_Port, BOOST_BTN_Pin) == GPIO_PIN_RESET;
}

bool input_stop_pressed(void)
{
    return s_stop_flag;
}

void input_stop_clear(void)
{
    s_stop_flag = false;
}

// Встановити прапор СТОП (може викликатися з ISR через motor_emergency_stop)
void input_stop_set(void)
{
    s_stop_flag = true;
}
