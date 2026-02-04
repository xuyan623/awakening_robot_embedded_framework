/*
 * @Description: CAN婵☆垪鈧櫕鍋ラ悗鍦仧楠炲洭寮崶锔筋偨
 * @date 2025-11-10
 * @author 濞达絾鐟﹂弮?
 */
#include "bsp_can.h"
#include <string.h>

// 濞寸姰鍎扮粭鍛偘鐏炵偓鏆堥梺鎻掞功濞堟垶鎷呭鍛殢闁?閻?clang-format
// off"闁?clang-format
// on"濞戞柨顑夊Λ鍧楁儍閸曨亜鏁╅柣顔荤閸櫻囨⒒椤撶喓澹愮€殿喖绻愮€?
// clang-format off
static uint32_t gBs1Table[CAN_TSEG1_MAX] =
{
    CAN_BS1_1TQ, CAN_BS1_2TQ, CAN_BS1_3TQ,
    CAN_BS1_4TQ, CAN_BS1_5TQ, CAN_BS1_6TQ,
    CAN_BS1_7TQ, CAN_BS1_8TQ, CAN_BS1_9TQ,
    CAN_BS1_10TQ, CAN_BS1_11TQ, CAN_BS1_12TQ,
    CAN_BS1_13TQ, CAN_BS1_14TQ, CAN_BS1_15TQ, 
    CAN_BS1_16TQ,
};

static uint32_t gBs2Table[CAN_TSEG2_MAX] =
{
    CAN_BS2_1TQ, CAN_BS2_2TQ, CAN_BS2_3TQ,
    CAN_BS2_4TQ, CAN_BS2_5TQ, CAN_BS2_6TQ,
    CAN_BS2_7TQ, CAN_BS2_8TQ,
};

// clang-format on

static inline uint32_t bsp_can_bs1_trans(CanBS1_e bs1)
{
    while (bs1 >= CAN_TSEG1_MAX || bs1 < 0)
    {
    } // TODO: assert
    return gBs1Table[bs1];
}

static inline uint32_t bsp_can_bs2_trans(CanBS2_e bs2)
{
    while (bs2 >= CAN_TSEG2_MAX || bs2 < 0)
    {
    } // TODO: assert
    return gBs2Table[bs2];
}

static uint32_t bsp_can_sjw_trans(CanSjw_e sjw)
{
    uint32_t ret = 0;
    switch (sjw)
    {
    case CAN_SYNCJW_1TQ:
        ret = CAN_SJW_1TQ;
        break;
    case CAN_SYNCJW_2TQ:
        ret = CAN_SJW_2TQ;
        break;
    case CAN_SYNCJW_3TQ:
        ret = CAN_SJW_3TQ;
        break;
    case CAN_SYNCJW_4TQ:
        ret = CAN_SJW_4TQ;
        break;
    default:
        while (1)
        {
        }; // TODO: assert
        break;
    }
    return ret;
}

static AwlfRet_e bsp_can_set_filter(CAN_HandleTypeDef* hcan, CanFilterCfg_t cfg)
{
    CAN_FilterTypeDef FilterConfig;
    FilterConfig.FilterBank = cfg->bank;
    FilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    FilterConfig.FilterActivation = ENABLE;
    FilterConfig.FilterFIFOAssignment = (cfg->bank % 2 == 0) ? CAN_FILTER_FIFO0 : CAN_FILTER_FIFO1;
    // STM32F4闁汇劌鍤婣N濞戞捁妗ㄧ€靛本绂掑顥缂備焦鎸婚悗顖炴晬鐎涘N1濞戞捁妗ㄧ€靛瓔AN闁挎稑鐡擜N2濞戞捁妗ㄧ划鐕橝N闁挎稑鑻崣锟犳偨?缂?8濞戞搩浜濋幎銈呪枖閵忕姵鐝ら柨娑樻湰濠€鐗堛仚閸楃偛袟缂佸顑呯花顓㈡焻婢跺顏ラ悗鐢垫嚀瀹曟劙宕?
    FilterConfig.SlaveStartFilterBank = 14;
    // 闂佹澘绉堕悿鍡楊煥閵堝棗鐨鹃柛锝冨妼瀵剟寮?
    if (cfg->workMode == CAN_FILTER_MODE_MASK)
    {
        uint32_t id;
        uint32_t mask;
        FilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
        if (cfg->idType == CAN_FILTER_ID_STD) // 濞寸姴鎳忛悥锝夊礄閸℃濮?
        {
            id = cfg->id << 5;
            mask = cfg->mask << 5;
            FilterConfig.FilterIdHigh = id;
            FilterConfig.FilterIdLow = 0;
            FilterConfig.FilterMaskIdHigh = mask;
            FilterConfig.FilterMaskIdLow = CAN_ID_EXT;
        }
        else
        {
            id = cfg->id << 3;
            mask = cfg->mask << 3;
            FilterConfig.FilterIdHigh = (id >> 16) & 0xffff;
            FilterConfig.FilterIdLow = (id & 0xffff);

            FilterConfig.FilterMaskIdHigh = (mask >> 16) & 0xffff;
            FilterConfig.FilterMaskIdLow = (mask & 0xffff);

            if (cfg->idType == CAN_FILTER_ID_EXT) // 濞寸姴鎳忕€氬洨浠﹂弴鐐村
            {
                FilterConfig.FilterIdLow |= CAN_ID_EXT;
                FilterConfig.FilterMaskIdLow |= CAN_ID_EXT;
            }
            else // 闁哄秴娲ら崳顖滄暜?+ 闁归攱鎸搁惈宥囨暜?
            {
                FilterConfig.FilterIdLow = (id & 0xffff);
                FilterConfig.FilterMaskIdLow &= ~CAN_ID_EXT;
            }
        }
    }
    else
    {
        FilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
        if (cfg->idType == CAN_FILTER_ID_STD)
        {
            FilterConfig.FilterIdHigh = cfg->id << 5;
            FilterConfig.FilterIdLow = 0;
        }
        else
        {
            if (cfg->idType == CAN_FILTER_ID_EXT)
                FilterConfig.FilterIdLow = ((cfg->id << 3) & 0xffff) | CAN_ID_EXT;
            else
                FilterConfig.FilterIdLow = ((cfg->id << 3) & 0xffff);
            FilterConfig.FilterIdHigh = ((cfg->id << 3) >> 16) & 0xffff;
        }
    }
    if (HAL_CAN_ConfigFilter(hcan, &FilterConfig) != HAL_OK)
        return AWLF_ERROR;
    return AWLF_OK;
}

// 婵炲鍨绘竟鎺楁偝閸ヮ剙甯崇紓鍐惧枦閵嗗啴鏁嶇€涘N闁哄啫鐖奸幐?2MHz闁挎稑濂旂粭澶愬触鐏炵厧鐦滃Λ鐗堝灴濞撳墎鎲版担閿嬪弿闁衡偓绾拋鍤夐悶?
static CanTimeCfg_s BspCanBitTimeTable[] = {
    {CAN_BAUD_10K, 300, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_20K, 150, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_50K, 60, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_100K, 30, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_125K, 24, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_250K, 12, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_500K, 6, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_800K, 4, {CAN_TSEG1_8TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
    {CAN_BAUD_1M, 3, {CAN_TSEG1_9TQ, CAN_TSEG2_4TQ, CAN_SYNCJW_2TQ}},
};

static CanTimeCfg_t bsp_can_time_cfg_matched(CanBaudRate_e baud)
{
    for (int i = 0; i < sizeof(BspCanBitTimeTable) / sizeof(BspCanBitTimeTable[0]); i++)
    {
        if (BspCanBitTimeTable[i].baudRate == baud)
        {
            return &BspCanBitTimeTable[i];
        }
    }
    while (1)
    {
    }; // TODO: assert
}

static AwlfRet_e bsp_can_configure(HalCanHandler_t Can, CanCfg_t cfg)
{
    BspCan_t bsp_can = (BspCan_t)Can->parent.handle;
    CAN_HandleTypeDef* hcan = (CAN_HandleTypeDef*)bsp_can;
    CanTimeCfg_t TimeCfg = bsp_can_time_cfg_matched(cfg->normalTimeCfg.baudRate);
    CanBS1_e bs1 = TimeCfg->bitTimeCfg.bs1;
    CanBS2_e bs2 = TimeCfg->bitTimeCfg.bs2;
    CanSjw_e sjw = TimeCfg->bitTimeCfg.syncJumpWidth;
    hcan->Init.Prescaler = TimeCfg->psc;
    switch (cfg->workMode)
    {
    case CAN_WORK_NORMAL:
        hcan->Init.Mode = CAN_MODE_NORMAL;
        break;
    case CAN_WORK_LOOPBACK:
        hcan->Init.Mode = CAN_MODE_LOOPBACK;
        break;
    case CAN_WORK_SILENT:
        hcan->Init.Mode = CAN_MODE_SILENT;
        break;
    case CAN_WORK_SILENT_LOOPBACK:
        hcan->Init.Mode = CAN_MODE_SILENT_LOOPBACK;
        break;
    default:
        while (1)
        {
        }; // TODO: assert
        break;
    }
    hcan->Init.SyncJumpWidth = bsp_can_sjw_trans(sjw);
    hcan->Init.TimeSeg1 = bsp_can_bs1_trans(bs1);
    hcan->Init.TimeSeg2 = bsp_can_bs2_trans(bs2);
    hcan->Init.ReceiveFifoLocked = (cfg->functionalCfg.rxFifoLockMode == 1) ? ENABLE : DISABLE;
    hcan->Init.TimeTriggeredMode = (cfg->functionalCfg.timeTriggeredMode == 1) ? ENABLE : DISABLE;
    hcan->Init.AutoBusOff = (cfg->functionalCfg.autoBusOff == 1) ? ENABLE : DISABLE;
    hcan->Init.AutoRetransmission = (cfg->functionalCfg.autoRetransmit == 1) ? ENABLE : DISABLE;
    hcan->Init.AutoWakeUp = (cfg->functionalCfg.autoWakeUp == 1) ? ENABLE : DISABLE;
    if (HAL_CAN_Init(hcan) != HAL_OK)
    {
        while (1)
        {
        }; // TODO: assert
    }
    HAL_CAN_ActivateNotification(hcan, CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_ERROR_PASSIVE | CAN_IT_ERROR_WARNING | CAN_IT_LAST_ERROR_CODE);
    return AWLF_OK;
}

static AwlfRet_e bsp_can_control(HalCanHandler_t Can, uint32_t cmd, void* arg)
{
    if (!Can || !Can->parent.handle)
        return AWLF_ERROR_PARAM;

    CAN_HandleTypeDef* hcan = (CAN_HandleTypeDef*)Can->parent.handle;
    AwlfRet_e ret = AWLF_OK;

    switch (cmd)
    {
    case CAN_CMD_GET_STATUS:
    {
        CanErrCounter_t errCounter = (CanErrCounter_t)arg;
        uint32_t errReg = READ_REG(hcan->Instance->ESR);
        errCounter->txErrCnt = (errReg >> 16) & 0xFF;
        errCounter->rxErrCnt = (errReg >> 24);
        ret = AWLF_OK;
    }
    break;
    case CAN_CMD_START:
        // 闁告凹鍨版慨銆N闂侇偅鐭穱?
        if (HAL_CAN_Start(hcan) != HAL_OK)
            ret = AWLF_ERROR;
        break;

    case CAN_CMD_CFG:
        // 闂佹澘绉堕悿鍜癆N
        ret = bsp_can_configure(Can, (CanCfg_t)arg);
        break;

    case CAN_CMD_SUSPEND:
        // 闁哄棗鍊告禒鐕橝N闂侇偅鐭穱?
        if (HAL_CAN_Stop(hcan) != HAL_OK)
            ret = AWLF_ERROR;
        break;

    case CAN_CMD_RESUME:
        // 闁诡厹鍨归ˇ鐫燗N闂侇偅鐭穱?
        if (HAL_CAN_Start(hcan) != HAL_OK)
            ret = AWLF_ERROR;
        break;

    case CAN_CMD_SET_IOTYPE:
        // 閻犱礁澧介悿鍜癆N IO缂侇偉顕ч悗鐑芥晬鐏炵厧鈻忛柤铏灊閼垫垿寮?
        while (arg == NULL)
        {
        }; // TODO: assert
        uint32_t io_type = *(uint32_t*)arg;
        if (io_type == CAN_REG_INT_TX)
        {
            // 濞达綀鍎婚崗姗€宕ｉ幋锔瑰亾娴ｇ柉鍘柡?
            uint32_t txIntEvents = CAN_IT_TX_MAILBOX_EMPTY;
            HAL_CAN_ActivateNotification(hcan, txIntEvents);
        }
        else if (io_type == CAN_REG_INT_RX)
        {
            uint32_t rxIntEvents = CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_RX_FIFO0_OVERRUN |
                                   CAN_IT_RX_FIFO1_OVERRUN | CAN_IT_RX_FIFO0_FULL | CAN_IT_RX_FIFO1_FULL;
            HAL_CAN_ActivateNotification(hcan, rxIntEvents);
        }
        break;

    case CAN_CMD_CLR_IOTYPE:
    {
        while (arg == NULL)
        {
        }; // TODO: assert
        // 婵炴挸鎳樺▍宥N IO缂侇偉顕ч悗鐑芥晬瀹€鈧々锕傛偨閵娿倛鍘柡?
        uint32_t io_type = *(uint32_t*)arg;
        if (io_type == CAN_REG_INT_TX)
        {
            // 濠㈡儼绮鹃崗姗€宕ｉ幋锔瑰亾娴ｇ柉鍘柡?
            HAL_CAN_DeactivateNotification(hcan, CAN_IT_TX_MAILBOX_EMPTY);
        }
        else if (io_type == CAN_REG_INT_RX)
        {
            uint32_t rxIntEvents = CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING | // 闁规亽鍎查弫瑙勭▔椤撶喐鐒?
                                   CAN_IT_RX_FIFO0_OVERRUN | CAN_IT_RX_FIFO1_OVERRUN |         // 闁规亽鍎查弫鐟扳攦閵忕姴姣?
                                   CAN_IT_RX_FIFO0_FULL | CAN_IT_RX_FIFO1_FULL;                // 闁规亽鍎查弫绗稩FO婵?
            HAL_CAN_DeactivateNotification(hcan, rxIntEvents);
        }
    }
    break;

    case CAN_CMD_CLOSE:
        // 闁稿繑濞婂Λ纰圓N閻犱焦鍎抽ˇ?
        if (HAL_CAN_Stop(hcan) != HAL_OK)
            ret = AWLF_ERROR;
        // TODO:
        // 閻熸瑱绲介崹鍨叏鐎ｎ亜顕ч柨娑樿嫰閸櫻囨⒒椤撶喐顦ч梺鐣屽櫐缁辨繄绮嬫担鐑樻殢濞戞搩鍘介弻?
        break;

    case CAN_CMD_FLUSH:
        // 婵炴挸鎳愰埞鏍磽閹惧磭鎽?- 閻庣敻鈧稓鑹維TM32
        // CAN闁挎稑鐭傚〒鍓佹啺娴ｅ湱顏哥紒宀€鍎ょ敮鎾绩缁傛┃FO
        hcan->Instance->RF0R |= CAN_RF0R_RFOM0;
        hcan->Instance->RF1R |= CAN_RF1R_RFOM1;
        break;

    case CAN_CMD_SET_FILTER:
        // 閻犱礁澧介悿鍡楊煥閵堝棗鐨鹃柛?
        while (!arg)
        {
        }; // TODO: assert
        {
            CanFilterCfg_t filter_cfg = (CanFilterCfg_t)arg;
            while ((hcan->Instance == CAN1) && filter_cfg->bank >= 14)
            {
            }; // TODO: assert
            while ((hcan->Instance == CAN2) && filter_cfg->bank >= 28)
            {
            }; // TODO: assert
            ret = bsp_can_set_filter(hcan, filter_cfg);
        }
        break;

    default:
        ret = AWLF_ERROR_PARAM;
        break;
    }

    return ret;
}

static AwlfRet_e bsp_can_recv_msg(HalCanHandler_t Can, CanUserMsg_t msg, int32_t rxfifo_bank)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t data[8];

    CAN_HandleTypeDef* hcan = (CAN_HandleTypeDef*)Can->parent.handle;

    // 婵☆偀鍋撻柡灞诲劜鐢挳寮ㄧ粋姗O闁哄嫷鍨伴幆浣圭▔閾忓厜鏁?
    uint32_t fifo_fill_level = HAL_CAN_GetRxFifoFillLevel(hcan, rxfifo_bank);
    if (fifo_fill_level == 0)
        return AWLF_ERROR_EMPTY;

    // 濞寸姴瀛╃€垫氨鈧顑慖FO閻犲洩顕цぐ鍥р槈閸喍绱?
    if (HAL_CAN_GetRxMessage(hcan, rxfifo_bank, &rx_header, data) != HAL_OK)
        return AWLF_ERROR;

    if (!msg) // 闁兼椿娅漵g濞戞捁娅ｉ埞鏍晬瀹€鍐惧殯闁哄嫬瀛╅、瀣几鐠鸿櫣婀碦xFifo濞戞捁娅ｉ埞鏍ㄧ▔閺傚墽鐟濋梺鎻掓搐瑜板洨鎲伴崱妤€鏅哥紒娑欑墱閺嗘劙鏁嶅畝鈧ú鍧楀箳閵夈劎绠查柛銉у仦鐎涒晠宕欏ú顏呮櫓閻?
        return AWLF_ERR_OVERFLOW;

    // 濠靛鍋勯崢鏍偨閵婏箑鐓曟繛鎴濈墛娴煎懐绱掗幘瀵糕偓?
    msg->dsc.id = (rx_header.IDE == CAN_ID_STD) ? rx_header.StdId : rx_header.ExtId;
    msg->dsc.idType = (rx_header.IDE == CAN_IDE_STD) ? CAN_IDE_STD : CAN_IDE_EXT;
    msg->dsc.msgType = (rx_header.RTR == CAN_RTR_DATA) ? CAN_MSG_TYPE_DATA : CAN_MSG_TYPE_REMOTE;
    msg->dsc.dataLen =
        rx_header
            .DLC; // 闁革负鍔庣划锟犲礂缁愩€濞戞搩鍙忕槐婕嘥M32闁汇劌鍤峀C濞戞挸瀛╅、瀣几閺堜絻鍘柣銊ュ灱ataLen闁哄嫷鍨粩鎾嚊鐎靛憡鐣遍柨?
                  // <= dataLen <= 8)
    // 閻犱礁澧介悿鍡楊煥閵堝棗鐨鹃柛锝冨妿缁鳖亪宕?
    msg->bank = rx_header.FilterMatchIndex;
    msg->dsc.timeStamp = rx_header.Timestamp;
    memcpy(msg->userBuf, data, msg->dsc.dataLen);
    return AWLF_OK;
}

static AwlfRet_e bsp_can_send_msg(HalCanHandler_t Can, CanUserMsg_t msg)
{
    CAN_TxHeaderTypeDef tx_header;
    if (!Can || !Can->parent.handle || !msg)
        return AWLF_ERROR_PARAM;

    CAN_HandleTypeDef* hcan = (CAN_HandleTypeDef*)Can->parent.handle;

    // 婵☆偀鍋撻柡灞诲劚瑜板倿鏌呮笟鈧崑鏍不鏉堛劍笑闁告熬绠戦崙鈥愁煥?
    uint32_t free_level = HAL_CAN_GetTxMailboxesFreeLevel(hcan);
    if (free_level == 0)
    {
        msg->bank = -1;
        return AWLF_ERR_OVERFLOW;
    }

    // 闁告垵妫楅ˇ顒勫矗閹达腹鍋撴担鎼炰粓
    tx_header.StdId = msg->dsc.id;
    tx_header.ExtId = msg->dsc.id;
    tx_header.IDE = (msg->dsc.idType == CAN_IDE_EXT) ? CAN_ID_EXT : CAN_ID_STD;
    tx_header.RTR = (msg->dsc.msgType == CAN_MSG_TYPE_REMOTE) ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    tx_header.DLC = msg->dsc.dataLen;
    tx_header.TransmitGlobalTime = DISABLE; // 鐟滅増鎸告晶鐘诲几閼哥數鈧垶寮抽崒娑欘槯濞戞挸绉甸弫顕€骞?
    // 闁告垵妫楅ˇ顒勫矗閹达腹鍋撴担瑙勬闁?
    uint8_t data[8];
    if (msg->userBuf != NULL && msg->dsc.dataLen > 0)
    {
        memcpy(data, msg->userBuf, msg->dsc.dataLen);
    }
    // 闁煎浜滄慨鈺呮焻婢跺顏ラ柛娆愬灴閳ь兛绶氶崑鏍不閸楃偠瀚欓柛娆愬灴閳ь兛鐒︾粔鐑藉箒?
    uint32_t txMailboxBank = 0;
    if (HAL_CAN_AddTxMessage(hcan, &tx_header, data, &txMailboxBank) != HAL_OK)
    {
        msg->bank = -1;
        return AWLF_ERROR;
    }

    // 濠靛鍋勯崢鏍矗閹达腹鍋撴笟鈧崑鏍不鏉堚晛鍋嶇€?
    switch (txMailboxBank)
    {
    case CAN_TX_MAILBOX0:
        msg->bank = 0;
        break;
    case CAN_TX_MAILBOX1:
        msg->bank = 1;
        break;
    case CAN_TX_MAILBOX2:
        msg->bank = 2;
        break;
    }
    return AWLF_OK;
}

static CanHwInterface_s gCanHwInterface = {
    .configure = bsp_can_configure,
    .control = bsp_can_control,
    .recv_msg = bsp_can_recv_msg,
    .send_msg_mailbox = bsp_can_send_msg,
};

BspCan_s gBspCan[] = {
#ifdef USE_CAN1
    BSP_CAN_STATIC_INIT(CAN1, "can1", CAN1_REG_PARAMS),
#endif
#ifdef USE_CAN2
    BSP_CAN_STATIC_INIT(CAN2, "can2", CAN2_REG_PARAMS),
#endif
};

static void bsp_can_pre_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
#ifdef USE_CAN1
    {
        CAN_HandleTypeDef* hcan = &gBspCan[BSP_CAN1_IDX].handle;
        (void)hcan;
    }
    __HAL_RCC_CAN1_CLK_ENABLE();
    if (__HAL_RCC_GPIOD_IS_CLK_DISABLED())
        __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    HAL_NVIC_SetPriority(
        CAN1_RX0_IRQn, 0,
        0); // 濞村吋锚閸樻稓鐥閻楁挳骞戦鈧〒鍓佹啺娴ｇ晫娈堕柡浣芥彧缁辨繈鎳撻崘顓燁€氶柛鎺旀珔M闁哄牆鎼▍鎺撶鏉炴媽鍘珻AN闁诡剝宕甸崵搴ㄦ儍閸曨垰娅㈤悷鏇氱劍閳ь儸宥囩閺夆晜鐟╅崳椋庣磼?
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
#endif
#ifdef USE_CAN2
    {
        CAN_HandleTypeDef* hcan = &gBspCan[BSP_CAN2_IDX].handle;
        __HAL_RCC_CAN2_CLK_ENABLE();
        if (__HAL_RCC_GPIOB_IS_CLK_DISABLED())
            __HAL_RCC_GPIOB_CLK_ENABLE();
        HAL_CAN_DeInit(hcan);
        GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
        HAL_NVIC_SetPriority(CAN2_RX1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN2_RX1_IRQn);
    }
#endif
}

void bsp_can_register(void)
{
    uint8_t cnt = sizeof(gBspCan) / sizeof(gBspCan[0]);
    AwlfRet_e ret = AWLF_OK;
    for (int i = 0; i < cnt; i++)
    {
        gBspCan[i].parent.hwInterface = &gCanHwInterface;
        gBspCan[i].parent.adapterInterface = hal_can_get_classic_adapter_interface();
        ret = hal_can_register(&gBspCan[i].parent, gBspCan[i].name, &gBspCan[i], gBspCan[i].regparams);
        while (ret != AWLF_OK)
        {
        }; // TODO: assert
    }
    bsp_can_pre_init();
}
