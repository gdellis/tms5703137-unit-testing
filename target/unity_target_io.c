/**
 * @file unity_target_io.c
 * @brief Unity output hooks for the TMS570LS3137: characters go out over an SCI.
 *
 * Wired in by target/unity_config.h. Uses the HALCoGen SCI driver in polled mode, so
 * the only HALCoGen configuration needed is: SCI driver enabled, the chosen instance
 * configured as an SCI, and its TX pin routed. Match tools/run_on_target.sh's
 * TMS570_BAUD to whatever baud rate you configured.
 *
 * UNITY_TARGET_SCI selects the instance (set from CMake: TMS570_UNITY_SCI). It has to
 * be one the generated sciInit() actually configures: on this device SCI1/LIN can be
 * generated as either LIN (linREG) or SCI (scilinREG), and if it was generated as LIN
 * then sciInit() never touches it and printing there produces nothing. Check
 * reg_sci.h and sci.c in your HALCoGen output, and your board's schematic.
 */
#include "sci.h"

/*
 * Enabling the FPU is this file's job, not the start-up code's. HALCoGen's _c_int00
 * does core register init, PBIST, ECC and memory init, but it does not call
 * _coreEnableVfp_() (checked against 04.07.01 output for the LS3137 HDK). The tests
 * are compiled with --float_support=VFPv3D16, so without it the first floating-point
 * instruction takes an undefined-instruction trap.
 *
 * Set TMS570_ENABLE_VFP=0 (see target/CMakeLists.txt) if your start-up code already
 * enables the FPU, or if the part has none - in that case also define
 * UNITY_EXCLUDE_FLOAT in unity_config.h so the float asserts do not compile.
 */
#ifndef TMS570_ENABLE_VFP
#define TMS570_ENABLE_VFP 1
#endif

#if TMS570_ENABLE_VFP
#include "sys_core.h"   /* HALCoGen: _coreEnableVfp_() */
#endif

#ifndef UNITY_TARGET_SCI
#define UNITY_TARGET_SCI scilinREG
#endif

/** Set to 1 once the run is over. Watch it, or break on unity_target_complete(). */
volatile uint32 unity_target_finished = 0U;

void unity_target_start(void)
{
    /* Unity calls this from UnityBegin(), which is the first thing the generated
     * runner's main() does, so this runs before any test body. */
#if TMS570_ENABLE_VFP
    _coreEnableVfp_();
#endif
    sciInit();
}

void unity_target_putc(int c)
{
    /* Terminals and the capture script expect CRLF. */
    if (c == '\n')
    {
        sciSendByte(UNITY_TARGET_SCI, (uint8)'\r');
    }
    sciSendByte(UNITY_TARGET_SCI, (uint8)c);
}

void unity_target_flush(void)
{
    /* sciSendByte() blocks until the TX buffer is free, so at most one byte is still
     * in flight here; wait for it before anyone halts the core. */
    while (sciIsTxReady(UNITY_TARGET_SCI) == 0U)
    {
    }
}

void unity_target_complete(void)
{
    unity_target_flush();
    unity_target_finished = 1U;

    /* Nothing sensible to return to on bare metal: park here. */
    for (;;)
    {
    }
}
