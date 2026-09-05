/**
 * @file unity_target_io.c
 * @brief Unity output hooks for the TMS570LS3137: characters go out over an SCI.
 *
 * Wired in by target/unity_config.h. Uses the HALCoGen SCI driver in polled mode, so
 * the only HALCoGen configuration needed is: SCI driver enabled, the chosen instance
 * configured (115200 8N1 is what tools/run_on_target.sh expects), TX pin routed.
 *
 * UNITY_TARGET_SCI selects the instance (set from CMake: TMS570_UNITY_SCI). On the
 * TMS570LS31x HDK the on-board USB serial port is on SCI1 (scilinREG in HALCoGen
 * terms); check your board's schematic.
 */
#include "sci.h"

#ifndef UNITY_TARGET_SCI
#define UNITY_TARGET_SCI scilinREG
#endif

/** Set to 1 once the run is over. Watch it, or break on unity_target_complete(). */
volatile uint32 unity_target_finished = 0U;

void unity_target_start(void)
{
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
