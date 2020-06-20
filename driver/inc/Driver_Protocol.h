#ifndef __DRIVER_PROTOCOL_H
#define __DRIVER_PROTOCOL_H

#include "stm32f4xx.h"

#pragma pack(1)

#define Protocol_Buffer_Length 128

#define Protocol_Interact_Id_Client_Delete 0x0100
#define Protocol_Interact_Id_Client_Single 0x0101
#define Protocol_Interact_Id_Client_Double 0x0102
#define Protocol_Interact_Id_Client_Five 0x0103
#define Protocol_Interact_Id_Client_Seven 0x0104
#define Protocol_Interact_Id_Client_Character 0x0110
#define Protocol_Interact_Id_Board 0x0302
#define Protocol_Interact_Id_Vision 0x0402

#define Protocol_Pack_Length_0001 3
#define Protocol_Pack_Length_0002 1
#define Protocol_Pack_Length_0003 32
#define Protocol_Pack_Length_0004 3
#define Protocol_Pack_Length_0005 3
#define Protocol_Pack_Length_0101 4
#define Protocol_Pack_Length_0102 4
#define Protocol_Pack_Length_0104 2
#define Protocol_Pack_Length_0105 1
#define Protocol_Pack_Length_0201 18
#define Protocol_Pack_Length_0202 16
#define Protocol_Pack_Length_0203 16
#define Protocol_Pack_Length_0204 1
#define Protocol_Pack_Length_0205 3
#define Protocol_Pack_Length_0206 1
#define Protocol_Pack_Length_0207 6
#define Protocol_Pack_Length_0208 2
#define Protocol_Pack_Length_0209 4
#define Protocol_Pack_Length_020A 12
#define Protocol_Pack_Length_0301_Client_Delete 2
#define Protocol_Pack_Length_0301_Client_Single 15
#define Protocol_Pack_Length_0301_Client_Double 30
#define Protocol_Pack_Length_0301_Client_Five 75
#define Protocol_Pack_Length_0301_Client_Seven 105
#define Protocol_Pack_Length_0301_Client_Character 45
#define Protocol_Pack_Length_0301_Header 6
#define Protocol_Pack_Length_0301_Robot 112
#define Protocol_Pack_Length_0302 20
#define Protocol_Pack_Length_0401 9
#define Protocol_Pack_Length_0402 32

#define PROTOCOL_HEADER 0xA5
#define PROTOCOL_HEADER_SIZE 5
#define PROTOCOL_CMD_SIZE 2
#define PROTOCOL_CRC16_SIZE 2
#define PROTOCOL_HEADER_CRC_LEN (PROTOCOL_HEADER_SIZE + PROTOCOL_CRC16_SIZE)
#define PROTOCOL_HEADER_CRC_CMDID_LEN (PROTOCOL_HEADER_SIZE + PROTOCOL_CRC16_SIZE + PROTOCOL_CMD_SIZE)
#define PROTOCOL_HEADER_CMDID_LEN (PROTOCOL_HEADER_SIZE + PROTOCOL_CMD_SIZE)

typedef union {
    uint8_t  U8[4];
    uint16_t U16[2];
    uint32_t U32;
    float    F;
    int      I;
} format_trans_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            uint8_t  robot_id;
            uint8_t  robot_level;
            uint16_t remain_HP;
            uint16_t max_HP;
            uint16_t shooter_heat0_cooling_rate;
            uint16_t shooter_heat0_cooling_limit;
            uint16_t shooter_heat1_cooling_rate;
            uint16_t shooter_heat1_cooling_limit;
            uint8_t  shooter_heat0_speed_limit;
            uint8_t  shooter_heat1_speed_limit;
            uint8_t  max_chassis_power;
            uint8_t  mains_power_gimbal_output : 1;
            uint8_t  mains_power_chassis_output : 1;
            uint8_t  mains_power_shooter_output : 1;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0201];
        };
    };
} ext_game_robot_state_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            uint16_t chassis_volt;
            uint16_t chassis_current;
            float    chassis_power;
            uint16_t chassis_power_buffer;
            uint16_t shooter_heat0;
            uint16_t shooter_heat1;
            uint16_t mobile_shooter_heat2;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0202];
        };
    };
} ext_power_heat_data_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            uint16_t energy_point;
            uint8_t  attack_time;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0205];
        };
    };
} aerial_robot_energy_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            uint8_t armor_id : 4;
            uint8_t hurt_type : 4;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0206];
        };
    };
} ext_robot_hurt_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            uint8_t bullet_type;
            uint8_t bullet_freq;
            float   bullet_speed;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0207];
        };
    };
} ext_shoot_data_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            float   yaw_angle_diff;
            float   pitch_angle_diff;
            uint8_t biu_biu_state;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0401];
        };
    };
} ext_gimbal_aim_data_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            float data1;
            float data2;
            float data3;
            float data4;
            float data5;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0302];
        };
    };
} board_interactive_data_t;

typedef struct {
    uint16_t seq;
    union {
        struct {
            uint16_t       data_cmd_id;
            uint16_t       send_id;
            uint16_t       receiver_id;
            format_trans_t transformer[Protocol_Pack_Length_0301_Robot / sizeof(float)];
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Robot];
        };
    };
} robot_interactive_data_t;

typedef struct {
    union {
        struct {
            uint16_t data_cmd_id;
            uint16_t send_id;
            uint16_t receiver_id;
            uint8_t  operate_tpye;
            uint8_t  layer;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Delete];
        };
    };
} client_custom_graphic_delete_t;

typedef struct {
    uint8_t  graphic_name[3];
    uint32_t operate_tpye : 3;
    uint32_t graphic_tpye : 3;
    uint32_t layer : 4;
    uint32_t color : 4;
    uint32_t start_angle : 9;
    uint32_t end_angle : 9;
    uint32_t width : 10;
    uint32_t start_x : 11;
    uint32_t start_y : 11;
    uint32_t radius : 10;
    uint32_t end_x : 11;
    uint32_t end_y : 11;
} graphic_data_struct_t;

typedef struct {
    union {
        struct {
            uint16_t              data_cmd_id;
            uint16_t              send_id;
            uint16_t              receiver_id;
            graphic_data_struct_t grapic_data_struct;
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Single];
        };
    };
} client_custom_graphic_single_t;

typedef struct {
    union {
        struct {
            uint16_t              data_cmd_id;
            uint16_t              send_id;
            uint16_t              receiver_id;
            graphic_data_struct_t grapic_data_struct[2];
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Double];
        };
    };
} client_custom_graphic_double_t;

typedef struct {
    union {
        struct {
            uint16_t              data_cmd_id;
            uint16_t              send_id;
            uint16_t              receiver_id;
            graphic_data_struct_t grapic_data_struct[5];
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Five];
        };
    };
} client_custom_graphic_five_t;

typedef struct {
    union {
        struct {
            uint16_t              data_cmd_id;
            uint16_t              send_id;
            uint16_t              receiver_id;
            graphic_data_struct_t grapic_data_struct[7];
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Seven];
        };
    };
} client_custom_graphic_seven_t;

typedef struct {
    union {
        struct {
            uint16_t              data_cmd_id;
            uint16_t              send_id;
            uint16_t              receiver_id;
            graphic_data_struct_t grapic_data_struct;
            uint8_t               data[30];
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0301_Header + Protocol_Pack_Length_0301_Client_Character];
        };
    };
} client_custom_character_t;

typedef struct {
    union {
        struct {
            format_trans_t transformer[Protocol_Pack_Length_0402 / sizeof(float)];
        };
        struct {
            uint8_t data[Protocol_Pack_Length_0402];
        };
    };
} vision_interactive_data_t;

typedef struct {
    uint8_t  sof;
    uint16_t data_length;
    uint8_t  seq;
    uint8_t  crc8;
} frame_header_t;

typedef enum {
    STEP_HEADER_SOF  = 0,
    STEP_LENGTH_LOW  = 1,
    STEP_LENGTH_HIGH = 2,
    STEP_FRAME_SEQ   = 3,
    STEP_HEADER_CRC8 = 4,
    STEP_DATA_CRC16  = 5,
} unpack_step_e;

typedef enum {
    STATE_IDLE = 0,
    STATE_WORK = 1,
} protocol_state_e;

typedef enum {
    MODE_CLIENT_DATA    = 0,
    MODE_ROBOT_INTERACT = 1,
    MODE_CLIENT_GRAPH   = 2,
} interact_mode_e;

typedef struct {
    uint8_t                        sendBuf[Protocol_Buffer_Length];    // DMA发送缓存
    uint8_t                        receiveBuf[Protocol_Buffer_Length]; // DMA接收缓存
    uint8_t                        packet[Protocol_Buffer_Length];     // 有效字节数组
    unpack_step_e                  step;                               // 当前解包步骤
    protocol_state_e               state;                              // 当前工作状态
    uint16_t                       index;                              // 当前包字节序
    uint16_t                       dataLength;                         // 包数据长度
    uint16_t                       seq;                                // 包序号
    uint16_t                       id;                                 // 包编号
    uint64_t                       lost;                               // 包丢失计数
    uint64_t                       received;                           // 包接收计数
    ext_game_robot_state_t         robotState;                         // 状态数据
    ext_power_heat_data_t          powerHeatData;                      // 热量数据
    aerial_robot_energy_t          aerialRobotEnergy;                  // 空中机器人数据
    ext_robot_hurt_t               robotHurt;                          // 伤害数据
    ext_shoot_data_t               shootData;                          // 射击数据
    ext_gimbal_aim_data_t           autoaimData;                        // 视觉自瞄数据
    interact_mode_e                mode;                               // 当前交互模式
    board_interactive_data_t       boardInteractiveData[2];            // 板间交互数据 0：发送 1：接收
    client_custom_graphic_delete_t clientGraphicDelete;                // 客户端自定义删除图形
    client_custom_graphic_single_t clientGraphicSingle;                // 客户端自定义绘制一个图形
    client_custom_graphic_double_t clientGraphicDouble;                // 客户端自定义绘制两个图形
    client_custom_graphic_five_t   clientGraphicFive;                  // 客户端自定义绘制五个图形
    client_custom_graphic_seven_t  clientGraphicSeven;                 // 客户端自定义绘制七个图形
    client_custom_character_t      clientGraphicCharacter;             // 客户端自定义绘制字符
    robot_interactive_data_t       robotInteractiveData[8];            // 车间交互数据 0：发送 1-7：接收
    vision_interactive_data_t      visionInteractiveData;              // 视觉交互数据
} Protocol_Type;

unsigned char Get_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength, unsigned char ucCRC8);
unsigned int  Verify_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength);
void          Append_CRC8_Check_Sum(unsigned char *pchMessage, unsigned int dwLength);
uint16_t      Get_CRC16_Check_Sum(uint8_t *pchMessage, uint32_t dwLength, uint16_t wCRC);
uint32_t      Verify_CRC16_Check_Sum(uint8_t *pchMessage, uint32_t dwLength);
void          Append_CRC16_Check_Sum(uint8_t *pchMessage, uint32_t dwLength);

void Protocol_Init(Protocol_Type *Protocol);
void Protocol_Update(Protocol_Type *Protocol);
void Protocol_Unpack(Protocol_Type *Protocol, uint8_t byte);
void Protocol_Load(Protocol_Type *Protocol);
void Protocol_Pack(Protocol_Type *Protocol, uint16_t dataLength, uint16_t id);

#endif
