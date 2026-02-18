#include "core/aw_def.h"
#include "core/data_struct/bitmap.h"
#include "drivers/peripheral/can/pal_can_dev.h"
#include "osal/osal_core.h"
#include <string.h>

static inline osal_irq_isr_state_t can_irq_lock(void)
{
    if (osal_is_in_isr())
        return osal_irq_lock_from_isr();

    osal_irq_lock_task();
    return (osal_irq_isr_state_t)0u;
}

static inline void can_irq_unlock(osal_irq_isr_state_t state)
{
    if (osal_is_in_isr())
    {
        osal_irq_unlock_from_isr(state);
        return;
    }
    osal_irq_unlock_task();
}

/**
 * @brief 根据硬件滤波器 bank 查找框架层 filter slot。
 * @note 仅在 slot 已占用时检查映射关系，返回值可直接作为 `filterHandle` 使用。
 */
static int32_t _can_find_slot_by_hwbank(HalCanHandler_t Can, int32_t hwBank)
{
    CanFilterResMgr_t mgr = &Can->filterResMgr;
    for (uint16_t slot = 0; slot < mgr->slotCount; slot++)
    {
        // 只在“已占用 slot”上做映射匹配，避免读取到历史残留的 slotToHwBank 值。
        if (!aw_bitmap_atomic_test(&mgr->slotUsedMap, slot))
            continue;
        if (mgr->slotToHwBank[slot] == hwBank)
            return slot;
    }
    return -1;
}

/**
 * @brief 释放指定 slot，并回收对应硬件 bank 占用状态。
 */
static void _can_release_slot(HalCanHandler_t Can, uint16_t slot)
{
    CanFilterResMgr_t mgr = &Can->filterResMgr;
    osal_irq_isr_state_t intLevel = can_irq_lock();
    int16_t hwBank = mgr->slotToHwBank[slot];
    // 释放顺序：先回收硬件 bank，再回收 slot，最后清空映射，保持资源状态一致。
    if (hwBank >= 0 && hwBank <= mgr->maxHwBank)
        aw_bitmap_atomic_clear(&mgr->hwBankUsedMap, (size_t)hwBank);
    aw_bitmap_atomic_clear(&mgr->slotUsedMap, slot);
    mgr->slotToHwBank[slot] = -1;
    can_irq_unlock(intLevel);
}

/**
 * @brief 分配一个 filter slot 和一个可用硬件 bank。
 * @note 该函数在临界区内同时更新 slot/hwBank 两个位图，保证分配原子性。
 */
static AwlfRet_e _can_reserve_slot(HalCanHandler_t Can, uint16_t* slotOut, int16_t* hwBankOut)
{
    CanFilterResMgr_t mgr = &Can->filterResMgr;
    osal_irq_isr_state_t intLevel = can_irq_lock();
    uint16_t slot = (uint16_t)0xFFFFu;
    int16_t hwBank = -1;

    // 第一步：在线性 slot 空间中找一个空闲逻辑句柄。
    for (uint16_t idx = 0; idx < mgr->slotCount; idx++)
    {
        if (!aw_bitmap_atomic_test(&mgr->slotUsedMap, idx))
        {
            slot = idx;
            break;
        }
    }
    if (slot == (uint16_t)0xFFFFu)
    {
        can_irq_unlock(intLevel);
        return AWLF_ERROR_BUSY;
    }

    // 第二步：只在 capability 给出的 bank 列表中挑选可用硬件 bank，
    // 不假设 bank 连续，也不依赖固定区间。
    for (uint16_t idx = 0; idx < mgr->slotCount; idx++)
    {
        uint16_t candidate = mgr->hwBankList[idx];
        if (!aw_bitmap_atomic_test(&mgr->hwBankUsedMap, candidate))
        {
            hwBank = (int16_t)candidate;
            break;
        }
    }
    if (hwBank < 0)
    {
        can_irq_unlock(intLevel);
        return AWLF_ERROR_BUSY;
    }

    // 第三步：CAS 方式占用 slot；失败说明并发下被其他路径抢占。
    if (!aw_bitmap_atomic_try_set(&mgr->slotUsedMap, slot))
    {
        can_irq_unlock(intLevel);
        return AWLF_ERROR_BUSY;
    }

    // 第四步：占用硬件 bank；若失败则回滚 slot，保证“不半成功”。
    if (!aw_bitmap_atomic_try_set(&mgr->hwBankUsedMap, (size_t)hwBank))
    {
        aw_bitmap_atomic_clear(&mgr->slotUsedMap, slot);
        can_irq_unlock(intLevel);
        return AWLF_ERROR_BUSY;
    }

    mgr->slotToHwBank[slot] = hwBank;
    can_irq_unlock(intLevel);

    *slotOut = slot;
    *hwBankOut = hwBank;
    return AWLF_OK;
}

/**
 * @brief 释放过滤器资源管理器内部动态资源。
 */
static void _can_filter_resmgr_deinit(HalCanHandler_t Can)
{
    CanFilterResMgr_t mgr = &Can->filterResMgr;
    // words 由 init 阶段动态分配，这里统一回收。
    if (mgr->slotUsedMap.words != NULL)
        osal_free((void*)mgr->slotUsedMap.words);
    if (mgr->hwBankUsedMap.words != NULL)
        osal_free((void*)mgr->hwBankUsedMap.words);
    if (mgr->hwBankList)
        osal_free(mgr->hwBankList);
    if (mgr->slotToHwBank)
        osal_free(mgr->slotToHwBank);
    // 清零后可安全重复初始化，避免悬挂指针。
    memset(mgr, 0, sizeof(CanFilterResMgr_s));
}

/**
 * @brief 从硬件能力初始化过滤器资源管理器。
 * @note slot 数量由 BSP 上报的 `hwBankCount` 决定，slot 与 hw bank 通过 `slotToHwBank` 建立映射。
 */
static AwlfRet_e _can_filter_resmgr_init(HalCanHandler_t Can)
{
    CanFilterResMgr_t mgr = &Can->filterResMgr;
    CanHwCapability_s capability = {0};
    AwlfRet_e ret = Can->hwInterface->control(Can, CAN_CMD_GET_CAPABILITY, &capability);
    if (ret != AWLF_OK || capability.hwBankCount == 0 || capability.hwBankList == NULL)
        return AWLF_ERROR_PARAM;

    // 支持重复 open/init：先清理旧资源，再按最新 capability 重建。
    _can_filter_resmgr_deinit(Can);

    mgr->slotCount = capability.hwBankCount;
    mgr->hwBankList = (uint8_t*)osal_malloc(mgr->slotCount);
    mgr->slotToHwBank = (int16_t*)osal_malloc(sizeof(int16_t) * mgr->slotCount);
    if (mgr->hwBankList == NULL || mgr->slotToHwBank == NULL)
    {
        _can_filter_resmgr_deinit(Can);
        return AWLF_ERROR_MEMORY;
    }
    // 拷贝 bank 列表，避免直接依赖 BSP 侧内存生命周期。
    memcpy(mgr->hwBankList, capability.hwBankList, mgr->slotCount);

    mgr->maxHwBank = 0;
    for (uint16_t i = 0; i < mgr->slotCount; i++)
    {
        // -1 表示该 slot 当前未绑定任何硬件 bank。
        mgr->slotToHwBank[i] = -1;
        if (mgr->hwBankList[i] > mgr->maxHwBank)
            mgr->maxHwBank = mgr->hwBankList[i];
    }

    // slot 位图按逻辑句柄数量分配；
    // hwBank 位图按“最大 bank 下标 + 1”分配，支持离散 bank 编号。
    size_t hwBankBitCount = (size_t)mgr->maxHwBank + 1u;
    aw_atomic_ulong_t *slotWords = aw_bitmap_atomic_buffer_alloc((size_t)mgr->slotCount, osal_malloc);
    aw_atomic_ulong_t *hwBankWords = aw_bitmap_atomic_buffer_alloc(hwBankBitCount, osal_malloc);
    if (slotWords == NULL || hwBankWords == NULL)
    {
        if (slotWords != NULL)
            aw_bitmap_buffer_free(slotWords, osal_free);
        if (hwBankWords != NULL)
            aw_bitmap_buffer_free(hwBankWords, osal_free);
        _can_filter_resmgr_deinit(Can);
        return AWLF_ERROR_MEMORY;
    }

    // 初始化后两张位图均为空闲状态（全 0）。
    ret = aw_bitmap_atomic_init(&mgr->slotUsedMap, slotWords, (size_t)mgr->slotCount);
    if (ret != AWLF_OK)
    {
        aw_bitmap_buffer_free(slotWords, osal_free);
        aw_bitmap_buffer_free(hwBankWords, osal_free);
        _can_filter_resmgr_deinit(Can);
        return ret;
    }
    ret = aw_bitmap_atomic_init(&mgr->hwBankUsedMap, hwBankWords, hwBankBitCount);
    if (ret != AWLF_OK)
    {
        aw_bitmap_buffer_free(slotWords, osal_free);
        aw_bitmap_buffer_free(hwBankWords, osal_free);
        _can_filter_resmgr_deinit(Can);
        return ret;
    }
    return AWLF_OK;
}

static void _can_status_timer_cb(osal_timer_t xTimer)
{
    HalCanHandler_t Can = (HalCanHandler_t)osal_timer_get_id(xTimer);
    CanStatusManager_t StatusManager = &Can->statusManager;
    // 检查 CAN 状态
    Can->hwInterface->control(Can, CAN_CMD_GET_STATUS, (void*)&StatusManager->errCounter);
    size_t canTxErrCnt = StatusManager->errCounter.txErrCnt;
    size_t canRxErrCnt = StatusManager->errCounter.rxErrCnt;
    if (canRxErrCnt < 127 && canTxErrCnt < 127)
        Can->statusManager.nodeErrStatus = CAN_NODE_STATUS_ACTIVE;
    else if (canRxErrCnt > 127 || canTxErrCnt > 127)
        Can->statusManager.nodeErrStatus = CAN_NODE_STATUS_PASSIVE;
    else if (canTxErrCnt > 255)
        Can->statusManager.nodeErrStatus = CAN_NODE_STATUS_BUSOFF;
    device_err_cb(&Can->parent, CAN_ERR_EVENT_BUS_STATUS, Can->statusManager.nodeErrStatus);
}

/*
 * @brief 初始化CAN接收FIFO
 * @param Can CAN句柄
 * @param msgNum 接收消息缓存数量
 * @retval  错误码（AwlfRet_e）描述
 *          AWLF_OK             成功
 *          AWLF_ERROR_MEMORY   内存分配失败
 */
static AwlfRet_e _can_fifo_init(HalCanHandler_t Can, CanMsgFifo_t canFifo, uint32_t msgNum, uint8_t isOverWrite)
{
    AwlfRet_e ret = AWLF_OK;
    INIT_LIST_HEAD(&canFifo->freeList);
    canFifo->msgBuffer = Can->adapterInterface->msgbuffer_alloc(&canFifo->freeList, msgNum);
    while (canFifo->msgBuffer == NULL)
    {
    };
    // 初始化链表
    INIT_LIST_HEAD(&canFifo->usedList);
    canFifo->freeCount = msgNum;
    canFifo->isOverwrite = isOverWrite;
    return ret;
}

/*
 * @brief 初始化CAN接收过滤器
 * @param RxHandler CAN接收句柄
 * @param filterNum 接收过滤器数量
 * @retval  错误码（AwlfRet_e）描述
 *          AWLF_OK             成功
 *          AWLF_ERROR_MEMORY   内存分配失败
 */
static AwlfRet_e _can_filter_init(CanRxHandler_t RxHandler, uint32_t filterNum)
{
    uint32_t filterSz;
    if (filterNum > 0)
    {
        filterSz = filterNum * sizeof(CanFilter_s);
        RxHandler->filterTable = (CanFilter_t)osal_malloc(filterSz);
        if (RxHandler->filterTable == NULL)
        {
            // TODO: ASSERT
            return AWLF_ERROR_MEMORY;
        }
        memset(RxHandler->filterTable, 0, filterSz);
        for (uint32_t i = 0; i < filterNum; i++)
            INIT_LIST_HEAD(&RxHandler->filterTable[i].msgMatchedList);
    }
    else
    {
        RxHandler->filterTable = NULL;
    }
    return AWLF_OK;
}

/*
 * @brief 初始化CAN接收句柄
 * @param Can CAN句柄
 * @param filterNum 接收过滤器数量
 * @param msgNum 接收消息缓存数量
 * @retval  错误码（AwlfRet_e）描述
 *          AWLF_OK             成功
 *          AWLF_ERROR_BUSY     句柄已初始化
 *          AWLF_ERROR_MEMORY   内存分配失败
 */
static AwlfRet_e _can_rxhandler_init(HalCanHandler_t Can, uint32_t iotype, uint32_t filterNum, uint32_t msgNum)
{
    CanRxHandler_t RxHandler;
    uint32_t regIoType;
    uint32_t isOparamValid;
    AwlfRet_e ret = AWLF_OK;
    // 至少初始化一个滤波器
    while (filterNum <= 0 || msgNum <= 0 || !Can->adapterInterface->msgbuffer_alloc)
    {
    }; // TODO: ASSERT

    // 检查IO类型是否有效
    regIoType = device_get_regparams(&Can->parent) & DEVICE_REG_RXTYPE_MASK;
    isOparamValid = (iotype & regIoType);
    if (!isOparamValid)
    {
        // TODO: ASSERT "Invalid IO type"
        return AWLF_ERROR_PARAM;
    }

    RxHandler = &Can->rxHandler;
    if (RxHandler->filterTable != NULL || RxHandler->rxFifo.msgBuffer != NULL)
        return AWLF_ERROR_BUSY; // TODO:LOG

    ret = _can_fifo_init(Can, &RxHandler->rxFifo, msgNum, Can->cfg.functionalCfg.isRxOverwrite);
    if (ret != AWLF_OK)
        return ret; // TODO: ASSERT
    ret = _can_filter_init(RxHandler, filterNum);
    if (ret != AWLF_OK)
    {
        osal_free(RxHandler->rxFifo.msgBuffer);
        RxHandler->rxFifo.msgBuffer = NULL;
        // TODO: ASSERT
        return ret;
    }
    Can->hwInterface->control(Can, CAN_CMD_SET_IOTYPE, (void*)&iotype);
    return ret;
}

static AwlfRet_e _can_status_manager_init(HalCanHandler_t Can)
{
    if (Can->statusManager.statusTimer != NULL)
        return AWLF_ERROR_BUSY;
    CanStatusManager_t StatusManager;
    char* name = device_get_name(&Can->parent);
    osal_status_t osal_status;
    StatusManager = &Can->statusManager;
    osal_status = osal_timer_create(&StatusManager->statusTimer, name, Can->cfg.statusCheckTimeout, OSAL_TIMER_PERIODIC,
                                    (void*)Can, _can_status_timer_cb);
    if (osal_status != OSAL_OK)
    {
        // TODO: ASSERT
        return AWLF_ERROR_MEMORY;
    }

    osal_status = osal_timer_start(StatusManager->statusTimer, Can->cfg.statusCheckTimeout);
    if (osal_status != OSAL_OK)
    {
        // TODO: ASSERT
        return AWLF_ERROR_TIMEOUT;
    }
    return AWLF_OK;
}

static void _can_status_manager_deinit(HalCanHandler_t Can)
{
    CanStatusManager_t StatusManager = &Can->statusManager;
    if (StatusManager->statusTimer != NULL)
    {
        osal_timer_delete(StatusManager->statusTimer, Can->cfg.statusCheckTimeout);
        StatusManager->statusTimer = NULL;
    }
}

__dbg_param_def(CanMailbox_t, dbgMailbox[3]) = {0};

static AwlfRet_e _can_txhandler_init(HalCanHandler_t Can, uint32_t iotype, size_t mailboxNum, uint32_t txMsgNum)
{
    CanTxHandler_t TxHandler;
    uint32_t regIoType;
    uint32_t isOparamValid;
    AwlfRet_e ret = AWLF_OK;
    while (mailboxNum <= 0 || txMsgNum <= 0)
    {
    }; // TODO: ASSERT

    // 检查IO类型是否有效
    regIoType = device_get_regparams(&Can->parent) & DEVICE_REG_TXTYPE_MASK;
    isOparamValid = (regIoType & iotype);
    if (!isOparamValid)
    {
        // TODO: ASSERT "Invalid IO type"
        return AWLF_ERROR_PARAM;
    }

    // 初始化FIFO
    TxHandler = &Can->txHandler;
    size_t txMailBoxsz = Can->cfg.mailboxNum * sizeof(CanMailbox_s);
    ret = _can_fifo_init(Can, &TxHandler->txFifo, txMsgNum, Can->cfg.functionalCfg.isTxOverwrite);
    while (ret != AWLF_OK)
    {
    }; // TODO: ASSERT
    // 初始化Mailbox
    TxHandler->pMailboxes = (CanMailbox_t)osal_malloc(txMailBoxsz);
    while (TxHandler->pMailboxes == NULL)
    {
    }; // TODO: ASSERT
    memset(TxHandler->pMailboxes, 0, txMailBoxsz);

    INIT_LIST_HEAD(&TxHandler->mailboxList);
    for (uint32_t i = 0; i < Can->cfg.mailboxNum; i++)
    {
        TxHandler->pMailboxes[i].bank = i;
        dbgMailbox[i] = &TxHandler->pMailboxes[i];
        INIT_LIST_HEAD(&TxHandler->pMailboxes[i].list);
        list_add_tail(&TxHandler->pMailboxes[i].list, &TxHandler->mailboxList);
    }
    Can->hwInterface->control(Can, CAN_CMD_SET_IOTYPE, (void*)&iotype);
    return ret;
}

static void _can_txhandler_deinit(CanTxHandler_t TxHandler)
{
    if (TxHandler->txFifo.msgBuffer != NULL)
    {
        osal_free(TxHandler->txFifo.msgBuffer);
        TxHandler->txFifo.msgBuffer = NULL;
    }
    TxHandler->txFifo.freeCount = 0;
    INIT_LIST_HEAD(&TxHandler->txFifo.freeList);
    INIT_LIST_HEAD(&TxHandler->txFifo.usedList);
}

/*
 * @brief CAN 接收模块反初始化
 * @param RxHandler CAN接收句柄
 */
static void _can_rxhandler_deinit(CanRxHandler_t RxHandler)
{
    if (RxHandler->filterTable != NULL)
    {
        osal_free(RxHandler->filterTable);
        RxHandler->filterTable = NULL;
    }
    if (RxHandler->rxFifo.msgBuffer != NULL)
    {
        osal_free(RxHandler->rxFifo.msgBuffer);
        RxHandler->rxFifo.msgBuffer = NULL;
    }
}

/*
 * @brief 将CAN消息链表项中的数据拷贝到CAN用户消息
 * @param msgList CAN消息链表
 * @param pUserRxMsg CAN用户消息指针
 */
static inline void _can_container_copy_to_usermsg(CanMsgList_t MsgList, CanUserMsg_t pUserRxMsg)
{
    MsgList->userMsg.userBuf = pUserRxMsg->userBuf; // 防止框架层的userBuf覆盖原有的用户内存指针
    *pUserRxMsg = MsgList->userMsg;
    // 拷贝数据到用户缓冲区
    memcpy((void*)pUserRxMsg->userBuf, (void*)MsgList->container, MsgList->userMsg.dsc.dataLen);
    MsgList->userMsg.userBuf = MsgList->container; // 恢复框架层的userBuf指针
}

/**
 * @brief 从CAN FIFO中获取一个空闲链表项
 * @param Fifo CAN FIFO
 * @return 错误
 * @retval 1. CAN_ERR_NONE          成功
 * @retval 2. CAN_ERR_SOFT_FIFO_OVERFLOW 接收FIFO溢出
 *
 * @note 在覆写模式下，返回CAN_ERR_SOFT_FIFO_OVERFLOW时MsgList将指向被取出的链表项
 *       否则MsgList将指向NULL
 * @note 该函数非线程安全，需要调用者自行保护数据安全。
 */
static CanErrorCode_e _can_get_free_msg_list(CanMsgFifo_t Fifo, CanMsgList_t* MsgList)
{
    CanErrorCode_e ret = CAN_ERR_NONE;

    // 如果空闲链表非空，则取出一个链表项
    if (!list_empty(&Fifo->freeList))
    {
        *MsgList = list_first_entry(&Fifo->freeList, CanMsgList_s, fifoListNode);
        list_del(&(*MsgList)->fifoListNode); // 将该节点从空闲链表中删除
        Fifo->freeCount--;
    }
    // 如果空闲链表空，则代表链表已满
    // 若开启覆盖模式，则从已用链表取出最旧消息，实现“覆盖旧消息”。
    // 若未开启覆盖模式，则返回FIFO溢出错误
    else if (!list_empty(&Fifo->usedList) && Fifo->isOverwrite)
    {
        ret = CAN_ERR_SOFT_FIFO_OVERFLOW;
        *MsgList = list_first_entry(&Fifo->usedList, CanMsgList_s, fifoListNode);
        list_del(&(*MsgList)->fifoListNode); // 将该节点从已用链表中删除
    }
    else if (!Fifo->isOverwrite) // 若未开启覆盖模式，则返回FIFO溢出错误
    {
        *MsgList = NULL;
        ret = CAN_ERR_SOFT_FIFO_OVERFLOW;
    }
    // 理论上空闲链表与已用链表的节点总数应恒定且为正整数。
    // 出现两表皆空的情况，只能是缓存区未初始化
    else
    {
        while (1)
        {
        }; // TODO: ASSERT "Receive message buffer is not initialized"
    }
    return ret;
}

/**
 * @brief 将CAN消息链表项添加回CAN接收FIFO的空闲链表中
 * @param RxFifo CAN接收FIFO
 * @param MsgList CAN消息链表
 *
 * @note 该函数非线程安全，需要调用者自行保护数据安全。
 */
static inline void _can_add_free_msg_list(CanMsgFifo_t Fifo, CanMsgList_t MsgList)
{
    list_add_tail(&MsgList->fifoListNode, &Fifo->freeList); // 将该节点添加到空闲链表中
    Fifo->freeCount++;
}

/**
 * @brief 将CAN消息链表项添加回CAN发送FIFO的已用链表中
 * @param Fifo CANFIFO
 * @param MsgList CAN消息链表
 *
 * @note 该函数非线程安全，需要调用者自行保护数据安全。
 */
static inline void _can_add_used_msg_list(CanMsgFifo_t Fifo, CanMsgList_t MsgList)
{
    list_add_tail(&MsgList->fifoListNode, &Fifo->usedList); // 将该节点添加到已用链表中
}

/**
 * @brief 从CAN接收FIFO中获取一个空闲链表项
 *
 * @param RxHandler CAN接收句柄
 * @param MsgList CAN消息链表项指针
 * @return CanErrorCode_e 错误
 * @retval 1. CAN_ERR_NONE          成功
 * @retval 2. CAN_ERR_SOFT_FIFO_OVERFLOW 接收FIFO溢出
 */
static CanErrorCode_e _canrx_get_free_msg_list(CanRxHandler_t RxHandler, CanMsgList_t* ppMsgList)
{
    uint32_t intLevel;
    CanErrorCode_e ret = CAN_ERR_NONE;
    intLevel = can_irq_lock();
    ret = _can_get_free_msg_list(&RxHandler->rxFifo, ppMsgList);
    if (*ppMsgList == NULL) // 若ppMsgList指向NULL，说明发生了某种错误，直接返回结果
    {
        can_irq_unlock(intLevel);
        return ret;
    }
    // 如果该链表项有匹配的滤波器，则将其从滤波器的匹配链表中删除
    if ((*ppMsgList)->owner != NULL)
    {
        list_del(&(*ppMsgList)->matchedListNode);
        // 如果是FIFO溢出，说明取得的是还来不及读取的数据，所以其原本匹配滤波器的消息数量需要减一
        if (ret == CAN_ERR_SOFT_FIFO_OVERFLOW)
            ((CanFilter_t)(*ppMsgList)->owner)->msgCount--;
        (*ppMsgList)->owner = NULL;
    }
    can_irq_unlock(intLevel);
    return ret;
}

/**
 * @brief 从CAN发送FIFO中获取一个空闲链表项
 * @param TxHandler CAN发送句柄
 * @param MsgList CAN消息链表项指针
 * @return CanErrorCode_e 错误
 * @retval 1. CAN_ERR_NONE          成功
 * @retval 2. CAN_ERR_SOFT_FIFO_OVERFLOW 发送FIFO溢出
 *
 */
static CanErrorCode_e _cantx_get_free_msg_list(CanTxHandler_t TxHandler, CanMsgList_t* ppMsgList)
{
    uint32_t intLevel;
    CanErrorCode_e ret = CAN_ERR_NONE;
    intLevel = can_irq_lock();
    ret = _can_get_free_msg_list(&TxHandler->txFifo, ppMsgList);
    if (*ppMsgList == NULL) // 若ppMsgList指向NULL，说明发生了某种错误，直接返回结果
    {
        can_irq_unlock(intLevel);
        return ret;
    }
    while ((*ppMsgList)->owner != NULL && !TxHandler->txFifo.isOverwrite)
    {
    }; // TODO: 非覆写模式下不应出现“取到正在发送报文”的情况
    // 如果该链表项是正在发送的报文(覆写模式下会触发)，则将其从发送邮箱的匹配链表中删除
    // TODO: 这个逻辑可能导致正在发送或等待重传的报文被错误地从匹配链表中删除
    if ((*ppMsgList)->owner != NULL)
    {
        list_del(&(*ppMsgList)->matchedListNode);
        // 如果是FIFO溢出，说明取得的是还来不及发送的数据，所以其原本匹配发送邮箱的消息需要中断发送
        if (ret == CAN_ERR_SOFT_FIFO_OVERFLOW)
            ((CanMailbox_t)(*ppMsgList)->owner)->isBusy = 0;
        (*ppMsgList)->owner = NULL;
    }
    // TODO: 未来拓展TxHandler数据结构时，此处可能需要做额外操作
    can_irq_unlock(intLevel);
    return ret;
}

static inline void _canrx_add_free_msg_list(CanRxHandler_t RxHandler, CanMsgList_t MsgList)
{
    uint32_t intLevel;
    intLevel = can_irq_lock();
    _can_add_free_msg_list(&RxHandler->rxFifo, MsgList); // 将该节点添加到空闲链表中
    can_irq_unlock(intLevel);
}

static inline void _cantx_add_free_msg_list(CanTxHandler_t TxHandler, CanMailbox_t mailbox)
{
    CanMsgList_t MsgList;
    uint32_t intLevel;

    intLevel = can_irq_lock();
    MsgList = mailbox->pMsgList;
    list_del(&MsgList->fifoListNode); // 从已用链表中删除
    list_del(&MsgList->matchedListNode);
    mailbox->isBusy = 0;
    mailbox->pMsgList = NULL;
    MsgList->owner = NULL;
    list_add_tail(&mailbox->list, &TxHandler->mailboxList);
    _can_add_free_msg_list(&TxHandler->txFifo, MsgList); // 将该节点添加到空闲链表中
    can_irq_unlock(intLevel);
}

/**
 * @brief 将填充好数据的CAN消息链表（已用项）添加回CAN接收FIFO和滤波器匹配链表
 * @param RxHandler CAN接收句柄
 * @param Filter 接收过滤器
 * @param msgList CAN消息链表
 * @note Filter 合法性由调用者保证，本函数不做检查。
 */
static inline void _canrx_add_used_msg_list(CanRxHandler_t RxHandler, CanFilter_t Filter, CanMsgList_t MsgList)
{
    uint32_t intLevel;
    intLevel = can_irq_lock();
    list_add_tail(&MsgList->matchedListNode, &Filter->msgMatchedList); // 将该节点添加到滤波器匹配链表
    MsgList->owner = Filter;                                           // 记录该消息链表项所属滤波器
    Filter->msgCount++;                                                // 增加滤波器匹配消息数量
    _can_add_used_msg_list(&RxHandler->rxFifo, MsgList);               // 将该节点添加到已用链表中
    can_irq_unlock(intLevel);
}

static inline void cantx_add_used_msg_list(CanTxHandler_t TxHandler, CanMsgList_t pMsgList)
{
    uint32_t intLevel;
    intLevel = can_irq_lock();
    _can_add_used_msg_list(&TxHandler->txFifo, pMsgList); // 将该节点添加到已用链表中
    can_irq_unlock(intLevel);
}

/**
 * @brief 从CAN接收FIFO中获取一个CAN消息链表项，同时将其从已用链表和滤波器匹配链表中删除
 * @param RxHandler CAN接收句柄
 * @param Filter 接收滤波器
 * @return CAN消息链表项指针
 * @note Filter 合法性由调用者保证，本函数不做检查。
 */
static CanMsgList_t canrx_get_used_msg_list(CanRxHandler_t RxHandler, CanFilter_t Filter)
{
    uint32_t intLevel;
    CanMsgList_t MsgList;
    intLevel = can_irq_lock();
    // 如果指定了滤波器，且该滤波器有匹配的消息链表项，则取出第一个匹配项
    if (Filter != NULL)
    {
        MsgList = list_first_entry(&Filter->msgMatchedList, CanMsgList_s, matchedListNode);
        Filter->msgCount--;
        MsgList->owner = NULL;
    }
    // 如果没有指定滤波器，则从已用链表中取出一个链表项
    else
    {
        MsgList = list_first_entry(&RxHandler->rxFifo.usedList, CanMsgList_s, fifoListNode);
    }
    list_del(&MsgList->fifoListNode);    // 将该节点从已用链表中删除
    list_del(&MsgList->matchedListNode); // 将该节点从滤波器匹配链表中删除
    can_irq_unlock(intLevel);
    return MsgList;
}

static inline CanMsgList_t cantx_get_used_msg_list(CanTxHandler_t TxHandler)
{
    uint32_t intLevel;
    CanMsgList_t MsgList;
    intLevel = can_irq_lock();
    MsgList = list_first_entry(&TxHandler->txFifo.usedList, CanMsgList_s, fifoListNode);
    list_del(&MsgList->fifoListNode); // 将该节点从已用链表中删除
    can_irq_unlock(intLevel);
    return MsgList;
}

/*
 * @brief 添加接收消息到CAN接收FIFO和滤波器匹配链表
 * @param Can CAN句柄
 * @param userRxMsg CAN用户接收消息指针
 */
static void canrx_msg_put(HalCanHandler_t Can, CanMsgList_t HwRxMsgList)
{
    CanRxHandler_t RxHandler;
    CanFilter_t Filter;
    CanUserMsg_t HwRxMsg;
    CanFilterHandle_t filterHandle;
    while (!HwRxMsgList)
    {
    }; // TODO: assert
    HwRxMsg = &HwRxMsgList->userMsg;
    // 参数检查与初始化
    // TODO: ASSERT "User message is NULL"
    while (!HwRxMsg->userBuf)
    {
    }; // TODO: assert
    filterHandle = HwRxMsg->filterHandle;
    // TODO: ASSERT "Filter bank %d is out of range"
    while (IS_CAN_FILTER_INVALID(Can, filterHandle))
    {
    };

    Filter = NULL;
    RxHandler = &Can->rxHandler;
    Filter = &RxHandler->filterTable[filterHandle];

    // 将消息链表添加回已用链表和滤波器匹配链表
    _canrx_add_used_msg_list(RxHandler, Filter, HwRxMsgList);

    if (Filter != NULL && Filter->request.rx_callback != NULL)
    {
        Filter->request.rx_callback(&Can->parent, Filter->request.param, filterHandle, Filter->msgCount);
    }
    else
    {
        // 由于当前CAN总线必须要设置一个滤波器，而设置滤波器必须要设置rx_callback，所以大概率不会跳到这里
        // 而且暂时也想不到device模型的read_cb在CAN中有什么价值，所以暂时不处理
        while (1)
        {
        }; // TODO: ASSERT "CAN filter bank %d RX callback is NULL"
        // // size_t msgCount = Can->cfg.rxMsgListBufSize - RxHandler->rxFifo.freeCount;
        // // device_read_cb(&Can->parent, msgCount);
    }
}

/**
 * @brief 从CAN接收FIFO中获取一个CAN消息，非阻塞
 * @param rxHandler CAN接收处理器
 * @param pUserRxMsg CAN用户接收消息指针
 * @retval  1. CAN_ERR_NONE           成功
 * @retval  2. CAN_ERR_FIFO_EMPTY     滤波器无可读消息或接收 FIFO 为空
 */
static CanErrorCode_e canrx_msg_get_noblock(CanRxHandler_t rxHandler, CanUserMsg_t pUserRxMsg)
{
    CanFilter_t Filter;
    size_t filterHandle;
    uint32_t intLevel;
    CanMsgList_t MsgList;
    filterHandle = pUserRxMsg->filterHandle;
    MsgList = NULL;
    Filter = NULL;

    intLevel = can_irq_lock();
    Filter = &rxHandler->filterTable[filterHandle];
    if (Filter->msgCount == 0) // 非阻塞，直接退出
    {
        // TODO: ASSERT "Filter bank %d is empty"
        can_irq_unlock(intLevel);
        return CAN_ERR_FIFO_EMPTY;
    }
    else if (list_empty(&rxHandler->rxFifo.usedList))
    {
        // TODO: ASSERT "Soft FIFO is empty"
        can_irq_unlock(intLevel);
        return CAN_ERR_FIFO_EMPTY;
    }
    can_irq_unlock(intLevel);
    // 开始获取报文
    // 从rxFifo中取出一个已用链表项
    MsgList = canrx_get_used_msg_list(rxHandler, Filter);
    // 从这里之后，MsgList已经从rxFifo和Filter中被取出
    // 此时除了当前操作之外，已经没有任何代码能访问这个链表项，所以针对MsgList的操作无需考虑竞态

    // 将容器中的报文拷贝到用户接收消息指针
    _can_container_copy_to_usermsg(MsgList, pUserRxMsg);
    // 将该消息链表项添加回空闲链表
    _can_add_free_msg_list(&rxHandler->rxFifo, MsgList);
    return CAN_ERR_NONE;
}

/**
 * @brief CAN 发送调度器
 * @param Can CAN句柄
 * @note 核心逻辑：从待发 FIFO 取数据 -> 写入空闲硬件邮箱 -> 触发发送。
 * @note 该函数可在用户线程触发，也可在 ISR 中调用。
 */
static void _can_tx_scheduler(HalCanHandler_t Can)
{
    CanTxHandler_t TxHandler = &Can->txHandler;
    CanMailbox_t mailbox;
    CanMsgList_t pMsgList;
    uint32_t intLevel;

    // 进入临界区，保护 FIFO 与 mailbox 状态
    intLevel = can_irq_lock();

    if (list_empty(&TxHandler->txFifo.usedList))
    {
        // 没有数据要发了，退出循环
        can_irq_unlock(intLevel);
        return;
    }
    // TODO: 完善mailbox机制，使我们能够主动调度mailbox
    // TODO: 当邮箱数量增多时，中断中循环次数可能过多；
    // 当前 CAN 负载与硬件邮箱规模可控，暂不做进一步拆分调度。
    // 各家CAN IP都不会有太多的邮箱，所以暂时不需要考虑。即便未来要改，由于架构设计得当，改动也会被限制在这个函数中
    while (!list_empty(&TxHandler->mailboxList))
    {
        pMsgList = list_first_entry(&TxHandler->txFifo.usedList, CanMsgList_s, fifoListNode);
        CanHwMsg_s hwMsg = {
            .dsc = pMsgList->userMsg.dsc,
            .hwFilterBank = -1,
            .hwTxMailbox = -1,
            .data = pMsgList->userMsg.userBuf,
        };
        AwlfRet_e ret = Can->hwInterface->send_msg_mailbox(Can, &hwMsg);
        if (ret == AWLF_OK)
        {
            // 发送成功
            while (IS_CAN_MAILBOX_INVALID(Can, hwMsg.hwTxMailbox))
            {
            }; // TODO: assert
            mailbox = &TxHandler->pMailboxes[hwMsg.hwTxMailbox];
            // 从等待队列（usedList）中移除消息
            list_del(&pMsgList->fifoListNode);
            // 从可用邮箱中移除邮箱
            list_del(&mailbox->list);
            // 标记邮箱
            mailbox->isBusy = 1;
            mailbox->pMsgList = pMsgList;
            pMsgList->owner = mailbox;
        }
        else
        {
            // TODO: 进入这里通常说明进入 BUS_OFF 状态或其他未知原因
            // 这里简单处理，直接退出循环
            // TODO: LOG "CAN status: %d，发送失败，bank: %d，msgID: 0x%08x"
            if (ret == AWLF_ERR_OVERFLOW)
                Can->statusManager.errCounter.txMailboxFullCnt++;
            break;
        }
        if (list_empty(&TxHandler->txFifo.usedList))
            break;
    }
    can_irq_unlock(intLevel);
}

/**
 * @brief 向CAN发送FIFO中添加一个CAN消息，非阻塞
 *
 * @param Can CAN句柄
 * @param pUserTxMsgBuf CAN用户发送消息数组指针
 * @param msgNum 消息数量
 * @return size_t 实际发送的消息数量
 */
static size_t cantx_msg_put_nonblock(HalCanHandler_t Can, CanUserMsg_t pUserTxMsgBuf, size_t msgNum)
{
    uint32_t intLevel;
    size_t msgCounter = 0;
    CanMsgList_t pMsgList = NULL;
    for (msgCounter = 0; msgCounter < msgNum; msgCounter++)
    {
        // 获取一个空闲消息链表项，用于存储信息
        if (_cantx_get_free_msg_list(&Can->txHandler, &pMsgList) == CAN_ERR_SOFT_FIFO_OVERFLOW)
        {
            if (!Can->txHandler.txFifo.isOverwrite)
            {
                intLevel = can_irq_lock();
                Can->statusManager.errCounter.txSoftOverFlowCnt += (msgNum - msgCounter);
                Can->statusManager.errCounter.txFailCnt += (msgNum - msgCounter); // 被覆盖的消息定义为发送失败的消息
                can_irq_unlock(intLevel);
                break; // 跳出循环，结束写入
            }
            else // 覆写
            {
                intLevel = can_irq_lock();
                Can->statusManager.errCounter.txFailCnt++; // 未被发送的消息定义为发送失败的消息
                Can->statusManager.errCounter.txSoftOverFlowCnt++;
                can_irq_unlock(intLevel);
            }
        }
        // 从这里之后，pMsgList已经从Fifo中被取出
        // 此时除了当前操作之外，已经没有任何代码能访问这个链表项，所以针对pMsgList的操作无需考虑竞态

        // 填充用户消息指针
        pMsgList->userMsg = pUserTxMsgBuf[msgCounter];
        // 填充CAN消息容器
        memcpy((void*)pMsgList->container, (void*)pUserTxMsgBuf[msgCounter].userBuf, pUserTxMsgBuf[msgCounter].dsc.dataLen);
        pMsgList->userMsg.userBuf = pMsgList->container;

        intLevel = can_irq_lock();
        _can_add_used_msg_list(&Can->txHandler.txFifo, pMsgList);
        can_irq_unlock(intLevel);
    }
    // 在中断上下文中不需要主动调度
    if (!osal_is_in_isr())
        _can_tx_scheduler(Can);
    return msgCounter;
}

/**
 * @brief CAN软件重传机制
 *
 * @param Can CAN设备指针
 * @param mailbox 邮箱指针
 */
static void cantx_soft_retransmit(HalCanHandler_t Can, uint32_t mailboxBank)
{
    // 相关邮箱与消息节点会先从共享链表摘除，再执行重传流程；
    // 因此该流程内对这两类对象的访问不与外部并发冲突（仅限该邮箱与该消息节点）。
    CanMailbox_t mailbox = &Can->txHandler.pMailboxes[mailboxBank];
    CanMsgList_t pMsgList = mailbox->pMsgList;
    while (!pMsgList)
    {
    }; // TODO: assert
    pMsgList->owner = NULL;
    uint32_t intLevel;
    intLevel = can_irq_lock();
    list_add(&pMsgList->fifoListNode, &Can->txHandler.txFifo.usedList); // 头插，确保先发送的消息优先重发
    can_irq_unlock(intLevel);
    mailbox->isBusy = 0;
    mailbox->pMsgList = NULL;
    list_add_tail(&mailbox->list, &Can->txHandler.mailboxList);
    _can_tx_scheduler(Can);
}

/*
 * @brief 初始化CAN设备
 * @param Dev CAN设备指针
 * @retval  1. AWLF_ERROR_PARAM 参数错误
 *          2. AWLF_ERROR_HW 硬件错误
 *          3. AWLF_ERROR_NONE 成功
 */
static AwlfRet_e can_init(Device_t Dev)
{
    AwlfRet_e ret;
    if (!Dev)
        return AWLF_ERROR_PARAM;
    HalCanHandler_t Can = (HalCanHandler_t)Dev;
    if (!Can->adapterInterface || !Can->hwInterface)
        return AWLF_ERROR_PARAM;
    _can_filter_resmgr_deinit(Can);
    _can_rxhandler_deinit(&Can->rxHandler);
    _can_txhandler_deinit(&Can->txHandler);
    ret = Can->hwInterface->configure(Can, &Can->cfg);
    return ret;
}

static AwlfRet_e can_open(Device_t Dev, uint32_t oparam)
{
    AwlfRet_e ret;
    uint32_t iotype;
    if (!Dev)
        return AWLF_ERROR_PARAM;
    HalCanHandler_t Can = (HalCanHandler_t)Dev;

    // 先初始化过滤器资源管理器，再初始化 RX/TX 子模块。
    // 这样后续模块可以直接使用稳定的 slot <-> hwBank 映射。
    ret = _can_filter_resmgr_init(Can);
    if (ret != AWLF_OK)
        return ret;
    // filterNum 由 capability 驱动，不再依赖静态配置值。
    Can->cfg.filterNum = Can->filterResMgr.slotCount;

    iotype = oparam & DEVICE_O_RXTYPE_MASK;

    ret = _can_rxhandler_init(Can, iotype, Can->filterResMgr.slotCount, Can->cfg.rxMsgListBufSize);
    if (ret != AWLF_OK)
    {
        // 回滚 filter_resmgr，避免 open 失败后残留资源占用。
        _can_filter_resmgr_deinit(Can);
        return ret;
    }

    iotype = oparam & DEVICE_O_TXTYPE_MASK;
    ret = _can_txhandler_init(Can, iotype, Can->cfg.mailboxNum, Can->cfg.txMsgListBufSize);
    if (ret != AWLF_OK)
    {
        // TODO: LOG ERR
        _can_rxhandler_deinit(&Can->rxHandler);
        // TX 初始化失败同样需要释放 filter_resmgr。
        _can_filter_resmgr_deinit(Can);
        return ret;
    }
    ret = _can_status_manager_init(Can);
    if (ret != AWLF_OK)
    {
        // TODO: LOG ERR
        _can_rxhandler_deinit(&Can->rxHandler);
        _can_txhandler_deinit(&Can->txHandler);
        // 状态管理器失败时，保持 open 过程全量回滚。
        _can_filter_resmgr_deinit(Can);
        return ret;
    }
    return ret;
}

static size_t can_write(Device_t Dev, void* pos, void* data, size_t len)
{
    size_t msgNum = 0;
    HalCanHandler_t Can = (HalCanHandler_t)Dev;
    // 目前只支持非阻塞发送策略
    msgNum = cantx_msg_put_nonblock(Can, (CanUserMsg_t)data, len);
    return msgNum;
}

/**
 * @brief 从CAN接收FIFO中读取多个CAN消息
 *
 * @param Dev CAN设备指针
 * @param pos CAN 不需要该参数，保持NULL即可
 * @param buf 接收消息缓冲区指针
 * @param len 接收消息缓冲区长度
 * @return size_t 实际读取的消息数量
 */
static size_t can_read(Device_t Dev, void* pos, void* buf, size_t len)
{
    size_t cnt = 0;
    HalCanHandler_t Can = (HalCanHandler_t)Dev;
    CanRxHandler_t rxHandler = &Can->rxHandler;
    CanErrorCode_e ret = CAN_ERR_NONE;
    CanUserMsg_t pUserRxMsgBuf = (CanUserMsg_t)buf;

    for (cnt = 0; cnt < len; cnt++)
    {
        while (!pUserRxMsgBuf[cnt].userBuf)
        {
        }; // TODO: assert
        size_t filterHandle = pUserRxMsgBuf[cnt].filterHandle;
        while (IS_CAN_FILTER_INVALID(Can, filterHandle))
        {
        }; // TODO: ASSERT "Filter bank %d is INVALID"
        while (rxHandler->filterTable[filterHandle].isActived == 0)
        {
        }; // TODO: ASSERT "Filter bank %d is not active"
        ret = canrx_msg_get_noblock(rxHandler, &pUserRxMsgBuf[cnt]);
        if (ret != CAN_ERR_NONE)
            break;
    }
    if (ret != CAN_ERR_NONE)
    {
        // TODO: LOG ERR
        device_err_cb(&Can->parent, ret, cnt);
    }
    return cnt;
}

/**
 * @brief 控制CAN设备
 * @param Dev CAN设备指针
 * @param cmd 命令 @ref CAN_CMD_DEF
 * @param args 参数指针
 * @retval  1. AWLF_ERROR_PARAM 参数错误
 *          2. AWLF_ERROR_NONE 成功
 */
static AwlfRet_e can_ctrl(Device_t Dev, size_t cmd, void* args)
{
    AwlfRet_e ret = AWLF_OK;
    if (!Dev)
        return AWLF_ERROR_PARAM;
    HalCanHandler_t Can = (HalCanHandler_t)Dev;
    switch (cmd)
    {
    case CAN_CMD_SET_IOTYPE:
        ret = Can->hwInterface->control(Can, CAN_CMD_SET_IOTYPE, args);
        break;

    case CAN_CMD_CLR_IOTYPE:
        ret = Can->hwInterface->control(Can, CAN_CMD_CLR_IOTYPE, args);
        break;

    // 学习者需注意这里的程序设计，检查参数合法性，并且操作失败后也不会产生副作用（即如果配置失败，CAN设备的状态或数据不会改变），这在架构设计中是必要的
    case CAN_CMD_CFG:
        if (!args)
        {
            ret = AWLF_ERROR_PARAM;
            break;
        }
        ret = Can->hwInterface->control(Can, CAN_CMD_CFG, args);
        if (ret == AWLF_OK)
            Can->cfg = *(CanCfg_t)args;
        break;

    // 学习者需注意这里的程序设计，检查参数合法性，并且操作失败后也不会产生副作用（即如果配置失败，CAN设备的状态或数据不会改变），这在架构设计中是必要的
    case CAN_CMD_FILTER_ALLOC:
    {
        CanFilterAllocArg_t allocArg = (CanFilterAllocArg_t)args;
        if (allocArg == NULL || allocArg->request.rx_callback == NULL)
        {
            ret = AWLF_ERROR_PARAM;
            break;
        }

        uint16_t slot = 0;
        int16_t hwBank = -1;
        // 先在 filter_resmgr 中占用 slot/bank，再下发硬件配置。
        ret = _can_reserve_slot(Can, &slot, &hwBank);
        if (ret != AWLF_OK)
            break;

        CanHwFilterCfg_s hwCfg = {
            .bank = (size_t)hwBank,
            .workMode = allocArg->request.workMode,
            .idType = allocArg->request.idType,
            .id = allocArg->request.id,
            .mask = allocArg->request.mask,
        };
        ret = Can->hwInterface->control(Can, CAN_CMD_FILTER_ALLOC, &hwCfg);
        if (ret != AWLF_OK)
        {
            // 硬件配置失败必须回滚资源管理器，避免“逻辑已占用/硬件未生效”不一致。
            _can_release_slot(Can, slot);
            break;
        }

        CanFilter_t filter = &Can->rxHandler.filterTable[slot];
        // 硬件配置成功后再发布框架态 filter 信息。
        filter->request = allocArg->request;
        filter->isActived = 1;
        filter->msgCount = 0;
        allocArg->handle = (CanFilterHandle_t)slot;
    }
    break;

    case CAN_CMD_FILTER_FREE:
    {
        if (args == NULL)
        {
            ret = AWLF_ERROR_PARAM;
            break;
        }
        CanFilterHandle_t handle = *(CanFilterHandle_t*)args;
        if (IS_CAN_FILTER_INVALID(Can, handle))
        {
            ret = AWLF_ERROR_PARAM;
            break;
        }

        CanFilter_t filter = &Can->rxHandler.filterTable[handle];
        if (!filter->isActived || filter->msgCount > 0)
        {
            ret = AWLF_ERROR_BUSY;
            break;
        }

        int16_t hwBank = Can->filterResMgr.slotToHwBank[handle];
        if (hwBank < 0)
        {
            ret = AWLF_ERROR_PARAM;
            break;
        }

        CanHwFilterCfg_s hwCfg = {
            .bank = (size_t)hwBank,
            .workMode = filter->request.workMode,
            .idType = filter->request.idType,
            .id = filter->request.id,
            .mask = filter->request.mask,
        };
        ret = Can->hwInterface->control(Can, CAN_CMD_FILTER_FREE, &hwCfg);
        if (ret != AWLF_OK)
            break;

        // 先清空框架态 filter，再释放 slot/bank 占用位。
        memset(filter, 0, sizeof(CanFilter_s));
        INIT_LIST_HEAD(&filter->msgMatchedList);
        _can_release_slot(Can, handle);
    }
    break;

    case CAN_CMD_START:
        ret = Can->hwInterface->control(Can, CAN_CMD_START, NULL);
        break;

    case CAN_CMD_CLOSE:
        ret = Can->hwInterface->control(Can, CAN_CMD_CLOSE, NULL);
        break;
    default:
        ret = Can->hwInterface->control(Can, cmd, args);
        break;
    }
    return ret;
}

static AwlfRet_e can_close(Device_t Dev)
{
    if (!Dev)
        return AWLF_ERROR_PARAM;
    HalCanHandler_t Can = (HalCanHandler_t)Dev;
    if (!Can->adapterInterface || !Can->hwInterface)
        return AWLF_ERROR_PARAM;
    uint32_t ioType = CAN_REG_INT_TX;
    Can->hwInterface->control(Can, CAN_CMD_CLR_IOTYPE, &ioType);
    ioType = CAN_REG_INT_RX;
    Can->hwInterface->control(Can, CAN_CMD_CLR_IOTYPE, &ioType);
    Can->hwInterface->control(Can, CAN_CMD_CLOSE, NULL);
    _can_rxhandler_deinit(&Can->rxHandler);
    _can_txhandler_deinit(&Can->txHandler);
    _can_status_manager_deinit(Can);
    // close 阶段必须回收 filter_resmgr，避免下次 open 看到陈旧映射。
    _can_filter_resmgr_deinit(Can);
    return AWLF_OK;
}

/*
 * @brief CAN错误中断处理函数
 * @param Can CAN设备指针
 * @param errorEvent 错误事件
 * @note 该函数主要更新 CAN 设备状态；更细粒度的恢复策略由状态管理线程处理。
 */
void can_error_isr(HalCanHandler_t Can, uint32_t errEvent, size_t param)
{
    switch (errEvent)
    {
    case CAN_ERR_EVENT_TX_FAIL:
        Can->statusManager.errCounter.txFailCnt++;
        while (IS_CAN_MAILBOX_INVALID(Can, param))
        {
        }; // TODO: assert
        cantx_soft_retransmit(Can, param);
        break;
    case CAN_ERR_EVENT_ARBITRATION_FAIL:
    {

        // TODO: LOG
        Can->statusManager.errCounter.txArbitrationFailCnt++;
        while (IS_CAN_MAILBOX_INVALID(Can, param))
        {
        }; // TODO: assert
        cantx_soft_retransmit(Can, param);
    }
    break;
    case CAN_ERR_EVENT_BUS_STATUS: // TODO: 处理总线错误
    {
        size_t canTxErrCnt = Can->statusManager.errCounter.txErrCnt;
        size_t canRxErrCnt = Can->statusManager.errCounter.rxErrCnt;
        if (canRxErrCnt < 127 && canTxErrCnt < 127)
            Can->statusManager.nodeErrStatus = CAN_NODE_STATUS_ACTIVE;
        else if (canRxErrCnt > 127 || canTxErrCnt > 127)
            Can->statusManager.nodeErrStatus = CAN_NODE_STATUS_PASSIVE;
        else if (canTxErrCnt > 255)
            Can->statusManager.nodeErrStatus = CAN_NODE_STATUS_BUSOFF;
        device_err_cb(&Can->parent, CAN_ERR_EVENT_BUS_STATUS, Can->statusManager.nodeErrStatus);
    }
    break;
    }
}

void hal_can_isr(HalCanHandler_t Can, CanIsrEvent_e event, size_t param)
{
    switch (event)
    {
    case CAN_ISR_EVENT_INT_RX_DONE: // 接收完成中断，param是接收消息的FIFO索引
    {
        AwlfRet_e ret;
        CanMsgList_t MsgList = NULL;
        CanHwMsg_s hwMsg = {0};
        uint8_t hwData[64];
        hwMsg.data = hwData;
        if (_canrx_get_free_msg_list(&Can->rxHandler, &MsgList) == CAN_ERR_SOFT_FIFO_OVERFLOW)
        {
            Can->statusManager.errCounter.rxSoftOverFlowCnt++; // 接收软件FIFO溢出计数器增加
            if (!Can->rxHandler.rxFifo.isOverwrite)            // 非覆写策略直接退出
            {
                Can->hwInterface->recv_msg(Can, NULL, param); // 丢弃当前帧（清中断）
                break;
            }
        }
        ret = Can->hwInterface->recv_msg(Can, &hwMsg, param);
        if (ret == AWLF_OK)
        {
            int32_t slot = _can_find_slot_by_hwbank(Can, hwMsg.hwFilterBank);
            if (slot < 0 || IS_CAN_FILTER_INVALID(Can, (size_t)slot))
            {
                _canrx_add_free_msg_list(&Can->rxHandler, MsgList);
                Can->statusManager.errCounter.rxFailCnt++;
                break;
            }
            MsgList->userMsg.dsc = hwMsg.dsc;
            MsgList->userMsg.filterHandle = (CanFilterHandle_t)slot;
            memcpy(MsgList->container, hwMsg.data, hwMsg.dsc.dataLen);
            canrx_msg_put(Can, MsgList);
            Can->statusManager.errCounter.rxMsgCount++; // 接收消息计数器增加
        }
        else
        {
            Can->statusManager.errCounter.rxFailCnt++; // 接收错误计数器增加
        }
    }
    break;
    case CAN_ISR_EVENT_INT_TX_DONE: // 发送完成中断，param 是邮箱编号
        while (IS_CAN_MAILBOX_INVALID(Can, (int32_t)param))
        {
        }; // TODO: assert
        CanMailbox_t mailbox = &Can->txHandler.pMailboxes[param];

        // 资源回收
        uint32_t intLevel = can_irq_lock();
        _cantx_add_free_msg_list(&Can->txHandler, mailbox);
        Can->statusManager.errCounter.txMsgCount++; // 发送消息计数器增加
        device_write_cb(&Can->parent, param);
        _can_tx_scheduler(Can);
        can_irq_unlock(intLevel);
        break;
    default:
        break;
    }
}

static DevInterface_s CanDevInterface = {
    .init = can_init,
    .open = can_open,
    .write = can_write,
    .read = can_read,
    .control = can_ctrl,
    .close = can_close,
};

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
AwlfRet_e hal_can_register(HalCanHandler_t can, char* name, void* handle, uint32_t regparams)
{
    AwlfRet_e ret;
    if (!can || !name || !can->adapterInterface)
        return AWLF_ERROR_PARAM;
    ret = device_register(&can->parent, name, regparams);
    if (ret != AWLF_OK)
        return ret;

    can->parent.handle = handle;
    can->parent.interface = &CanDevInterface;
    can->cfg = CAN_DEFUALT_CFG;
    return ret;
}


