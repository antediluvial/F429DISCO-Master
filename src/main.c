/**
 * @file main.c
 * @brief Main source code file
 * @version 0.1
 * @date 2021-11-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */

/*
*********************************************************************************************************
*                                            LOCAL INCLUDES
*********************************************************************************************************
*/

#include "main.h"

/*!
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR err;

    OSInit(&err);

    OSMutexCreate((OS_MUTEX *)&AppMutex,
                  (CPU_CHAR *)"My application Mutex",
                  (OS_ERR *)err);

    OSTaskCreate((OS_TCB *)&AppTaskStartTCB,
                 (CPU_CHAR *)"App Task Start",
                 (OS_TASK_PTR)AppTaskStart,
                 (void *)0,
                 (OS_PRIO)APP_TASK_START_PRIO,
                 (CPU_STK *)&AppTaskStartStk[0],
                 (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)TASK_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSStart(&err);
}

/*!
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg)
{
    OS_ERR err;

    HAL_Init();

    SystemClock_Config();

    GPIO_Init();
    USART_Init();
    USART3_Init();
    LCD_Init();

    HAL_GPIO_WritePin(GPIOB, 4, GPIO_PIN_SET);

    OSTaskCreate(
        (OS_TCB *)&UpdateLCDTaskTCB,
        (CPU_CHAR *)"Update LCD Task",
        (OS_TASK_PTR)UpdateLCDTask,
        (void *)0,
        (OS_PRIO)UPDATE_LCD_TASK_PRIO,
        (CPU_STK *)&UpdateLCDTaskStk[0],
        (CPU_STK_SIZE)TASK_STK_SIZE / 10,
        (CPU_STK_SIZE)TASK_STK_SIZE,
        (OS_MSG_QTY)5u,
        (OS_TICK)0u,
        (void *)0,
        (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
        (OS_ERR *)&err);

    OSTaskCreate(
        (OS_TCB *)&ReadUsartTaskTCB,
        (CPU_CHAR *)"Read USART tasl",
        (OS_TASK_PTR)ReadUsartTask,
        (void *)0,
        (OS_PRIO)READ_USART_TASK_PRIO,
        (CPU_STK *)&ReadUsartTaskStk[0],
        (CPU_STK_SIZE)TASK_STK_SIZE / 10,
        (CPU_STK_SIZE)TASK_STK_SIZE,
        (OS_MSG_QTY)5u,
        (OS_TICK)0u,
        (void *)0,
        (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
        (OS_ERR *)&err);
}

/*!
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

static void ReadUsartTask(void *p_arg)
{
    while (DEF_TRUE)
    {
        OS_ERR err;
        CPU_TS ts; //CPU Timestamp

        OSMutexPend((OS_MUTEX *)&AppMutex,
        (OS_TICK)0,
        (OS_OPT)OS_OPT_PEND_BLOCKING,
        (CPU_TS *)&ts,
        (OS_ERR *)&err);


        strcpy(DATABUFFER,"");
        //REIVE ADC DATA
        //Get a huge buffer and parse it for the correct values
        //Possibly the worst way to do this, but it almost works so ¯\_(ツ)_/¯
        HAL_UART_Receive(&h_UARTHandle, (uint8_t *)DATABUFFER, 20, HAL_MAX_DELAY);
        HAL_UART_Transmit(&h_UARTHandle3, (uint8_t *)DATABUFFER, 20, HAL_MAX_DELAY);

        int parseflag=0;
        for (int i = 0; i < strlen(DATABUFFER); i++)
        {
            if(DATABUFFER[i] == 'T')
            {
                for (int j = 0; j < 4; j++)
                {
                    ADCBUFFER[j]=DATABUFFER[i+1+j];
                }
            parseflag++;
            }
            if(DATABUFFER[i] == 'P')
            {
                for (int j = 0; j < 4; j++)
                {
                    RPMBUFFER[j]=DATABUFFER[i+1+j];
                }
            parseflag++;        
            }
            if (parseflag==2)
            {
                i=strlen(DATABUFFER);
            }
            
        }

        Adc_value = atoi(ADCBUFFER);
        

        OSMutexPost((OS_MUTEX *)&AppMutex,
        (OS_OPT)OS_OPT_POST_NONE,
        (OS_ERR *)&err);

        OSTimeDlyHMSM(0, 0, 0, 50, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

static void UpdateLCDTask(void *p_arg)
{

    while (DEF_TRUE)
    {
        OS_ERR err;
        CPU_TS ts; //CPU Timestamp

        OSMutexPend((OS_MUTEX *)&AppMutex,
                    (OS_TICK)0,
                    (OS_OPT)OS_OPT_PEND_BLOCKING,
                    (CPU_TS *)&ts,
                    (OS_ERR *)&err);
        BSP_LCD_Clear(LCD_COLOR_WHITE);

        LCD_ADC();
        LCDProgressBar();

        OSMutexPost((OS_MUTEX *)&AppMutex,
                    (OS_OPT)OS_OPT_POST_NONE,
                    (OS_ERR *)&err);

        OSTimeDlyHMSM(0, 0, 0, 500, OS_OPT_TIME_HMSM_STRICT, &err);
    }
}

/*!
*********************************************************************************************************
*                                      NON-TASK FUNCTIONS
*********************************************************************************************************
*/


static void LCDProgressBar(void)
{
    for (int i = 0; i < 2; i++)
    {
        BSP_LCD_DrawLine(108 + i, 320, 108 + i, (320 + ((Adc_value * BARADJUST) * -1)));
        BSP_LCD_DrawLine(131 + i, 320, 131 + i, (320 + ((Adc_value * BARADJUST) * -1)));
    }

    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    for (int i = 0; i < 21; i++)
    {
        BSP_LCD_DrawLine(110 + i, 320, 110 + i, (320 + ((Adc_value * BARADJUST) * -1)));
    }
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
}

static void LCD_ADC(void)
{

    char ADCStr[16] = "ADC Value ";
    sprintf(ADCStr + strlen(ADCStr), "%s", ADCBUFFER);
    BSP_LCD_DisplayStringAtLine(1, (uint8_t *)ADCStr);

    char VOLStr[15] = "Voltage ";
    int tens = (Adc_value * 80) / 1000000;
    int decimals = (Adc_value * 80) % 100000;

    sprintf(VOLStr + strlen(VOLStr), "%d", tens);
    sprintf(VOLStr + strlen(VOLStr), ".%d", decimals);
    BSP_LCD_DisplayStringAtLine(2, (uint8_t *)VOLStr);
    char test2[10] = "";
    sprintf(test2, "%d", testcount);
    BSP_LCD_DisplayStringAtLine(3, (uint8_t *)test2);
    testcount++;

    char RPMStr[16] = "RPM: ";
    sprintf(RPMStr + strlen(RPMStr), "%s", RPMBUFFER);
    BSP_LCD_DisplayStringAtLine(4, (uint8_t *)RPMStr);
}

// void HAL_Delay(uint32_t Delay)
// {
//     OS_ERR err;
//     OSTimeDly((OS_TICK)Delay,
//               (OS_OPT)OS_OPT_TIME_DLY,
//               (OS_ERR *)&err);
// }

void HAL_Delay(uint32_t milliseconds) {

   /* Initially clear flag */

   (void) SysTick->CTRL;

   while (milliseconds != 0) {

      /* COUNTFLAG returns 1 if timer counted to 0 since the last flag read */

      milliseconds -= (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) >> SysTick_CTRL_COUNTFLAG_Pos;

   }

}

/*!
*********************************************************************************************************
*                                      System Initializations
*********************************************************************************************************
*/

static void USART_Init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    h_UARTHandle.Instance = USART1;
    h_UARTHandle.Init.BaudRate = 9600;
    h_UARTHandle.Init.WordLength = UART_WORDLENGTH_8B;
    h_UARTHandle.Init.StopBits = UART_STOPBITS_1;
    h_UARTHandle.Init.Parity = UART_PARITY_NONE;
    h_UARTHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    h_UARTHandle.Init.Mode = UART_MODE_TX_RX;

    HAL_UART_Init(&h_UARTHandle);

    //NVIC_EnableIRQ(USART1_IRQn); 

}

static void USART3_Init(void)
{
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_8; //SERIAL3 TX
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3; //Alternate mode 7 is USART_RX/TX
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL; //CHANGED FROM NOPULL DUE TO PINOUT FILE
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Alternate = GPIO_AF7_USART3; //SERIAL1 RX
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    h_UARTHandle3.Instance = USART3;
    h_UARTHandle3.Init.BaudRate = 9600;
    h_UARTHandle3.Init.WordLength = UART_WORDLENGTH_8B;
    h_UARTHandle3.Init.StopBits = UART_STOPBITS_1;
    h_UARTHandle3.Init.Parity = UART_PARITY_NONE;
    h_UARTHandle3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    h_UARTHandle3.Init.Mode = UART_MODE_TX_RX;

    HAL_UART_Init(&h_UARTHandle3); 


}

static void LCD_Init(void)
{
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(LCD_BACKGROUND_LAYER, LCD_FRAME_BUFFER);
    BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, LCD_FRAME_BUFFER);
    BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);
    BSP_LCD_DisplayOn();
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
}

static void GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_6; //SERIAL1 TX
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1; //Alternate mode 7 is USART_RX/TX
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pull = GPIO_NOPULL; //CHANGED FROM NOPULL DUE TO PINOUT FILE
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Alternate = GPIO_AF7_USART1; //SERIAL1 RX
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitTypeDef GPIOPin;
    GPIOPin.Pin = GPIO_PIN_4;           // Select pin PA3/A2
    GPIOPin.Mode = GPIO_MODE_OUTPUT_PP; // Select Digital output
    GPIOPin.Pull = GPIO_NOPULL;         // Disable internal pull-up or pull-down resistor
    HAL_GPIO_Init(GPIOB, &GPIOPin);     // initialize PA3 as analog input pin
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /** Configure the main internal regulator output voltage 
  */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the CPU, AHB and APB busses clocks 
  */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 180;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /** Activate the Over-Drive mode 
  */
    HAL_PWREx_EnableOverDrive();

    /** Initializes the CPU, AHB and APB busses clocks 
  */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 216;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

}