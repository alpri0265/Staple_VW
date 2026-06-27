/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ENABLE_Pin GPIO_PIN_2
#define ENABLE_GPIO_Port GPIOE
#define HX_DAT_Pin GPIO_PIN_0
#define HX_DAT_GPIO_Port GPIOC
#define HX_CLK_Pin GPIO_PIN_1
#define HX_CLK_GPIO_Port GPIOC
#define STOP_BTN_Pin GPIO_PIN_0
#define STOP_BTN_GPIO_Port GPIOA
#define STOP_BTN_EXTI_IRQn EXTI0_IRQn
#define ANGLE_Pin GPIO_PIN_1
#define ANGLE_GPIO_Port GPIOA
#define ENC_DT_Pin GPIO_PIN_1
#define ENC_DT_GPIO_Port GPIOB
#define ENC_SW_Pin GPIO_PIN_2
#define ENC_SW_GPIO_Port GPIOB
#define LIMIT_TOP_Pin GPIO_PIN_0
#define LIMIT_TOP_GPIO_Port GPIOD
#define LIMIT_BOT_Pin GPIO_PIN_1
#define LIMIT_BOT_GPIO_Port GPIOD
#define JOY_UP_Pin GPIO_PIN_2
#define JOY_UP_GPIO_Port GPIOD
#define JOY_DOWN_Pin GPIO_PIN_3
#define JOY_DOWN_GPIO_Port GPIOD
#define ZERO_BTN_Pin GPIO_PIN_4
#define ZERO_BTN_GPIO_Port GPIOD
#define ENC_CLK_Pin GPIO_PIN_3
#define ENC_CLK_GPIO_Port GPIOB
#define ENC_CLK_EXTI_IRQn EXTI3_IRQn
#define STEP_Pin GPIO_PIN_0
#define STEP_GPIO_Port GPIOE
#define DIR_Pin GPIO_PIN_1
#define DIR_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
// PB3 = JTDO/SWO debug pin — конфлікт з налагоджувачем, CLK завжди HIGH.
// Encoder CLK фізично підключений до PC4.
#undef  ENC_CLK_Pin
#undef  ENC_CLK_GPIO_Port
#undef  ENC_CLK_EXTI_IRQn
#define ENC_CLK_Pin         GPIO_PIN_4
#define ENC_CLK_GPIO_Port   GPIOC
#define ENC_CLK_EXTI_IRQn   EXTI4_IRQn
#define POT_Pin             GPIO_PIN_2
#define POT_GPIO_Port       GPIOA
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
