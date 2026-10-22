/*============================================================================
* Super-Simple Tasker (SST/C) Example for STM32 BLUEPILL-F103C6
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005 Quantum Leaps, <state-machine.com>.
*
* SPDX-License-Identifier: MIT
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
============================================================================*/
#include "sst.h"
#include "bsp.h"
#include "blinky_button.h"

#include "stm32f1xx.h"  /* CMSIS-compliant header file for the MCU used */
/* add other drivers if necessary... */

DBC_MODULE_NAME("bsp_bluepill-f103c6")

/* Local-scope defines -----------------------------------------------------*/

/* test pins on GPIO PB */
#define TST1_PIN  0U  /* PB.0  SysTick */
#define TST2_PIN  14U /* PB.14 Blinky 3 */
#define TST3_PIN  3U  /* PB.3  Button 2b */
#define TST4_PIN  5U  /* PB.5  Button 2a */
#define TST5_PIN  6U  /* PB.6  Blinky 1 */
#define TST6_PIN  7U  /* PB.7  IDLE */

/* buttons on GPIO PC */
#define B1_PIN    14U

/* ISRs used in the application ============================================*/
void SysTick_Handler(void);  /* prototype */

void SysTick_Handler(void) { /* system clock tick ISR */
    BSP_d1on();

    SST_TimeEvt_tick(); /* process all SST time events */

    /* Perform the debouncing of buttons. The algorithm for debouncing
    * adapted from the book "Embedded Systems Dictionary" by Jack Ganssle
    * and Michael Barr, page 71.
    */
    static struct ButtonsDebouncing {
        uint32_t depressed;
        uint32_t previous;
    } buttons = { 0U, 0U };
    uint32_t current = GPIOC->IDR; /* read GPIO PortC */
    uint32_t tmp = buttons.depressed; /* save the debounced depressed */
    buttons.depressed |= (buttons.previous & current); /* set depressed */
    buttons.depressed &= (buttons.previous | current); /* clear released */
    buttons.previous   = current; /* update the history */
    tmp ^= buttons.depressed;     /* changed debounced depressed */
    if ((tmp & (1U << B1_PIN)) != 0U) { /* debounced B1 state changed? */
        if ((buttons.depressed & (1U << B1_PIN)) != 0U) { /* depressed? */
            /* immutable button-press event */
            static ButtonWorkEvt const pressEvt = {
                .super.sig = BUTTON_PRESSED_SIG,
                .toggles = 60U
            };
            /* immutable forward-press event */
            static ButtonWorkEvt const fPressEvt = {
                .super.sig = FORWARD_PRESSED_SIG,
                .toggles = 60U
            };
            SST_Task_post(AO_Button2a, &fPressEvt.super);
            SST_Task_post(AO_Button2a, &pressEvt.super);
        }
        else { /* B1 is released */
            /* immutable button-release event */
            static ButtonWorkEvt const releaseEvt = {
                .super.sig = BUTTON_RELEASED_SIG,
                .toggles = 80U
            };
            /* immutable forward-release event */
            static ButtonWorkEvt const fReleaseEvt = {
                .super.sig = FORWARD_RELEASED_SIG,
                .toggles = 80U
            };
            SST_Task_post(AO_Button2a, &fReleaseEvt.super);
            SST_Task_post(AO_Button2a, &releaseEvt.super);
        }
    }

    BSP_d1off();
}

/* Assertion handler ======================================================*/
DBC_NORETURN
void DBC_fault_handler(char const * const module, int const label) {
    /*
    * NOTE: add here your application-specific error handling
    */
    (void)module;
    (void)label;

    /* set PRIMASK to disable interrupts and stop SST right here */
    __asm volatile ("cpsid i");

#ifndef NDEBUG
    for (;;) { /* keep blinking LED2 */
        BSP_d6on();  /* turn LED2 on */
        uint32_t volatile ctr;
        for (ctr = 1000000U; ctr > 0U; --ctr) {
        }
        BSP_d6off(); /* turn LED2 off */
        for (ctr = 1000000U; ctr > 0U; --ctr) {
        }
    }
#endif
    NVIC_SystemReset();
}
/*..........................................................................*/
void assert_failed(char const * const module, int const label);/* prototype */
void assert_failed(char const * const module, int const label) {
    DBC_fault_handler(module, label);
}

/* SST task activations ====================================================*/
/* preprocessor switch to choose between regular and reserved IRQs */
#define REGULAR_IRQS

#ifdef REGULAR_IRQS
/* repurpose regular IRQs for SST Tasks */
/* prototypes */
void USB_HP_CAN1_TX_IRQHandler(void);
void USB_LP_CAN1_RX0_IRQHandler(void);
void CAN1_RX1_IRQHandler(void);
void CAN1_SCE_IRQHandler(void);

void USB_HP_CAN1_TX_IRQHandler(void)  { SST_Task_activate(AO_Blinky3);  }
void USB_LP_CAN1_RX0_IRQHandler(void) { SST_Task_activate(AO_Button2b); }
void CAN1_RX1_IRQHandler(void)        { SST_Task_activate(AO_Button2a); }
void CAN1_SCE_IRQHandler(void)        { SST_Task_activate(AO_Blinky1);  }

#else /* use reserved IRQs for SST Tasks */
/* prototypes */
void Reserved30_IRQHandler(void);
void Reserved33_IRQHandler(void);
void Reserved34_IRQHandler(void);
void Reserved36_IRQHandler(void);

void Reserved30_IRQHandler(void) { SST_Task_activate(AO_Blinky3);  }
void Reserved33_IRQHandler(void) { SST_Task_activate(AO_Button2b); }
void Reserved34_IRQHandler(void) { SST_Task_activate(AO_Button2a); }
void Reserved36_IRQHandler(void) { SST_Task_activate(AO_Blinky1);  }
#endif

/* BSP functions ===========================================================*/
/*
 * GPIOx_CRL: reset value is 0x4(FLOATING+INPUT)
 *   4y+3 4y+2  4y+1    4y
 *   3       2  1        0         <== y = 0
 *   CNFy[1:0]  MODEy[1:0]  PxODR
 *   00         01/10/11    0/1    <== OUT_PP
 *   01         01/10/11    0/1    <== OUT_OD
 *   10         00          \      <== AF_PP
 *   11         00          \      <== AF_OD
 *   00         00          \      <== IN_ANALOG
 *   01         00          0      <== IN_PD
 *   10         00          1      <== IN_PU
 */
#define GPIO_CFG_MSK      (0xFU) /* CNF[1:0] + MODE[1:0] */
#define GPIO_OUT_PP_50MHZ (0x3U) /* CNF=00, MODE=11 */
#define GPIO_IN_PULL      (0x8U) /* CNF=10, MODE=00 */

static void BSP_gpio_cr_reg(GPIO_TypeDef *port, uint32_t pin, uint32_t cfg) {
    volatile uint32_t *cr = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t const shift = 4U * (pin & 7U);
    *cr = (*cr & ~(GPIO_CFG_MSK << shift)) | ((cfg & GPIO_CFG_MSK) << shift);
}

void BSP_init(void) {
    /* NOTE: NO MPU */

    /* assign IRQs to tasks. NOTE: critical for SST... */
#ifdef REGULAR_IRQS
    /* repurpose regular IRQs for SST Tasks */
    SST_Task_setIRQ(AO_Blinky3,  USB_HP_CAN1_TX_IRQn);
    SST_Task_setIRQ(AO_Button2b, USB_LP_CAN1_RX0_IRQn);
    SST_Task_setIRQ(AO_Button2a, CAN1_RX1_IRQn);
    SST_Task_setIRQ(AO_Blinky1,  CAN1_SCE_IRQn);
#else
    /* use reserved IRQs for SST Tasks */
    SST_Task_setIRQ(AO_Blinky3,  30U);
    SST_Task_setIRQ(AO_Button2b, 33U);
    SST_Task_setIRQ(AO_Button2a, 34U);
    SST_Task_setIRQ(AO_Blinky1,  36U);
#endif

    /* enable GPIOB port clock for LEds and test pins */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    /* set all used GPIOB pins as push-pull output, no pull-up, pull-down */
    BSP_gpio_cr_reg(GPIOB, TST1_PIN, GPIO_OUT_PP_50MHZ);
    BSP_gpio_cr_reg(GPIOB, TST2_PIN, GPIO_OUT_PP_50MHZ);
    BSP_gpio_cr_reg(GPIOB, TST3_PIN, GPIO_OUT_PP_50MHZ);
    BSP_gpio_cr_reg(GPIOB, TST4_PIN, GPIO_OUT_PP_50MHZ);
    BSP_gpio_cr_reg(GPIOB, TST5_PIN, GPIO_OUT_PP_50MHZ);
    BSP_gpio_cr_reg(GPIOB, TST6_PIN, GPIO_OUT_PP_50MHZ);

    /* enable GPIOC clock port for the Button B1 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* configure Button B1 pin on GPIOC as input, no pull-up, pull-down */
    BSP_gpio_cr_reg(GPIOC, B1_PIN, GPIO_IN_PULL);
    GPIOC->ODR &= ~(1U << B1_PIN); /* ODR=0 selects the pull-down */
}

/*..........................................................................*/
void BSP_d1on(void) {
    GPIOB->BSRR = (1U << TST1_PIN);
}
void BSP_d1off(void) {
    GPIOB->BSRR = (1U << (TST1_PIN + 16U));
}
/*..........................................................................*/
void BSP_d2on(void) {
    GPIOB->BSRR = (1U << TST2_PIN);
}
void BSP_d2off(void) {
    GPIOB->BSRR = (1U << (TST2_PIN + 16U));
}
/*..........................................................................*/
void BSP_d3on(void) {
    GPIOB->BSRR = (1U << TST3_PIN);
}
void BSP_d3off(void) {
    GPIOB->BSRR = (1U << (TST3_PIN + 16U));
}
/*..........................................................................*/
void BSP_d4on(void) {
    GPIOB->BSRR = (1U << TST4_PIN);
}
void BSP_d4off(void) {
    GPIOB->BSRR = (1U << (TST4_PIN + 16U));
}
/*..........................................................................*/
void BSP_d5on(void) {
    GPIOB->BSRR = (1U << TST5_PIN);
}
void BSP_d5off(void) {
    GPIOB->BSRR = (1U << (TST5_PIN + 16U));
}
/*..........................................................................*/
void BSP_d6on(void) {
    GPIOB->BSRR = (1U << TST6_PIN);
}
void BSP_d6off(void) {
    GPIOB->BSRR = (1U << (TST6_PIN + 16U));
}

/*..........................................................................*/
SST_Evt const *BSP_getWorkEvtBlinky1(uint8_t num) {
    /* immutable work events for Blinky1 */
    static BlinkyWorkEvt const workBlinky1[] = {
        {
            .super.sig = BLINKY_WORK_SIG,
            .toggles = 40U,
            .ticks = 5U,
        },
        {
            .super.sig = BLINKY_WORK_SIG,
            .toggles = 30U,
            .ticks = 7U,
        }
    };
    DBC_REQUIRE(500, num < ARRAY_NELEM(workBlinky1)); /* must be in range */
    return &workBlinky1[num].super;
}
/*..........................................................................*/
SST_Evt const *BSP_getWorkEvtBlinky3(uint8_t num) {
    /* immutable work events for Blinky3 */
    static BlinkyWorkEvt const workBlinky3[] = {
        {
            .super.sig = BLINKY_WORK_SIG,
            .toggles = 20U,
            .ticks = 5U,
        },
        {
            .super.sig = BLINKY_WORK_SIG,
            .toggles = 10U,
            .ticks = 3U,
        }
    };
    DBC_REQUIRE(600, num < ARRAY_NELEM(workBlinky3)); /* must be in range */
    return &workBlinky3[num].super;
}

/* SST callbacks ===========================================================*/
void SST_onStart(void) {
    SystemCoreClockUpdate();

    /* set up the SysTick timer to fire at BSP_TICKS_PER_SEC rate */
    SysTick_Config((SystemCoreClock / BSP_TICKS_PER_SEC) + 1U);

    /* set priorities of ISRs used in the system */
    NVIC_SetPriority(SysTick_IRQn, 0U);
    /* ... */
}
/*..........................................................................*/
void SST_onIdle(void) {
    BSP_d6on();  /* turn LED2 on */
#ifdef NDEBUG
    /* Put the CPU and peripherals to the low-power mode.
    * you might need to customize the clock management for your application,
    * see the datasheet for your particular Cortex-M MCU.
    */
    BSP_d6off(); /* turn LED2 off */
    __WFI(); /* Wait-For-Interrupt */
    BSP_d6on();  /* turn LED2 on */
#endif
    BSP_d6off(); /* turn LED2 off */
}

