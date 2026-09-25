/* Adding table in database (available from third release)
 *
 * This example illustrate adding table to database
 */

#include <PdiStack.h>


// ids 1 to 9 belong to the framework tables, so take one from 10 onwards. An id
// is permanent, never give a new table the id a removed one had.
// where the record is kept is worked out by the framework, so there is no address to pick.
// The database is a few kilobytes, so put only important configs in tables.

#define MAX_STUDENTS			5
#define STUDENT_NAME_MAX_SIZE	20
#define STUDENT_TABLE_ID		10

enum sex{ MALE,	FEMALE };

typedef struct {
	char _name[STUDENT_NAME_MAX_SIZE];
	uint8_t _age;
	sex _sex;
} student_t;

struct student_table {

	// the framework reads a record into a default constructed struct, so give it one
	student_table(){
		memset(students, 0, sizeof(students));
		student_count = 0;
	}

  	student_t students[MAX_STUDENTS];
	int student_count;
};

/**
 * StudentTable class should extends public DatabaseTable as its base/parent class
 */
class StudentTable : public DatabaseTable<STUDENT_TABLE_ID, student_table> {};

/**
 * this should be defined before framework initialization
 */
StudentTable __student_table;


void setup() {

	// NOTE : framework console log ships switched off, so only this demo log prints.
	// If you have uncommented ENABLE_CONSOLE_LOG_* in devices/DeviceConfig.h of this
	// framework library, comment them again or both logs will mix on the console.

	Serial.begin(115200);
	Serial.printf("Hold on!!!, Stack will initialize and begin within next %d seconds !\n", WIFI_STATION_CONNECT_ATTEMPT_TIMEOUT);

	PdiStack.initialize();

	student_table _students_data;

	// Adding sample records to table
	memset(_students_data.students[0]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_ro(_students_data.students[0]._name, "suraj inamdar");
	_students_data.students[0]._age = 20;
	_students_data.students[0]._sex = MALE;
	_students_data.student_count = 1;

	memset(_students_data.students[1]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_ro(_students_data.students[1]._name, "sajeev reddy");
	_students_data.students[1]._age = 21;
	_students_data.students[1]._sex = MALE;
	_students_data.student_count++;


	memset(_students_data.students[2]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_ro(_students_data.students[2]._name, "sakshi mehta");
	_students_data.students[2]._age = 19;
	_students_data.students[2]._sex = FEMALE;
	_students_data.student_count++;

	memset(_students_data.students[3]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_ro(_students_data.students[3]._name, "john stephens");
	_students_data.students[3]._age = 22;
	_students_data.students[3]._sex = MALE;
	_students_data.student_count++;

	memset(_students_data.students[4]._name, 0, STUDENT_NAME_MAX_SIZE);
	strcpy_ro(_students_data.students[4]._name, "angelina jolie");
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
