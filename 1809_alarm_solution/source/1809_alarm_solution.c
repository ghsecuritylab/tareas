/*
 * Copyright 2016-2018 NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    1809_alarm_solution.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"

#define PRINTF_MIN_STACK (110)
#define GET_ARGS(args,type) *((type*)args)

#define EVENT_ALARM_SECONDS (1<<0)
#define EVENT_ALARM_MINUTES (1<<1)
#define EVENT_ALARM_HOURS	(1<<2)

typedef enum{seconds_type, minutes_type, hours_type} time_types_t;

typedef struct
{
	time_types_t time_type;
	uint8_t value;
}time_msg_t;

typedef struct
{
	uint32_t hours;
	uint32_t minutes;
	uint32_t seconds;
}time_alarm_t;

typedef struct
{
	time_alarm_t alarm;
	SemaphoreHandle_t seconds60_sem;
	SemaphoreHandle_t minutes60_sem;
	EventGroupHandle_t alarm_events;
	QueueHandle_t time_queue;
	SemaphoreHandle_t uart_mutex;
}task_args_t;

void seconds_task(void *arg)
{
	TickType_t xLastWakeTime;
	task_args_t args = GET_ARGS(arg,task_args_t);
	uint32_t alarm_value = args.alarm.seconds;
	const TickType_t xPeriod = pdMS_TO_TICKS( 1000 );
	xLastWakeTime = xTaskGetTickCount();

	uint8_t seconds = 20;

	time_msg_t *msg;

	for(;;)
	{
		EventBits_t events = xEventGroupGetBits(args.alarm_events);
		if(seconds == alarm_value && events&EVENT_ALARM_MINUTES && events&EVENT_ALARM_HOURS)
		{
			xEventGroupSetBits(args.alarm_events, EVENT_ALARM_SECONDS);
		}
		vTaskDelayUntil( &xLastWakeTime, xPeriod );
		seconds++;
		if(60 == seconds)
		{
			seconds = 0;
			xSemaphoreGive(args.seconds60_sem);
		}
		msg = pvPortMalloc(sizeof(time_msg_t));
		msg->time_type = seconds_type;
		msg->value = seconds;
		xQueueSend(args.time_queue,&msg,portMAX_DELAY);
	}
}

void minutes_task(void *arg)
{
	uint8_t minutes = 0;
	task_args_t args = GET_ARGS(arg,task_args_t);
	uint32_t alarm_value = args.alarm.minutes;
	time_msg_t *msg;
	for(;;)
	{
		EventBits_t events = xEventGroupGetBits(args.alarm_events);
		if(minutes == alarm_value && events&EVENT_ALARM_HOURS)
		{
			xEventGroupSetBits(args.alarm_events, EVENT_ALARM_MINUTES);
		}
		xSemaphoreTake(args.seconds60_sem,portMAX_DELAY);
		minutes++;
		if(60 == minutes)
		{
			minutes = 0;
			xSemaphoreGive(args.minutes60_sem);

		}
		msg = pvPortMalloc(sizeof(time_msg_t));
		msg->time_type = minutes_type;
		msg->value = minutes;
		xQueueSend(args.time_queue,&msg,portMAX_DELAY);
	}
}

void hours_task(void *arg)
{
	uint8_t hours = 0;
	task_args_t args = GET_ARGS(arg,task_args_t);
	uint32_t alarm_value = args.alarm.hours;
	time_msg_t *msg;
	for(;;)
	{
		if(hours == alarm_value)
		{
			xEventGroupSetBits(args.alarm_events, EVENT_ALARM_HOURS);
		}
		xSemaphoreTake(args.minutes60_sem, portMAX_DELAY);
		hours++;
		if(24 == hours)
		{
			hours = 0;
		}
		msg = pvPortMalloc(sizeof(time_msg_t));
		msg->time_type = hours_type;
		msg->value = hours;
		xQueueSend(args.time_queue,&msg,portMAX_DELAY);
	}
}

void print_task(void *arg)
{
	time_msg_t *msg;
	task_args_t args = GET_ARGS(arg,task_args_t);
	uint8_t seconds = 0;
	uint8_t minutes = 0;
	uint8_t hours = 0;
	while(1)
	{
		do
		{
			xQueueReceive(args.time_queue,&msg,portMAX_DELAY);
			switch(msg->time_type)
			{
			case seconds_type:
				seconds = msg->value;
				break;
			case minutes_type:
				minutes = msg->value;
				break;
			case hours_type:
				hours = msg->value;
				break;
			}
			vPortFree(msg);
		}while(0!=uxQueueMessagesWaiting(args.time_queue));

		xSemaphoreTake(args.uart_mutex,portMAX_DELAY);
		PRINTF("\r%i:%i:%i\n",hours,minutes,seconds);
		xSemaphoreGive(args.uart_mutex);
	}
}

void alarm_task(void *arg)
{
	task_args_t args = GET_ARGS(arg,task_args_t);
	while(1)
	{
		xEventGroupWaitBits(args.alarm_events,
				EVENT_ALARM_HOURS|EVENT_ALARM_MINUTES|EVENT_ALARM_SECONDS,
				pdTRUE, pdTRUE, portMAX_DELAY);
		xSemaphoreTake(args.uart_mutex,portMAX_DELAY);
		PRINTF("\rAlarm reached!!\n");
		xSemaphoreGive(args.uart_mutex);
	}
}

int main(void)
{
	static task_args_t args;
	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	/* Init FSL debug console. */
	BOARD_InitDebugConsole();

	time_alarm_t alarm = {0,1,30};

	args.minutes60_sem = xSemaphoreCreateBinary();
	args.seconds60_sem = xSemaphoreCreateBinary();
	args.time_queue = xQueueCreate(3,sizeof(time_msg_t*));
	args.alarm_events = xEventGroupCreate();
	args.uart_mutex = xSemaphoreCreateMutex();
	args.alarm = alarm;


	xTaskCreate(seconds_task, "Seconds",PRINTF_MIN_STACK,(void *)&args,configMAX_PRIORITIES-3 ,NULL);
	xTaskCreate(minutes_task, "Minutes",PRINTF_MIN_STACK,(void *)&args,configMAX_PRIORITIES-2 ,NULL);
	xTaskCreate(hours_task, "Hours",PRINTF_MIN_STACK,(void *)&args,configMAX_PRIORITIES-1 ,NULL);
	xTaskCreate(print_task, "Printer",PRINTF_MIN_STACK,(void *)&args,configMAX_PRIORITIES-4 ,NULL);
	xTaskCreate(alarm_task, "Alarm",PRINTF_MIN_STACK,(void *)&args,configMAX_PRIORITIES-4 ,NULL);
	vTaskStartScheduler();

	for(;;)
	{

	}

	return 0 ;
}
