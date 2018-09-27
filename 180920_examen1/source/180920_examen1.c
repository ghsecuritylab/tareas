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
 * @file    180920_examen1.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
/* TODO: insert other include files here. */
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"
/* TODO: insert other definitions and declarations here. */
#define PRINTF_MIN_STACK (110)
#define GET_ARGS(args,type) *((type*)args)
#define EVENT_T1 (1<<0)
#define EVENT_T2 (1<<1)



typedef struct
{
	char buffer[10];

	EventGroupHandle_t t1t2_signals;
	SemaphoreHandle_t serial_port_mutex;
	SemaphoreHandle_t shared_memory_mutex;
	EventBits_t uxBits;

}task_args_t;
SemaphoreHandle_t shared_memory_mutex2;

typedef enum {t1_id,t2_id,t3_id} id_t;

typedef struct
{
	id_t id;
	uint8_t data;
	char * msg;
}msg_t;

void initial_task(void*args);
void task_1_send(void*args);
void task_2_send(void*args);
void task_3_receive(void*args);
/*
 * @brief   Application entry point.
 */
void initial_task(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	//for(;;)
	//{
	    xTaskCreate(task_1_send,   "1_send",   configMINIMAL_STACK_SIZE+30, (void*)&args, configMAX_PRIORITIES-2, NULL);
	    xTaskCreate(task_2_send,   "2_send",   configMINIMAL_STACK_SIZE+30, (void*)&args, configMAX_PRIORITIES-2, NULL);
	    xTaskCreate(task_3_receive,"3_receive",PRINTF_MIN_STACK,         (void*)&args, configMAX_PRIORITIES-2, NULL);
	    vTaskSuspend(NULL);
	//}
}

void task_1_send(void*args)
{
	char t1_msg[] = "Soy la tarea 1\n";
//	msg_t msg;
//	msg.id = t1_id;
//	msg.msg = t1_msg;
	task_args_t task_args = GET_ARGS(args,task_args_t);
	for(;;)
	{
		xSemaphoreTake(shared_memory_mutex2,portMAX_DELAY);
		//task_args.buffer[0] = t1_msg[0];
//		msg.data = task_args.buffer;
		xSemaphoreGive(task_args.shared_memory_mutex);

		xEventGroupSetBits(task_args.t1t2_signals, EVENT_T1);

		vTaskDelay(pdMS_TO_TICKS(150));
	}
}

void task_2_send(void*args)
{
	char t2_msg[] = "Soy la tarea 2\n";
//	msg_t msg;
//	msg.id = t2_id;
//	msg.msg = t2_msg;
	task_args_t task_args = GET_ARGS(args,task_args_t);
	for(;;)
	{
		xSemaphoreTake(task_args.shared_memory_mutex,portMAX_DELAY);
		//task_args.buffer[0] = t2_msg[0];
//		msg.data = task_args.buffer;
		xSemaphoreGive(task_args.shared_memory_mutex);

		xEventGroupSetBits(task_args.t1t2_signals, EVENT_T2);

		vTaskDelay(pdMS_TO_TICKS(200));

	}
}

void task_3_receive(void*args)
{
//	EventBits_t uxBits;
	task_args_t task_args = GET_ARGS(args,task_args_t);
	for(;;)
	{
		task_args.uxBits = xEventGroupWaitBits(task_args.t1t2_signals, EVENT_T1|EVENT_T2, pdFALSE, pdFALSE, portMAX_DELAY);

		if      (  ( task_args.uxBits & EVENT_T1 ) != 0  )
		{
			xSemaphoreTake(task_args.serial_port_mutex,portMAX_DELAY);
			PRINTF(task_args.buffer);
			xSemaphoreGive(task_args.serial_port_mutex);
		}
		else if(  ( task_args.uxBits & EVENT_T2 ) != 0  )
		{
			xSemaphoreTake(task_args.serial_port_mutex,portMAX_DELAY);
			PRINTF(task_args.buffer);
			xSemaphoreGive(task_args.serial_port_mutex);
		}
		else
		{
			xSemaphoreTake(task_args.serial_port_mutex,portMAX_DELAY);
			PRINTF("\rError\n");
			xSemaphoreGive(task_args.serial_port_mutex);
		}

	}
}

int main(void) {

	static task_args_t args;
	args.t1t2_signals = xEventGroupCreate();
	args.shared_memory_mutex = xSemaphoreCreateMutex();
	shared_memory_mutex2 = xSemaphoreCreateMutex();

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    PRINTF("Hello World\n");

    /* Force the counter to be placed into memory. */
    /* Enter an infinite loop, just incrementing a counter. */

    xTaskCreate(initial_task, "init_task", configMINIMAL_STACK_SIZE, (void*)&args, configMAX_PRIORITIES, NULL);
    vTaskStartScheduler();


    while(1) {

    }
    return 0 ;
}
