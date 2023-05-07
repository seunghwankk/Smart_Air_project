#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx.h"
#include "uart4.h"
#include "uart2.h"
#include "tim10_motor.h"
#include "key.h"
#include "step.h"
#include "fan.h"
#include "servo.h"

/******************************************************************************
* General-purpose timers TIM10

  포트 연결:

  PA0,PA1 : UART4 RX,TX  Bluetooth 
  PA2,PA3 : UART2 RX,TX  Debug
    
  PA5 : FAN

  PB8  : SERVO Motor

  PB9  : DC Motor M_EN
  PB10 : DC Motor M_D2
  PB11 : DC Motor M_D1

  PC0~3 : STEP Motor ST_A~ST_B/
 
  PC14 ==> Push Button 1

******************************************************************************/
#define CMD_SIZE 50
#define ARR_CNT 9  

volatile int pwm = 50;
volatile int sendFlag=0;
volatile int pushFlag=0;
double temp = 0, humi = 0, dust = 0;
extern volatile unsigned long systick_sec;
extern volatile int systick_secFlag;
extern volatile unsigned char rx2Flag;
extern char rx2Data[50];
extern volatile unsigned char rx4Flag;
extern char rx4Data[50];
extern int key;
extern int step_count;
char sendData[50]={0};
char recvData[50]={0};

void Motor_Right();
void Motor_Stop();
void Fan_On();
void Fan_Off();
void Motor_Pwm(int val);
void Step_Motor_Drive(int steps, int direction);
void Step_Motor_Stop();


static void Delay(const uint32_t Count)
{
  __IO uint32_t index = 0;
  for(index = (16800 * Count); index != 0; index--);
}

int main()
{
  int old_pwm=50;
  int ccr1;
  int pwmCount = 0;

  Key_Init();
  UART2_init();
  UART4_init();
  TIM10_Motor_Init();
  Step_Motor_Init();
  Fan_Init();
  pwmCount = pwm * 100;
  Serial2_Send_String("start main()\r\n");
  
  while(1)
  {
    if(pushFlag)
    {
      Serial4_Send_String("[FCM]TooHighFineDust\r\n");
      pushFlag = 0;
    }
    
    if(rx2Flag)
    {
      printf("rx2Data : %s\r\n",rx2Data);
      Serial4_Send_String(rx2Data);
      Serial4_Send_String("\r");
      rx2Flag = 0;
    }  
    if(rx4Flag)
    {
      Serial4_Event();
      rx4Flag = 0;
    }
    if(sendFlag){
      Serial4_Send_String("[SQL]GETSENSOR\r\n");
      sendFlag=0;
    }
    if(key != 0)
    {
      printf("Key : %d  \r\n",key);
      if(key == 2)
      {
          sendFlag = 1;
          key=0;
      }
    }
        
    if(pwm != old_pwm  )
    {
        if(pwm == 0)          
             ccr1 = 1;
        else if(pwm == 100)
             ccr1 = 177 * 100 - 1;
        else
             ccr1 = 177 * pwm;
        
        TIM10->CCR1 = ccr1;
        old_pwm = pwm;
        //pwmCount = pwm * 100;
        printf("PWM : %d\r\n",pwm);
    }
    
    
  }
}

void Serial4_Event()
{
  int i=0;
  int temp = 0;
  int humi = 0;
  int dust = 0;
  char * pToken;
  char * pArray[ARR_CNT]={0};
  char recvBuf[CMD_SIZE]={0};
  char sendBuf[CMD_SIZE]={0};
  
  strcpy(recvBuf,rx4Data);

  //Serial2_Send_String(recvBuf);
  //Serial2_Send_String("\n\r");
  
  printf("rx4Data : %s\r\n",recvBuf);
     
  pToken = strtok(recvBuf,"[@]");

  while(pToken != NULL)
  {
    pArray[i] =  pToken;
    if(++i >= ARR_CNT)
      break;
    pToken = strtok(NULL,"[@]");
  }
  
  if(!strcmp(pArray[1],"VAL"))
  {
    humi = atoi(pArray[2]);
    temp = atoi(pArray[3]);
    dust = atoi(pArray[4]);
    
    if(pArray[2] != NULL)                  //DC모터(제습기)
    {
      if(humi > 40)
      {
        Motor_Right();                       //제습기 가동
      }
      else
      {
        Motor_Stop();
      }
    }
    
    if(pArray[3] != NULL)               //STEP모터(에어컨)
    {
      if(temp > 30)
      {
        Step_Motor_Drive(2000, 1);   //에어컨 가동
      }
      else
      {
        Step_Motor_Stop();
      }
    }
    
    if(pArray[4] != NULL)  //SERVO모터(창문), FAN(공기청정기)
    {
      if(dust < 10)
      {
        TIM10->CCR1 = 1500;                     
        Fan_Off();
      }
      else if(dust < 50)
      {
        TIM10->CCR1 = 700;         //창문 열기
        Fan_Off();
      }
      else
      {
        TIM10->CCR1 = 700;
        Fan_On();                                      
        pushFlag = 1;
      }
    }
    
  }
  else if(!strncmp(pArray[1]," New conn",sizeof(" New conn")))
  {
      return;
  }
  else if(!strncmp(pArray[1]," Already log",sizeof(" Already log")))
  {
      return;
  }    
  else
    return;
  
  //sprintf(sendBuf,"[%s]%s@%s\n",pArray[0],pArray[1],pArray[2]);
  //Serial4_Send_String(sendBuf);
}

void Motor_Right()
{
  GPIO_WriteBit(GPIOB,GPIO_Pin_9,Bit_SET); 
  GPIO_WriteBit(GPIOB,GPIO_Pin_10,Bit_SET);
  GPIO_WriteBit(GPIOB,GPIO_Pin_11,Bit_RESET);
}

void Motor_Pwm(int val)
{
    pwm = val;
}

void Motor_Stop()
{
  GPIO_WriteBit(GPIOB,GPIO_Pin_9,Bit_RESET);  
  GPIO_WriteBit(GPIOB,GPIO_Pin_10,Bit_RESET);
  GPIO_WriteBit(GPIOB,GPIO_Pin_11,Bit_RESET);
}
