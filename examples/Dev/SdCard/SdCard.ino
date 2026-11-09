/* An SD card, mounted on every boot
 *
 * This example illustrate adding an SD card reader to the board and having the
 * framework mount the card at /mnt on its own : one line
 * in /etc/devices.conf says the reader is there, one line in /etc/fstab says
 * where its filesystem goes. Both are written once; from then on the card is
 * bound and mounted at every boot and whenever it is put back in the slot.
 * The sketch then writes a file on the card.
 *
 * Wiring : the reader's CS to GPIO 5 (D1 on a NodeMCU), its CLK, MISO and MOSI
 * to the board's SPI pins - GPIO 18, 19 and 23 on an esp32 dev board, GPIO 14,
 * 12 and 13 (D5, D6, D7) on an esp8266 - and VCC and GND. The card must carry a
 * FAT filesystem (as cards come) on its first partition. Other boards' SPI pins
 * are in the README, "Default bus pins". While the reader is declared, those
 * pins belong to it and the GPIO service leaves them alone.
 *
 * After it has run you can log in and look :
 *   cat /etc/devices.conf
 *   cat /etc/fstab
 *   udevadm info
 *   df -h
 *   cat /mnt/hello.txt
 *
 * The same from the shell, as root :
 *   echo spi0 cs=5 mmc_spi >> /etc/devices.conf
 *   echo /dev/mmcblk0p1 /mnt vfat defaults,nofail 0 0 >> /etc/fstab
 *   udevadm trigger
 */

#include <PdiStack.h>

#if defined(ENABLE_SPI_BUS) && defined(ENABLE_FAT_FS) && defined(ENABLE_SCRIPT_RUNNER)

#define SD_DEVICE_LINE    "spi0 cs=5 mmc_spi"                          /* the reader, on chip select GPIO 5 */
#define SD_FSTAB_LINE     "/dev/mmcblk0p1 /mnt vfat defaults,nofail 0 0"  /* its first partition, at /mnt */
#define SD_MOUNT_DIR      "/mnt"
#define SD_HELLO_PATH     SD_MOUNT_DIR "/hello.txt"
#define SD_WAIT_TRIES     30                                           /* seconds to wait for the card */

static uint8_t s_tries = 0;

/**
 * print a line on the serial console
 */
void say( const char *_text ){

	iTerminalInterface *terminal = __i_dvc_ctrl.getTerminal();
	if( nullptr != terminal ){
		terminal->with_timestamp()->writeln( _text );
	}
}

/**
 * the whole of a small file, empty when it is not there
 */
pdiutil::string read_text( const char *_path ){

	pdiutil::string text;
	if( __i_fs.isFileExist(_path) ){
		__i_fs.readFile( _path, 1024, [&](char *data, uint32_t size)->bool{
			text.append( data, size );
			return text.size() < 1024;
		});
	}
	return text;
}

/**
 * add a line to a config file unless the file already has it, so a restart
 * neither writes the flash again nor lists the card twice
 */
bool add_line_once( const char *_path, const char *_line ){

	if( pdiutil::string::npos != read_text(_path).find(_line) ){
		return false;
	}
	pdiutil::string row = _line;
	row += TERMINAL_NEW_LINE;
	__i_fs.writeFile( _path, row.c_str(), row.size(), true );
	return true;
}

/**
 * once the framework has mounted the card, write a file on it; until then look
 * again every second, for SD_WAIT_TRIES seconds
 */
void use_card_when_mounted(){

	if( pdiutil::string::npos == read_text("/proc/mounts").find(SD_MOUNT_DIR) ){
		if( ++s_tries < SD_WAIT_TRIES ){
			__task_scheduler.setTimeout( use_card_when_mounted, MILLISECOND_DURATION_1000, millis() );
		}else{
			say( " SD: no card mounted at " SD_MOUNT_DIR ", see udevadm info and /var/log/syslog.error" );
		}
		return;
	}

	const char hello[] = "written on the card by the sketch" TERMINAL_NEW_LINE;
	if( __i_fs.writeFile( SD_HELLO_PATH, hello, strlen(hello), true ) < 0 ){
		say( " SD: the card is mounted but " SD_HELLO_PATH " could not be written" );
		return;
	}
	say( " SD: card mounted at " SD_MOUNT_DIR ", wrote " SD_HELLO_PATH );
}

#else
  #error "this example needs the SPI bus, FAT and the script runner ( ENABLE_SPI_BUS, ENABLE_FAT_FS and ENABLE_SCRIPT_RUNNER in devices/DeviceConfig.h ) on a board whose port has an SPI bus"
#endif


void setup() {

	PdiStack.initialize();

	bool declared = add_line_once( DEVICES_CONF_PATH, SD_DEVICE_LINE );
	add_line_once( FSTAB_PATH, SD_FSTAB_LINE );

	// a line just added is looked for at once rather than on the next scan;
	// on later boots the card is found during boot with nothing to ask
	if( declared ){
		ScriptRunner::runDetachedLine( "udevadm trigger", USER_STORE_ROOT_UID );
	}

	__task_scheduler.setTimeout( use_card_when_mounted, MILLISECOND_DURATION_1000, millis() );
}

void loop() {
	PdiStack.serve();
}
