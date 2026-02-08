/* Task Scheduling in framework
 *
 * This example illustrate adding tasks in framework
 */

#include <PdiStack.h>

int variable_time_task_id = 0;

// should run only one time at specified duration after PdiStack initialization
void timeout_task(){

	__i_dvc_ctrl.getTerminal()->with_timestamp()->writeln_ro(RODT_ATTR(" Running inline timeout task once"));
}

// should run continueously at specific intervals after PdiStack initialization
void interval_task(){

	__i_dvc_ctrl.getTerminal()->with_timestamp()->writeln_ro(RODT_ATTR(" Running inline interval task"));
}

// update interval task after some time to run it at rate 1000 milliseconds
void update_interval_task(){

	__i_dvc_ctrl.getTerminal()->with_timestamp()->writeln_ro(RODT_ATTR(" ******* updating inline interval task @1000 milliseconds ************"));

	variable_time_task_id = __task_scheduler.updateInterval(
		variable_time_task_id,
		interval_task,
		MILLISECOND_DURATION_1000
	);
}

// clear interval task after specified duration
void clear_interval_task(){

	__i_dvc_ctrl.getTerminal()->with_timestamp()->writeln_ro(RODT_ATTR(" ******* clearing inline interval task ********"));
	__task_scheduler.clearInterval( variable_time_task_id );
}

#ifdef ENABLE_CONTEXTUAL_EXECUTION

// should run cooperatively along with inline tasks
void cooperative_task(){

	uint32_t counter = 1;
	while(1){

		__i_dvc_ctrl.getTerminal()->with_timestamp()->write_ro(RODT_ATTR(" Running cooperative task : "));
		__i_dvc_ctrl.getTerminal()->writeln((uint32_t)counter);
		__i_cooperative_scheduler.sleep(500);	// sleep for 500ms
		counter++;
		if(counter > 10) break; 
	}
}

// should run preemptively along with inline/cooperative tasks
// may face minor fractional delay in case preemptive sched is disabled during loop yield execution.
void preemptive_task(){

	uint32_t counter = 1;
	while(1){

		__i_dvc_ctrl.getTerminal()->with_timestamp()->write_ro(RODT_ATTR(" Running preemptive task : "));
		__i_dvc_ctrl.getTerminal()->writeln((uint32_t)counter);
		__i_preemptive_scheduler.sleep(500);	// sleep for 500ms
		counter++;
		if(counter > 10) break; 
	}
}

#endif

void setup() {

	// NOTE : Please disable framework serial log for this demo or framework log will get printed alongwith this demo log
	// Disable it by commenting ==> #define ENABLE_LOG_* lines in devices/DeviceConfig.h file of this framework library

	Serial.begin(115200);

	PdiStack.initialize();

	// run timeout task one time at 1000 milliseconds after PdiStack initialization
	// dont worry about clearing timeout tasks, they will die automatically after their one time execution
	__task_scheduler.setTimeout( timeout_task, MILLISECOND_DURATION_1000, millis() );

	// run interval task every 3000 milliseconds after PdiStack initialization
	// schedular provide unique id for each task to update task later in runtime if we want
	variable_time_task_id = __task_scheduler.setInterval( interval_task, MILLISECOND_DURATION_1000*3, millis() );

	// adding timeout task which will update above interval task duration after 15000 milliseconds
	__task_scheduler.setTimeout( update_interval_task, MILLISECOND_DURATION_5000*3, millis() );

	// adding timeout task which will clear above interval task duration after 30000 milliseconds
	__task_scheduler.setTimeout( clear_interval_task, MILLISECOND_DURATION_5000*6, millis() );

	#ifdef ENABLE_CONTEXTUAL_EXECUTION

	// adding cooperative task
	int cooperative_task_id = __task_scheduler.register_task(cooperative_task);
	__task_scheduler.scheduleUnderExecSched(&__i_cooperative_scheduler, cooperative_task_id, TASK_MODE_COOPERATIVE, 1*1024);

	// adding preemptive task
	int preemptive_task_id = __task_scheduler.register_task(preemptive_task);
	__task_scheduler.scheduleUnderExecSched(&__i_preemptive_scheduler, preemptive_task_id, TASK_MODE_PREEMPTIVE, 1*1024);

	#endif
}

void loop() {
	PdiStack.serve();
}
