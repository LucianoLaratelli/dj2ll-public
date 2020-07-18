#ifndef __TEST_HPP_
#define __TEST_HPP_

#include <map>
#include <string>
#include <vector>

std::string trimFromLastOccurrence(std::string trimFrom, std::string thisChar);

std::vector<std::string> parseFile(FILE *file);

std::vector<std::string>
runTestGetResult(std::string fileName,
                 std::map<std::string, bool> compilerOptions, char **argv);

#endif
