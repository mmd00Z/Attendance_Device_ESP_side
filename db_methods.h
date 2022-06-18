#ifndef DB_METHODS_H_INCLUDED
#define DB_METHODS_H_INCLUDED

#include <FS.h>
#include <Arduino.h>
#include <LinkedList.h>
#include "Student.h"

#define DEFAULT_PATH_FIRST_NAME "/fnames.txt"
#define DEFAULT_PATH_LAST_NAME "/lnames.txt"
#define DEFAULT_PATH_NUMBER_PHONE "/numbers.txt"
#define DEFAULT_PATH_NATIONAL_CODE "/codes.txt"

// Pack the students' data to a single unified string. 
// Spmple: pack_string('*') 
// --return--> "*student[1]_name*student[2]_name*student[3]_name*...*student[n]_name*"
String pack_string_property(LinkedList <Student*> *students_input, StudentProperty selected_property, char separator_char);

// Makes student-type objects
void make_students_in_list(LinkedList <Student*> *students_list, uint16_t n);

// Unpack the packed data to a list of student names. 
// Spmple: unpack_string(pack_string('*'), '*') or unpack_string(pack_string(' '), ' ')
// --return--> list of student names
bool unpack_string_property(LinkedList <Student*> *students_input, StudentProperty selected_property, String str_input, char separator_char);

// Save the student data with formathing by pack_string() in the file given in input function from SPIFFS esp8266 memory
// Spmple: String path_file="/my_file.txt";  save(path_file);
// --return--> Returns true if saving is successful
bool save_property(LinkedList <Student*> *students_input, String path, StudentProperty selected_property);

// Save all property whit default path
bool save(LinkedList <Student*> *students_input);

// Load the student data with formathing by pack_string() in the file given in input function from SPIFFS esp8266 memory
// Spmple: String path_file="/my_file.txt";  load(path_file);
// --return--> Returns true if loading is successful
bool load_property(LinkedList <Student*> *students_input, String path, StudentProperty selected_property);

// Load all property from default path
bool load(LinkedList <Student*> *students_input);


#endif
