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
#define ENABLE_Pin GPIO_PIN_12
#define ENABLE_GPIO_Port GPIOB
#define HX_DAT_Pin GPIO_PIN_0
#define HX_DAT_GPIO_Port GPIOB
#define HX_CLK_Pin GPIO_PIN_7
#define HX_CLK_GPIO_Port GPIOA
#define STOP_BTN_Pin GPIO_PIN_0
#define STOP_BTN_GPIO_Port GPIOA
#define STOP_BTN_EXTI_IRQn EXTI0_IRQn
#define ANGLE_Pin GPIO_PIN_1
#define ANGLE_GPIO_Port GPIOA
#define ENC_DT_Pin GPIO_PIN_1
#define ENC_DT_GPIO_Port GPIOB
#define ENC_SW_Pin GPIO_PIN_2
#define ENC_SW_GPIO_Port GPIOB
#define LIMIT_TOP_Pin GPIO_PIN_15
#define LIMIT_TOP_GPIO_Port GPIOB
#define LIMIT_BOT_Pin GPIO_PIN_8
#define LIMIT_BOT_GPIO_Port GPIOA
#define JOY_UP_Pin GPIO_PIN_9
#define JOY_UP_GPIO_Port GPIOA
#define JOY_DOWN_Pin GPIO_PIN_10
#define JOY_DOWN_GPIO_Port GPIOA
#define ZERO_BTN_Pin GPIO_PIN_15
#define ZERO_BTN_GPIO_Port GPIOA
#define ENC_CLK_Pin GPIO_PIN_10
#define ENC_CLK_GPIO_Port GPIOB
#define ENC_CLK_EXTI_IRQn EXTI15_10_IRQn
#define STEP_Pin GPIO_PIN_13
#define STEP_GPIO_Port GPIOB
#define DIR_Pin GPIO_PIN_14
#define DIR_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define POT_Pin             GPIO_PIN_2
#define POT_GPIO_Port       GPIOA
/*
 * Тимчасова карта пінів для переходу на STM32F411CEU6 Black Pill:
 * PB0=HX_DAT, PA7=HX_CLK,
 * PB1=ENC_DT, PB2=ENC_SW, PB10=ENC_CLK,
 * PB12=ENABLE, PB13=STEP, PB14=DIR,
 * PB15=LIMIT_TOP, PA8=LIMIT_BOT, PA9=JOY_UP, PA10=JOY_DOWN, PA15=ZERO_BTN.
 */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
