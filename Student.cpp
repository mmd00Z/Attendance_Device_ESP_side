#include "Student.h"

Student::Student(){
  timeToCome.hour = 0;
  timeToCome.minute = 0;
  timeToCome.seconds = 0;
}

Student::Student(String Fname, String Lname){
  this-> first_name = Fname;
  this-> last_name = Lname;
  timeToCome.hour = 0;
  timeToCome.minute = 0;
  timeToCome.seconds = 0;
}

Student::Student(String Fname, String Lname, String number_phone){
  this-> first_name = Fname;
  this-> last_name = Lname;
  this-> number_phone = number_phone;
  timeToCome.hour = 0;
  timeToCome.minute = 0;
  timeToCome.seconds = 0;
}

Student::Student(String Fname, String Lname, String number_phone, String national_code){
  this-> first_name = Fname;
  this-> last_name = Lname;
  this -> number_phone = number_phone;   
  this -> national_code = national_code;
  timeToCome.hour = 0;
  timeToCome.minute = 0;
  timeToCome.seconds = 0;
}           

// Seters
void Student::setFirstName(String Fname) {
  this -> first_name = Fname;
}

void Student::setLastName(String Lname) {
  this -> last_name = Lname;
}

void Student::setNumberPhone(String number_phone) {
  this -> number_phone = number_phone; 
}

void Student::setNationalCode(String national_code) {
  this -> national_code = national_code;
}

void Student::setTimeToCome(int h, int m, int s) {
  this->timeToCome.hour = h;
  this->timeToCome.minute = m;
  this->timeToCome.seconds = s;
}

// Geters
String Student::getFirstName() {
  return this -> first_name;
}

String Student::getLastName() {
  return this -> last_name;
}

String Student::getNumberPhone() {
  return this -> number_phone;
}

String Student::getNationalCode() {
  return this -> national_code;
}

String Student::pack_data(char separator_char) {
  return String(""+separator_char+first_name+separator_char+last_name+separator_char+number_phone+separator_char+national_code+separator_char);
}
