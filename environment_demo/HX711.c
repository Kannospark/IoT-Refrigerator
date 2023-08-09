/*
 * Copyright (c) 2020 HiHope Community.
 * Description: mq2 demo
 * Author: HiSpark Product Team.
 * Create: 2020-5-20
 */
#include <math.h>
#include <hi_early_debug.h>
#include <hi_task.h>
#include <hi_time.h>
#include <hi_adc.h>
#include <hi_stdlib.h>
#include <hi_watchdog.h>
#include <ssd1306_oled.h>
#include <hi_pwm.h>
#include <hi_io.h>
#include <hi_gpio.h>
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "..\iottencent_demo\iot_main.h"

//校准参数
//因为不同的传感器特性曲线不是很一致，因此，每一个传感器需要矫正这里这个参数才能使测量值很准确。
//当发现测试出来的重量偏大时，增加该数值。
//如果测试出来的重量偏小时，减小改数值。
//该值可以为小数,可参考误差1g调整10
#define GapValue 1310

unsigned long Sensor_Read(void);
double Get_Sensor_Read(void);

hi_void gpio_init() {
	hi_io_set_func(HI_IO_NAME_GPIO_13, HI_IO_FUNC_GPIO_13_I2C0_SDA);
    hi_io_set_func(HI_IO_NAME_GPIO_14, HI_IO_FUNC_GPIO_14_I2C0_SCL);
}

double hx711_weight = 0;
float getWeight(){
	if(hx711_weight < 1){
		return (float)0;
	}
	else{
		return (float)hx711_weight;
	}
}

void hi_hx711_task(void)
{	
	while(getBeginTemp() == 0){
        hi_sleep(1000);
    }
	printf("[zzx]weight sensor intial begin\n");
	//gpio_init();
	//初始化GPIO11为GPIO输入，为传感器DT引脚
    hi_io_set_func(HI_IO_NAME_GPIO_11, HI_IO_FUNC_GPIO_11_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_11,HI_GPIO_DIR_IN);
	//初始化GPIO12为GPIO输出，为传感器SCK引脚
    hi_io_set_func(HI_IO_NAME_GPIO_12, HI_IO_FUNC_GPIO_12_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_12,HI_GPIO_DIR_OUT);
    hi_gpio_set_ouput_val(HI_IO_NAME_GPIO_12,0);
	//初始化i2c,oled屏使用
    //hi_i2c_init(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE); /* baudrate: 400kbps */
    //hi_i2c_set_baudrate(HI_I2C_IDX_0, HI_I2C_IDX_BAUDRATE);
	//oled初始化并显示对应信息
    //while( HI_ERR_SUCCESS != oled_init()) ;
	//oled_fill_screen(OLED_CLEAN_SCREEN);
    //oled_show_str(0,3, "Weigth : ",1);

    double base_data = 0;
	hi_u8 buf[10] = {0};
    base_data = Get_Sensor_Read(); //获取基准值
	printf("[zzx]weight sensor intial success\n");
    while (1)
    {
        hx711_weight = (Get_Sensor_Read() - base_data) / GapValue; //获取重量
        printf("[zzx]weight :   %.2f\r\n" ,hx711_weight);  
		//printf("getSensor_Read():%lf ,basedata:%lf\n",Get_Sensor_Read(), base_data);
        //sprintf_s(buf,10,"%.2f g ",hx711_weight);
        //oled_show_str(20,5, buf,1);
        hi_sleep(3000);
    }
}

unsigned long Sensor_Read(void)
{
	unsigned long value = 0;
	unsigned char i = 0;
	hi_gpio_value input = 0;
	hi_udelay(2);
	//时钟线拉低 空闲时时钟线保持低电位
	hi_gpio_set_ouput_val(HI_IO_NAME_GPIO_12,0);
	hi_udelay(2);	
	hi_gpio_get_input_val(HI_IO_NAME_GPIO_11,&input);
	//等待AD转换结束
	while(input)
    {
        hi_gpio_get_input_val(HI_IO_NAME_GPIO_11,&input);
    }
	for(i=0;i<24;i++)
	{
		//时钟线拉高 开始发送时钟脉冲
		hi_gpio_set_ouput_val(HI_IO_NAME_GPIO_12,1);
		hi_udelay(2);
		//左移位 右侧补零 等待接收数据
		value = value << 1;
		//时钟线拉低
		hi_gpio_set_ouput_val(HI_IO_NAME_GPIO_12,0);
		hi_udelay(2);
		//读取一位数据
        hi_gpio_get_input_val(HI_IO_NAME_GPIO_11,&input);
		if(input){
			value ++;
        }
	}
	//第25个脉冲
	hi_gpio_set_ouput_val(HI_IO_NAME_GPIO_12,1);
	hi_udelay(2);
	value = value^0x800000;	
	//第25个脉冲结束
	hi_gpio_set_ouput_val(HI_IO_NAME_GPIO_12,0);	
	hi_udelay(2);	
	return value;
}

double Get_Sensor_Read(void)
{
  	double sum = 0;    // 为了减小误差，一次取出10个值后求平均值。
  	for (int i = 0; i < 10; i++) // 循环的越多精度越高，当然耗费的时间也越多
    	sum += Sensor_Read();  // 累加
  	return (sum/10); // 求平均值进行均差
}


static void HX711ExampleEntry(void)
{
    osThreadAttr_t attr;
    IoTWatchDogDisable();

    attr.name = "HX711_task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 2; // 任务栈大小*1024 stack size 5*1024
    attr.priority = 31;

    if (osThreadNew((osThreadFunc_t)hi_hx711_task, NULL, &attr) == NULL) {
        printf("[UartTask] Failed to create UartTask!\n");
    }
}

SYS_RUN(HX711ExampleEntry);