/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "config.h"
#include "motor.h"
#include "loadcell.h"
#include "input.h"
#include "display.h"
#include "calibration.h"
#include "preset.h"
#include "speedpot.h"
#include "torque_angle.h"
#include "buzzer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  AUTO_STATE_IDLE = 0,
  AUTO_STATE_APPROACH,
  AUTO_STATE_SOFT,
  AUTO_STATE_SEEK,
  AUTO_STATE_SETTLE,
  AUTO_STATE_HOLD,
  AUTO_STATE_DONE,
  AUTO_STATE_ERROR
} AutoState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static float    s_target_kg    = FORCE_DEFAULT_KG;
static int8_t   s_preset_idx   = -1;     // активний пресет (-1 = не вибрано)
static float    s_manual_angle_target = ANGLE_DEFAULT_DEG;
static uint8_t  s_preset_edit_idx = 0;
static uint8_t  s_preset_edit_field = 0;
static bool     s_preset_edit_joy_up_last = false;
static float    s_settings_angle_step_deg = 10.0f;
static bool     s_settings_step_joy_up_last = false;
static bool     s_angle_beep_armed = false;
static bool     s_angle_beep_fired = false;
static float    s_angle_beep_target_deg = 0.0f;
static uint32_t s_display_tick = 0;
static bool     s_fine_used    = false;  // ENC hold використовувався для руху мотора
static uint32_t s_fine_tick    = 0;      // час останнього тіку енкодера у fine mode
static AutoState s_auto_state  = AUTO_STATE_IDLE;
static uint32_t  s_auto_deadline = 0;
static uint16_t  s_auto_cmd_speed = 0;
static uint16_t  s_pot_raw_filtered = 2048U;
static uint32_t  s_zero_btn_tick = 0;
static bool      s_zero_btn_last = false;
static bool      s_ignore_next_enc_release = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static float absf_local(float x)
{
  return (x < 0.0f) ? -x : x;
}

static uint16_t clamp_u16_local(uint16_t value, uint16_t min_value, uint16_t max_value)
{
  if (value < min_value) return min_value;
  if (value > max_value) return max_value;
  return value;
}

static uint16_t pot_read_filtered_raw(void)
{
  uint16_t raw = speedpot_get_raw();
  s_pot_raw_filtered = (uint16_t)(((uint32_t)s_pot_raw_filtered * 3U + raw + 2U) / 4U);
  return s_pot_raw_filtered;
}

static uint16_t pot_map_speed(uint16_t raw, uint16_t min_speed, uint16_t max_speed)
{
  uint32_t span;
  uint32_t scaled;

  if (max_speed <= min_speed) return min_speed;

  raw = clamp_u16_local(raw, POT_ADC_MIN_ACTIVE, POT_ADC_MAX_ACTIVE);
  span = (uint32_t)(POT_ADC_MAX_ACTIVE - POT_ADC_MIN_ACTIVE);
  scaled = (uint32_t)(raw - POT_ADC_MIN_ACTIVE) * (uint32_t)(max_speed - min_speed);

  return (uint16_t)(min_speed + (uint16_t)((scaled + (span / 2U)) / span));
}

static uint16_t manual_up_speed_from_pot(uint16_t pot_raw, float force)
{
  if (force < APPROACH_SOFT_KG) {
    return pot_map_speed(pot_raw, SPEED_PRESS, SPEED_APPROACH);
  }
  if (force < APPROACH_CONTACT_KG) {
    return pot_map_speed(pot_raw, SPEED_PRESS, SPEED_APPROACH_SOFT);
  }
  return SPEED_PRESS;
}

static uint16_t manual_down_speed_from_pot(uint16_t pot_raw)
{
  return pot_map_speed(pot_raw, POT_DOWN_MIN_SPEED, POT_DOWN_MAX_SPEED);
}

static void clear_active_preset(void)
{
  s_preset_idx = -1;
  s_angle_beep_armed = false;
  s_angle_beep_fired = false;
  buzzer_stop();
  torque_angle_set_target(s_manual_angle_target);
  display_set_active_preset(-1);
}

static float current_target_angle(void)
{
  if (s_preset_idx >= 0 && s_preset_idx < PRESET_COUNT) {
    return preset_get_angle_deg((uint8_t)s_preset_idx);
  }
  return s_manual_angle_target;
}

static void apply_target_angle(float angle_deg)
{
  if (s_preset_idx >= 0 && s_preset_idx < PRESET_COUNT) {
    preset_set_angle_deg((uint8_t)s_preset_idx, angle_deg);
    torque_angle_set_target(preset_get_angle_deg((uint8_t)s_preset_idx));
  } else {
    s_manual_angle_target = angle_deg;
    torque_angle_set_target(s_manual_angle_target);
  }
}

static bool ui_back_requested(bool joy_down)
{
  if (joy_down) {
    return true;
  }

  if (input_enc_sw_released()) {
    if (s_ignore_next_enc_release) {
      s_ignore_next_enc_release = false;
      return false;
    }
    return true;
  }

  return false;
}

static bool auto_is_active(void)
{
  return (s_auto_state == AUTO_STATE_APPROACH ||
          s_auto_state == AUTO_STATE_SOFT ||
          s_auto_state == AUTO_STATE_SEEK ||
          s_auto_state == AUTO_STATE_SETTLE ||
          s_auto_state == AUTO_STATE_HOLD);
}

static bool auto_is_visible(void)
{
  return auto_is_active() ||
         s_auto_state == AUTO_STATE_DONE ||
         s_auto_state == AUTO_STATE_ERROR;
}

static void auto_reset(void)
{
  s_auto_state = AUTO_STATE_IDLE;
  s_auto_deadline = 0;
  s_auto_cmd_speed = 0;
  display_set_screen(SCREEN_MAIN);
}

static void auto_start_cycle(void)
{
  motor_stop();
  s_fine_used = false;
  s_auto_state = AUTO_STATE_APPROACH;
  s_auto_deadline = 0;
  s_auto_cmd_speed = 0;
  display_set_screen(SCREEN_AUTO);
}

static uint16_t auto_select_burst_steps(float err_kg)
{
  if (err_kg > AUTO_BURST_LARGE_ERR_KG) return AUTO_BURST_LARGE_STEPS;
  if (err_kg > AUTO_BURST_MED_ERR_KG)   return AUTO_BURST_MED_STEPS;
  return AUTO_BURST_SMALL_STEPS;
}

static void auto_update(float force, DisplayMotionMode *motion_mode, uint16_t *motion_speed)
{
  float err_kg = s_target_kg - force;

  switch (s_auto_state) {
    case AUTO_STATE_APPROACH:
      *motion_mode = DISPLAY_MODE_APPROACH;
      *motion_speed = SPEED_APPROACH;
      s_auto_cmd_speed = SPEED_APPROACH;

      if (err_kg <= AUTO_CRUISE_ENTRY_KG || force >= APPROACH_SOFT_KG) {
        motor_stop();
        s_auto_state = AUTO_STATE_SOFT;
      } else {
        motor_move_up(SPEED_APPROACH);
      }
      break;

    case AUTO_STATE_SOFT:
      *motion_mode = DISPLAY_MODE_SOFT;
      *motion_speed = SPEED_APPROACH_SOFT;
      s_auto_cmd_speed = SPEED_APPROACH_SOFT;

      if (err_kg <= AUTO_CRUISE_ENTRY_KG || force >= APPROACH_CONTACT_KG) {
        motor_stop();
        s_auto_state = AUTO_STATE_SEEK;
      } else {
        motor_move_up(SPEED_APPROACH_SOFT);
      }
      break;

    case AUTO_STATE_SEEK:
      *motion_mode = DISPLAY_MODE_ASEEK;
      *motion_speed = s_auto_cmd_speed;

      if (force > (s_target_kg + AUTO_FORCE_TOLERANCE_KG)) {
        motor_stop();
        s_auto_state = AUTO_STATE_ERROR;
        display_show_error("AUTO overshoot");
        break;
      }

      if (absf_local(err_kg) <= AUTO_FORCE_TOLERANCE_KG) {
        motor_stop();
        s_auto_state = AUTO_STATE_HOLD;
        s_auto_deadline = HAL_GetTick() + AUTO_HOLD_MS;
        *motion_mode = DISPLAY_MODE_HOLD;
        *motion_speed = 0;
        break;
      }

      if (err_kg > AUTO_CRUISE_ENTRY_KG) {
        s_auto_cmd_speed = SPEED_PRESS;
        *motion_speed = s_auto_cmd_speed;
        motor_move_up(SPEED_PRESS);
      } else {
        uint16_t burst_steps = auto_select_burst_steps(err_kg);
        s_auto_cmd_speed = SPEED_PRESS;
        *motion_speed = s_auto_cmd_speed;
        motor_burst_up(SPEED_PRESS, burst_steps);
        s_auto_state = AUTO_STATE_SETTLE;
        s_auto_deadline = 0;
      }
      break;

    case AUTO_STATE_SETTLE:
      *motion_mode = DISPLAY_MODE_ASEEK;
      *motion_speed = 0;

      if (motor_is_running() || motor_is_burst_active()) {
        *motion_speed = s_auto_cmd_speed;
        break;
      }

      if (s_auto_deadline == 0U) {
        s_auto_deadline = HAL_GetTick() + AUTO_SETTLE_MS;
      } else if (HAL_GetTick() >= s_auto_deadline) {
        s_auto_deadline = 0;
        s_auto_state = AUTO_STATE_SEEK;
      }
      break;

    case AUTO_STATE_HOLD:
      *motion_mode = DISPLAY_MODE_HOLD;
      *motion_speed = 0;

      if (force > (s_target_kg + AUTO_FORCE_TOLERANCE_KG)) {
        s_auto_state = AUTO_STATE_ERROR;
        display_show_error("AUTO overshoot");
      } else if ((s_target_kg - force) > AUTO_FORCE_TOLERANCE_KG) {
        s_auto_state = AUTO_STATE_SEEK;
      } else if (HAL_GetTick() >= s_auto_deadline) {
        s_auto_state = AUTO_STATE_DONE;
        s_auto_deadline = HAL_GetTick() + AUTO_DONE_MS;
      }
      break;

    case AUTO_STATE_DONE:
      *motion_mode = DISPLAY_MODE_DONE;
      *motion_speed = 0;
      if (HAL_GetTick() >= s_auto_deadline) {
        auto_reset();
      }
      break;

    case AUTO_STATE_ERROR:
      *motion_mode = DISPLAY_MODE_IDLE;
      *motion_speed = 0;
      break;

    case AUTO_STATE_IDLE:
    default:
      *motion_mode = DISPLAY_MODE_IDLE;
      *motion_speed = 0;
      break;
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  // I2C bus recovery: 10 SCL pulses to unlock PCF8574 after crash/reset
  HAL_I2C_DeInit(&hi2c1);
  {
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_6;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);
    for (int i = 0; i < 10; i++) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET); HAL_Delay(1);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);   HAL_Delay(1);
    }
  }
  MX_I2C1_Init();
  MX_GPIO_ZERO_BTN_Init();
  MX_GPIO_BUZZER_Init();

  preset_init();
  motor_init();
  speedpot_init();
  torque_angle_init();
  buzzer_init();
  loadcell_init();
  input_init();
  calib_init();

  // Завантажити збережену калібровку з Flash
  {
    float   saved_scale;
    int32_t saved_offset;
    float   saved_target;
    float   saved_angle;
    float   saved_preset_targets[PRESET_COUNT];
    float   saved_preset_angles[PRESET_COUNT];
    preset_export_targets(saved_preset_targets, PRESET_COUNT);
    preset_export_angles(saved_preset_angles, PRESET_COUNT);
    if (flash_load(&saved_scale, &saved_offset, &saved_target, &saved_angle,
                   saved_preset_targets, saved_preset_angles, PRESET_COUNT)) {
      loadcell_set_scale(saved_scale);
      loadcell_set_offset(saved_offset);
      if (saved_target > 0.0f && saved_target <= FORCE_MAX_KG) {
        s_target_kg = saved_target;
      }
      if (saved_angle >= 0.0f && saved_angle <= ANGLE_MAX_DEG) {
        s_manual_angle_target = saved_angle;
      }
      preset_import_targets(saved_preset_targets, PRESET_COUNT);
      preset_import_angles(saved_preset_angles, PRESET_COUNT);
    }
  }

  torque_angle_set_target(current_target_angle());
  display_init();
  display_set_screen(SCREEN_MAIN);
  display_set_force(0.0f, s_target_kg / KN_TO_KG);
  display_set_angle(0.0f, current_target_angle(), false);

  HAL_TIM_Base_Start_IT(&htim3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    loadcell_update();
    input_update();
    motor_update();
    buzzer_update();

    float force = loadcell_get_kg();
    float force_fast = loadcell_get_fast_kg();
    uint16_t pot_raw = pot_read_filtered_raw();
    int8_t enc_delta = input_enc_get_delta();
    DisplayScreen cur_screen = display_get_screen();
    DisplayMotionMode motion_mode = DISPLAY_MODE_IDLE;
    uint16_t motion_speed = 0;
    bool joy_up = input_joy_up();
    bool joy_down = input_joy_down();

    // --- Кнопка СТОП ---
    if (input_stop_pressed()) {
      input_stop_clear();
      auto_reset();
      s_angle_beep_armed = false;
      s_angle_beep_fired = false;
      buzzer_stop();
      if (cur_screen == SCREEN_ERROR) {
        motor_clear_error();
        display_set_screen(SCREEN_MAIN);
        cur_screen = SCREEN_MAIN;
      } else {
        s_fine_used = false;  // скидаємо fine mode при аварійній зупинці
        display_show_error("STOP: press again");
        cur_screen = SCREEN_ERROR;
      }
    }

    // --- HX711 timeout → аварія ---
    if (loadcell_has_error() && motor_is_running()) {
      motor_stop();
      auto_reset();
      display_show_error("HX711 no response");
    }

    // --- Кнопка ZERO кутового датчика ---
    // 1-е натискання (з режиму спокою): обнулити кут + озброїти сигналізацію на цільовий кут.
    // 2-е натискання (поки озброєно/сигналізує): лише вимкнути бузер, БЕЗ переобнулення кута.
    {
      bool zero_now = (HAL_GPIO_ReadPin(ZERO_BTN_GPIO_Port, ZERO_BTN_Pin) == GPIO_PIN_RESET);
      if (zero_now && !s_zero_btn_last) {
        if ((HAL_GetTick() - s_zero_btn_tick) >= DEBOUNCE_MS) {
          if (s_angle_beep_armed || s_angle_beep_fired) {
            s_angle_beep_armed = false;
            s_angle_beep_fired = false;
            buzzer_stop();
          } else {
            torque_angle_zero();
            s_angle_beep_armed = true;
            s_angle_beep_fired = false;
            s_angle_beep_target_deg = current_target_angle();
          }
        }
        s_zero_btn_tick = HAL_GetTick();
      }
      s_zero_btn_last = zero_now;
    }

    if (s_angle_beep_armed && !s_angle_beep_fired) {
      if (torque_angle_get_deg() >= s_angle_beep_target_deg) {
        buzzer_on();
        s_angle_beep_fired = true;
      }
    }

    // --- Логіка по екранах ---
    if (cur_screen == SCREEN_MAIN || cur_screen == SCREEN_AUTO) {

      // Безпека: захист по зусиллю
      if (motor_is_running()) {
        if (force >= s_target_kg * OVERLOAD_FACTOR) {
          motor_emergency_stop();
          auto_reset();
          display_show_error("OVERLOAD!");
        } else if (force >= s_target_kg) {
          motor_stop();
        }
      }

      if (cur_screen == SCREEN_AUTO && joy_down) {
        motor_stop();
        auto_reset();
      } else if (cur_screen == SCREEN_AUTO && joy_up && !auto_is_visible()) {
        auto_start_cycle();
      } else if (auto_is_active() || s_auto_state == AUTO_STATE_DONE) {
        auto_update(force_fast, &motion_mode, &motion_speed);
      } else {
        // Джойстик керує двигуном (пріоритет над ENC fine mode)
        bool enc_held = input_enc_sw_held();

        if (cur_screen == SCREEN_MAIN && joy_up && !motor_is_limit_top()) {
          uint16_t spd = manual_up_speed_from_pot(pot_raw, force);
          if (force < APPROACH_SOFT_KG) {
            motion_mode = DISPLAY_MODE_APPROACH;
          } else if (force < APPROACH_CONTACT_KG) {
            motion_mode = DISPLAY_MODE_SOFT;
          } else {
            motion_mode = DISPLAY_MODE_PRESS;
          }
          motion_speed = spd;
          motor_move_up(spd);
        } else if (cur_screen == SCREEN_MAIN && joy_down && !motor_is_limit_bot()) {
          motion_mode = DISPLAY_MODE_RETRACT;
          motion_speed = manual_down_speed_from_pot(pot_raw);
          motor_move_down(motion_speed);
        } else if (!joy_up && !joy_down && !enc_held && motor_is_running()) {
          motor_stop();
        }

        // Тонка підстройка: ENC утримано + обертання → рух мотора без розгону
        if (cur_screen == SCREEN_MAIN && enc_held && !joy_up && !joy_down) {
          if (enc_delta > 0 && !motor_is_limit_top()) {
            motion_mode = DISPLAY_MODE_FINE;
            motion_speed = SPEED_ENC;
            motor_nudge_up(SPEED_ENC);
            s_fine_tick = HAL_GetTick();
            s_fine_used = true;
          } else if (enc_delta < 0 && !motor_is_limit_bot()) {
            motion_mode = DISPLAY_MODE_FINE;
            motion_speed = SPEED_ENC;
            motor_nudge_down(SPEED_ENC);
            s_fine_tick = HAL_GetTick();
            s_fine_used = true;
          } else if (s_fine_used && (HAL_GetTick() - s_fine_tick) >= FINE_TIMEOUT_MS) {
            motor_stop();
          }
        }

        // ENC відпущено: якщо fine mode не використовувався → меню
        if (input_enc_sw_released()) {
          if (s_fine_used) {
            motor_stop();
          } else {
            // Споживаємо pressed_flag щоб він не спрацював у SCREEN_MENU
            // одразу в наступній ітерації (stale flag від поточного натискання)
            (void)input_enc_sw_pressed();
            display_set_screen(SCREEN_MENU);
          }
          s_fine_used = false;
        }

        // Енкодер змінює задане зусилля тільки якщо ENC не утримано
        if (!enc_held && enc_delta != 0) {
          s_target_kg += (float)enc_delta * FORCE_STEP_KG;
          if (s_target_kg < FORCE_STEP_KG)  s_target_kg = FORCE_STEP_KG;
          if (s_target_kg > FORCE_MAX_KG)   s_target_kg = FORCE_MAX_KG;
        }
      }

      // Оновити дисплей ~10 Гц, передаємо значення в кН
      if ((HAL_GetTick() - s_display_tick) >= 100U) {
        s_display_tick = HAL_GetTick();
        display_set_force(loadcell_get_kN(), s_target_kg / KN_TO_KG);
        display_set_angle(torque_angle_get_deg(),
                          torque_angle_get_target(),
                          torque_angle_is_reached());
      }

      display_set_motion_mode(motion_mode);
      display_set_motion_speed(motion_speed);
      display_set_auto_metrics(loadcell_get_fast_kN(),
                               s_target_kg / KN_TO_KG,
                               (loadcell_get_fast_kg() - s_target_kg) / KN_TO_KG);

    } else if (cur_screen == SCREEN_MENU) {

      if (enc_delta > 0)       display_menu_next();
      else if (enc_delta < 0)  display_menu_prev();

      if (ui_back_requested(joy_down)) {
        display_set_screen(SCREEN_MAIN);  // уніфікований вихід на головний екран
      } else if (input_enc_sw_pressed()) {
        uint8_t item = display_menu_get_item();
        switch (item) {
          case 0: auto_start_cycle(); break;
          case 1:
            s_ignore_next_enc_release = true;
            display_set_screen(SCREEN_PRESET);
            break;
          case 2:
            s_ignore_next_enc_release = true;
            display_set_screen(SCREEN_PRESET_EDIT_LIST);
            break;
          case 3: calib_start(s_target_kg); break;
          case 4:
            s_ignore_next_enc_release = true;
            s_settings_step_joy_up_last = false;
            display_settings_set_angle(current_target_angle());
            display_settings_set_step(s_settings_angle_step_deg);
            display_set_screen(SCREEN_SETTINGS);
            break;
          case 5:
            s_ignore_next_enc_release = true;
            clear_active_preset();
            motor_reset_position();
            display_set_screen(SCREEN_MAIN);
            break;
          default: break;
        }
      }

    } else if (cur_screen == SCREEN_PRESET) {

      if (ui_back_requested(joy_down)) {
        display_set_screen(SCREEN_MAIN);  // уніфікований вихід на головний екран
      } else {
        if (enc_delta != 0) display_preset_scroll(enc_delta);
        if (input_enc_sw_pressed()) {
          uint8_t idx = display_preset_get_item();
          s_target_kg  = preset_target_kg(idx);
          s_preset_idx = (int8_t)idx;
          torque_angle_set_target(preset_get_angle_deg(idx));
          display_set_active_preset(s_preset_idx);
          s_ignore_next_enc_release = true;
          display_set_screen(SCREEN_MAIN);
        }
      }

    } else if (cur_screen == SCREEN_PRESET_EDIT_LIST) {

      if (ui_back_requested(joy_down)) {
        display_set_screen(SCREEN_MENU);
      } else {
        if (enc_delta != 0) display_preset_scroll(enc_delta);
        if (input_enc_sw_pressed()) {
          uint8_t idx = display_preset_get_item();
          s_preset_edit_idx = idx;
          s_preset_edit_field = 0;
          s_preset_edit_joy_up_last = false;
          display_preset_edit_set(idx, preset_get_target_kN(idx), preset_get_angle_deg(idx), s_preset_edit_field);
          s_ignore_next_enc_release = true;
          display_set_screen(SCREEN_PRESET_EDIT);
        }
      }

    } else if (cur_screen == SCREEN_PRESET_EDIT) {

      if (ui_back_requested(joy_down)) {
        display_set_screen(SCREEN_PRESET_EDIT_LIST);
      } else {
        bool joy_up_edge = joy_up && !s_preset_edit_joy_up_last;
        s_preset_edit_joy_up_last = joy_up;

        if (joy_up_edge) {
          s_preset_edit_field = (s_preset_edit_field == 0) ? 1 : 0;
        }

        if (enc_delta != 0) {
          if (s_preset_edit_field == 0) {
            float new_target = preset_get_target_kN(s_preset_edit_idx) + (float)enc_delta * FORCE_STEP_KN;
            preset_set_target_kN(s_preset_edit_idx, new_target);
            if (s_preset_idx == (int8_t)s_preset_edit_idx) {
              s_target_kg = preset_target_kg(s_preset_edit_idx);
            }
          } else {
            float new_angle = preset_get_angle_deg(s_preset_edit_idx) + (float)enc_delta * ANGLE_STEP_DEG;
            preset_set_angle_deg(s_preset_edit_idx, new_angle);
            if (s_preset_idx == (int8_t)s_preset_edit_idx) {
              torque_angle_set_target(preset_get_angle_deg(s_preset_edit_idx));
            }
          }
        }

        display_preset_edit_set(s_preset_edit_idx,
                               preset_get_target_kN(s_preset_edit_idx),
                               preset_get_angle_deg(s_preset_edit_idx),
                               s_preset_edit_field);

        if (input_enc_sw_pressed()) {
          float preset_targets[PRESET_COUNT] = {0};
          float preset_angles[PRESET_COUNT] = {0};
          preset_export_targets(preset_targets, PRESET_COUNT);
          preset_export_angles(preset_angles, PRESET_COUNT);
          flash_save(loadcell_get_scale(), loadcell_get_offset(),
                     s_target_kg, s_manual_angle_target,
                     preset_targets, preset_angles, PRESET_COUNT);
          display_set_screen(SCREEN_PRESET_EDIT_LIST);
        }
      }

    } else if (cur_screen == SCREEN_CALIBRATION) {

      if (calib_is_active()) {
        calib_adjust(enc_delta);
        if (input_enc_sw_pressed()) calib_confirm();
        calib_update();
      } else {
        display_set_screen(SCREEN_MAIN);
      }

    } else if (cur_screen == SCREEN_SETTINGS) {

      bool settings_joy_up_edge = joy_up && !s_settings_step_joy_up_last;
      s_settings_step_joy_up_last = joy_up;

      if (settings_joy_up_edge) {
        s_settings_angle_step_deg = (s_settings_angle_step_deg >= 10.0f) ? 1.0f : 10.0f;
        display_settings_set_step(s_settings_angle_step_deg);
      }

      if (enc_delta != 0) {
        float new_angle = current_target_angle() + (float)enc_delta * s_settings_angle_step_deg;
        apply_target_angle(new_angle);
        display_settings_set_angle(current_target_angle());
      }

      if (input_enc_sw_pressed()) {
        float preset_targets[PRESET_COUNT] = {0};
        float preset_angles[PRESET_COUNT] = {0};
        preset_export_targets(preset_targets, PRESET_COUNT);
        preset_export_angles(preset_angles, PRESET_COUNT);
        flash_save(loadcell_get_scale(), loadcell_get_offset(),
                   s_target_kg, s_manual_angle_target,
                   preset_targets, preset_angles, PRESET_COUNT);
        display_set_screen(SCREEN_MENU);
      } else if (ui_back_requested(joy_down)) {
        display_set_screen(SCREEN_MAIN);  // уніфікований вихід на головний екран
      }
    }

    display_update();
    /* USER CODE END 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  * HSE (зовнішній кварц 25MHz) через PLL -> 100MHz, штатна частота
  * плати Black Pill F411CE (f_cpu=100MHz). VCO_in=25/25=1MHz (в межах
  * дозволеного діапазону ST 1-2MHz), VCO_out=1*200=200MHz, SYSCLK=200/2=100MHz.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  * APB1 max 50MHz -> DIV2 (50MHz), APB2 max 100MHz -> DIV1 (100MHz).
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == STOP_BTN_Pin) {
        motor_emergency_stop();
        input_stop_set();
    }
    /* ENC_CLK handled by polling in input_debounce_tick (TIM3 IRQ) */
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called when a timer period elapses.
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM5)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM3)
  {
    motor_tim_tick();
    input_debounce_tick();
    buzzer_tim_tick();
  }
  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
