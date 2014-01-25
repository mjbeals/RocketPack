#include "sensors.h"

#define ACC_I2C_ADDRESS                     0x0F        //00001111
#define GYRO_I2C_ADDRESS                    0x68        //01101000
#define MAG_I2C_ADDRESS          	    0x0E        //00001110
#define BARO_I2C_ADDRESS                    0x77        //01110111
#define HUMID_I2C_ADDRESS                   0x40        //01000000
#define TMP006_I2C_ADDRESS                  0x44        //01000100

void sensor_int_init(void)
{  
    //Gyro Interrupt  P0.1
    P0SEL &= ~(BV(1));    /* Set pin function to GPIO */
    P0DIR &= ~(BV(1));    /* Set pin direction to Input */
    //P0IEN |=   BV(1);     /* enable interrupt generation at port */
    
    //Accelerometer Interrupt  P0.2
    P0SEL &= ~(BV(2));    /* Set pin function to GPIO */
    P0DIR &= ~(BV(2));    /* Set pin direction to Input */
    //P0IEN |=   BV(2);     /* enable interrupt generation at port */
    
    //Magnetometer Interrupt  P0.6
    P0SEL &= ~(BV(6));    /* Set pin function to GPIO */
    P0DIR &= ~(BV(6));    /* Set pin direction to Input */
    //P0IEN |=   BV(6);     /* enable interrupt generation at port */

    //IR Temp Interrupt  P0.3
    P0SEL &= ~(BV(3));    /* Set pin function to GPIO */
    P0DIR &= ~(BV(3));    /* Set pin direction to Input */
    //P0IEN |=   BV(3);     /* enable interrupt generation at port */
    
    P0INP |= BV(1) | BV(2) | BV(6) | BV(3);
    //P2INP |= BV(5);    //enable pull down resistors on P0
    
    /* Clear any pending interrupts */
    P0IFG = ~(BV(1));  //gyro 
    P0IFG = ~(BV(2));  //accelerometer
    P0IFG = ~(BV(6));  //magnetometer
    P0IFG = ~(BV(3));  //IR Therm
    
    P0IFG = 0;
    P0IF  = 0;
    
    IEN1  |= BV(5);    /* enable CPU interrupt for all of port 0*/ 
}


void start_interrupts(uint8 mask)
{
   /* Clear any pending interrupts */
    P0IFG = ~(mask);   
    P0IEN |= mask;
    
    EA = 1;  //make sure interrupts are on
}

void stop_interrupts(uint8 mask)
{
    P0IEN &= ~mask;
}

void sensors_init(void)
{
  //Gyro is powered through P1.1
  P1DIR |= BV(1);
  GYRO_OFF();    //start off
  
  //Make sure the DCDC converter is on
  P0_7 = 1;
}

bool read_sensor(uint8 device)
{
  
  switch(device)
  {
  case ACC_I2C_ADDRESS :
    read_acc();
    break;
  
  case GYRO_I2C_ADDRESS :
    read_gyro();
    break;
    
  case MAG_I2C_ADDRESS :
    read_mag();
    break;
  
  case BARO_I2C_ADDRESS :
    baro_read_press();
    break;
  
  case HUMID_I2C_ADDRESS :
    humid_read_humidity();
    break;
  }
  return TRUE; 
}

/*------------------------------------------------------------------------------
###############################################################################
Gyro
------------------------------------------------------------------------------*/
void start_gyro(void)
{
  GYRO_ON();
  
  HalI2CInit(GYRO_I2C_ADDRESS,   i2cClock_533KHZ);   //select the gyro
  
   //turn on all axes and lock clock to x
  uint8 command = ~GYRO_PWR_MGM_STBY_ALL | GYRO_PWR_MGM_CLOCK_PLL_X | GYRO_PWR_MGM_SLEEP;
  HalSensorWriteReg(GYRO_REG_PWR_MGM, &command,1);
  
  command = GYRO_INT_LATCH | GYRO_INT_CLEAR| GYRO_INT_DATA;
  HalSensorWriteReg(GYRO_REG_INT_CFG, &command, 1);
  
  //initialize fifo bank
 // command = GYRO_FIFO_T | GYRO_FIFO_X | GYRO_FIFO_Y | GYRO_FIFO_Z;
 // HalSensorWriteReg(GYRO_REG_FIFO_EN, &command, 1);
  
  //set sensor poll rate
  command = 0xff;
  HalSensorWriteReg(GYRO_REG_SMPLRT_DIV, &command, 1);
}

void read_gyro(void)
{ 
  uint8 byte;

  HalI2CInit(GYRO_I2C_ADDRESS,   i2cClock_533KHZ);   //select the gyro
  
  byte = GYRO_I2C_ADDRESS;
  flushByte( &byte );
  
  HalSensorReadReg(GYRO_REG_GYRO_XOUT_H,&byte,1);
  flushByte(&byte);  
  HalSensorReadReg(GYRO_REG_GYRO_XOUT_L,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(GYRO_REG_GYRO_YOUT_H,&byte,1);
  flushByte(&byte);
  HalSensorReadReg(GYRO_REG_GYRO_YOUT_L,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(GYRO_REG_GYRO_ZOUT_H,&byte,1);
  flushByte(&byte);
  HalSensorReadReg(GYRO_REG_GYRO_ZOUT_L,&byte,1);
  flushByte(&byte);
}
  
void read_gyro_temp(void)
{
  uint8 byte;
  
  HalI2CInit(GYRO_I2C_ADDRESS,   i2cClock_533KHZ);   //select the gyro
  
  HalSensorReadReg(GYRO_REG_TEMP_OUT_H,&byte,1);
  flushByte(&byte);
  HalSensorReadReg(GYRO_REG_TEMP_OUT_L,&byte,1);
  flushByte(&byte);
}


/****************************************************************************
 Accelerometer
*****************************************************************************/
void init_acc(void)
{
  //Select Accelerometer
  HalI2CInit(ACC_I2C_ADDRESS, i2cClock_267KHZ);
    
  uint8 command = 0x00;          //put to sleep
  HalSensorWriteReg(ACC_CTRL_REG1, &command, 1);
  
  //configure CTRL2 - 6.25Hz
  command = ACC_OWUFB | ACC_OWUFC;
  HalSensorWriteReg(ACC_CTRL_REG2, &command, 1);
  
  // Pulse the interrupt, active high and enable
  command = ACC_IEL | ACC_IEA | ACC_IEN;
  HalSensorWriteReg(ACC_INT_CTRL_REG1, &command, 1);
   
  //Enable interrupt for all axes
  command = ACC_ALLWU;
  HalSensorWriteReg(ACC_INT_CTRL_REG2, &command, 1);
}


void read_acc(void)
{ 
  uint8 byte;

  HalI2CInit(ACC_I2C_ADDRESS, i2cClock_267KHZ);
  
  HalSensorReadReg(ACC_XOUT_L,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(ACC_XOUT_H,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(ACC_YOUT_L,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(ACC_YOUT_H,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(ACC_ZOUT_L,&byte,1);
  flushByte(&byte);
  HalSensorReadReg(ACC_ZOUT_H,&byte,1);
  flushByte(&byte);
}



/*------------------------------------------------------------------------------
###############################################################################
Magnetometer
------------------------------------------------------------------------------*/
void start_mag(void)
{
  //Select Magnetometer
  HalI2CInit(HAL_MAG3110_I2C_ADDRESS,i2cClock_267KHZ);
    
  uint8 command = MAG_AUTO_MRST;          //enable resets
  HalSensorWriteReg(MAG_CTRL_2, &command, 1);
  
  //command = MAG_CTRL1_TM | MAG_640_5HZ| MAG_CTRL1_AC;  //Triggered updates at 5Hz
  command = MAG_1280_10HZ | MAG_CTRL1_TM;
  HalSensorWriteReg(MAG_CTRL_1, &command, 1);
}

void read_mag(void)
{ 
  uint8 byte;

  HalI2CInit(HAL_MAG3110_I2C_ADDRESS,i2cClock_267KHZ);
  
  HalSensorReadReg(MAG_X_LSB,&byte,1);
  flushByte(&byte);
  HalSensorReadReg(MAG_X_MSB,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(MAG_Y_LSB,&byte,1);
  flushByte(&byte);
  HalSensorReadReg(MAG_Y_MSB,&byte,1);
  flushByte(&byte);
  
  HalSensorReadReg(MAG_Z_LSB,&byte,1);
  flushByte(&byte);
  HalSensorReadReg(MAG_Z_MSB,&byte,1);
  flushByte(&byte);
}


/****************************************************************************
 Barometer
*****************************************************************************/
void start_baro(void)
{
  //Select Barometer
  HalI2CInit(BARO_I2C_ADDRESS, i2cClock_267KHZ);
  
  uint8 command = BARO_OFF_COMMAND;
  HalSensorWriteReg(BARO_COMMAND_CTRL, &command, sizeof(command));
}


uint16 baro_capture_press(uint8 resolution)
{
    HalI2CInit(BARO_I2C_ADDRESS, i2cClock_267KHZ); 
    
    uint8 command = BARO_PRESS_READ_COMMAND;
    uint16 delay;
    
        switch(resolution)
        {
        case 2  : command |= BARO_RES_2;  delay = 64;  break;
        case 8  : command |= BARO_RES_8;  delay = 256; break;
        case 16 : command |= BARO_RES_16; delay = 512; break;
        case 64 : command |= BARO_RES_64; delay = 2048; break;
        default : command |= BARO_RES_2;  delay = 64; break;
        }
        
    HalSensorWriteReg(BARO_COMMAND_CTRL, &command, 1);
    return delay;
}

void baro_read_press(void)
{
   uint8  byte;
   
   HalI2CInit(BARO_I2C_ADDRESS, i2cClock_267KHZ); 
   
   HalSensorReadReg(BARO_PRESS_LSB,&byte,1);
   flushByte(&byte);
   
   HalSensorReadReg(BARO_PRESS_MSB,&byte,1);
   flushByte(&byte);
   
}


uint16 baro_capture_temp(uint8 resolution)
{
    HalI2CInit(BARO_I2C_ADDRESS, i2cClock_267KHZ); 
    
    uint8 command = BARO_TEMP_READ_COMMAND;
    uint16 delay;
    
        switch(resolution)
        {
        case 2  : command |= BARO_RES_2;  delay = 64;  break;
        case 8  : command |= BARO_RES_8;  delay = 256; break;
        case 16 : command |= BARO_RES_16; delay = 512; break;
        case 64 : command |= BARO_RES_64; delay = 2048; break;
        default : command |= BARO_RES_2;  delay = 64; break;
        }
        
    HalSensorWriteReg(BARO_COMMAND_CTRL, &command, 1);
    return delay;
}

void baro_read_temp(void)
{
   uint8 byte;
   
   HalI2CInit(BARO_I2C_ADDRESS, i2cClock_267KHZ); 
   
   HalSensorReadReg(BARO_TEMP_LSB,&byte,1);
   flushByte(&byte);
   
   HalSensorReadReg(BARO_TEMP_MSB,&byte,1);
   flushByte(&byte);
}

void baro_shutdown(void)
{
   HalI2CInit(BARO_I2C_ADDRESS, i2cClock_267KHZ); 
   uint8 command = BARO_OFF_COMMAND;
   HalSensorWriteReg(BARO_COMMAND_CTRL, &command, 1);
}


/****************************************************************************
  Humidity
*****************************************************************************/
#define HUMID_RES_DEFAULT      HUMID_RES1
 
uint8 humid_resolution = HUMID_RES_DEFAULT;
static uint8 humid_usr;

void humid_init(void)
{
  HalI2CInit(HUMID_I2C_ADDRESS,i2cClock_267KHZ);

  // Set 11 bit resolution
  HalSensorReadReg(HUMID_READ_U_R,&humid_usr,1);
  humid_usr &= HUMID_RES_MASK;  //zero out resolution bits
  humid_usr |= humid_resolution;
  HalSensorWriteReg(HUMID_WRITE_U_R,&humid_usr,1);
}


void humid_read_humidity(void)
{
  
  uint8 buffer[6];
  
  HalI2CInit(HUMID_I2C_ADDRESS,i2cClock_267KHZ);
  
  HumidWriteCmd(HUMID_TEMP_T_H);
  HumidReadData(buffer, DATA_LEN);
  
  HumidWriteCmd(HUMID_HUMI_T_H);
  HumidReadData(buffer+DATA_LEN, DATA_LEN);
  
  buffer[1] &= ~0x03;
  buffer[4] &= ~0x03;
  
  flushByte(&buffer[0]);
  flushByte(&buffer[1]);
  
  flushByte(&buffer[3]);
  flushByte(&buffer[4]);
}


static bool HumidWriteCmd(uint8 cmd)
{
  /* Send command */
  return HalI2CWrite(1,&cmd) == 1;
}

static bool HumidReadData(uint8 *pBuf, uint8 nBytes)
{
  /* Read data */
  return HalI2CRead(nBytes,pBuf ) == nBytes;
}












