/* Adding table in database (available from third release)
 *
 * This example illustrate adding table to database
 */

#include <PdiStack.h>


#if defined(ENABLE_HTTP_SERVER)

// be carefull about table address or else table will not register.
// it should be on minimum distant from last added table address + last added table size
// keep safe distance between table addresses accordings their size
// addresses upto 2499 are reserved for default framework tables so choose addresses from 2500 onwards
// Eeprom is using only one spi flash sector so we have max size of 4096 bytes only.
// hence put only importanat configs in tables

#define MAX_STUDENTS			5
#define STUDENT_NAME_MAX_SIZE	20
#define STUDENT_TABLE_ADDRESS	2500

enum sex{ MALE,	FEMALE };

typedef struct {
	char _name[STUDENT_NAME_MAX_SIZE];
	uint8_t _age;
	sex _sex;
} student_t;

struct student_table {
  	student_t students[MAX_STUDENTS];
	int student_count;
};

const student_table PROGMEM _student_table_defaults = {NULL, 0};

/**
 * StudentTable class should extends public DatabaseTable as its base/parent class
 */
class StudentTable : public DatabaseTable<student_table> {

  public:
    /**
     * StudentTable constructor.
     */
    StudentTable(){
	}

	/**
     * register tables to database
     */
    void boot(){
      this->register_table( STUDENT_TABLE_ADDRESS );
    }

    /**
     * get/fetch table from database.
     *
     * @return student_table
     */
    bool get( student_table* _table ){
      return this->get_table( STUDENT_TABLE_ADDRESS, _table );
    }

    /**
     * set table in database.
     *
     * @param student_table* _table
     */
    void set( student_table* _table ){
      this->set_table( STUDENT_TABLE_ADDRESS, _table );
    }

    /**
     * clear table in database.
     */
    void clear(){
      this->clear_table( STUDENT_TABLE_ADDRESS );
    }
};

/**
 * this should be defined before framework initialization
 */
StudentTable __student_table;
#else
  #error "HTTP server is disabled ( in config/Common.h of framework library ). please enable(uncomment) it for this example"
#endif


void setup() {

	// NOTE : Please disable framework serial log for this demo or framework log will get printed alongwith this demo log
	// Disable it by commenting ==> #define ENABLE_LOG_* lines in src/config/Common.h file of this framework library

	Serial.begin(115200);
	Serial.printf("Hold on!!!, Stack will initialize and begin within next %d seconds !\n", WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT);

	PdiStack.initialize();

	student_table _students_data;

	// Adding sample records to table
	memset(_students_data.students[0]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_P(_students_data.students[0]._name, "suraj inamdar");
	_students_data.students[0]._age = 20;
	_students_data.students[0]._sex = MALE;
	_students_data.student_count = 1;

	memset(_students_data.students[1]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_P(_students_data.students[1]._name, "sajeev reddy");
	_students_data.students[1]._age = 21;
	_students_data.students[1]._sex = MALE;
	_students_data.student_count++;


	memset(_students_data.students[2]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_P(_students_data.students[2]._name, "sakshi mehta");
	_students_data.students[2]._age = 19;
	_students_data.students[2]._sex = FEMALE;
	_students_data.student_count++;

	memset(_students_data.students[3]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_P(_students_data.students[3]._name, "john stephens");
	_students_data.students[3]._age = 22;
	_students_data.students[3]._sex = MALE;
	_students_data.student_count++;

	memset(_students_data.students[4]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_P(_students_data.students[4]._name, "angelina jolie");
	_students_data.students[4]._age = 18;
	_students_data.students[4]._sex = FEMALE;
	_students_data.student_count++;

	// now set table in database
	__student_table.set(&_students_data);

	// add task of print stdudent table every 5000 milliseconds
	__task_scheduler.setInterval( printStudents, MILLISECOND_DURATION_5000, millis() );
}

void loop() {
	PdiStack.serve();
}

/**
 * prints log as per defined duration
 */
void printStudents(){

	//get student table from database
	student_table _students_data;
	__student_table.get(&_students_data);

	Serial.println(F("\n**************************Adding Database Example Log**************************"));
	Serial.println( F("Student table : \nname\t\tage\tsex\n") );
	for (int i = 0; i < _students_data.student_count; i++) {

		Serial.print( _students_data.students[i]._name ); Serial.print(F("\t"));
		Serial.print( _students_data.students[i]._age ); Serial.print(F("\t"));
		Serial.println( _students_data.students[i]._sex==MALE? F("male"):F("female"));
	}
	Serial.println(F("******************************************************************************"));
}
