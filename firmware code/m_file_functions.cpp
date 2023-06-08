/*
 Copyright (c) 2023 Miguel Tomas, http://www.aeonlabs.science

License Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
You are free to:
   Share — copy and redistribute the material in any medium or format
   Adapt — remix, transform, and build upon the material

The licensor cannot revoke these freedoms as long as you follow the license terms. Under the following terms:
Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. 
You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.

NonCommercial — You may not use the material for commercial purposes.

ShareAlike — If you remix, transform, or build upon the material, you must distribute your contributions under
 the same license as the original.

No additional restrictions — You may not apply legal terms or technological measures that legally restrict others
 from doing anything the license permits.

Notices:
You do not have to comply with the license for elements of the material in the public domain or where your use
 is permitted by an applicable exception or limitation.
No warranties are given. The license may not give you all of the permissions necessary for your intended use. 
For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.


Before proceeding to download any of AeonLabs software solutions for open-source development
 and/or PCB hardware electronics development make sure you are choosing the right license for your project. See 
https://github.com/aeonSolutions/PCB-Prototyping-Catalogue/wiki/AeonLabs-Solutions-for-Open-Hardware-&-Source-Development
 for Open Hardware & Source Development for more information.

*/

#include "m_file_functions.h"

// Convert compile time to Time Struct
tm CompileDateTime(char const *dateStr, char const *timeStr){
    char s_month[5];
    int year;
    struct tm t;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(dateStr, "%s %d %d", s_month, &t.tm_mday, &year);
    sscanf(timeStr, "%2d %*c %2d %*c %2d", &t.tm_hour, &t.tm_min, &t.tm_sec);
    // Find where is s_month in month_names. Deduce month value.
    t.tm_mon = (strstr(month_names, s_month) - month_names) / 3;    
    t.tm_year = year - 1900;    
    return t;
}

// **************************************************************

bool isStringAllSpaces(String str){
  for (int i = 0; i < str.length(); i++){
    if ( str.charAt(i) != ' ' )
      return false;
  }
  return true;
}


// **************************************************************
/* https://stackoverflow.com/a/54336178/13794189 */

String addThousandSeparators(std::string value, char thousandSep, char decimalSep , char sourceDecimalSep){
    int len = value.length();
    int negative = ((len && value[0] == '-') ? 1: 0);
    int dpos = value.find_last_of(sourceDecimalSep);
    int dlen = 3 + (dpos == std::string::npos ? 0 : (len - dpos));

    if (dpos != std::string::npos && decimalSep != sourceDecimalSep) {
        value[dpos] = decimalSep;
    }

    while ((len - negative) > dlen) {
        value.insert(len - dlen, 1, thousandSep);
        dlen += 4;
        len += 1;
    }
    return String(value.c_str() );
}
