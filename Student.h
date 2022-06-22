#ifndef STUDENT_H_INCLUDED
#define STUDENT_H_INCLUDED

#include <Arduino.h>
#include <LinkedList.h>
#include "Config.h"

enum StudentProperty {FIRST_NAME, LAST_NAME, NUMBER_PHONE, NATIONAL_CODE};

class Student {
  private:
    String first_name;
    String last_name;
    String number_phone;
    String national_code;
  public:
    // When student come
    Config::Time timeToCome;

    // Constractors
    Student();
    Student(String Fname, String Lname);
    Student(String Fname, String Lname, String number_phone);
    Student(String Fname, String Lname, String number_phone, String national_code);         

    // Seters
    void setFirstName(String Fname);
    void setLastName(String Lname);
    void setNumberPhone(String number_phone);
    void setNationalCode(String national_code);
    void setTimeToCome(int h, int m, int s);

    // Geters
    String getFirstName();
    String getLastName();
    String getNumberPhone();
    String getNationalCode();

    // Print All input students to Serial monitor
    static void showAllStudent(LinkedList <Student*> *input_student) {
      // Serial.println(F("_______________"));
      if(input_student->size() <= 0) 
        Serial.println("No input_student->added!");
      else {
        Serial.println("Fname\tLname\tnumber\tcode");
        for(int i=0; i<input_student->size(); i++) {
          Serial.print(i+1);
          Serial.println(". "+input_student->get(i)->getFirstName()+"\t"+input_student->get(i)->getLastName()+"\t"+input_student->get(i)->getNumberPhone()+"\t"+input_student->get(i)->getNationalCode());
        }
      }
      // Serial.println(F("_______________"));
    }
    String pack_data(char separator_char);
};


#endif
