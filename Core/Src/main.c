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
#include "torque_angle.h"
#include "speedpot.h"
#include "gpio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    AUTO_IDLE = 0,   // пресет вибрано, очікуємо джойстик
    AUTO_PRESSING,   // рухаємось вниз до brake_point
    AUTO_HOLDING     // досягли цілі, ручне управління в [min, max]
} AutoState_t;
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
static int8_t   s_preset_idx   = -1;
static uint32_t s_display_tick = 0;
static bool     s_fine_used    = false;
static uint32_t s_fine_tick    = 0;

static uint32_t s_zero_btn_tick = 0;
static bool     s_zero_btn_last = false;

// Редактор пресету
static PresetEditStep_t s_pedit_step   = PEDIT_INJ;
static uint8_t          s_pedit_inj    = 0;
static uint8_t          s_pedit_op     = 0;
static float            s_pedit_fmin   = 8.5f;
static float            s_pedit_fmax   = 9.0f;
static int8_t           s_pedit_idx    = -1;   // -1 = новий, >=0 = редагування

// Довге натискання ENC у SCREEN_PRESET
static uint32_t s_preset_hold_tick  = 0;
static bool     s_preset_long_fired = false;

// Стан авторежиму (замість двох bool-прапорів)
static AutoState_t s_auto_state = AUTO_IDLE;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */

  // Відновлення I2C шини: 9 тактів SCL щоб розблокувати PCF8574 після зависання.
  // Дозволяє відновити роботу кнопкою RESET замість зняттям живлення.
  HAL_I2C_DeInit(&hi2c1);
  {
    GPIO_InitTypeDef i2c_gpio = {0};
    i2c_gpio.Pin   = GPIO_PIN_6;        // PB6 = I2C1_SCL
    i2c_gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    i2c_gpio.Pull  = GPIO_NOPULL;
    i2c_gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &i2c_gpio);
    for (int i = 0; i < 10; i++) {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
      HAL_Delay(1);
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
      HAL_Delay(1);
    }
  }
  MX_I2C1_Init();  // повторна ініціалізація I2C після відновлення шини

  motor_init();
  loadcell_init();
  input_init();
  calib_init();
  MX_GPIO_ZERO_BTN_Init();
  speedpot_init();
  torque_angle_init();

  // Завантажити збережену калібровку з Flash
  {
    float   saved_scale;
    int32_t saved_offset;
    float   saved_target;
    float   saved_angle;
    if (flash_load(&saved_scale, &saved_offset, &saved_target, &saved_angle)) {
      loadcell_set_scale(saved_scale);
      loadcell_set_offset(saved_offset);
      if (saved_target > 0.0f && saved_target <= FORCE_MAX_KG) {
        s_target_kg = saved_target;
      }
      if (saved_angle > 0.0f && saved_angle <= ANGLE_MAX_DEG) {
        torque_angle_set_target(saved_angle);
      }
    }
  }

  display_init();
  display_set_screen(SCREEN_MAIN);
  display_set_force(0.0f, s_target_kg / KN_TO_KG);
  display_set_angle(0.0f, torque_angle_get_target(), false);

  HAL_TIM_Base_Start_IT(&htim7);
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

    float force    = loadcell_get_kg();
    float force_kN = force / KN_TO_KG;
    int8_t enc_delta = input_enc_get_delta();
    DisplayScreen cur_screen = display_get_screen();

    // --- Кнопка СТОП ---
    if (input_stop_pressed()) {
      input_stop_clear();
      motor_stop();
      motor_clear_error();
      s_auto_state = AUTO_IDLE;
      s_fine_used  = false;
      if (cur_screen == SCREEN_PRESET_EDIT) {
        display_set_screen(SCREEN_PRESET);
        cur_screen = SCREEN_PRESET;
      } else {
        display_set_screen(SCREEN_MAIN);
        cur_screen = SCREEN_MAIN;
      }
    }

    // --- HX711 timeout → аварія ---
    if (loadcell_has_error() && motor_is_running()) {
      motor_stop();
      display_show_error("HX711 no response");
    }

    // --- Логіка по екранах ---
    if (cur_screen == SCREEN_MAIN) {

      bool joy_up   = input_joy_up();
      bool joy_down = input_joy_down();
      bool enc_held = input_enc_sw_held();

      // Потенціометр завжди задає швидкість подачі
      uint16_t pot_spd = speedpot_get_speed();

      bool preset_active = (s_preset_idx >= 0 &&
                            s_preset_idx < (int8_t)g_preset_count);

      if (preset_active) {
        // ===== АВТОРЕЖИМ =====
        float min_kN = g_presets[s_preset_idx].force_min_kN;
        float max_kN = g_presets[s_preset_idx].force_max_kN;
        float mid_kN = (min_kN + max_kN) * 0.5f;
        float brake_point = mid_kN - BRAKE_KN;
        if (brake_point < min_kN) brake_point = min_kN;

        switch (s_auto_state) {

          case AUTO_IDLE:
            // Очікуємо джойстик DOWN → починаємо пресування
            // Joy UP дозволений — знімаємо залишкове навантаження
            if (joy_down && !motor_is_limit_bot()) {
              s_auto_state = AUTO_PRESSING;
            }
            if (joy_up) {
              motor_move_up(pot_spd);
            } else if (!joy_up && motor_is_running()) {
              motor_stop();
            }
            display_set_status("AUTO:IDLE  [v]=start");
            break;

          case AUTO_PRESSING:
            // Три ступені швидкості для точної авто-зупинки
            {
              uint16_t spd;
              if (force_kN >= mid_kN * PRESET_ENC_THRESHOLD) {
                spd = SPEED_ENC;
              } else if (force_kN >= mid_kN * PRESET_SLOW_THRESHOLD) {
                spd = SPEED_SLOW;
              } else {
                spd = pot_spd;
              }

              if (joy_up) {
                // Оператор скасував — зупинити і повернутись в IDLE
                motor_stop();
                s_auto_state = AUTO_IDLE;
              } else if (joy_down && !motor_is_limit_bot()) {
                motor_move_down(spd);
              } else if (!joy_down && motor_is_running()) {
                // Відпустив джойстик до цілі — зупинити і повернутись в IDLE
                motor_stop();
                s_auto_state = AUTO_IDLE;
              }

              // Авто-зупинка: brake_point = mid - BRAKE_KN
              if (force_kN >= brake_point) {
                motor_stop();
                s_auto_state = AUTO_HOLDING;
              }
            }
            display_set_status("AUTO:PRESS [^]=cncl ");
            break;

          case AUTO_HOLDING:
            // Joy UP: знімаємо навантаження; коли відпустили → IDLE
            // Joy DOWN: дотиснути повільно (в межах [mid, max])
            // Encoder: точне підлаштування
            {
              MotorState ms = motor_get_state();
              if (joy_up) {
                motor_move_up(pot_spd);
              } else if (joy_down && !motor_is_limit_bot()) {
                motor_move_down(SPEED_ENC);
              } else if (!joy_up && !joy_down && motor_is_running()) {
                motor_stop();
                // Якщо рухались вгору (знімали навантаження) → IDLE
                if (ms == MOTOR_MOVING_UP) {
                  s_auto_state = AUTO_IDLE;
                }
              }

              if (!enc_held && enc_delta != 0) {
                uint16_t n = (uint16_t)(enc_delta > 0 ? enc_delta : -enc_delta) * ENC_JOG_STEPS;
                if (enc_delta > 0) {
                  motor_jog_steps(true, n, SPEED_ENC);  // вгору — менше зусилля
                } else {
                  motor_jog_steps(false, n, SPEED_ENC); // вниз — більше зусилля
                }
              }
            }
            display_set_status("AUTO:HOLD  ENC=fine ");
            break;
        }

        // ENC відпущено → меню (з будь-якого стану)
        if (input_enc_sw_released()) {
          motor_stop();
          s_auto_state = AUTO_IDLE;
          display_set_screen(SCREEN_MENU);
          s_fine_used = false;
        }

      } else {
        // ===== РУЧНИЙ РЕЖИМ (пресет не вибрано) =====
        display_set_status("MANUAL  [^v] ENC=tgt");

        if (motor_is_running() && force >= s_target_kg) {
          motor_stop();
        }

        // Уповільнення при наближенні до заданого зусилля
        uint16_t joy_speed = (force >= s_target_kg * SLOWDOWN_THRESHOLD)
                              ? SPEED_SLOW : pot_spd;

        if (joy_up) {
          motor_move_up(joy_speed);
        } else if (joy_down && !motor_is_limit_bot()) {
          motor_move_down(joy_speed);
        } else if (!joy_up && !joy_down && !enc_held && motor_is_running()) {
          motor_stop();
        }

        if (enc_held && !joy_up && !joy_down) {
          if (enc_delta > 0) {
            motor_nudge_up(SPEED_ENC);
            s_fine_tick = HAL_GetTick();
            s_fine_used = true;
          } else if (enc_delta < 0 && !motor_is_limit_bot()) {
            motor_nudge_down(SPEED_ENC);
            s_fine_tick = HAL_GetTick();
            s_fine_used = true;
          } else if (s_fine_used && (HAL_GetTick() - s_fine_tick) >= FINE_TIMEOUT_MS) {
            motor_stop();
          }
        }

        if (input_enc_sw_released()) {
          if (s_fine_used) { motor_stop(); }
          else             { display_set_screen(SCREEN_MENU); }
          s_fine_used = false;
        }

        // Енкодер змінює задане зусилля
        if (!enc_held && enc_delta != 0) {
          s_target_kg += (float)enc_delta * FORCE_STEP_KG;
          if (s_target_kg < FORCE_STEP_KG) s_target_kg = FORCE_STEP_KG;
          if (s_target_kg > FORCE_MAX_KG)  s_target_kg = FORCE_MAX_KG;
        }
      }

      // Кнопка ZERO (спільна для обох режимів)
      {
        bool zero_now = (HAL_GPIO_ReadPin(ZERO_BTN_GPIO_Port, ZERO_BTN_Pin) == GPIO_PIN_RESET);
        if (zero_now && !s_zero_btn_last) {
          if ((HAL_GetTick() - s_zero_btn_tick) >= DEBOUNCE_MS) {
            torque_angle_zero();
          }
          s_zero_btn_tick = HAL_GetTick();
        }
        s_zero_btn_last = zero_now;
      }

      // Оновити дисплей ~10 Гц
      if ((HAL_GetTick() - s_display_tick) >= 100U) {
        s_display_tick = HAL_GetTick();
        display_set_force(force_kN, s_target_kg / KN_TO_KG);
        display_set_angle(torque_angle_get_deg(),
                          torque_angle_get_target(),
                          torque_angle_is_reached());
      }

    } else if (cur_screen == SCREEN_MENU) {

      if (enc_delta > 0)       display_menu_next();
      else if (enc_delta < 0)  display_menu_prev();

      if (input_enc_sw_pressed()) {
        uint8_t item = display_menu_get_item();
        switch (item) {
          case 0: display_set_screen(SCREEN_MAIN); break;
          case 1: display_set_screen(SCREEN_PRESET); break;
          case 2: calib_start(s_target_kg); break;
          case 3:
            display_settings_set_angle(torque_angle_get_target());
            display_set_screen(SCREEN_SETTINGS);
            break;
          case 4: motor_reset_position(); display_set_screen(SCREEN_MAIN); break;
          default: break;
        }
      }

    } else if (cur_screen == SCREEN_PRESET) {

      // Навігація енкодером — скидаємо таймер довгого натискання при прокрутці
      if (enc_delta != 0) {
        display_preset_scroll(enc_delta);
        s_preset_hold_tick  = 0;
        s_preset_long_fired = false;
      }

      uint8_t idx = display_preset_get_item();
      bool is_add_new = (idx >= g_preset_count);

      // Довге натискання ENC (1 с) → режим редагування існуючого пресету
      if (input_enc_sw_held()) {
        if (s_preset_hold_tick == 0) {
          s_preset_hold_tick = HAL_GetTick() | 1U;
        } else if (!s_preset_long_fired &&
                   (HAL_GetTick() - s_preset_hold_tick) >= 1000U) {
          s_preset_long_fired = true;
          if (!is_add_new) {
            s_pedit_idx  = (int8_t)idx;
            s_pedit_inj  = preset_find_inj_idx(g_presets[idx].inj_name);
            s_pedit_op   = preset_find_op_idx(g_presets[idx].op_name);
            s_pedit_fmin = g_presets[idx].force_min_kN;
            s_pedit_fmax = g_presets[idx].force_max_kN;
            s_pedit_step = PEDIT_INJ;
            display_preset_edit_set(s_pedit_step, s_pedit_inj, s_pedit_op,
                                    s_pedit_fmin, s_pedit_fmax, false);
            display_set_screen(SCREEN_PRESET_EDIT);
          }
        }
      } else {
        s_preset_hold_tick = 0;
      }

      // Відпускання ENC → якщо не було довгого натискання
      if (input_enc_sw_released()) {
        if (!s_preset_long_fired) {
          if (is_add_new) {
            // Додати новий пресет
            s_pedit_idx  = -1;
            s_pedit_inj  = 0;
            s_pedit_op   = 0;
            s_pedit_fmin = 8.5f;
            s_pedit_fmax = 9.0f;
            s_pedit_step = PEDIT_INJ;
            display_preset_edit_set(s_pedit_step, s_pedit_inj, s_pedit_op,
                                    s_pedit_fmin, s_pedit_fmax, true);
            display_set_screen(SCREEN_PRESET_EDIT);
          } else {
            // Короткий натиск → вибрати пресет і встановити ціль
            s_target_kg  = preset_target_kg(idx);
            s_preset_idx = (int8_t)idx;
            s_auto_state = AUTO_IDLE;
            display_set_active_preset(s_preset_idx);
            display_set_screen(SCREEN_MAIN);
          }
        }
        s_preset_long_fired = false;
        s_preset_hold_tick  = 0;
      }

    } else if (cur_screen == SCREEN_PRESET_EDIT) {

      // Змінюємо поточне поле енкодером
      if (enc_delta != 0) {
        switch (s_pedit_step) {
          case PEDIT_INJ: {
            int16_t v = (int16_t)s_pedit_inj + enc_delta;
            if (v < 0) v += INJ_TYPES_COUNT;
            if (v >= INJ_TYPES_COUNT) v -= INJ_TYPES_COUNT;
            s_pedit_inj = (uint8_t)v;
            break;
          }
          case PEDIT_OP: {
            int16_t v = (int16_t)s_pedit_op + enc_delta;
            if (v < 0) v += OP_TYPES_COUNT;
            if (v >= OP_TYPES_COUNT) v -= OP_TYPES_COUNT;
            s_pedit_op = (uint8_t)v;
            break;
          }
          case PEDIT_FMIN:
            s_pedit_fmin += (float)enc_delta * 0.1f;
            if (s_pedit_fmin < 0.1f)   s_pedit_fmin = 0.1f;
            if (s_pedit_fmin > 200.0f) s_pedit_fmin = 200.0f;
            break;
          case PEDIT_FMAX:
            s_pedit_fmax += (float)enc_delta * 0.1f;
            if (s_pedit_fmax < 0.1f)   s_pedit_fmax = 0.1f;
            if (s_pedit_fmax > 200.0f) s_pedit_fmax = 200.0f;
            break;
          case PEDIT_CONFIRM:
            // enc_delta < 0 → перейти до DELETE (тільки для існуючого)
            if (enc_delta < 0 && s_pedit_idx >= 0) {
              s_pedit_step = PEDIT_DELETE;
            }
            break;
          case PEDIT_DELETE:
            // enc_delta > 0 → повернутись до CONFIRM
            if (enc_delta > 0) {
              s_pedit_step = PEDIT_CONFIRM;
            }
            break;
        }
        display_preset_edit_set(s_pedit_step, s_pedit_inj, s_pedit_op,
                                s_pedit_fmin, s_pedit_fmax, s_pedit_idx < 0);
      }

      // Натискання ENC: перейти до наступного поля або виконати дію
      if (input_enc_sw_pressed()) {
        switch (s_pedit_step) {
          case PEDIT_INJ:
            s_pedit_step = PEDIT_OP;
            display_preset_edit_set(s_pedit_step, s_pedit_inj, s_pedit_op,
                                    s_pedit_fmin, s_pedit_fmax, s_pedit_idx < 0);
            break;
          case PEDIT_OP:
            s_pedit_step = PEDIT_FMIN;
            display_preset_edit_set(s_pedit_step, s_pedit_inj, s_pedit_op,
                                    s_pedit_fmin, s_pedit_fmax, s_pedit_idx < 0);
            break;
          case PEDIT_FMIN:
            if (s_pedit_fmax < s_pedit_fmin) s_pedit_fmax = s_pedit_fmin;
            s_pedit_step = PEDIT_FMAX;
            display_preset_edit_set(s_pedit_step, s_pedit_inj, s_pedit_op,
                                    s_pedit_fmin, s_pedit_fmax, s_pedit_idx < 0);
            break;
          case PEDIT_FMAX:
            s_pedit_step = PEDIT_CONFIRM;
            display_preset_edit_set(s_pedit_step, s_pedit_inj, s_pedit_op,
                                    s_pedit_fmin, s_pedit_fmax, s_pedit_idx < 0);
            break;
          case PEDIT_CONFIRM:
            // Зберегти пресет
            if (s_pedit_idx < 0) {
              preset_add(s_pedit_inj, s_pedit_op, s_pedit_fmin, s_pedit_fmax);
            } else {
              preset_set((uint8_t)s_pedit_idx, s_pedit_inj, s_pedit_op,
                         s_pedit_fmin, s_pedit_fmax);
            }
            flash_save(loadcell_get_scale(), loadcell_get_offset(),
                       s_target_kg, torque_angle_get_target());
            display_set_screen(SCREEN_PRESET);
            break;
          case PEDIT_DELETE:
            // Видалити існуючий пресет
            if (s_pedit_idx >= 0) {
              preset_delete((uint8_t)s_pedit_idx);
              if (s_preset_idx == s_pedit_idx) {
                s_preset_idx = -1;
                display_set_active_preset(-1);
              } else if (s_preset_idx > s_pedit_idx) {
                s_preset_idx--;
                display_set_active_preset(s_preset_idx);
              }
            }
            flash_save(loadcell_get_scale(), loadcell_get_offset(),
                       s_target_kg, torque_angle_get_target());
            display_set_screen(SCREEN_PRESET);
            break;
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

      // Енкодер: змінюємо цільовий кут затяжки
      if (enc_delta != 0) {
        float new_angle = torque_angle_get_target() + (float)enc_delta * ANGLE_STEP_DEG;
        torque_angle_set_target(new_angle);
        display_settings_set_angle(torque_angle_get_target());
      }

      // Кнопка енкодера: зберегти кут у Flash і повернутись до меню
      if (input_enc_sw_pressed()) {
        flash_save(loadcell_get_scale(), loadcell_get_offset(),
                   s_target_kg, torque_angle_get_target());
        display_set_screen(SCREEN_MENU);
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
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
    // ENC_CLK обробляється через polling у input_update(), не через ISR
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */
  if (htim->Instance == TIM7)
  {
    motor_tim_tick();
    input_debounce_tick();
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
