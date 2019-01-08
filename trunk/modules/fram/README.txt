FRAM device driver
===============================================================================
Copyright (C) 2009 Matthias Mächler (maechler@mm-computing.ch, thismaechler@gmx.ch)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Error Codes:
------------
FRAM_ERROR_TRANSFER_TYPE // bad transfer type selected
FRAM_ERROR_DEVICE_BLOCKED  // IIC device is occupied

// IIC layer errors
MIOS32_IIC_ERROR_TIMEOUT 
MIOS32_IIC_ERROR_ARBITRATION_LOST
MIOS32_IIC_ERROR_BUS
MIOS32_IIC_ERROR_SLAVE_NOT_CONNECTED
MIOS32_IIC_ERROR_UNEXPECTED_EVENT