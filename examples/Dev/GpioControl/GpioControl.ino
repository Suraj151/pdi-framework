/* Driving and reading pins from the sketch
 *
 * This example illustrate the pins the way the shell sees them : as files under
 * /sys/class/gpio. It makes one pin blink, takes one pin as a digital input and
 * one as an analog input, and prints on the serial console whenever an input
 * changes.
 *
 * The same pins from the shell :
 *   echo 3 > /sys/class/gpio/4/mode       # blink
 *   echo 500 > /sys/class/gpio/4/value    # every 500 ms
 *   cat /sys/class/gpio/5/value           # read an input
 *
 * The numbers are the framework's pins, not the chip's GPIO numbers. On a
 * NodeMCU pin 4 is D4, the on-board LED; the table of what each number drives
 * on each board is in doc/services.md, "Pin numbers".
 *
 * NOTE : writing a pin's mode or value also saves the pin settings to flash, so
 * the board comes back the same after a restart. That is right for a setting and
 * wrong for something that changes all the time : never toggle a pin by writing
 * its value in a loop. Let the GPIO service do the work, the way the blink mode
 * does here, and only write when the setting itself changes.
 */

#include <PdiStack.h>

#if defined(ENABLE_GPIO_SERVICE) && defined(ENABLE_STORAGE_SERVICE)

#define BLINK_PIN         4                       /* framework pin that blinks */
#define BLINK_EVERY_MS    500                     /* blink interval */
#define INPUT_PIN         5                       /* framework pin read as a digital input */
#define ANALOG_PIN        MAX_DIGITAL_GPIO_PINS   /* the first analog pin of this board */
#define ANALOG_STEP       50                      /* smallest analog change worth printing */
#define CHECK_EVERY_MS    MILLISECOND_DURATION_1000

/* mode numbers, the same ones `echo <n> > /sys/class/gpio/<pin>/mode` takes */
#define MODE_DIGITAL_READ 2
#define MODE_BLINK        3
#define MODE_ANALOG_READ  5

static int32_t s_last_input = -1;
static int32_t s_last_analog = -1;

/**
 * the path of one leaf of a pin, mode or value
 */
pdiutil::string pin_path( uint8_t _pin, const char *_leaf ){

	pdiutil::string path = "/sys/class/gpio/";
	path += pdiutil::to_string( (uint32_t)_pin );
	path += "/";
	path += _leaf;
	return path;
}

/**
 * read the number a pin leaf holds, -1 when it cannot be read
 */
int32_t read_pin( uint8_t _pin, const char *_leaf ){

	char text[16] = {0};
	uint32_t got = 0;
	int status = __i_fs.readFile( pin_path(_pin, _leaf).c_str(), sizeof(text) - 1, [&](char *data, uint32_t size)->bool{
		uint32_t take = pdistd::min( size, (uint32_t)(sizeof(text) - 1 - got) );
		memcpy( text + got, data, take );
		got += take;
		return got < sizeof(text) - 1;
	});
	if( status < 0 || 0 == got ){
		return -1;
	}
	return (int32_t)StringToUint32( text );
}

/**
 * write a number to a pin leaf, but only when it differs from what is there,
 * which keeps a restart from writing the flash again for nothing
 */
void set_pin( uint8_t _pin, const char *_leaf, uint32_t _value ){

	if( read_pin(_pin, _leaf) == (int32_t)_value ){
		return;
	}
	pdiutil::string text = pdiutil::to_string( _value );
	__i_fs.writeFile( pin_path(_pin, _leaf).c_str(), text.c_str(), text.size() );
}

/**
 * print one line on the serial console
 */
void say( const char *_what, uint8_t _pin, int32_t _value ){

	iTerminalInterface *terminal = __i_dvc_ctrl.getTerminal();
	if( nullptr == terminal ){
		return;
	}
	terminal->with_timestamp()->write( _what );
	terminal->write( (uint32_t)_pin );
	terminal->write_ro( RODT_ATTR(" now ") );
	terminal->writeln( (int32_t)_value );
}

/**
 * look at both inputs and print the ones that changed.
 * the GPIO service samples the pins on its own, so this only reads what it saw
 */
void check_inputs(){

	int32_t input = read_pin( INPUT_PIN, "value" );
	if( input >= 0 && input != s_last_input ){
		s_last_input = input;
		say( " digital pin ", INPUT_PIN, input );
	}

	int32_t analog = read_pin( ANALOG_PIN, "value" );
	if( analog >= 0 && (s_last_analog < 0 || analog > s_last_analog + ANALOG_STEP || analog + ANALOG_STEP < s_last_analog) ){
		s_last_analog = analog;
		say( " analog pin ", ANALOG_PIN, analog );
	}
}

#else
  #error "GPIO or storage service is disabled ( in devices/DeviceConfig.h of framework library ). please enable(uncomment ENABLE_GPIO_SERVICE and ENABLE_STORAGE_SERVICE) it for this example"
#endif


void setup() {

	PdiStack.initialize();

	// mode first, then value : a value written to a pin that is off is kept but drives nothing
	set_pin( BLINK_PIN, "mode", MODE_BLINK );
	set_pin( BLINK_PIN, "value", BLINK_EVERY_MS );

	set_pin( INPUT_PIN, "mode", MODE_DIGITAL_READ );
	set_pin( ANALOG_PIN, "mode", MODE_ANALOG_READ );

	__task_scheduler.setInterval( check_inputs, CHECK_EVERY_MS, millis() );
}

void loop() {
	PdiStack.serve();
}
