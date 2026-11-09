/* Self monitor : the board watches its own memory and flash
 *
 * This example puts several features together the way a real device would :
 *   - a shell script reads the free memory with `free` and the free flash with
 *     `df`, and compares each with a limit,
 *   - past a limit it writes a warning into the system log with `logger`,
 *   - cron runs the script every five minutes, with nothing else in the sketch.
 *
 * The sketch only writes the script and its crontab row, once. Everything else
 * is the shell, so you can change the limits or the schedule on the board
 * itself, with fedit, and the change survives a reboot.
 *
 * After it has run you can log in and look :
 *   cat /etc/selfmon.sh
 *   crontab
 *   source /etc/selfmon.sh            run the check now
 *   cat /var/log/syslog.warning        what it found
 *
 * NOTE : cron needs a real clock. A board that never reaches the internet
 * never gets one, so the row sits in the table and never fires; sourcing the
 * script by hand works either way.
 *
 * Built from the single feature examples : Dev/ScriptAndCron and Dev/Logging.
 */

#include <PdiStack.h>

#if defined(ENABLE_SCRIPT_RUNNER) && defined(ENABLE_CRON_SERVICE) && defined(ENABLE_SYSLOG_SERVICE)

#define SELFMON_SCRIPT_PATH "/etc/selfmon.sh"
#define SELFMON_FREE_MIN    "8192"     /* bytes of free memory below which it warns */
#define SELFMON_DISK_MIN    "65536"    /* bytes of free flash below which it warns */

// the first line names the user the script runs as : a script cron runs that
// names nobody is not run at all.
// a `for` list does not run $(...) itself, so each answer is kept in a name
// first and the name is walked a word at a time : the free memory is the
// second word of free's Mem row, the free flash the fifth of df's rootfs row.
static const char SELFMON_SCRIPT[] PROG_RODT_ATTR =
	"# UID 0" TERMINAL_NEW_LINE
	"export SM_L=$(free -b | grep Mem)" TERMINAL_NEW_LINE
	"export SM_N=0" TERMINAL_NEW_LINE
	"for SM_W in $SM_L" TERMINAL_NEW_LINE
	"export SM_N=$((SM_N + 1))" TERMINAL_NEW_LINE
	"if [ $SM_N -eq 2 ]" TERMINAL_NEW_LINE
	"export SM_FREE=$SM_W" TERMINAL_NEW_LINE
	"fi" TERMINAL_NEW_LINE
	"done" TERMINAL_NEW_LINE
	"if [ $SM_FREE -lt " SELFMON_FREE_MIN " ]" TERMINAL_NEW_LINE
	"logger -p warning selfmon: free memory low, $SM_FREE bytes" TERMINAL_NEW_LINE
	"fi" TERMINAL_NEW_LINE
	"export SM_L=$(df | grep rootfs)" TERMINAL_NEW_LINE
	"export SM_N=0" TERMINAL_NEW_LINE
	"for SM_W in $SM_L" TERMINAL_NEW_LINE
	"export SM_N=$((SM_N + 1))" TERMINAL_NEW_LINE
	"if [ $SM_N -eq 5 ]" TERMINAL_NEW_LINE
	"export SM_DISK=$SM_W" TERMINAL_NEW_LINE
	"fi" TERMINAL_NEW_LINE
	"done" TERMINAL_NEW_LINE
	"if [ $SM_DISK -lt " SELFMON_DISK_MIN " ]" TERMINAL_NEW_LINE
	"logger -p warning selfmon: flash nearly full, $SM_DISK bytes free" TERMINAL_NEW_LINE
	"fi" TERMINAL_NEW_LINE
	"unset SM_L" TERMINAL_NEW_LINE
	"unset SM_N" TERMINAL_NEW_LINE
	"unset SM_W" TERMINAL_NEW_LINE
	"unset SM_FREE" TERMINAL_NEW_LINE
	"unset SM_DISK" TERMINAL_NEW_LINE;

// five time fields then the command : every fifth minute
static const char SELFMON_CRON_ROW[] PROG_RODT_ATTR =
	"*/5 * * * * source " SELFMON_SCRIPT_PATH TERMINAL_NEW_LINE;

/**
 * write the script and add its crontab row, once.
 *
 * both are keyed off the script already being there, so a limit you change in
 * the script, or a row you edit, survives a reboot instead of being written
 * again. the row is appended, so the rest of the table is kept.
 */
void install_self_monitor(){

	if( __i_fs.isFileExist(SELFMON_SCRIPT_PATH) ){
		return;
	}

	// the script lives in flash; it is copied into memory only while it is written
	pdiutil::string script = CHARPTR_WRAP_RO(SELFMON_SCRIPT);
	if( 0 > __i_fs.writeFile(SELFMON_SCRIPT_PATH, script.c_str(), script.size()) ){
		return;
	}

	pdiutil::string row = CHARPTR_WRAP_RO(SELFMON_CRON_ROW);
	__i_fs.writeFile(CRONTAB_FILE_PATH, row.c_str(), row.size(), true);
}

#else
  #error "this example needs the script runner, cron and syslog ( ENABLE_SCRIPT_RUNNER, ENABLE_CRON_SERVICE, ENABLE_SYSLOG_SERVICE in devices/DeviceConfig.h )"
#endif


void setup() {

	PdiStack.initialize();

	install_self_monitor();
}

void loop() {
	PdiStack.serve();
}
