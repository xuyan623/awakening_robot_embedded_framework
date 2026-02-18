#ifndef __HAL_CAN_CORE_H
#define __HAL_CAN_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "core/data_struct/bitmap.h"
#include "core/data_struct/corelist.h"
#include "drivers/model/device.h"
#include "osal/osal_timer.h"

/* CAN 数据帧长度*/
typedef enum {
    CAN_DLC_0 = 0U, // 0 字节
    CAN_DLC_1 = 1U,
    CAN_DLC_2,
    CAN_DLC_3,
    CAN_DLC_4,
    CAN_DLC_5,
    CAN_DLC_6,
    CAN_DLC_7,
    CAN_DLC_8,
    /* 以下仅用于 CAN FD*/
    CAN_DLC_12 = 9U,
    CAN_DLC_16,
    CAN_DLC_20,
    CAN_DLC_24,
    CAN_DLC_32,
    CAN_DLC_48,
    CAN_DLC_64,
} CanDlc_e;

/* CAN 同步跳转宽度 */
typedef enum {
    CAN_SYNCJW_1TQ = 0U,
    CAN_SYNCJW_2TQ,
    CAN_SYNCJW_3TQ,
    CAN_SYNCJW_4TQ
} CanSjw_e;

typedef enum {
    CAN_TSEG1_1TQ = 0U,
    CAN_TSEG1_2TQ,
    CAN_TSEG1_3TQ,
    CAN_TSEG1_4TQ,
    CAN_TSEG1_5TQ,
    CAN_TSEG1_6TQ,
    CAN_TSEG1_7TQ,
    CAN_TSEG1_8TQ,
    CAN_TSEG1_9TQ,
    CAN_TSEG1_10TQ,
    CAN_TSEG1_11TQ,
    CAN_TSEG1_12TQ,
    CAN_TSEG1_13TQ,
    CAN_TSEG1_14TQ,
    CAN_TSEG1_15TQ,
    CAN_TSEG1_16TQ,
    CAN_TSEG1_MAX,
} CanBS1_e;

typedef enum {
    CAN_TSEG2_1TQ = 0U,
    CAN_TSEG2_2TQ,
    CAN_TSEG2_3TQ,
    CAN_TSEG2_4TQ,
    CAN_TSEG2_5TQ,
    CAN_TSEG2_6TQ,
    CAN_TSEG2_7TQ,
    CAN_TSEG2_8TQ,
    CAN_TSEG2_MAX,
} CanBS2_e;

typedef enum {
    CAN_BAUD_10K  = 50000U,
    CAN_BAUD_20K  = 250000U,
    CAN_BAUD_50K  = 500000U,
    CAN_BAUD_100K = 1000000U,
    CAN_BAUD_125K = 1250000U,
    CAN_BAUD_250K = 2500000U,
    CAN_BAUD_500K = 5000000U,
    CAN_BAUD_800K = 8000000U,
    CAN_BAUD_1M   = 10000000U,
} CanBaudRate_e;

/* CAN 工作模式 */
typedef enum {
    CAN_WORK_NORMAL = 0U,     // 正常模式：向总线发送，从总线接收
    CAN_WORK_LOOPBACK,        // 环回模式：向总线和本机发送，不接受总线信号
    CAN_WORK_SILENT,          // 静默模式：自发自收，不向总线发送，但接收总线信号
    CAN_WORK_SILENT_LOOPBACK, // 静默环回模式：自发送自收，不向总线发送，不接收总线信号
} CanWorkMode_e;

/* CAN 节点错误状态*/
typedef enum can_node_status {
    CAN_NODE_STATUS_ACTIVE,  // CAN 主动错误
    CAN_NODE_STATUS_PASSIVE, // CAN 被动错误
    CAN_NODE_STATUS_BUSOFF,  // CAN 离线
} CanNodeErrStatus_e;

/* CAN 滤波器模式*/
typedef enum {
    CAN_FILTER_MODE_LIST, // 滤波器模式 列表模式
    CAN_FILTER_MODE_MASK, // 滤波器模式 掩码模式
} CanFilterMode_e;

/* CAN ID标识*/
typedef enum {
    CAN_IDE_STD = 0U,
    CAN_IDE_EXT,
} CanIdType_e;

typedef enum {
    CAN_FILTER_ID_STD = 0U, // 仅过滤标准ID
    CAN_FILTER_ID_EXT,      // 仅过滤扩展ID
    CAN_FILTER_ID_STD_EXT,  // 过滤标准ID和扩展ID
} CanFilterIdType_e;

/* CAN报文类型 */
typedef enum {
    CAN_MSG_TYPE_DATA = 0U, // 数据报文
    CAN_MSG_TYPE_REMOTE     // 远程报文
} CanMsgType_e;

/* CANFD报文波特率变化*/
typedef enum {
    CANFD_MSG_BRS_OFF = 0U,
    CANFD_MSG_BRS_ON
} CanFdMsgBrs_e;

typedef enum {
    CAN_MSG_CLASS = 0U,
    CAN_MSG_FD,
} CanFdMsgFormat_e;

typedef enum {
    CANFD_MSG_ESI_ACITVE,
    CANFD_MSG_ESI_PASSIVE,
} CanFdESI_e;

/* CAN接收侧软件错误状态*/
typedef enum {
    CAN_ERR_NONE = 0U,
    CAN_ERR_FILTER_EMPTY, // 滤波器为空
    CAN_ERR_FILTER_BANK,  // 滤波器编号错误
    CAN_ERR_UNKNOWN,      // 未知错误

    // FIFO相关错误
    CAN_ERR_FIFO_EMPTY,
    CAN_ERR_SOFT_FIFO_OVERFLOW, // FIFO溢出

} CanErrorCode_e;

/* CAN中断事件  */
typedef enum {
    CAN_ISR_EVENT_INT_RX_DONE = 0U, // 接收中断
    CAN_ISR_EVENT_INT_TX_DONE,      // 发送中断
} CanIsrEvent_e;

typedef enum {
    CAN_ERR_EVENT_NONE,
    CAN_ERR_EVENT_RX_OVERFLOW      = 1U,      // 接收FIFO溢出
    CAN_ERR_EVENT_TX_FAIL          = 1U << 1, // 发送失败
    CAN_ERR_EVENT_ARBITRATION_FAIL = 1U << 2, // 仲裁失败
    CAN_ERR_EVENT_BUS_STATUS       = 1U << 3, // 总线状态改变，主动->被动->离线

    // 错误计数转移到硬件层执行

    // CAN_ERR_EVENT_CRC_ERROR = 1U << 4,        // CRC错误
    // CAN_ERR_EVENT_FORMAT_ERROR = 1U << 5,     // 格式错误
    // CAN_ERR_EVENT_STUFF_ERROR = 1U << 6,      // 位填充错误
    // CAN_ERR_EVENT_BITDOMINANT_ERROR = 1U << 7, // 位显性错误
    // CAN_ERR_EVENT_BITRECESSIVE_ERROR = 1U << 8, // 位隐性错误
    // CAN_ERR_EVENT_ACK_ERROR = 1U << 9,        // 确认错误
} CanErrEvent_e;

typedef uint16_t CanFilterHandle_t;

/**
 * @defgroup CAN_TX_MODE_DEF
 *  @brief CAN发送模式定义
 *  @{
 */
#define CAN_TX_MODE_UNOVERWRTING (0U) // 不覆写模式，即不覆盖老数据
#define CAN_TX_MODE_OVERWRITING  (1U) // 覆写模式，即覆盖老数据
/**
 * @defgroup CAN_TX_MODE_DEF
 * @}
 */

/**
 * @defgroup CAN_FILTER_CFG_DEF
 * @brief CAN滤波器配置参数定义
 * @{
 */
#define CAN_FILTER_REQUEST_INIT(_mode, _idType, _id, _mask, _rx_callback, _param)                                          \
    (CanFilterRequest_s)                                                                                                    \
    {                                                                                                                       \
        .workMode = _mode, .idType = _idType, .id = _id, .mask = _mask, .rx_callback = _rx_callback, .param = _param \
    }
/**
 * @defgroup CAN_FILTER_CFG_DEF
 * @}
 */

/**
 * @defgroup CAN_REG_DEF
 * @brief CAN收发 注册参数定义
 * @{
 */
#define CAN_REG_INT_TX        DEVICE_REG_INT_TX     // 中断发送
#define CAN_REG_INT_RX        DEVICE_REG_INT_RX     // 中断接收
#define CAN_REG_IS_STANDALONG DEVICE_REG_STANDALONG // CAN 独占设备
/**
 * @defgroup CAN_REG_DEF
 * @}
 */

/**
 * @brief CAN打开参数定义
 * @{
 */
#define CAN_O_INT_TX DEVICE_O_INT_TX // 中断发送
#define CAN_O_INT_RX DEVICE_O_INT_RX // 中断接收
                                     /**
                                      * @brief CAN打开参数定义
                                      * @}
                                      */

/**
 * @defgroup CAN_CMD_DEF
 * @brief CAN命令定义
 * @{
 */
#define CAN_CMD_CFG                  (0x00U) // 配置CAN
#define CAN_CMD_SUSPEND              (0x01U) // 暂停CAN
#define CAN_CMD_RESUME               (0x02U) // 恢复CAN
#define CAN_CMD_SET_IOTYPE           (0x03U) // 设置CAN IO类型，使能中断等
#define CAN_CMD_CLR_IOTYPE           (0x04U) // 清除CAN IO类型，关闭中断等
#define CAN_CMD_CLOSE                (0x05U) // 关闭CAN
#define CAN_CMD_FLUSH                (0x06U) // 清空缓存
#define CAN_CMD_FILTER_ALLOC         (0x07U) // 申请并激活滤波器
#define CAN_CMD_FILTER_FREE          (0x08U) // 释放滤波器
#define CAN_CMD_START                (0x09U) // 启动CAN
#define CAN_CMD_GET_STATUS           (0x0AU) // 获取 CAN 状态
#define CAN_CMD_GET_CAPABILITY       (0x0BU) // 获取CAN硬件能力
/**
 * @defgroup CAN_CMD_DEF
 * @}
 */

/**
 * @defgroup CAN_MSG_DSC_STATIC_INIT_DEF
 * @brief CAN帧描述符初始化定义
 * @param id: CAN消息ID
 * @param idType: CAN消息ID类型
 * @param len: CAN数据长度
 * @return CanMsgDsc_s
 * @{
 */
/**
 * @brief CAN数据帧描述符初始化
 */
#define CAN_DATA_MSG_DSC_INIT(_id, _idType, _len)                                   \
    (CanMsgDsc_s)                                                                   \
    {                                                                               \
        .id = _id, .idType = _idType, .msgType = CAN_MSG_TYPE_DATA, .dataLen = _len \
    }

/**
 * @brief CAN远程帧描述符初始化
 */
#define CAN_REMOTE_MSG_DSC_INIT(_id, _idType, _len)                                   \
    (CanMsgDsc_s)                                                                     \
    {                                                                                 \
        .id = _id, .idType = _idType, .msgType = CAN_MSG_TYPE_REMOTE, .dataLen = _len \
    }
/**
 * @defgroup CAN_MSG_DSC_STATIC_INIT_DEF
 * @}
 */

typedef struct CanErrCounter *CanErrCounter_t;
typedef struct CanErrCounter {
    // 软件（逻辑/统计）相关
    size_t rxMsgCount;        // CAN接收报文数量
    size_t txMsgCount;        // CAN发送报文数量
    size_t rxSoftOverFlowCnt; // CAN接收软件FIFO溢出计数器，记录了软件FIFO溢出的次数
    size_t rxFailCnt;         // CAN接收失败计数器，包括所有软件和硬件接收失败的总次数
    size_t
        txSoftOverFlowCnt; // CAN发送软件FIFO溢出计数器，在覆写模式下代表的是被覆盖的消息数量，在非覆写模式下代表的是超出FIFO容量的消息数量
    size_t txFailCnt;      // CAN发送失败计数器，所有应该发送出去但是却没发出去的消息，都会触发txFail错误
    // CAN 硬件相关
    size_t txArbitrationFailCnt; // CAN发送仲裁失败计数器
    size_t rxHwOverFlowCnt;      // CAN接收硬件FIFO溢出计数器
    size_t txMailboxFullCnt;     // CAN发送硬件邮箱满计数器
    // CAN 规范中的错误，详情请参考CAN协议关于错误处理的说明
    size_t rxErrCnt;           // CAN接收错误计数器（REC）
    size_t txErrCnt;           // CAN发送错误计数器（TEC）
    size_t bitDominantErrCnt;  /**
                                * @brief CAN位显性错误计数器 当节点试图发送一个隐性位(逻辑
                                * 1)，但在总线上回读到的却为显性位(逻辑 0)时，就会触发这个错误
                                * @note 仲裁段和应答段允许发生该情况，但是在其他段就会触发该错误
                                * @note 触发条件：它通常代表着“有人捣乱”，如：
                                *       1. ID冲突（最常见） 2. 硬件干扰或短路
                                *       3. 波特率不匹配      4. 收发器损坏或供电不稳(请检查5V供电)
                                */
    size_t bitRecessiveErrCnt; /**
                                * @brief CAN位隐性错误计数器 当节点试图向总线发送一个显性位(逻辑
                                * 0)，但在回读总线时，读到的却为隐性位 (逻辑 1)时，就会触发这个错误
                                * @note 通常发生在Start Bit、Arbitration Field、Control Field 与 Data Field
                                * 中，触发该错误通常是硬件有问题，如:
                                *       1. 物理连接断路（最常见）  2. 收发器处于“静默”或“待机”模式
                                *       3. 收发器供电异常(请检查5V供电)         4. 硬件短路
                                *       5. GPIO 引脚配置错误
                                */
    size_t formatErrCnt;       // CAN格式错误计数器
    size_t crcErrCnt;          // CAN CRC 错误计数器
    size_t ackErrCnt;          // CAN 应答错误计数器
    size_t stuffErrCnt;        // CAN位填充错误计数器
} CanErrCounter_s;

/*
    CAN的每一个bit时间被细分为：SS段，Prop段，Phase1段，Phase2段
    其中SS段固定为1位，其余段可编程
    更多CAN知识详见https://www.canfd.net/canbasic.html
*/
typedef struct CanBitTime {
    CanBS1_e bs1;           // 传播段 + 相位缓冲段1
    CanBS2_e bs2;           // 相位缓冲段2
    CanSjw_e syncJumpWidth; // 同步跳转宽度，设置每次再同步的步长
} CanBitTime_s;

/*
    对于CAN而言，通常baudrate >= 500k时，采样位设置为0.8
    0 < baudrate <= 500k 时，采样率设置为0.75
*/
typedef struct CanTimeCfg *CanTimeCfg_t;
typedef struct CanTimeCfg {
    CanBaudRate_e baudRate;
    uint16_t psc; // 预分频系数
    CanBitTime_s bitTimeCfg;
} CanTimeCfg_s;

/*  CAN滤波器配置结构体 */
typedef struct CanFilterRequest *CanFilterRequest_t;
typedef struct CanFilterRequest {
    CanFilterMode_e workMode;
    CanFilterIdType_e idType;
    uint32_t id;
    uint32_t mask;
    void (*rx_callback)(Device_t dev, void *param, CanFilterHandle_t handle, size_t msgCount);
    void *param;
} CanFilterRequest_s;

typedef struct CanFilterAllocArg *CanFilterAllocArg_t;
typedef struct CanFilterAllocArg {
    CanFilterRequest_s request;
    CanFilterHandle_t handle;
} CanFilterAllocArg_s;

typedef struct CanHwFilterCfg *CanHwFilterCfg_t;
typedef struct CanHwFilterCfg {
    size_t bank;
    CanFilterMode_e workMode;
    CanFilterIdType_e idType;
    uint32_t id;
    uint32_t mask;
} CanHwFilterCfg_s;

/* CAN过滤器结构体 */
typedef struct CanFilter *CanFilter_t;
typedef struct CanFilter {
    CanFilterRequest_s request;              // CAN过滤器配置
    struct list_head msgMatchedList; // 匹配该过滤器的报文链表
    uint32_t msgCount;               // 该滤波器接收到的消息数量
    uint8_t isActived;               // 该滤波器是否激活
} CanFilter_s;

/**
 * @brief CAN报文描述符结构体
 * @note 该结构体用于描述CAN报文的基本信息，包括ID、ID类型、报文类型、数据长度、接收时间戳等
 * @note
 */
typedef struct CanMsgDsc *CanMsgDsc_t;
typedef struct CanMsgDsc {
    uint32_t id : 29;         // 帧ID
    CanIdType_e idType : 1;   // ID标识
    CanMsgType_e msgType : 1; // 报文类型
    uint32_t __reserved : 1;  // 保留位，凑满32位
    uint32_t dataLen;         // 数据长度，value can be @ref CanDlc_e
    uint32_t timeStamp;       // 报文接收/发送时间戳
} CanMsgDsc_s;

/* CAN 用户报文，面向用户*/
typedef struct CanUserMsg *CanUserMsg_t;
typedef struct CanUserMsg {
    CanMsgDsc_s dsc; /**
                      * @brief CAN报文描述符
                      * @ref CAN_MSG_DSC_STATIC_INIT_DEF
                      */
    CanFilterHandle_t filterHandle;    // 过滤器句柄/发送邮箱编号，-1 表示未绑定任何滤波器或发送邮箱
    /*
        数据指针，根据所处层级不同，有不同的含义
        应用层：
        1.
        CanUserMsg 作为 read 接口的 buf 参数时，应指向应用层缓存区，数据从 RX FIFO 容器拷贝到 pUserBuf 中。
        2.
        CanUserMsg 作为 write 接口的 data 参数时，应指向应用层数据源，数据从 pUserBuf 拷贝到 TX FIFO 容器中。
        框架层：
        1. CanUserMsg 作为框架层 msgListBuffer 中的 UserMsg，始终指向自身的 container 字段。
        硬件底层：
        CanUserMsg 作为硬件接收报文时，userBuf 指向硬件提供的数据地址，在 ISR 中转存到 RX FIFO 容器。
    */
    uint8_t *userBuf;
} CanUserMsg_s;

/* CAN 硬件消息，供 CAN core/BSP 使用 */
typedef struct CanHwMsg *CanHwMsg_t;
typedef struct CanHwMsg {
    CanMsgDsc_s dsc;
    int16_t hwFilterBank;
    int8_t hwTxMailbox;
    uint8_t *data;
} CanHwMsg_s;

/* CAN报文链表结构体*/
typedef struct CanMsgList *CanMsgList_t;
typedef struct CanMsgList {
    CanUserMsg_s userMsg;             // 用户层报文
    struct list_head fifoListNode;    // Fifo链表节点，链接RxFifo的used/free链表
    struct list_head matchedListNode; // 匹配链表节点，链接到滤波器匹配链表或是发送邮箱的fifoListNode
    void *owner;                      // 报文所属者，滤波器或发送邮箱
    uint8_t container[0];             // 柔性数组，将框架层实际内存（如CanMsgList_s和CanFdList_s）映射到container字段
} CanMsgList_s;

/* CAN接收FIFO结构体*/
typedef struct CanMsgFifo *CanMsgFifo_t;
typedef struct CanMsgFifo {
    void *msgBuffer;           // 报文缓冲区
    struct list_head usedList; // 已使用的报文列表
    struct list_head freeList; // 空闲的报文列表
    uint32_t freeCount;        // 空闲的报文数量
    uint8_t isOverwrite;       // FIFO 覆写策略：0 不覆写，1 覆写
} CanMsgFifo_s;

typedef struct CanMailbox *CanMailbox_t;
typedef struct CanMailbox {
    uint8_t isBusy;        // 发送邮箱是否已被使用
    CanMsgList_t pMsgList; // 发送邮箱中的消息链表项
    uint32_t bank;         // 发送邮箱编号
    struct list_head list; // 发送邮箱链表节点
} CanMailbox_s;

/* CAN 功能配置选项 */
// TODO: 待完善
typedef struct CanFunctionalCfg *CanFunctionalCfg_t;
typedef struct CanFunctionalCfg {
    uint16_t autoRetransmit : 1;    // 自动重传使能：0 禁用，1 使能
    uint16_t txPriority : 1;        // 发送优先级：0 基于 ID，1 基于请求顺序
    uint16_t rxFifoLockMode : 1;    // 接收锁定模式：0 不锁定，1 锁定新消息
    uint16_t timeTriggeredMode : 1; // 时间触发通信模式：0 禁用，1 使能
    uint16_t autoWakeUp : 1;        // 自动唤醒：0 禁用，1 使能
    uint16_t autoBusOff : 1;        // 自动总线离线：0 禁用，1 使能
    uint16_t isRxOverwrite : 1;     // 接收 FIFO 覆写策略：0 不覆写，1 覆写
    uint16_t isTxOverwrite : 1;     // 发送 FIFO 覆写策略：0 不覆写，1 覆写
    uint16_t rsv : 9;               // 保留位
} CanFunctionalCfg_s;

typedef struct CanCfg *CanCfg_t;
typedef struct CanCfg {
    CanWorkMode_e workMode;           // 工作模式
    CanTimeCfg_s normalTimeCfg;       // 波特率配置
    size_t filterNum;                 // CAN过滤器个数
    size_t mailboxNum;                // CAN发送邮箱个数，和硬件发送邮箱一一对应
    size_t rxMsgListBufSize;          // CAN接收FIFO消息缓冲区大小（单位：报文数量）
    size_t txMsgListBufSize;          // CAN发送FIFO消息缓冲区大小（单位：报文数量）
    uint32_t statusCheckTimeout;      // 检查超时时间（单位：毫秒）
    CanFunctionalCfg_s functionalCfg; // 功能配置选项
} CanCfg_s;

// CAN 默认配置
#define CAN_DEFUALT_CFG                                                                                                \
    (CanCfg_s)                                                                                                         \
    {                                                                                                                  \
        .workMode = CAN_WORK_NORMAL, .rxMsgListBufSize = 32, .txMsgListBufSize = 32, .mailboxNum = 3, .filterNum = 28, \
        .functionalCfg =                                                                                               \
            {                                                                                                          \
                .autoRetransmit    = 0,                                                                                \
                .autoBusOff        = 0,                                                                                \
                .autoWakeUp        = 0,                                                                                \
                .rxFifoLockMode    = 0,                                                                                \
                .timeTriggeredMode = 0,                                                                                \
                .txPriority        = 0,                                                                                \
                .isRxOverwrite     = 0,                                                                                \
                .isTxOverwrite     = 0,                                                                                \
                .rsv               = 0,                                                                                \
            },                                                                                                         \
        .normalTimeCfg =                                                                                               \
            {                                                                                                          \
                .baudRate = CAN_BAUD_1M,                                                                               \
            },                                                                                                         \
        .statusCheckTimeout = 10,                                                                                      \
    }


typedef struct CanHwCapability *CanHwCapability_t;
typedef struct CanHwCapability {
    uint8_t hwBankCount;
    const uint8_t *hwBankList;
} CanHwCapability_s;

typedef struct CanFilterResMgr *CanFilterResMgr_t;
typedef struct CanFilterResMgr {
    uint16_t slotCount;             // 过滤器槽位数量
    uint16_t maxHwBank;             // 最大硬件过滤器bank数量
    AwBitmapAtomic_s slotUsedMap;   // 过滤器槽位使用状态映射表
    AwBitmapAtomic_s hwBankUsedMap; // 硬件过滤器bank使用状态映射表
    uint8_t *hwBankList;            // 硬件过滤器bank列表
    int16_t *slotToHwBank;          // 过滤器槽位到硬件过滤器bank的映射表
} CanFilterResMgr_s;                // CAN过滤器资源管理结构体
typedef struct HalCanHandler *HalCanHandler_t;

typedef struct CanAdapterInterface *CanAdapterInterface_t;
typedef struct CanAdapterInterface {
    /**
     * @brief 分配FIFO的消息缓冲区，并链接到CAN接收FIFO的free链表
     * @param msgNum 消息缓冲区大小（单位：消息数量）
     * @return void* 指向分配的消息缓冲区的指针
     * @retval NULL 失败
     * @retval 非NULL 指向分配的消息缓冲区的指针
     */
    void *(*msgbuffer_alloc)(struct list_head *freeListHead, uint32_t msgNum);
} CanAdapterInterface_s;

typedef struct CanHwInterface *CanHwInterface_t;
typedef struct CanHwInterface {
    /**
     * @brief 配置CAN硬件
     * @param can CAN设备句柄
     * @param cfg CAN配置结构体指针
     * @return AwlfRet_e 配置结果
     * @retval AWLF_OK 成功
     * @retval AWLF_ERROR_PARAM 参数错误
     * @note 成功时，配置CAN硬件为指定的工作模式、波特率、过滤器、发送邮箱、接收FIFO等参数
     */
    AwlfRet_e (*configure)(HalCanHandler_t can, CanCfg_t cfg);
    /**
     * @brief 控制CAN硬件
     * @param can CAN设备句柄
     * @param cmd 控制命令 @ref CAN_CMD_DEF
     * @param arg 控制参数指针
     * @retval AWLF_OK 成功
     * @retval AWLF_ERROR_PARAM 参数错误
     * @note 成功时，根据cmd执行对应的控制操作，如启动、停止、设置参数等
     */
    AwlfRet_e (*control)(HalCanHandler_t can, uint32_t cmd, void *arg);
    /**
     * @brief 发送一帧 CAN 报文到硬件邮箱
     * @param can CAN设备句柄
     * @param msg CAN硬件消息结构体指针
     * @return AwlfRet_e 发送结果
     * @retval AWLF_OK 成功
     * @retval AWLF_ERROR_PARAM 参数错误
     * @retval AWLF_ERR_OVERFLOW CAN发送邮箱已满
     * @note 成功时，硬件应回填 `msg->hwTxMailbox`（实际发送邮箱编号）。
     */
    AwlfRet_e (*send_msg_mailbox)(HalCanHandler_t can, CanHwMsg_t msg);
    /**
     * @brief 指定CAN接收FIFO，将数据拷贝进msg->userBuf
     * @param can CAN设备句柄
     * @param msg CAN用户消息结构体指针
     * @param rxfifoBank 接收FIFO编号
     * @return AwlfRet_e 接收结果
     * @retval AWLF_OK 成功
     * @retval AWLF_ERROR_PARAM 参数错误
     * @retval AWLF_ERROR_EMPTY CAN接收FIFO为空
     * @retval AWLF_ERR_OVERFLOW CAN接收FIFO为空
     *
     * @note rxfifoBank 该值为bsp传入的接收FIFO编号
     * @note 若框架 RX FIFO 为空且不采用覆写策略，框架层会将 msg 设为 NULL；
     *       BSP 层应读取并丢弃当前帧以清中断。
     *
     */
    AwlfRet_e (*recv_msg)(HalCanHandler_t can, CanHwMsg_t msg, int32_t rxfifoBank);
} CanHwInterface_s;

/**
 * @brief CAN状态管理结构体
 */
typedef struct CanStatusManager *CanStatusManager_t;
typedef struct CanStatusManager {
    osal_timer_t statusTimer;         // 状态定时器句柄
    CanNodeErrStatus_e nodeErrStatus; // CAN节点错误状态
    CanErrCounter_s errCounter;
} CanStatusManager_s;

/* CAN接收处理结构体*/
typedef struct CanRxHandler *CanRxHandler_t;
typedef struct CanRxHandler {
    CanFilter_t filterTable; // CAN过滤器表
    CanMsgFifo_s rxFifo;     // CAN接收FIFO
} CanRxHandler_s;

/* CAN发送处理结构体 */
typedef struct CanTxHandler *CanTxHandler_t;
typedef struct CanTxHandler {
    CanMailbox_t pMailboxes;      // 发送邮箱数组指针
    CanMsgFifo_s txFifo;          // 目前只有一个发送FIFO
    struct list_head mailboxList; // 可用的发送邮箱链表
} CanTxHandler_s;

/* CAN设备句柄结构体*/
// TODO: CAN 架构是典型的单生产者多消费者模型，可在后续重构为无锁队列架构
// 遵循make it work first, then make it right, finnally make it fast 的原则，暂时先采用锁机制
typedef struct HalCanHandler {
    Device_s parent;                        // 父设备
    CanRxHandler_s rxHandler;               // CAN接收处理
    CanTxHandler_s txHandler;               // CAN发送处理
    CanStatusManager_s statusManager;       // CAN状态管理，TODO: 加入状态管理相关机制
    CanFilterResMgr_s filterResMgr;         // CAN 过滤器资源管理器
    CanHwInterface_t hwInterface;           // CAN硬件接口
    CanAdapterInterface_t adapterInterface; // CAN适配器接口，屏蔽CAN、CANFD 的差异
    CanCfg_s cfg;                           // 配置结构体
} HalCanHandler_s;

#define IS_CAN_FILTER_INVALID(pCan, handle)  ((handle) >= (pCan)->filterResMgr.slotCount)
#define IS_CAN_MAILBOX_INVALID(pCan, bank) ((bank) < 0 || (bank) >= (pCan)->cfg.mailboxNum)

/**
 * @brief 获取经典CAN的适配器接口
 * @return CanAdapterInterface_t 经典CAN的适配器接口指针
 */
CanAdapterInterface_t hal_can_get_classic_adapter_interface(void);

/**
 * @brief 获取CANFD的适配器接口
 * @return CanAdapterInterface_t CANFD的适配器接口指针
 */
CanAdapterInterface_t hal_can_get_canfd_adapter_interface(void);

/**
 * @brief CAN设备注册（兼容经典 CAN 和 CANFD）
 * @param can CAN设备句柄
 * @param name 设备名称
 * @param handle 硬件句柄
 * @param regparams 注册参数 @ref CAN_REG_DEF
 * @return AwlfRet_e 注册结果
 * @retval AWLF_OK 成功
 * @retval AWLF_ERROR_PARAM 参数错误
 * @retval AWLF_ERR_CONFLICT 设备名称冲突
 */
AwlfRet_e hal_can_register(HalCanHandler_t can, char *name, void *handle, uint32_t regparams);

/**
 * @brief CAN中断处理函数
 * @param Can CAN设备句柄
 * @param event 中断事件
 * @param param 中断参数
 * @return void
 * @note 中断参数的含义根据具体的CAN事件而不同
 * @note  1. CAN_ISR_EVENT_INT_RX_DONE: param 为接收到的 RX FIFO 索引
 * @note  2. CAN_ISR_EVENT_INT_TX_DONE: param为发送邮箱索引
 * @note  3. CAN_ISR_EVENT_INT_ERROR:   param为错误码
 */
void hal_can_isr(HalCanHandler_t Can, CanIsrEvent_e event, size_t param);

/**
 * @brief CAN错误中断处理函数
 * @param Can CAN设备句柄
 * @param errEvent 错误事件
 */
void can_error_isr(HalCanHandler_t Can, uint32_t errEvent, size_t param);

#ifdef __cplusplus
}
#endif

#endif


