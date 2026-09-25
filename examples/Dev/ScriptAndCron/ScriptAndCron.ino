/* Running a script, and running it on a schedule
 *
 * This example illustrate writing a shell script from the sketch, running it
 * once, and adding a crontab row so the framework keeps running it every
 * minute on its own.
 *
 * After it has run you can log in and look at what it did :
 *   cat /hello.log
 *   cat /etc/crontab
 *   crontab
 *
 * NOTE : cron needs a real clock. A board that never reaches the internet
 * never gets one, so rows sit in the table and never fire. Running the script
 * from the sketch works either way.
 */

#include <PdiStack.h>

#if defined(ENABLE_SCRIPT_RUNNER) && defined(ENABLE_CRON_SERVICE)

#define DEMO_SCRIPT_PATH  "/hello.sh"     /* script the sketch writes */
#define DEMO_LOG_PATH     "/hello.log"    /* file the script appends to */

// the first line names the user the script runs as.
// a detached script that names nobody is not run at all.
static const char DEMO_SCRIPT[] =
	"# UID 0" TERMINAL_NEW_LINE
	"echo a line from the demo script >> " DEMO_LOG_PATH TERMINAL_NEW_LINE;

// five time fields then the command, the same row you would type into crontab
static const char DEMO_CRON_ROW[] =
	"* * * * * source " DEMO_SCRIPT_PATH TERMINAL_NEW_LINE;

/**
 * write the script and add its crontab row, once.
 *
 * both are keyed off the script already being there, so a row you add or
 * remove yourself later survives a reboot instead of being written again.
 * writing the whole table instead of appending would throw those edits away.
 */
void install_demo_script(){

	if( __i_fs.isFileExist(DEMO_SCRIPT_PATH) ){
		return;
	}

	if( 0 > __i_fs.writeFile(DEMO_SCRIPT_PATH, DEMO_SCRIPT, strlen(DEMO_SCRIPT)) ){
		return;
	}

	// append, so the header and any rows already in the table are kept
	__i_fs.writeFile(CRONTAB_FILE_PATH, DEMO_CRON_ROW, strlen(DEMO_CRON_ROW), true);
}

/**
 * run the script now, without waiting for the clock.
 *
 * runScheduledScript is the one to use from a sketch. it runs the file as the
 * user the header names and needs nobody watching, which is exactly how the
 * framework runs /etc/rc.local at the end of boot.
 *
 * the two that look right and are not : ScriptRunner::run wants the session of
 * whoever asked for the script, and __cmd_service.executeCommand wants a
 * terminal and a session as well. A sketch has neither, so both answer
 * CMD_ERROR_NOTTY.
 */
void run_demo_script_now(){

	ScriptRunner::runScheduledScript(DEMO_SCRIPT_PATH);
}

/**
 * run one command line the same way, with no file involved.
 * the uid is given here rather than read from a header.
 */
void run_one_line(){

	ScriptRunner::runDetachedLine("echo a line from the sketch >> " DEMO_LOG_PATH, USER_STORE_ROOT_UID);
}

#else
  #error "Script runner or cron service is disabled ( in devices/DeviceConfig.h of framework library ). please enable(uncomment ENABLE_SCRIPT_RUNNER and ENABLE_CRON_SERVICE) it for this example"
#endif


void setup() {

	PdiStack.initialize();

	install_demo_script();
	run_demo_script_now();
	run_one_line();
}

void loop() {
	PdiStack.serve();
}
