#include "drivers/model/device.h"
#include "drivers/peripheral/pal_dev.h"


void can_rx_callback(Device_t dev, void *param, CanFilterHandle_t handle, size_t msgCount)
{
    CanUserMsg_s msg;
    device_read(dev, 0, &msg, 1);
}

int main(void)
{
    Device_t can = device_find("can1");
    CanFilterAllocArg_s filter_arg = {
       // .request = CAN_FILTER_REQUEST_INIT(CAN_FILTER_MODE_LIST, CAN_FILTER_ID_STD, )
    };
    device_open(can, CAN_O_INT_RX | CAN_O_INT_TX);
    
    device_ctrl(can, CAN_CMD_FILTER_ALLOC, &filter_arg);
    for(;;)
    {
        
    }
}