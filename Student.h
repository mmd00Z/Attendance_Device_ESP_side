#ifndef STUDENT_H_INCLUDED
#define STUDENT_H_INCLUDED

#include <Arduino.h>
    
enum StudentProperty {FIRST_NAME, LAST_NAME, NUMBER_PHONE, NATIONAL_CODE};

class Student {
  private:
    String first_name;
    String last_name;
    String number_phone;
    String national_code;
  public:
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

    // Geters
    String getFirstName();
    String getLastName();
    String getNumberPhone();
    String getNationalCode();

    String pack_data(char separator_char);
};


#endif
