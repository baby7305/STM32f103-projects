#include "stm32f10x.h"
#include "delay.h"
#include "myus.h"
#include "zjjm.h"

typedef union //定义结构体，参考https://blog.csdn.net/ls667/article/details/50811519博客
{
	float fdata;
	unsigned long ldata;
} FloatLongType;
void Float_to_Byte(float f, unsigned char byte[]);
extern int circle_count;
extern float v_real;
extern float val;

int main(void)
{
	u8 t_test = 0;
	u8 byte[4] = {0}; //定义数组
	u8 send_date[4] = {0};

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	delay_init();
	TIM4_Mode_Config();
	My_US_Init();
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);
	while (1)
	{
		printf("$%d;   ",TIM4->CNT);
		delay_ms(10);
		// int num = TIM4->CNT; // 1024*4=4096  0-4095

		// //发送数据，绘制图像
		// Float_to_Byte(num * 1.0, byte);
		// for (t_test = 0; t_test < 4; t_test++)
		// {
		// 	USART_SendData(USART1, byte[t_test]); //向串口1发送数据
		// 	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET)
		// 		; //等待发送结束
		// }

		// send_date[0] = 0X00;
		// send_date[1] = 0X00;
		// send_date[2] = 0X80;
		// send_date[3] = 0X7f;
		// for (t_test = 0; t_test < 4; t_test++)
		// {
		// 	USART_SendData(USART1, send_date[t_test]); //向串口1发送数据
		// 	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET)
		// 		; //等待发送结束
		// }
	}
}

/****************************************************
将浮点数f转化为4个字节数据存放在byte[4]中
*****************************************************/
void Float_to_Byte(float f, unsigned char byte[]) //参考https://blog.csdn.net/ls667/article/details/50811519博客
{
	FloatLongType fl;
	fl.fdata = f;
	byte[0] = (unsigned char)fl.ldata;
	byte[1] = (unsigned char)(fl.ldata >> 8);
	byte[2] = (unsigned char)(fl.ldata >> 16);
	byte[3] = (unsigned char)(fl.ldata >> 24);
}