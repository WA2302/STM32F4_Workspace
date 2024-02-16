/**
  ******************************************************************************
  * @file    minos.c 
  * @author  Windy Albert
  * @version V2.1
  * @date    04-November-2023
  * @brief   This is the core file for MinOS, a NO GRAB OS for embedded system.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "minos.h"

OS_TCB *OSTCBCur;                    /* Pointer to currently running TCB      */
static OS_TCB OSTCBTbl[OS_MAX_TASKS];/* Table of TCBs                         */

uint64_t SysTime = 0;

/**
  * @brief  All tasks MUST be created completed before OSSTart().
  * @param  task     is a pointer to the task's code
  *         ptos     is a pointer to the task's top of stack.
  * @retval None
  */
void  TaskCreate(void (*task)(void), uint32_t *stk)
{ 
    static uint8_t task_num = 0;
    
    if( task_num >= OS_MAX_TASKS )
    {
        while(1);                       /* Never run to here.                 */
    }
                                        /* Registers auto-saved on exception  */
    *(stk)    = (uint32_t)0x01000000L;  /* xPSR                               */
    *(--stk)  = (uint32_t)task;         /* Entry Point                        */
    *(--stk)  = (uint32_t)0xFFFFFFFEL;  /* R14 (LR) (So task CANNOT return)   */
    *(--stk)  = (uint32_t)0x12121212L;  /* R12                                */
    *(--stk)  = (uint32_t)0x03030303L;  /* R3                                 */
    *(--stk)  = (uint32_t)0x02020202L;  /* R2                                 */
    *(--stk)  = (uint32_t)0x01010101L;  /* R1                                 */
    *(--stk)  = (uint32_t)0x00000000L;  /* R0 : argument                      */
                                        /* Registers saved on PSP             */
    *(--stk)  = (uint32_t)0x11111111L;  /* R11                                */
    *(--stk)  = (uint32_t)0x10101010L;  /* R10                                */
    *(--stk)  = (uint32_t)0x09090909L;  /* R9                                 */
    *(--stk)  = (uint32_t)0x08080808L;  /* R8                                 */
    *(--stk)  = (uint32_t)0x07070707L;  /* R7                                 */
    *(--stk)  = (uint32_t)0x06060606L;  /* R6                                 */
    *(--stk)  = (uint32_t)0x05050505L;  /* R5                                 */
    *(--stk)  = (uint32_t)0x04040404L;  /* R4                                 */

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    *(--stk) = (uint32_t)0x02000000u;   /* FPSCR                              */
                                        /* Initialize S0-S31 FP registers     */
    *(--stk) = (uint32_t)0x41F80000u;   /* S31                                */
    *(--stk) = (uint32_t)0x41F00000u;   /* S30                                */
    *(--stk) = (uint32_t)0x41E80000u;   /* S29                                */
    *(--stk) = (uint32_t)0x41E00000u;   /* S28                                */
    *(--stk) = (uint32_t)0x41D80000u;   /* S27                                */
    *(--stk) = (uint32_t)0x41D00000u;   /* S26                                */
    *(--stk) = (uint32_t)0x41C80000u;   /* S25                                */
    *(--stk) = (uint32_t)0x41C00000u;   /* S24                                */
    *(--stk) = (uint32_t)0x41B80000u;   /* S23                                */
    *(--stk) = (uint32_t)0x41B00000u;   /* S22                                */
    *(--stk) = (uint32_t)0x41A80000u;   /* S21                                */
    *(--stk) = (uint32_t)0x41A00000u;   /* S20                                */
    *(--stk) = (uint32_t)0x41980000u;   /* S19                                */
    *(--stk) = (uint32_t)0x41900000u;   /* S18                                */
    *(--stk) = (uint32_t)0x41880000u;   /* S17                                */
    *(--stk) = (uint32_t)0x41800000u;   /* S16                                */
    *(--stk) = (uint32_t)0x41700000u;   /* S15                                */
    *(--stk) = (uint32_t)0x41600000u;   /* S14                                */
    *(--stk) = (uint32_t)0x41500000u;   /* S13                                */
    *(--stk) = (uint32_t)0x41400000u;   /* S12                                */
    *(--stk) = (uint32_t)0x41300000u;   /* S11                                */
    *(--stk) = (uint32_t)0x41200000u;   /* S10                                */
    *(--stk) = (uint32_t)0x41100000u;   /* S9                                 */
    *(--stk) = (uint32_t)0x41000000u;   /* S8                                 */
    *(--stk) = (uint32_t)0x40E00000u;   /* S7                                 */
    *(--stk) = (uint32_t)0x40C00000u;   /* S6                                 */
    *(--stk) = (uint32_t)0x40A00000u;   /* S5                                 */
    *(--stk) = (uint32_t)0x40800000u;   /* S4                                 */
    *(--stk) = (uint32_t)0x40400000u;   /* S3                                 */
    *(--stk) = (uint32_t)0x40000000u;   /* S2                                 */
    *(--stk) = (uint32_t)0x3F800000u;   /* S1                                 */
    *(--stk) = (uint32_t)0x00000000u;   /* S0                                 */
#endif

    OSTCBCur                = &OSTCBTbl[task_num];
    OSTCBCur->OSTCBStkPtr   = stk;        /* Initialize the task's stack      */
    OSTCBCur->OSTCBNext     = &OSTCBTbl[0];
    OSTCBCur->OSTCBWakeTime = 0;

    if( task_num > 0 )
    {
        OSTCBTbl[task_num-1].OSTCBNext = OSTCBCur;
    }

    task_num++;
}

/**
  * @brief  Trigger the PendSV.
  * @param  None
  * @retval None
  */
__inline void __Sched (void) 
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/**
  * @brief  You MUST have created at least one task when you going to call OSStart().
  * @param  None
  * @retval None
  */
void OSStart (void)
{
    __NVIC_SetPriority ( PendSV_IRQn, 0xFF );  /** SCB->SHP[10] = 0xFF;      **/
    __set_PSP(0);
    __Sched();
    __enable_irq();
}

/**
  * @brief  This function is called to delay execution of the currently running 
  *         task until the specified number of system ticks expires.
  * @param  ticks     is the time delay that the task will be suspended.
  * @retval None
  */
void OSTimeDly( uint16_t ticks )
{
    OSTCBCur->OSTCBWakeTime = OSTime_Now() + ticks;
    while( OSTime_Now() < OSTCBCur->OSTCBWakeTime )
    {
        __Sched();
    }
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
__ASM void PendSV_Handler (void)
{
    extern OSTCBCur

    PRESERVE8
    
    CPSID I               /* Prevent interruption during context switch       */
    MRS   R0, PSP         /* PSP is process stack pointer                     */
    CBZ   R0, _nosave     /* Skip register save the first time                */
                          /*                                                  */
    STMDB R0!, {R4-R11}   /* PUSH r4-11 to current process stack              */
                          /*                                                  */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    VMRS    R1, FPSCR     /* Save the FPU registers                           */
    STR R1, [R0, #-4]!    /*                                                  */
    VSTMDB  R0!, {S0-S31} /* PUSH r0-31 to current process stack              */
#endif
                          /*                                                  */
    LDR   R1, =OSTCBCur   /* OSTCBCur->OSTCBStkPtr = PSP;                     */
    LDR   R1, [R1]        /*                                                  */
    STR   R0, [R1]        /* R0 is SP of process being switched out           */
                          /* Now, entire context of process has been saved    */
_nosave                   /*                                                  */
    LDR   R0, =OSTCBCur   /* OSTCBCur  = OSTCBCur->OSTCBNext;                 */
    LDR   R2, [R0]        /*                                                  */
    ADD   R2, R2, #0x04   /*                                                  */
    LDR   R2, [R2]        /*                                                  */
    STR   R2, [R0]        /*                                                  */
                          /*                                                  */
    LDR   R0, [R2]        /* PSP = OSTCBCur->OSTCBStkPtr                      */
                          /*                                                  */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)    
    VLDMIA  R0!, {S0-S31} /* Restore the FPU registers                        */
    LDMIA   R0!, {R1}     /*                                                  */
    VMSR    FPSCR, R1     /* POP r0-31 from current process stack             */
#endif
                          /*                                                  */
    LDMIA R0!, {R4-R11}   /* POP r4-11 from new process stack                 */
                          /*                                                  */
    MSR   PSP, R0         /* Load PSP with new process SP                     */

#if 0                     /* Fill the stack for debug mode                    */
    MOVS  R1, #0x00       /* Fill with 00                                     */
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    STMDB R0!,{R1}        /* Fill the stack "S0"                              */
    STMDB R0!,{R1}        /* Fill the stack "S1"                              */
    STMDB R0!,{R1}        /* Fill the stack "S2"                              */
    STMDB R0!,{R1}        /* Fill the stack "S3"                              */
    STMDB R0!,{R1}        /* Fill the stack "S4"                              */
    STMDB R0!,{R1}        /* Fill the stack "S5"                              */
    STMDB R0!,{R1}        /* Fill the stack "S6"                              */
    STMDB R0!,{R1}        /* Fill the stack "S7"                              */
    STMDB R0!,{R1}        /* Fill the stack "S8"                              */
    STMDB R0!,{R1}        /* Fill the stack "S9"                              */
    STMDB R0!,{R1}        /* Fill the stack "S10"                             */
    STMDB R0!,{R1}        /* Fill the stack "S11"                             */
    STMDB R0!,{R1}        /* Fill the stack "S12"                             */
    STMDB R0!,{R1}        /* Fill the stack "S13"                             */
    STMDB R0!,{R1}        /* Fill the stack "S14"                             */
    STMDB R0!,{R1}        /* Fill the stack "S15"                             */
    STMDB R0!,{R1}        /* Fill the stack "S16"                             */
    STMDB R0!,{R1}        /* Fill the stack "S17"                             */
    STMDB R0!,{R1}        /* Fill the stack "S18"                             */
    STMDB R0!,{R1}        /* Fill the stack "S19"                             */
    STMDB R0!,{R1}        /* Fill the stack "S20"                             */
    STMDB R0!,{R1}        /* Fill the stack "S21"                             */
    STMDB R0!,{R1}        /* Fill the stack "S22"                             */
    STMDB R0!,{R1}        /* Fill the stack "S23"                             */
    STMDB R0!,{R1}        /* Fill the stack "S24"                             */
    STMDB R0!,{R1}        /* Fill the stack "S25"                             */
    STMDB R0!,{R1}        /* Fill the stack "S26"                             */
    STMDB R0!,{R1}        /* Fill the stack "S27"                             */
    STMDB R0!,{R1}        /* Fill the stack "S28"                             */
    STMDB R0!,{R1}        /* Fill the stack "S29"                             */
    STMDB R0!,{R1}        /* Fill the stack "S30"                             */
    STMDB R0!,{R1}        /* Fill the stack "S31"                             */
    STMDB R0!,{R1}        /* Fill the stack "FPSCR"                           */
    #endif
    STMDB R0!,{R1}        /* Fill the stack "R4"                              */
    STMDB R0!,{R1}        /* Fill the stack "R5"                              */
    STMDB R0!,{R1}        /* Fill the stack "R6"                              */
    STMDB R0!,{R1}        /* Fill the stack "R7"                              */
    STMDB R0!,{R1}        /* Fill the stack "R8"                              */
    STMDB R0!,{R1}        /* Fill the stack "R9"                              */
    STMDB R0!,{R1}        /* Fill the stack "R10"                             */
    STMDB R0!,{R1}        /* Fill the stack "R11"                             */
#endif

    ORR   LR, LR, #0x04   /* Ensure exception return uses process stack       */
    CPSIE I               /*                                                  */
                          /*                                                  */
    BX    LR              /* Exception return will restore remaining context  */

    ALIGN
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    SysTime++;
}
/**************** (C) COPYRIGHT 2023 Windy Albert ******** END OF FILE ********/
