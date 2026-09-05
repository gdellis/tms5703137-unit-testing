/*
 * Flash an image onto the TMS570 and let it run, using CCS Debug Server Scripting.
 * Alternative to UniFlash for TMS570_FLASH_CMD (see docs/03-on-target.md):
 *
 *   <ccs>/ccs/ccs_base/scripting/bin/dss.sh tools/dss_flash_and_run.js <board.ccxml> <image.out>
 *
 * The session regex matches the Cortex-R4 CPU in the target configuration; adjust it
 * if your .ccxml names it differently (open the ccxml in CCS -> Advanced to see).
 * Verify against the DSS reference for your CCS version: this file cannot be
 * exercised without a board.
 */
importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.ccstudio.scripting.environment);
importPackage(Packages.java.lang);

var ccxml = arguments[0];
var image = arguments[1];
if (!ccxml || !image) {
    System.err.println("usage: dss.sh dss_flash_and_run.js <board.ccxml> <image.out>");
    System.exit(2);
}

var script = ScriptingEnvironment.instance();
script.traceSetConsoleLevel(TraceLevel.INFO);

var debugServer = script.getServer("DebugServer.1");
debugServer.setConfig(ccxml);

var session = debugServer.openSession(".*CortexR4.*");
try {
    session.target.connect();
    session.target.reset();
    session.memory.loadProgram(image);   /* flashes .out via the on-chip flash loader */
    session.target.runAsynch();          /* leave it running; Unity talks over SCI    */
} finally {
    /* Disconnecting keeps the core running from reset; the serial capture does the rest. */
    session.target.disconnect();
    session.terminate();
    debugServer.stop();
}
