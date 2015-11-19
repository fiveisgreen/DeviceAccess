#include "MapException.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <fstream>
#include "DMapFileParser.h"

namespace mtca4u{

DeviceInfoMapPointer DMapFileParser::parse(const std::string &file_name) {
    std::ifstream file;
    std::string line;
    std::istringstream is;
    DeviceInfoMap::DeviceInfo deviceInfo;
    uint32_t line_nr = 0;

    file.open(file_name.c_str());
    if (!file) {        
        throw DMapFileParserException("Cannot open dmap file: \"" + file_name + "\"", LibMapException::EX_CANNOT_OPEN_DMAP_FILE);
    }

    DeviceInfoMapPointer dmap(new DeviceInfoMap(file_name));
    while (std::getline(file, line)) {
        line_nr++;
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int, int>(isspace))));
        if (!line.size()) {
            continue;
        }
        if (line[0] == '#') {
            continue;
        }
        is.str(line);
        is >> deviceInfo.deviceName >> deviceInfo.uri >> deviceInfo.mapFileName;
        if (is) {
            deviceInfo.dmapFileName = file_name;
            deviceInfo.dmapFileLineNumber = line_nr;
            dmap->insert(deviceInfo);
        } else {
            std::ostringstream os;
            os << line_nr;
            throw DMapFileParserException("Error in dmap file: \"" + file_name + "\" in line (" + os.str() + ") \"" + line + "\"", LibMapException::EX_DMAP_FILE_PARSE_ERROR);
        }       
        is.clear();
    }
    file.close();
    if (dmap->getSize() == 0) {
        throw DMapFileParserException("No data in in dmap file: \"" + file_name + "\"", LibMapException::EX_NO_DMAP_DATA);
    }
    return dmap;
}

}//namespace mtca4u
