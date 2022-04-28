/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
//STM32操作相关头文件
#include "stm32f10x.h"
#include "stm32f10x_it.h"
/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
  if(xRxEnable)
  {
    //使能接收和接收中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    //MAX485操作 低电平为接收模式
    GPIO_ResetBits(GPIOD,GPIO_Pin_7);
  }
  else
  {
    USART_ITConfig(USART2, USART_IT_RXNE, DISABLE); 
		//MAX485操作 高电平为发送模式
    GPIO_SetBits(GPIOD,GPIO_Pin_7);
  }

  if(xTxEnable)
  {
    //使能发送完成中断
    USART_ITConfig(USART2, USART_IT_TC, ENABLE);
		//MAX485操作 高电平为发送模式
    GPIO_SetBits(GPIOD,GPIO_Pin_7);
  }
  else
  {
    //禁止发送完成中断
    USART_ITConfig(USART2, USART_IT_TC, DISABLE);
		    //MAX485操作 低电平为接收模式
    GPIO_ResetBits(GPIOD,GPIO_Pin_7);
  }
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	(void)ucPORT;     //不修改串口
  (void)ucDataBits; //不修改数据位长度
  (void)eParity;    //不修改校验格式

  //使能USART1，GPIOA
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 , ENABLE);
  //GPIOA9 USART1_Tx
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;             //推挽输出
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  //GPIOA.10 USART1_Rx
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;       //浮动输入
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = ulBaudRate;            //只修改波特率
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  //串口初始化
  USART_Init(USART2, &USART_InitStructure);
  //使能USART1
  USART_Cmd(USART2, ENABLE);
  

  
  //设定USART1 中断优先级
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  //最后配置485发送和接收模式
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
  //GPIOG.9
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOD, &GPIO_InitStructure); 
  return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
	  //发送数据
		USART_SendData(USART2, ucByte);
//		while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET) {}
		while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET){}//等待发送完成
    return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
  //接收数据
  *pucByte = USART_ReceiveData(USART2);
    return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}

/**
  * @brief  USART2中断服务函数
  * @param  None
  * @retval None
  */
void USART2_IRQHandler(void)
{
  //发生接收中断
  if(USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
  {
    prvvUARTRxISR(); 
    //清除中断标志位    
    USART_ClearITPendingBit(USART2, USART_IT_RXNE);   
  }
  
  //发生完成中断
  if(USART_GetITStatus(USART2, USART_IT_TC) == SET)
  {
    prvvUARTTxReadyISR();
    //清除中断标志
    USART_ClearITPendingBit(USART2, USART_IT_TC);
  }
  
  //测试看是否可以去除 2012-07-23
  //溢出-如果发生溢出需要先读SR,再读DR寄存器 则可清除不断入中断的问题
  /*
  if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET)
  {
    USART_ClearFlag(USART1,USART_FLAG_ORE); //读SR
    USART_ReceiveData(USART1);              //读DR
  }
  */
}