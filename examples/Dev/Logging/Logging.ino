/* Logging from a sketch
 *
 * This example illustrate the two families of log macros and the difference
 * between them. Log* writes to the console only. SysLog* writes to the console
 * and also appends to a file under /var/log, which you can read later with
 * cat over serial, telnet or ssh.
 *
 * NOTE : console logging ships switched off. Uncomment ENABLE_CONSOLE_LOG_ALL
 * in devices/DeviceConfig.h of this framework library, or nothing below prints
 * and the sketch will look like it is doing nothing.
 */

#include <PdiStack.h>

uint32_t reading_count = 0;

/**
 * a plain message. with no arguments it must not carry a bare % sign,
 * the text goes straight to the formatter
 */
void log_plain_messages(){

	LogI("sensor loop started");
	SysLogI("sensor loop started, this one is kept in /var/log/syslog.info");
}

/**
 * a message with values in it.
 * the formatter understands %d %i %u %x %X %f %s %c and %%.
 * it has no precision, so %.2f will not do what it does on a desktop.
 */
void log_a_reading(uint32_t reading){

	LogI("reading %u is %u", reading_count, reading);
}

/**
 * when a path throws away data or gives up on work, use SysLog.
 * a console only warning disappears completely on a board whose console log is
 * off, and the loss then leaves no trace anywhere.
 */
void log_a_failure(){

	SysLogE("could not store reading %u, dropping it", reading_count);
}

/**
 * pretend sensor work, logs a value every time it runs.
 * everything counts off the run counter rather than the clock, so both log
 * files fill at a rate you can predict and check.
 */
void sensor_task(){

	reading_count++;

	uint32_t reading = (reading_count * 7) % 100;

	log_a_reading(reading);

	// every third reading, so the error path shows up without a long wait
	if( 0 == reading_count % 3 ){
		log_a_failure();
	}
}


void setup() {

	PdiStack.initialize();

	log_plain_messages();

	__task_scheduler.setInterval( sensor_task, MILLISECOND_DURATION_5000, __i_dvc_ctrl.millis_now() );
}

void loop() {
	PdiStack.serve();
}
