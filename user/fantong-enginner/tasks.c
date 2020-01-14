/**
 * @brief 工程
 * @version 1.2.0
 */
#include "main.h"

void Task_Safe_Mode(void *Parameters) {
    while (1) {
        if (remoteData.switchRight == 2 && remoteData.switchLeft == 1) {
            vTaskSuspendAll();
            while (1) {
                Can_Send(CAN1, 0x200, 0, 0, 0, 0);
                Can_Send(CAN1, 0x1ff, 0, 0, 0, 0);
                Can_Send(CAN2, 0x200, 0, 0, 0, 0);
                GO_OFF;
                GET_OFF;
                RESCUE_HOOK_UP;
                vTaskDelay(2);
            }
        }
        vTaskDelay(2);
    }
    vTaskDelete(NULL);
}

void Task_Control(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();
    uint8_t    LastMode     = 0;
    GetMode                 = 0;
    GoMode                  = 0;
    ControlMode             = 1;
    UpMode                  = 0;
    while (1) {
        if (remoteData.switchLeft == 1) {
            ControlMode = 1;
        } else if (remoteData.switchLeft == 2 || remoteData.switchLeft == 3) {
            ControlMode = 2;
            if (remoteData.switchRight == 1 && remoteData.switchLeft == 3) {
                UpMode = 1;
            } else if (remoteData.switchRight == 3 && remoteData.switchLeft == 3) {
                GoMode == 1;
            } else if (remoteData.switchRight == 2 && remoteData.switchLeft == 3) {
                GetMode == 1;
            } else if (remoteData.switchLeft == 2 && remoteData.switchRight == 2) {
                GetMode = 0;
            } else if (remoteData.switchLeft == 2 && remoteData.switchRight == 3) {
                GoMode = 0;
            } else if (remoteData.switchLeft == 2 && remoteData.switchRight == 1) {
                UpMode = 0;
            }
        }
        if (LastMode != ControlMode && LastMode != 0) {
            if (LastMode == 1) {
                Can_Send(CAN1, 0x200, 0, 0, 0, 0);
            } else if (LastMode == 2) {
                UpMode  = 0;
                GoMode  = 0;
                GetMode = 0;
                Can_Send(CAN2, 0x200, 0, 0, 0, 0);
            }
        }

        LastMode = ControlMode;

        // 调试视觉用
        // PsAimEnabled   = (remoteData.switchLeft == 1) && (remoteData.switchRight != 3);
        vTaskDelayUntil(&LastWakeTime, 5);
    }
    vTaskDelete(NULL);
}

void Task_Chassis(void *Parameters) {
    // 任务
    TickType_t LastWakeTime = xTaskGetTickCount(); // 时钟
    float      interval     = 0.01;                // 任务运行间隔 s
    int        intervalms   = interval * 1000;     // 任务运行间隔 ms

    // 运动模式
    int   mode           = 2; // 底盘运动模式,1直线,2转弯
    int   lastMode       = 2; // 上一次的运动模式
    float yawAngleTarget = 0; // 目标值
    float yawAngle, yawSpeed; // 反馈值

    // 底盘运动
    float vx = 0;
    float vy = 0;
    float vw = 0;

    // 初始化麦轮角速度PID
    PID_Init(&PID_LFCM, 32, 0, 0, 12000, 4000);
    PID_Init(&PID_LBCM, 32, 0, 0, 12000, 4000);
    PID_Init(&PID_RBCM, 32, 0, 0, 12000, 4000);
    PID_Init(&PID_RFCM, 32, 0, 0, 12000, 4000);

    // 初始化航向角角度PID和角速度PID
    PID_Init(&PID_YawAngle, 5, 0.1, 0, 1000, 1000);
    PID_Init(&PID_YawSpeed, 3, 0, 0, 4000, 1000);

    // 初始化底盘
    Chassis_Init(&ChassisData);

    while (1) {

        // 更新运动模式
        mode = ABS(remoteData.rx) < 5 ? 1 : 2;

        // 设置反馈值
        yawAngle = Gyroscope_EulerData.yaw;    // 航向角角度反馈
        yawSpeed = ImuData.gz / GYROSCOPE_LSB; // 逆时针为正

        vx = -remoteData.lx / 660.0f * 4;
        vy = remoteData.ly / 660.0f * 2;
        vw = PID_YawSpeed.output / 4000.0f * 6;

        // 切换运动模式
        if (mode != lastMode) {
            yawAngleTarget = yawAngle; // 更新角度PID目标值
            lastMode       = mode;     // 更新lastMode
        }

        // 根据运动模式计算PID
        if (mode == 1) {
            PID_Calculate(&PID_YawAngle, yawAngleTarget, yawAngle);      // 计算航向角角度PID
            PID_Calculate(&PID_YawSpeed, PID_YawAngle.output, yawSpeed); // 计算航向角角速度PID
        } else {
            PID_Calculate(&PID_YawSpeed, -remoteData.rx, yawSpeed); // 计算航向角角速度PID
        }

        // 设置底盘总体移动速度
        // Chassis_Update(&ChassisData, (float) -remoteData.lx / 660.0f, (float) remoteData.ly / 660.0f, (float) PID_YawSpeed.output / 660.0f);
        Chassis_Update(&ChassisData, vx, vy, vw);

        // 麦轮解算
        Chassis_Calculate_Rotor_Speed(&ChassisData);

        //稍微限制移动速度
        Chassis_Limit_Rotor_Speed(&ChassisData, 800);

        // 计算输出电流PID
        PID_Calculate(&PID_LFCM, ChassisData.rotorSpeed[0], Motor_LF.speed * RPM2RPS);
        PID_Calculate(&PID_LBCM, ChassisData.rotorSpeed[1], Motor_LB.speed * RPM2RPS);
        PID_Calculate(&PID_RBCM, ChassisData.rotorSpeed[2], Motor_RB.speed * RPM2RPS);
        PID_Calculate(&PID_RFCM, ChassisData.rotorSpeed[3], Motor_RF.speed * RPM2RPS);

        // 输出电流值到电调
        if (ControlMode != 2) {
            Can_Send(CAN1, 0x200, PID_LFCM.output, PID_LBCM.output, PID_RBCM.output, PID_RFCM.output);
        } else {
            Can_Send(CAN1, 0x200, 0, 0, 0, 0);
        }

        // 底盘运动更新频率
        vTaskDelayUntil(&LastWakeTime, intervalms);
    }

    vTaskDelete(NULL);
}

void Task_Fetch(void *Parameters) {
    // 任务
    TickType_t LastWakeTime = xTaskGetTickCount(); // 时钟
    float      interval     = 0.005;               // 任务运行间隔 s
    int        intervalms   = interval * 1000;     // 任务运行间隔 ms

    // 斜坡
    float pitchAngleTargetProgress = 0;
    float pitchAngleTargetStart    = 0;
    float pitchAngleTargetStop     = 10;

    // 反馈值
    float xSpeed;
    float pitchLeftAngle;
    float pitchRightAngle;

    // 目标值
    float xSpeedTarget     = 0;
    float pitchAngleTarget = 0;

    // 视觉辅助
    int   lastSeq        = 0;
    float xSpeedTargetPs = 0;

    // 初始化PID
    PID_Init(&PID_Fetch_X, 30, 0, 0, 4000, 2000);
    PID_Init(&PID_Fetch_Pitch_Left, 800, 0.1, 0, 8000, 2000);
    PID_Init(&PID_Fetch_Pitch_Right, 800, 0.1, 0, 8000, 2000);

    while (1) {
        // 设置反馈
        xSpeed          = Motor_Fetch_X.speed * RPM2RPS;
        pitchLeftAngle  = Motor_Fetch_LP.angle;
        pitchRightAngle = Motor_Fetch_RP.angle;

        // 斜坡
        if (pitchAngleTargetProgress < 1) {
            pitchAngleTargetProgress += 0.01;
            // pitchAngleTargetProgress += CHOOSER(0.005, 0.01, 0.015);
        }

        // 视觉辅助
        PsAimEnabled = (remoteData.switchRight == 1);
        if (!PsAimEnabled) {
            xSpeedTargetPs = 0;
            lastSeq        = Ps.autoaimData.seq;
        } else if (lastSeq != Ps.autoaimData.seq) {
            lastSeq        = Ps.autoaimData.seq;
            xSpeedTargetPs = Ps.autoaimData.yaw_angle_diff;
            if (Ps.autoaimData.pitch_angle_diff > 0) {
                pitchAngleTargetProgress = 0;
                pitchAngleTargetStart    = pitchAngleTargetStop;
                pitchAngleTargetStop     = Ps.autoaimData.pitch_angle_diff;
            }
        }
        xSpeedTarget = xSpeedTargetPs;

        // 遥控器
        if (ABS(remoteData.lx) > 30) xSpeedTarget = remoteData.lx / 660.0f * 200;
        if (remoteData.rx > 500 && pitchAngleTargetProgress >= 1) {
            pitchAngleTargetProgress = 0;
            pitchAngleTargetStart    = 10;
            pitchAngleTargetStop     = 150;
        } else if (remoteData.rx < -500 && pitchAngleTargetProgress >= 1) {
            pitchAngleTargetProgress = 0;
            pitchAngleTargetStart    = 150;
            pitchAngleTargetStop     = 10;
        }

        pitchAngleTarget = RAMP(pitchAngleTargetStart, pitchAngleTargetStop, pitchAngleTargetProgress);

        // 计算PID
        PID_Calculate(&PID_Fetch_X, xSpeedTarget, xSpeed);
        PID_Calculate(&PID_Fetch_Pitch_Left, -1 * pitchAngleTarget, pitchLeftAngle);
        PID_Calculate(&PID_Fetch_Pitch_Right, pitchAngleTarget, pitchRightAngle);

        if (GoMode == 1) {
            GO_ON;
        } else if (GoMode == 0) {
            GO_OFF;
        }

        if (GetMode == 1) {
            GET_ON;
        } else if (GetMode == 0) {
            GET_OFF;
        }

        // 输出电流
        if (ControlMode != 1) {

            Can_Send(CAN2, 0x200, PID_Fetch_X.output, PID_Fetch_Pitch_Left.output, PID_Fetch_Pitch_Right.output, 0);
        }

        // 更新频率
        vTaskDelayUntil(&LastWakeTime, intervalms);

        // 调试信息
        DebugData.debug1 = GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1);
        DebugData.debug2 = GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_3);
        // DebugData.debug4 = pitchAngleTargetProgress * 1000;
        // DebugData.debug5 = Ps.autoaimData.seq;
        // DebugData.debug6 = Ps.autoaimData.yaw_angle_diff;
        // DebugData.debug7 = Ps.autoaimData.pitch_angle_diff;
    }

    vTaskDelete(NULL);
}
void Task_Up(void *Parameter) {
    TickType_t LastWakeTime = xTaskGetTickCount();

    float upthrowAngleTarget = 0;
    float upthrowProgress    = 0;
    float rampStart          = 0;
    int   lastState          = 0; // 上一次抬升机构位置

    PID_Init(&PID_Upthrow1_Angle, 18, 0.015, 0, 340, 170);   // 18 0.015
    PID_Init(&PID_Upthrow1_Speed, 30, 1, 0, 10000, 5000);    // 30 1
    PID_Init(&PID_Upthrow2_Angle, 24, 0.02, 0, 290, 145);    // 24 0.018
    PID_Init(&PID_Upthrow2_Speed, 40, 0.85, 0, 10000, 5000); // 35 0.5

    while (1) {
        if (UpMode == 1) {
            if (lastState != 1) {
                upthrowProgress = 0;
                rampStart       = Motor_Upthrow1.angle;
            }
            upthrowAngleTarget = RAMP(rampStart, 375, upthrowProgress);
            if (upthrowProgress < 1) {
                upthrowProgress += 0.05f;
            }
            lastState = 1;
        } else if (UpMode == 0) {
            if (lastState != 0) {
                upthrowProgress = 0;
                rampStart       = Motor_Upthrow1.angle;
            }
            upthrowAngleTarget = RAMP(rampStart, 0, upthrowProgress);
            if (upthrowProgress < 1) {
                upthrowProgress += 0.05f;
                // upthrowProgress += 1.0f;
            }
            lastState = 0;
        }
        // 主从控制
        PID_Calculate(&PID_Upthrow1_Angle, upthrowAngleTarget, Motor_Upthrow1.angle);
        PID_Calculate(&PID_Upthrow1_Speed, PID_Upthrow1_Angle.output, Motor_Upthrow1.speed * RPM2RPS);
        PID_Calculate(&PID_Upthrow2_Angle, -Motor_Upthrow1.angle, Motor_Upthrow2.angle);
        PID_Calculate(&PID_Upthrow2_Speed, PID_Upthrow2_Angle.output, Motor_Upthrow2.speed * RPM2RPS);

        Can_Send(CAN1, 0x1FF, PID_Upthrow1_Speed.output, PID_Upthrow2_Speed.output, 0, 0);

        vTaskDelayUntil(&LastWakeTime, 10);
    }

    vTaskDelete(NULL);
}
void Task_Client_Communication(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount(); // 时钟
    float      interval     = 0.1;                 // 任务运行间隔 s
    int        intervalms   = interval * 1000;     // 任务运行间隔 ms

    while (1) {
        int      index = 0;
        uint16_t dataLength;

        while (DMA_GetFlagStatus(DMA2_Stream6, DMA_IT_TCIF6) != SET) {
        }
        DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6);
        DMA_Cmd(DMA2_Stream6, DISABLE);

        switch (Judge.mode) {
        case MODE_CLIENT_DATA: {
            // 客户端自定义数据
            Judge.clientCustomData.data_cmd_id = Protocol_Interact_Id_Client_Data;
            Judge.clientCustomData.send_id     = Judge.robotState.robot_id;
            Judge.clientCustomData.receiver_id = (Judge.clientCustomData.send_id % 10) | (Judge.clientCustomData.send_id / 10) << 4 | (0x01 << 8);

            Judge.clientCustomData.data1 = 1;
            Judge.clientCustomData.data2 = 1.1;
            Judge.clientCustomData.data3 = 1.11;
            Judge.clientCustomData.masks = 0x3c;

            dataLength = Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Data;

            Protocol_Pack(&Judge, dataLength, Protocol_Interact_Id_Client_Data);

            Judge.mode = MODE_ROBOT_INTERACT;
        } break;

        case MODE_ROBOT_INTERACT: {
            // 机器人间通信
            Judge.robotInteractiveData[0].data_cmd_id = 0x0200;
            Judge.robotInteractiveData[0].send_id     = Judge.robotState.robot_id;
            Judge.robotInteractiveData[0].receiver_id = 1;

            Judge.robotInteractiveData[0].transformer[index++].F = 1;
            Judge.robotInteractiveData[0].transformer[index++].F = 1.1;
            Judge.robotInteractiveData[0].transformer[index++].F = 1.11;
            Judge.robotInteractiveData[0].transformer[index++].F = 1.111;

            dataLength = Protocol_Pack_Length_0301_Header + index * sizeof(float);

            Protocol_Pack(&Judge, dataLength, 0x0200);

            Judge.mode = MODE_CLIENT_GRAPH;
        } break;

        case MODE_CLIENT_GRAPH: {
            // 客户端自定义图形
            Judge.clientGraphicDraw.data_cmd_id = Protocol_Interact_Id_Client_Graph;
            Judge.clientGraphicDraw.send_id     = Judge.robotState.robot_id;
            Judge.clientGraphicDraw.receiver_id = (Judge.clientCustomData.send_id % 10) | (Judge.clientCustomData.send_id / 10) << 4 | (0x01 << 8);

            Judge.clientGraphicDraw.operate_tpye = 1; // 0:空操作 1:增加图形 2:修改图形 3:删除单个图形 5:删除一个图层 6:删除所有图形
            Judge.clientGraphicDraw.graphic_tpye = 3; // 0:空形 1:直线 2:矩形 3:正圆 4:椭圆 5:弧形 6:文本（ASCII 字码）
            Judge.clientGraphicDraw.layer        = 5; // 图层0-9
            Judge.clientGraphicDraw.width        = 4; // 线宽
            Judge.clientGraphicDraw.color        = 4; // 官方 0:红/蓝 1:黄 2:绿 3:橙 4:紫 5:粉 6:青 7:黑 8:白
                                                      // 自测 0:红 1:橙 2:黄 3:绿 4:青 5:蓝 6:紫 7:粉 8:黑

            Judge.clientGraphicDraw.graphic_name[0] = 0;
            Judge.clientGraphicDraw.graphic_name[1] = 0;
            Judge.clientGraphicDraw.graphic_name[2] = 0;
            Judge.clientGraphicDraw.graphic_name[3] = 0;
            Judge.clientGraphicDraw.graphic_name[4] = 1;

            Judge.clientGraphicDraw.start_x = 960;
            Judge.clientGraphicDraw.start_y = 540;
            Judge.clientGraphicDraw.radius  = 100;

            dataLength = Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Graph;

            Protocol_Pack(&Judge, dataLength, Protocol_Interact_Id_Client_Graph);

            Judge.mode = MODE_CLIENT_DATA;
        } break;

        default:
            break;
        }

        DMA_SetCurrDataCounter(DMA2_Stream6, PROTOCOL_HEADER_CRC_CMDID_LEN + dataLength);
        DMA_Cmd(DMA2_Stream6, ENABLE);

        // 发送频率
        vTaskDelayUntil(&LastWakeTime, intervalms);

        // 调试信息
        // DebugData.debug1 = Judge.robotInteractiveData[1].transformer[0].F * 1000;
        // DebugData.debug2 = Judge.robotInteractiveData[1].transformer[1].F * 1000;
        // DebugData.debug3 = Judge.robotInteractiveData[1].transformer[2].F * 1000;
        // DebugData.debug4 = Judge.robotInteractiveData[1].transformer[3].F * 1000;
    }
    vTaskDelete(NULL);
}

void Task_Vision_Communication(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount(); // 时钟
    float      interval     = 0.1;                 // 任务运行间隔 s
    int        intervalms   = interval * 1000;     // 任务运行间隔 ms

    while (1) {
        int      index = 0;
        uint16_t dataLength;

        while (DMA_GetFlagStatus(DMA1_Stream0, DMA_IT_TCIF0) != SET) {
        }
        DMA_ClearFlag(DMA1_Stream0, DMA_FLAG_TCIF0);
        DMA_Cmd(DMA1_Stream0, DISABLE);

        // 视觉通信
        Ps.visionInteractiveData.transformer[index].U16[1]   = 0x6666;
        Ps.visionInteractiveData.transformer[index++].U16[2] = 0x6666;

        dataLength = index * sizeof(float);

        Protocol_Pack(&Ps, dataLength, Protocol_Interact_Id_Vision);

        DMA_SetCurrDataCounter(DMA1_Stream0, index);
        DMA_Cmd(DMA1_Stream0, ENABLE);

        // 发送频率
        vTaskDelayUntil(&LastWakeTime, intervalms);
    }
    vTaskDelete(NULL);
}

void Task_Debug_Magic_Send(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();
    while (1) {
        taskENTER_CRITICAL(); // 进入临界段
        printf("Yaw: %f \r\n", Gyroscope_EulerData.yaw);
        taskEXIT_CRITICAL(); // 退出临界段
        vTaskDelayUntil(&LastWakeTime, 500);
    }
    vTaskDelete(NULL);
}

void Task_Blink(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();
    while (1) {
        LED_Run_Horse_XP(); // XP开机动画,建议延时200ms
        // LED_Run_Horse(); // 跑马灯,建议延时20ms
        vTaskDelayUntil(&LastWakeTime, 200);
    }

    vTaskDelete(NULL);
}

void Task_Startup_Music(void *Parameters) {
    TickType_t LastWakeTime = xTaskGetTickCount();
    while (1) {
        if (KTV_Play(Music_XP)) break;
        vTaskDelayUntil(&LastWakeTime, 150);
    }
    vTaskDelete(NULL);
}

void Task_Sys_Init(void *Parameters) {

    // 初始化全局变量
    Handle_Init();

    // 初始化硬件
    BSP_Init();

    // 初始化陀螺仪
    Gyroscope_Init(&Gyroscope_EulerData);

    // 调试任务
#if DEBUG_ENABLED
    // xTaskCreate(Task_Debug_Magic_Receive, "Task_Debug_Magic_Receive", 500, NULL, 6, NULL);
    // xTaskCreate(Task_Debug_Magic_Send, "Task_Debug_Magic_Send", 500, NULL, 6, NULL);
    // xTaskCreate(Task_Debug_RTOS_State, "Task_Debug_RTOS_State", 500, NULL, 6, NULL);
    // xTaskCreate(Task_Debug_Gyroscope_Sampling, "Task_Debug_Gyroscope_Sampling", 400, NULL, 6, NULL);
#endif

    // 低级任务
    // xTaskCreate(Task_Safe_Mode, "Task_Safe_Mode", 500, NULL, 7, NULL);
    xTaskCreate(Task_Blink, "Task_Blink", 400, NULL, 3, NULL);
    xTaskCreate(Task_Startup_Music, "Task_Startup_Music", 400, NULL, 3, NULL);

    // 等待遥控器开启
    while (!remoteData.state) {
    }

    //模式切换任务
    xTaskCreate(Task_Control, "Task_Control", 400, NULL, 9, NULL);

    // 运动控制任务
    xTaskCreate(Task_Chassis, "Task_Chassis", 400, NULL, 3, NULL);
    xTaskCreate(Task_Fetch, "Task_Fetch", 400, NULL, 3, NULL);
    xTaskCreate(Task_Up, "Task_Up", 400, NULL, 3, NULL); // 抬升

    // DMA发送任务
    // xTaskCreate(Task_Client_Communication, "Task_Client_Communication", 500, NULL, 6, NULL);
    // xTaskCreate(Task_Vision_Communication, "Task_Vision_Communication", 500, NULL, 6, NULL);

    // 完成使命
    vTaskDelete(NULL);
}
