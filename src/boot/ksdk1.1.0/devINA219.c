#include <stdlib.h>

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;



/*
 *	INA219 thing.
 */
void
initINA219(const uint8_t i2cAddress, WarpI2CDeviceState volatile *  deviceStatePointer)
{
	deviceStatePointer->i2cAddress	= i2cAddress;
	return;
}

WarpStatus
readSensorRegisterINA219(uint8_t deviceRegister)
{	// Create variables
        uint8_t txBuf[2]        = {0xFF, 0xFF};
	uint8_t 	cmdBuf[1]	= {0xFF};
	i2c_status_t	returnValue;

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};
	SEGGER_RTT_WriteString(0, "Set I2C parameters");
	
	txBuf[0] = 0xFF; // Setting these equal to zero will lead to powerdown...
        txBuf[1] = 0xFF;
        cmdBuf[0] = 0x00; // Configuration address

        returnValue = I2C_DRV_MasterSendDataBlocking(
                                                      0, // I2C peripheral instance,
                                                      &slave,
                                                      cmdBuf,
                                                      1,
                                                      txBuf,
                                                      2,
                                                      100); // timeout in milliseconds //
        
	SEGGER_RTT_printf(0, "Sent something to the sensor");
	
	// Reset
	cmdBuf[0] = 0x05; // Calibration register address

	txBuf[0] = 0xC8; // Calibration value MSB
	txBuf[1] = 0x00; // Calibration value LSB

	// Send bits to the calibration register
	returnValue = I2C_DRV_MasterSendDataBlocking(
                                                      0 /* I2C peripheral instance */,
                                                      &slave,
                                                      cmdBuf,
                                                      1,
                                                      txBuf,
                                                      2,
                                                      100 /* timeout in milliseconds */);

        OSA_TimeDelay(100);
	SEGGER_RTT_WriteString(0, "Set calibration register");
	// Write to the current sensor (prod it)
	txBuf[0] = 0x01;
	txBuf[1] = 0x01;
	cmdBuf[0] = 0x04;

	returnValue = I2C_DRV_MasterSendDataBlocking(
		0,
		&slave,
		cmdBuf,
		1,
		txBuf,
		0,
		100);
        
        // Read current/power/voltage
        
	cmdBuf[0] = 0x04; // Current register address
	for(int i = 0; i<1000; i++){
        	returnValue = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							NULL,
							0,
							(uint8_t *)deviceINA219State.i2cBuffer,
							2,// 2 bytes of data received
							500 /* timeout in milliseconds */);
		SEGGER_RTT_printf(0, "\n\r %02x%02x", deviceINA219State.i2cBuffer[0], deviceINA219State.i2cBuffer[1]);
	}

	SEGGER_RTT_printf(0, "\r\nI2C_DRV_MasterReceiveData returned [%d]\n", returnValue);

	if (returnValue == kStatus_I2C_Success)
	{
		SEGGER_RTT_printf(0, "\r[0x%02x]	0x%02x\n", cmdBuf[0], deviceINA219State.i2cBuffer[0]);
		SEGGER_RTT_WriteString(0, "HOORAY!");
	}
	else
	{
//		SEGGER_RTT_printf(0, kWarpConstantStringI2cFailure, cmdBuf[0], returnValue);
		SEGGER_RTT_WriteString(0, "Didn't work");
		return kWarpStatusDeviceCommunicationFailed;
	}	


	return kWarpStatusOK;
}
