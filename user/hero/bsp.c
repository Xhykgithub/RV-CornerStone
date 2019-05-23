/**
 * @brief 硬件初始化
 * @note 该函数应该在task.c中被执行
 */

#include "bsp.h"
#include "handle.h"
#include "Driver_BSP.h"

void BSP_Init(void) {
    BSP_CAN_Init();
    BSP_DBUS_Init(remoteBuffer);
    BSP_TIM2_Init();
    BSP_IMU_Init();
    BSP_USART3_Init(9600, USART_IT_RXNE);
    BSP_USART6_Init(115200, 0);
    BSP_UART7_Init(9600, 0);
    BSP_UART8_Init(9600, 0);
    BSP_Laser_Init();
    BSP_Beep_Init();
    BSP_LED_Init();

    BSP_PWM_Set_Port(&PWM_Magazine_Servo, PWM_PORT_PD12);
    BSP_PWM_Init(&PWM_Magazine_Servo, 9000, 200, TIM_OCPolarity_Low);
    BSP_DMA2_Init(&USART6->DR, Judge.buf, JudgeBufferLength);

    // BSP_PWM_Set_Port(&PWM_Test, PWM_PORT_PD12);
    // BSP_PWM_Init(&PWM_Test, 9000, 200, TIM_OCPolarity_Low);
}
