/**
 * @brief 功能任务
 */

#include "main.h"

/**
 * @brief  LED闪烁任务 确认存活
 */

void Task_Blink(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();
    while (1) {
        GREEN_LIGHT_TOGGLE;
        vTaskDelayUntil(&LastWakeTime, 250);
    }

    vTaskDelete(NULL);
}

/**
 * @brief  底盘运动
 */

void Task_Chassis(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount(); // 时钟
    int        wheelSpeed[4];                      // 麦轮速度
    float      kFeedback      = 3.14 / 60;         // 转子的转速(RPM,RoundPerMinute)换算成角速度(rad/s)
    int        mode           = 2;                 // 底盘运动模式,1直线,2转弯
    int        lastMode       = 2;                 // 上一次的运动模式
    float      yawAngleTarget = 0;                 // 目标值
    float      yawAngleFeed, yawSpeedFeed;         // 反馈值

    // 初始化麦轮角速度PID
    PID_Init(&PID_LFCM, 15, 0.3, 0, 4000, 2000);
    PID_Init(&PID_LBCM, 15, 0.3, 0, 4000, 2000);
    PID_Init(&PID_RBCM, 15, 0.3, 0, 4000, 2000);
    PID_Init(&PID_RFCM, 15, 0.3, 0, 4000, 2000);

    // 初始化航向角角度PID和角速度PID
    PID_Init(&PID_YawAngle, 10, 0, 0, 1000, 1000);
    PID_Init(&PID_YawSpeed, 2, 0, 0, 4000, 1000);

    while (1) {

        // 更新运动模式
        mode = ABS(remoteData.ch1) < 5 ? 1 : 2;

        // 设置反馈值
        yawAngleFeed = EulerAngle.Yaw;         // 航向角角度反馈
        yawSpeedFeed = mpu6500_data.gz / 16.4; // 航向角角速度反馈

        // 切换运动模式
        if (mode != lastMode) {
            PID_YawAngle.output_I = 0;            // 清空角度PID积分
            PID_YawSpeed.output_I = 0;            // 清空角速度PID积分
            yawAngleTarget        = yawAngleFeed; // 更新角度PID目标值
            lastMode              = mode;         // 更新lastMode
        }

        // 根据运动模式计算PID
        if (mode == 1) {
            PID_Calculate(&PID_YawAngle, yawAngleTarget, yawAngleFeed);      // 计算航向角角度PID
            PID_Calculate(&PID_YawSpeed, PID_YawAngle.output, yawSpeedFeed); // 计算航向角角速度PID
        } else {
            PID_Calculate(&PID_YawSpeed, -remoteData.ch1, yawSpeedFeed); // 计算航向角角速度PID
        }

        // 设置底盘总体移动速度
        Chassis_Set_Speed(remoteData.ch4, -remoteData.ch3, -PID_YawSpeed.output);

        // 麦轮解算&限幅,获得轮子转速
        Chassis_Update_Wheel_Speed(wheelSpeed);

        // 计算输出电流PID
        PID_Calculate(&PID_LFCM, wheelSpeed[0], Motor_Feedback.motor201Speed * kFeedback);
        PID_Calculate(&PID_RFCM, wheelSpeed[1], Motor_Feedback.motor204Speed * kFeedback);
        PID_Calculate(&PID_RBCM, wheelSpeed[2], Motor_Feedback.motor203Speed * kFeedback);
        PID_Calculate(&PID_LBCM, wheelSpeed[3], Motor_Feedback.motor202Speed * kFeedback);

        // 输出电流值到电调
        Can_Set_CM_Current(CAN1, PID_LFCM.output, PID_LBCM.output, PID_RBCM.output, PID_RFCM.output);

        // 底盘运动更新频率
        vTaskDelayUntil(&LastWakeTime, 10);
    }

    vTaskDelete(NULL);
}

/**
 * @brief  安全模式
 */
void Task_Safe_Mode(void *Parameters) {

    while (1) {
        if (remoteData.switchRight == 2) {
            Can_Set_CM_Current(CAN1, 0, 0, 0, 0);
            vTaskSuspendAll();
        }
        vTaskDelay(100);
    }

    vTaskDelete(NULL);
}
