#include "main.h"

/**
 * @brief  LED闪烁任务 确认存活
 * @param  void *Parameters
 * @return void
 */
// int diff    = 0;
int _target = 0;
int debug4  = 0;
int debug5  = 0;
int debug6  = 0;
int debug7  = 0;
int debug8  = 0;

void Task_Blink(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();
    while (1) {
        GREEN_LIGHT_TOGGLE;
        vTaskDelayUntil(&LastWakeTime, 250);
    }

    vTaskDelete(NULL);
}

/**
 * @brief  底盘@三星轮组
 * @param  void *Parameters
 * @return void
 */
void Task_Chassis(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();
    // int        WheelSpeedRes[4], Buffer[4];
    float kFeedback        = 3.14 / 60;
    int   minCurrentToMove = 600;
    // int timesToMove = 0;

    PID_Init(&ChassisAnglePID1, 1.5, 0, 0, 100); // 1.5
    PID_Init(&ChassisAnglePID2, 1.5, 0, 0, 100); // 1.5

    PID_Init(&CM1PID, 55, 0.01, 0, 5000); // 35   0.01 //空载情况最佳状态p=35  下地状态p=55时大致可以驱动
    PID_Init(&CM2PID, 55, 0.01, 0, 5000); // 35   0.01

    // CM1PID.maxOutput_I = 5000;
    // PID_Init(&CM2PID, 0, 0, 0, 1000);
    // PID_Init(&CM3PID, 0, 0, 0, 1000);
    // PID_Init(&CM4PID, 0, 0, 0, 1000);
    CM1_Encoder.ecdBias = Motor_Feedback.motor201Angle;
    CM2_Encoder.ecdBias = Motor_Feedback.motor202Angle;

    CAN_Get_Encoder_Bias(&CM1_Encoder);
    CAN_Get_Encoder_Bias(&CM2_Encoder);

    while (1) {
        // if（DBusData.switchRight == 2){
        //     Can_Set_CM_Current(CAN1, 0, 0, 0, 0);
        //     Can2_Set_CM_Current(CAN2, 0, 0, 0, 0);
        // }

        // CM1PID.i = (float) magic.value / 1000.0;
        // CM1PID.p = magic.value;

        // if (DBusData.switchLeft == 2) {
        CAN_Update_Encoder_Data(&CM1_Encoder, Motor_Feedback.motor201Angle);
        CAN_Update_Encoder_Data(&CM2_Encoder, Motor_Feedback.motor202Angle);
        // CAN_Update_Encoder_Data(&CM1_Encoder, Motor_Feedback.motor203Angle);
        // CAN_Update_Encoder_Data(&CM1_Encoder, Motor_Feedback.motor204Angle);

        PID_Calculate(&ChassisAnglePID1, 600, CM1_Encoder.ecdAngle / 19);
        PID_Calculate(&ChassisAnglePID2, -600, CM2_Encoder.ecdAngle / 19);

        PID_Calculate(&CM1PID, ChassisAnglePID1.output, Motor_Feedback.motor201Speed * kFeedback);
        PID_Calculate(&CM2PID, ChassisAnglePID2.output, Motor_Feedback.motor202Speed * kFeedback);

        debug4 = CM1_Encoder.ecdAngle / 19;
        debug5 = ChassisAnglePID1.output;
        debug6 = Motor_Feedback.motor201Speed * kFeedback;
        debug7 = CM1PID.output;
        debug8 = 720;
        _Set_CM_Current(CM1PID.output, CM2PID.output);
        // }

        vTaskDelayUntil(&LastWakeTime, 5);

        // yawSpeedFeed = mpu6500_data.gz / 16.4;
        // yawAngleFeed = EulerAngle.Yaw;
        // PID_Init(&YawAnglePID, 15, 0, 0, 660);
        // PID_Init(&YawSpeedPID, 2, 0, 0, 660);

        // PID_Calculate(&YawAnglePID, DBusData.ch1, yawAngleFeed);
        // PID_Calculate(&YawSpeedPID, YawAnglePID.output, yawSpeedFeed);
        // Chassis_Set_Wheel_Speed(DBusData.ch4, DBusData.ch3, YawSpeedPID.output); //设定XYZ三个轴的速度

        // Chassis_Update_Mecanum_Data(Buffer); //麦轮的解算

        // Chassis_Limit_Wheel_Speed(Buffer, WheelSpeedRes, MAXWHEELSPEED); //限幅

        // PID_Calculate(&CM5PID, DBusData.ch4, Motor_Feedback.motor205Speed * kFeedback);
        // PID_Calculate(&CM2PID, WheelSpeedRes[1], Motor_Feedback.motor202Speed * kFeedback);
        // PID_Calculate(&CM3PID, WheelSpeedRes[2], Motor_Feedback.motor203Speed * kFeedback);
        // PID_Calculate(&CM4PID, WheelSpeedRes[3], Motor_Feedback.motor204Speed * kFeedback);
        // diff = Motor_Feedback.motor201Speed * kFeedback;
    }

    vTaskDelete(NULL);
}
void Task_Wheel(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();

    while (1) {
        Can2_Set_CM_Current(CAN2, 500, 0, 0, 0);
        vTaskDelayUntil(&LastWakeTime, 50);
    }
    vTaskDelete(NULL);
}

void _Set_CM_Current(int16_t i_201, int16_t i_202) {
    int minCurrent    = 900;  // 250
    int currentToMove = 1800; // 2000
    int threshold     = 850;
    if (ABS(i_201) > threshold) {
        if (Motor_Feedback.motor201Speed == 0) {
            Can_Set_CM_Current(CAN1, currentToMove, i_202, 0, 0);
        } else {
            if (i_201 < 0 && i_201 > -minCurrent) i_201 = -minCurrent;
            if (i_201 > 0 && i_201 < minCurrent) i_201 = minCurrent;

            Can_Set_CM_Current(CAN1, i_201, i_202, 0, 0);
        }
    } else {
        Can_Set_CM_Current(CAN1, i_201, i_202, 0, 0);
    }
    if (ABS(i_202) > threshold) {
        if (Motor_Feedback.motor202Speed == 0) {
            Can_Set_CM_Current(CAN1, i_201, currentToMove, 0, 0);
        } else {
            if (i_202 < 0 && i_202 > -minCurrent) i_202 = -minCurrent;
            if (i_202 > 0 && i_202 < minCurrent) i_202 = minCurrent;

            Can_Set_CM_Current(CAN1, i_201, i_202, 0, 0);
        }
    } else {
        Can_Set_CM_Current(CAN1, i_201, i_202, 0, 0);
    }
}
