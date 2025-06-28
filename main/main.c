/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */
 
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "communication.h"
#include "common.h"
#include "state_machine.h"
#include "peripheral.h"
#include "main.h"
 
suSystemData        systemData;
 
void app_main()
{
    network_init();
    peripheral_init();
    state_machine_init();
}