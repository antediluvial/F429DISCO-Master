/*!
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
    LCD_Init();
    //test

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
        CPU_TS ts;  //CPU Timestamp

       //SEND INTERRUPT

       //REIVE ADC DATA

        HAL_UART_Receive(&h_UARTHandle,&test,5,200);      
        Adc_value = (atoi(test) / 10);
        OSTimeDlyHMSM(0, 0, 0, 50,OS_OPT_TIME_HMSM_STRICT, &err);
    }

}

static void UpdateLCDTask(void *p_arg)
{

    while (DEF_TRUE)
    {
        OS_ERR err;
        CPU_TS ts;  //CPU Timestamp

        OSMutexPend((OS_MUTEX *)&AppMutex,
                    (OS_TICK )0,
                    (OS_OPT )OS_OPT_PEND_BLOCKING,
                    (CPU_TS *)&ts,
                    (OS_ERR *)&err);
        BSP_LCD_Clear(LCD_COLOR_WHITE);
        LCD_ADC();

        LCDProgressBar(Adc_value);

        OSMutexPost((OS_MUTEX *)&AppMutex,
                    (OS_OPT )OS_OPT_POST_NONE,
                    (OS_ERR *)&err);

        OSTimeDlyHMSM(0, 0, 0, 500,OS_OPT_TIME_HMSM_STRICT, &err);
    }
}


/*!
*********************************************************************************************************
*                                      NON-TASK FUNCTIONS
*********************************************************************************************************
*/

static void LCDProgressBar(uint32_t Adc)
{
        for (int i = 0; i < 2; i++)
        {
            BSP_LCD_DrawLine(108+i,320,108+i,(320+((Adc_value*0.07324)*-1)));
            BSP_LCD_DrawLine(131+i,320,131+i,(320+((Adc_value*0.07324)*-1)));
        }

        BSP_LCD_SetTextColor(LCD_COLOR_RED);
        for (int i = 0; i < 21; i++)
        {
            BSP_LCD_DrawLine(110+i,320,110+i,(320+((Adc_value*0.07324)*-1)));
        }
        BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
}

static void LCD_ADC(void)
{
        char ADCStr[15]="ADC Value ";
        sprintf(ADCStr+strlen(ADCStr),"%d",(int)Adc_value);
        BSP_LCD_DisplayStringAtLine(1,(uint8_t*)ADCStr);

        char VOLStr[15]="Voltage ";
        int tens = (Adc_value*122) / 100000;
        int decimals = (Adc_value*122) % 10000;

        sprintf(VOLStr+strlen(VOLStr),"%d",tens);
        sprintf(VOLStr+strlen(VOLStr),".%d",decimals);
        BSP_LCD_DisplayStringAtLine(2,(uint8_t*)VOLStr);
}

void HAL_Delay(uint32_t Delay)
{
    OS_ERR err;
    OSTimeDly((OS_TICK)Delay,
              (OS_OPT)OS_OPT_TIME_DLY,
              (OS_ERR *)&err);
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
    h_UARTHandle.Instance        = USART1;
    h_UARTHandle.Init.BaudRate   = 9600;
    h_UARTHandle.Init.WordLength = UART_WORDLENGTH_8B;
    h_UARTHandle.Init.StopBits   = UART_STOPBITS_1;
    h_UARTHandle.Init.Parity     = UART_PARITY_NONE;
    h_UARTHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    h_UARTHandle.Init.Mode       = UART_MODE_TX_RX;

    HAL_UART_Init(&h_UARTHandle);
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

    x = BSP_LCD_GetXSize();
    y = BSP_LCD_GetYSize();
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
    HAL_GPIO_Init(GPIOB,&GPIO_InitStruct);

    GPIO_InitStruct.Alternate = GPIO_AF7_USART1; //SERIAL1 RX
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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