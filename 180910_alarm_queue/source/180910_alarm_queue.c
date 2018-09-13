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
 * @file    180910_alarm_queue.c
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
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"
/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */
#define PRINTF_MIN_STACK (110)
#define GET_ARGS(args,type) *((type*)args)
#define EVENT_SECONDS (1<<0)
#define EVENT_MINUTES (1<<1)
#define EVENT_HOURS   (1<<2)

typedef struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}alarm_t;

alarm_t alarm;

typedef struct
{
	int8_t shared_memory; //todo
	SemaphoreHandle_t minutes_semaphore;
	EventGroupHandle_t event_second_signal;//todo
	SemaphoreHandle_t serial_port_mutex; //todo
//	SemaphoreHandle_t shared_memory_mutex; //todo
	QueueHandle_t mailbox; //todo
}task_args_t;

//estructura para los mensajes en el mailbox
typedef enum {	seconds_type,	minutes_type,	hours_type } time_types_t;

typedef struct
{
	time_types_t time_type;
	uint8_t value;
}time_msg_t;

/*
 * @brief   Application entry point.
 */

void seconds_task(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	uint32_t seconds  = 0;
	TickType_t last_wake_time = xTaskGetTickCount();
	time_msg_t msg;
	msg.time_type = seconds_type;
	for(;;)
	{
		seconds++;

		if (60 == seconds)
		{
			xSemaphoreGive(task_args.minutes_semaphore);
			seconds = 0;
		}

		msg.value = seconds;

		if (alarm.second == seconds)
		{
			xEventGroupSetBits(task_args.event_second_signal, EVENT_SECONDS);
		}

		xQueueSend(task_args.mailbox,&msg,portMAX_DELAY);

		vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));
	}
}
int main(void) {

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    alarm.hour = 0;
    alarm.minute = 1;
    alarm.second = 0;

    PRINTF("Hello World\n");
    //TODO verificar (void*)&args
    //TODO configMAX_PRIORITIES
    xTaskCreate(seconds_task, "seconds", configMINIMAL_STACK_SIZE, (void*)&args, configMAX_PRIORITIES, NULL);
    xTaskCreate(minutes_task, "minutes", configMINIMAL_STACK_SIZE, (void*)&args, configMAX_PRIORITIES, NULL);
    xTaskCreate(hours_task,   "hours",   configMINIMAL_STACK_SIZE, (void*)&args, configMAX_PRIORITIES, NULL);
    xTaskCreate(alarm_task,   "alarm",   PRINTF_MIN_STACK,         (void*)&args, configMAX_PRIORITIES, NULL);
    xTaskCreate(print_task,   "print",   PRINTF_MIN_STACK,         (void*)&args, configMAX_PRIORITIES, NULL);

    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {

    }
    return 0 ;
}
