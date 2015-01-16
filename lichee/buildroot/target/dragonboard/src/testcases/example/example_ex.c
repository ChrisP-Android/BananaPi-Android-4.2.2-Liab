/*
 * \file        example.c
 * \brief       just an example. show how to use script in test case.
 *
 * \version     1.0.0
 * \date        2012年05月31日
 * \author      James Deng <csjamesdeng@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */

/* include system header */
#include <string.h>

/* include "dragonboard_inc.h" */
#include "dragonboard_inc.h"
#include "script.h"

/* C entry.
 *
 * \param argc the number of arguments.
 * \param argv the arguments.
 *
 * DO NOT CHANGES THE NAME OF PARAMETERS, otherwise your program will get 
 * a compile error if you are using INIT_CMD_PIPE macro.
 */
int main(int argc, char *argv[])
{
    char binary[16];
    int shmid;

    /* init cmd pipe after local variable declaration */
    INIT_CMD_PIPE();

    /* get the share memory segment id of script */
    shmid = atoi(argv[2]);
    /* init script before you use it */
    init_script(shmid);
    /* right now you can call script_fetch and script_fetch_gpio_set */
    /* TODO */

    strcpy(binary, argv[0]);
    db_msg("%s: here is an example.\n", binary);

    /* send OK to core if test OK, otherwise send FAIL
     * by using SEND_CMD_PIPE_FAIL().
     */
    SEND_CMD_PIPE_OK();

    /* remembers release script before exit */
    deinit_script();

    return 0;
}
