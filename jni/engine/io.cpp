///----------------------------------------------------------------------------------------
/**
 * \file       io.cpp
 * \author     Vonasek Lubos
 * \date       2014/12/30
 * \brief      Common input/output utils used in program.
**/
///----------------------------------------------------------------------------------------

#include "engine/io.h"

#ifdef ANDROID
#include <android/log.h>
#define STRINGIFY(x) #x
#define LOG_TAG    __FILE__ ":" STRINGIFY(__LINE__)
#endif

std::vector<std::string> imports;     ///< Config data list

std::string fixPath(std::string filename) {
  while(true) {
    int index = 0;
    for (int i = 1; i < filename.size(); i++) {
      if ((filename[i] == '/') && (filename[i + 1] != '.'))
        index = i;
      else if ((filename[i - 1] == '/') && (filename[i] == '.')) {
        filename = filename.substr(0, index) + filename.substr(i + 2, filename.size() - i);
        break;
      }
    }
    return filename;
  }
}

/**
* @brief getConfig gets config value from file
* @param item is item to read
* @param source is source storage
* @return value in float
*/
float getConfig(std::string item, std::vector<std::string> source) {
   for (unsigned int i = 0; i < source.size(); i++) {
       float value;
       char name[256];
       sscanf(source[i].c_str(), "%s %f", &name[0], &value);
       if (strcmp(name,item.c_str()) == 0)
           return value;
   }
   return 0;
}

/**
* @brief getConfigStr gets config value from file
* @param item is item to read
* @param source is source storage
* @return value in char*
*/
std::string getConfigStr(std::string item, std::vector<std::string> source) {
   for (unsigned int i = 0; i < source.size(); i++) {
       char name[256];
       char value[256];
       sscanf(source[i].c_str(), "%s %s", &name[0], value);
       if (strcmp(name,item.c_str()) == 0) {
           return std::string(value);
       }
   }
   return std::string();
}

/**
* @brief getExtension gets file extension of file
* @param filename is filename to get extension
* @return extension as char*
*/
std::string getExtension(std::string filename) {
   char string[1024];
   for (int i = filename.length() - 1; i > 0; i--) {
       if (filename[i] == '.') {
           for (unsigned int j = i + 1; j < filename.length(); j++) {
               string[j - i - 1] = filename[j];
           }
           string[filename.length() - i - 1] = '\0';
           return string;
       }
   }
   return std::string();
}

/**
* @brief getList gets list of string from script file
* @param tag is tag of script part
* @param filename is name of file
* @return list as vector of char*
*/
std::vector<std::string> getList(std::string tag, std::string filename) {
#ifdef ZIP_ARCHIVE
    zip_file* file = zip_fopen(getZip(""), filename.c_str(), 0);
#else
    FILE* file = fopen(filename.c_str(), "r");
#endif

    std::vector<std::string> output;
    bool hasTag = false;
    if (tag.length() == 0)
        hasTag = true;
    while(true) {
       char value[1024];
       strcpy(value, "");
#ifdef ZIP_ARCHIVE
       getsEx(value, file);
#else
       fgets(value, 1024, file);
#endif

       /// fix end of line
       if (strlen(value) > 0)
           if (value[strlen(value) - 1] == '\n')
               value[strlen(value) - 1] = '\0';

       /// begin of script
       if (strcmp(value,tag.c_str()) == 0){
           hasTag = true;
       }

       /// import other file
       else if (value[0] == '#') {
           /// import file
           char name[1024];
           sscanf(value,"#include %s", name);
           imports.push_back(std::string(name));
       }

       /// load data
       else if (value[0] != '/') {
           if (strlen(value) > 1) {
               while((value[strlen(value) - 1] == '\n') || (value[strlen(value) - 1] == '\r')) {
                   value[strlen(value) - 1] = '\0';
                   if (strlen(value) <= 1) {
                       break;
                   }
               }
           }
           if (hasTag && strcmp(value,"END") == 0){
#ifdef ZIP_ARCHIVE
               zip_fclose(file);
#else
               fclose(file);
#endif
               return output;
           }
           if (hasTag)
             output.push_back(value);
       }

       /// end of file
#ifdef ZIP_ARCHIVE
       if (value[1023] == '1') {
           if (imports.size() > 0) {
               zip_fclose(file);
               file = zip_fopen(getZip(""), imports[0].c_str(), 0);
               imports.clear();
               continue;
           } else {
               zip_fclose(file);
               return output;
           }
       }
#else
       if (feof(file)) {
           if (imports.size() > 0) {
               fclose(file);
               file = fopen(imports[0].c_str(), "r");
               imports.clear();
               continue;
           } else {
               fclose(file);
               return output;
           }
       }
#endif
   }
}

/**
 * @brief gets custom implementation of syntax fgets
 * @param line is data to read
 * @param file is input stream
 */
void gets(char* line, zip_file* file) {
    char character[2];
    for (int i = 0; i < 1020; i++) {
        zip_fread(file, character, 1);
        line[i] = character[0];
        if (line[i] == '\n') {
            line[i + 1] = '\000';
            return;
        }
    }
    int i = 1020;
    line[i] = '\n';
    line[i + 1] = '\000';
}

/**
 * @brief gets custom implementation of syntax fgets
 * @param line is data to read
 * @param file is input stream
 */
void gets(char* line, FILE* file) {
    char character[2];
    for (int i = 0; i < 1020; i++) {
        fread(character, 1, 1, file);
        line[i] = character[0];
        if ((line[i] == 10) || (line[i] == 13)) {
            line[i] = '\n';
            line[i + 1] = '\000';
            return;
        }
    }
    int i = 1020;
    line[i] = '\n';
    line[i + 1] = '\000';
}


/**
* @brief getsEx gets config value from zip archive
* @param line is item to read
* @param zip_file is source storage
* @return value in char*
*/
char* getsEx(char* line, zip_file* file) {
    line[1023] = '0';
    char character[2];
    for (int i = 0; i < 1020; i++) {
        int ok = zip_fread(file, character, 1);
        if (ok <= 0) {
            line[i] = '\n';
            line[i + 1] = '\000';
            if (i == 0) {
                line[1023] = '1';
            }
            return line;
        }
        line[i] = character[0];
        if (line[i] == '\n') {
            line[i + 1] = '\000';
            return line;
        }
    }
    return line;
}

/**
* @brief loge logs an error
* @param value1 is a first value
* @param value2 is a second value
*/
void loge(std::string value1, std::string value2) {
 printf("%s %s\n", value1.c_str(), value2.c_str());
#ifdef ANDROID
 __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,"com.lvonasek.o4s:%s %s", value1.c_str(), value2.c_str());
#endif
}

/**
* @brief logi logs an information
* @param value1 is a first value
* @param value2 is a second value
*/
void logi(std::string value1, std::string value2) {
 printf("%s %s\n", value1.c_str(), value2.c_str());
#ifdef ANDROID
 __android_log_print(ANDROID_LOG_INFO,LOG_TAG,"com.lvonasek.o4s:%s %s", value1.c_str(), value2.c_str());
#endif
}

/**
 * @brief path gets path of filename
 * @param filename is full filename with path
 * @return path as string
 */
std::string path(std::string filename) {
  int index = 0;
  for (unsigned int i = 0; i < filename.length(); i++)
    if (filename[i] == '/')
      index = i;
  return filename.substr(0, index + 1);
}

/**
 * @brief scandec read number from chars
 * @param line is chars to read
 * @return number as int
 */
int scandec(char* line) {
    int number = 0;
    for (int i = 0; i < 1024; i++) {
        if (line[i] != 10) {
            number = number * 10 + line[i] - '0';
        } else {
            return number;
        }
    }
    return number;
}