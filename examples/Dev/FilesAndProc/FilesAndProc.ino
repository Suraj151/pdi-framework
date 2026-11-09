/* Files, directories and /proc from the sketch
 *
 * This example illustrate the filesystem the shell uses, reached from code : it
 * makes a directory, writes and appends a file and reads it back, reads the live
 * system state under /proc, and keeps a small log of the free heap that never
 * grows past a size you choose.
 *
 * After it has run you can log in and look at what it did :
 *   ls -la /data
 *   cat /data/notes.txt
 *   cat /data/heap.log
 *   cat /proc/meminfo
 *
 * The flash is small and wears with every write, so a log that grows for ever
 * fills it and a log written every second wears it out. Keep logs capped, as the
 * heap log is here, and write them no more often than you need.
 */

#include <PdiStack.h>

#if defined(ENABLE_STORAGE_SERVICE)

#define DATA_DIR          "/data"
#define NOTES_PATH        DATA_DIR "/notes.txt"
#define HEAP_LOG_PATH     DATA_DIR "/heap.log"
#define HEAP_LOG_OLD_PATH DATA_DIR "/heap.log.1"
#define HEAP_LOG_MAX      2048                        /* bytes the log may hold before it is rotated */
#define HEAP_LOG_EVERY_MS (60 * MILLISECOND_DURATION_1000)

/**
 * print a line of text on the serial console
 */
void say( const char *_label, const char *_text ){

	iTerminalInterface *terminal = __i_dvc_ctrl.getTerminal();
	if( nullptr == terminal ){
		return;
	}
	terminal->with_timestamp()->write( _label );
	terminal->writeln( _text );
}

/**
 * read a whole small file into a string.
 * readFile hands the content over in pieces, so each piece is appended as it comes
 */
pdiutil::string read_text( const char *_path, uint32_t _max ){

	pdiutil::string text;
	int status = __i_fs.readFile( _path, _max, [&](char *data, uint32_t size)->bool{
		text.append( data, size );
		return text.size() < _max;
	});
	if( status < 0 ){
		text.clear();
	}
	return text;
}

/**
 * the first line of a /proc file that starts with the key given, as in
 * "MemFree:      143648 B"
 */
pdiutil::string proc_line( const char *_path, const char *_key ){

	pdiutil::string all = read_text( _path, 512 );
	pdiutil::string::size_type at = all.find( _key );
	if( pdiutil::string::npos == at ){
		return pdiutil::string();
	}
	pdiutil::string::size_type end = all.find( '\n', at );
	return all.substr( at, (pdiutil::string::npos == end) ? pdiutil::string::npos : end - at );
}

/**
 * make the directory and write a file in it once, then add a line to it on
 * every boot and read it back
 */
void files_demo(){

	if( !__i_fs.isDirExist(DATA_DIR) ){
		__i_fs.createDirectory( DATA_DIR );
	}

	if( !__i_fs.isFileExist(NOTES_PATH) ){
		const char first[] = "written once by the sketch" TERMINAL_NEW_LINE;
		__i_fs.writeFile( NOTES_PATH, first, strlen(first) );
	}

	// append, so the lines of earlier boots are kept
	const char again[] = "one more boot" TERMINAL_NEW_LINE;
	__i_fs.writeFile( NOTES_PATH, again, strlen(again), true );

	int64_t size = __i_fs.getFileSize( NOTES_PATH );
	say( " notes.txt bytes : ", pdiutil::to_string( (uint32_t)size ).c_str() );
	say( " notes.txt : ", read_text( NOTES_PATH, 256 ).c_str() );
}

/**
 * read the live system state : how long the board has been up and how much
 * memory it has. These are not files on the flash, they are made when read
 */
void proc_demo(){

	say( " uptime : ", read_text( "/proc/uptime", 64 ).c_str() );
	say( " ", proc_line( "/proc/meminfo", "MemFree" ).c_str() );
	say( " ", proc_line( "/proc/meminfo", "MemMaxBlock" ).c_str() );
}

/**
 * add one line to the heap log, rotating it first when it is full : the full
 * log becomes heap.log.1, replacing the one before, and a fresh log starts.
 * So the two together never hold more than twice HEAP_LOG_MAX
 */
void log_heap(){

	if( __i_fs.getFileSize(HEAP_LOG_PATH) >= HEAP_LOG_MAX ){
		__i_fs.deleteFile( HEAP_LOG_OLD_PATH );
		__i_fs.rename( HEAP_LOG_PATH, HEAP_LOG_OLD_PATH );
	}

	pdiutil::string line = read_text( "/proc/uptime", 32 );
	pdiutil::string::size_type space = line.find( ' ' );
	if( pdiutil::string::npos != space ){
		line.erase( space );
	}
	line += " s, free heap ";
	line += pdiutil::to_string( __i_dvc_ctrl.get_free_heap() );
	line += TERMINAL_NEW_LINE;

	__i_fs.writeFile( HEAP_LOG_PATH, line.c_str(), line.size(), true );
}

#else
  #error "Storage service is disabled ( in devices/DeviceConfig.h of framework library ). please enable(uncomment ENABLE_STORAGE_SERVICE) it for this example"
#endif


void setup() {

	PdiStack.initialize();

	files_demo();
	proc_demo();

	log_heap();
	__task_scheduler.setInterval( log_heap, HEAP_LOG_EVERY_MS, millis() );
}

void loop() {
	PdiStack.serve();
}
