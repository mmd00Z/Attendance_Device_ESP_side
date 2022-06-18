#include "db_methods.h"

String pack_string_property(LinkedList <Student*> *students_input, StudentProperty selected_property, char separator_char){
  String str = "";
  str += separator_char;
  for(uint16_t i=0; i<students_input->size(); i++) {
    switch (selected_property) {
      case FIRST_NAME:
        str += students_input->get(i)->getFirstName() + separator_char;     break;
      case LAST_NAME:
        str += students_input->get(i)->getLastName() + separator_char;      break;
      case NUMBER_PHONE:
        str += students_input->get(i)->getNumberPhone() + separator_char;  break;
      case NATIONAL_CODE:
        str += students_input->get(i)->getNationalCode() + separator_char; break;
    }
  }
  return str;
}

void make_students_in_list(LinkedList <Student*> *students_list, uint16_t n){
  while(students_list->size() < n)
    students_list->add(new Student());
}

bool unpack_string_property(LinkedList <Student*> *students_input, StudentProperty selected_property, String str_input, char separator_char){
  uint16_t number_of_strings = 0;
  for (uint16_t i=0; i<str_input.length(); i++)
    number_of_strings += (str_input.charAt(i) == separator_char)? 1:0;
  // --number_of_strings;
  
  make_students_in_list(students_input, number_of_strings);

  uint16_t index_of_separator_char = str_input.indexOf(""+separator_char);
  for (uint16_t i=0; i<number_of_strings; i++) {
    uint16_t last_index_of_separator_char = index_of_separator_char;
    for(uint16_t j=last_index_of_separator_char+1; j<str_input.length(); j++){
        if(str_input.charAt(j) == separator_char){
          index_of_separator_char=j;
          break;
        }
    }
    String str_output = "";
    for(uint16_t k=last_index_of_separator_char+1; k<index_of_separator_char; k++)
      str_output += str_input.charAt(k);
    switch (selected_property){
      case FIRST_NAME:
        students_input->get(i)->setFirstName(str_output);    break;
      case LAST_NAME:
        students_input->get(i)->setLastName(str_output);     break;
      case NUMBER_PHONE:
        students_input->get(i)->setNumberPhone(str_output);  break;
      case NATIONAL_CODE:
        students_input->get(i)->setNationalCode(str_output); break;
    }
  }
  return true;
}

bool save_property(LinkedList <Student*> *students_input, String path, StudentProperty selected_property){
  File file = SPIFFS.open(path, "w");
  file.print(pack_string_property(students_input, selected_property, '*'));
  file.close();
  return true;
}

bool save(LinkedList <Student*> *students_input) {
  return save_property(students_input, DEFAULT_PATH_FIRST_NAME, FIRST_NAME) && save_property(students_input, DEFAULT_PATH_LAST_NAME, LAST_NAME) && save_property(students_input, DEFAULT_PATH_NUMBER_PHONE, NUMBER_PHONE) && save_property(students_input, DEFAULT_PATH_NATIONAL_CODE, NATIONAL_CODE);
}

bool load_property(LinkedList <Student*> *students_input, String path, StudentProperty selected_property){
  if(SPIFFS.exists(path)){
    // file is exist
    File file = SPIFFS.open(path, "r");
    if(!file){ 
      Serial.println("Failed to open file for reading");
      return false;
    }
    else
      // file is ready 
      // starrt packing...
      unpack_string_property(students_input, selected_property, file.readString(), '*');
      file.close();
  }
  else return false;
}

bool load(LinkedList <Student*> *students_input){
  return load_property(students_input, DEFAULT_PATH_FIRST_NAME, FIRST_NAME) && load_property(students_input, DEFAULT_PATH_LAST_NAME, LAST_NAME) && load_property(students_input, DEFAULT_PATH_NUMBER_PHONE, NUMBER_PHONE) && load_property(students_input, DEFAULT_PATH_NATIONAL_CODE, NATIONAL_CODE);
}
