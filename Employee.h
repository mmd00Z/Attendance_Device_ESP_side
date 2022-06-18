#ifndef EMPLOYEE_H_INCLUDED
#define EMPLOYEE_H_INCLUDED

#include <Arduino.h>
    
enum EmployeeProperty {NAME, NUMBER_PHONE, NUID};

class Employee {
  private:
    String name;
    String number_phone;
    String uid;
  public:
    // Constractors
    Employee();
    Employee(String name);
    Employee(String name, String number_phone);
    Employee(String name, String number_phone, String uid);         

    // Seters
    void setName(String name);
    void setNumber_phone(String number_phone);
    void setUid(String uid);

    // Geters
    String getName();
    String getNumber_phone();
    String getUid();
};


#endif
