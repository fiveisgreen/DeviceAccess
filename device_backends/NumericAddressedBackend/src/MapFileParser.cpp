#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <boost/algorithm/string.hpp>

#include "Exception.h"
#include "MapFileParser.h"
#include "NumericAddressedBackendMuxedRegisterAccessor.h" // for the MULTIPLEXED_SEQUENCE_PREFIX constant

namespace ChimeraTK {

  /********************************************************************************************************************/

  std::pair<NumericAddressedRegisterCatalogue, MetadataCatalogue> MapFileParser::parse(const std::string& file_name_) {
    file_name = file_name_;
    std::ifstream file;

    file.open(file_name.c_str());
    if(!file) {
      throw ChimeraTK::logic_error("Cannot open file \"" + file_name + "\"");
    }

    std::string line;
    while(std::getline(file, line)) {
      line_nr++;

      // Remove whitespace from beginning of line
      line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) { return !isspace(c); }));

      // Remove comments from the end of the line
      auto pos = line.find('#');
      if(pos != std::string::npos) {
        line.erase(pos, std::string::npos);
      }

      // Ignore empty lines (including all-comment lines)
      if(line.empty()) {
        continue;
      }

      // Parse meta data line
      if(line[0] == '@') {
        parseMetaData(line);
        continue;
      }

      // Parse register line
      parsedLines.push_back(parseLine(line));
    }

    // create map of registeer names to parsed lines
    // This cannot be done in the above parsing loop, as the vector might get resized which invalidates the references
    for(const auto& pl : parsedLines) {
      parsedLinesMap.emplace(pl.pathName, pl);
    }

    // add registers to the catalogue
    for(const auto& pl : parsedLines) {
      if(isScalarOr1D(pl.pathName)) {
        auto registerInfo = NumericAddressedRegisterInfo(pl.pathName, pl.nElements, pl.address, pl.nBytes, pl.bar,
            pl.width, pl.nFractionalBits, pl.signedFlag, pl.registerAccess, pl.type, pl.interruptCtrlNumber,
            pl.interruptNumber);
        pmap.addRegister(std::move(registerInfo));
      }
      else if(is2D(pl.pathName)) {
        handle2D(pl);
      }
    }

    return {std::move(pmap), std::move(metadataCatalogue)};
  }

  /********************************************************************************************************************/

  std::pair<RegisterPath, std::string> MapFileParser::splitStringAtLastDot(RegisterPath moduleDotName) {
    moduleDotName.setAltSeparator(".");
    auto regname = moduleDotName.getComponents().back();
    moduleDotName--;
    return {moduleDotName, regname};
  }

  /********************************************************************************************************************/

  std::pair<NumericAddressedRegisterInfo::Type, int> MapFileParser::getTypeAndNFractionalBits(
      std::string bitInterpretation, unsigned int width) {
    if(width == 0) return {NumericAddressedRegisterInfo::Type::VOID, 0};
    if(bitInterpretation == "IEEE754") return {NumericAddressedRegisterInfo::Type::IEEE754, 0};
    if(bitInterpretation == "ASCII") return {NumericAddressedRegisterInfo::Type::ASCII, 0};

    // If it is a digit the implicit interpretation is FixedPoint
    try {
      int nBits = std::stoi(bitInterpretation, nullptr,
          0); // base 0 = auto, hex or dec or oct
      return {NumericAddressedRegisterInfo::Type::FIXED_POINT, nBits};
    }
    catch(std::exception& e) {
      throw ChimeraTK::logic_error(std::string("Map file error in bitInterpretation: wrong argument '") +
          bitInterpretation + "', caught exception: " + e.what());
    }
  }

  /********************************************************************************************************************/

  std::pair<bool, std::pair<unsigned int, unsigned int>> MapFileParser::getInterruptData(std::string accessTypeStr) {
    std::string strToFind("INTERRUPT");
    auto pos = accessTypeStr.find(strToFind);
    if(pos == std::string::npos) return {false, {0, 0}};

    unsigned int interruptCtrlNumber = 0;
    unsigned int interruptNumber = 0;
    accessTypeStr.erase(pos, strToFind.length());

    auto delimiterPos = accessTypeStr.find(":");
    if(delimiterPos != std::string::npos) {
      std::string interruptCtrlNumberStr = accessTypeStr.substr(0, delimiterPos);
      std::string interrupNumberStr = accessTypeStr.substr(delimiterPos + 1);
      try {
        interruptCtrlNumber = std::stoul(interruptCtrlNumberStr, nullptr, 0); // base 0 = auto, hex or dec or oct
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            std::string("Map file error in accessString: wrong argument in interrupt controller number. Argument: '") +
            interruptCtrlNumberStr + "', caught exception: " + e.what());
      }

      try {
        interruptNumber = std::stoul(interrupNumberStr, nullptr, 0); // base 0 = auto, hex or dec or oct
      }
      catch(std::exception& e) {
        throw ChimeraTK::logic_error(
            std::string("Map file error in accessString: wrong argument in interrupt number. Argument: '") +
            interrupNumberStr + "', caught exception: " + e.what());
      }
    }
    else
      throw ChimeraTK::logic_error(
          std::string("Map file error in accessString: Delimiter ':' not found in INTERRPUT description "));

    return {true, {interruptCtrlNumber, interruptNumber}};
  }

  /********************************************************************************************************************/

  void MapFileParser::checkFileConsitencyAndThrowIfError(NumericAddressedRegisterInfo::Access registerAccessMode,
      NumericAddressedRegisterInfo::Type registerType, uint32_t nElements, uint64_t address, uint32_t nBytes,
      uint64_t bar, uint32_t width, int32_t nFractionalBits, bool signedFlag) {
    //
    // if type is VOID, access mode cannot me read only
    if(registerType == NumericAddressedRegisterInfo::Type::VOID &&
        registerAccessMode == NumericAddressedRegisterInfo::Access::READ_ONLY)
      throw ChimeraTK::logic_error(std::string("Map file error. Register Type is VOID and access mode is READ only. "));
    //
    // if register type is VOID and push-type. then all fields must be '0'
    if(registerType == NumericAddressedRegisterInfo::Type::VOID &&
        registerAccessMode == NumericAddressedRegisterInfo::Access::INTERRUPT) {
      if(width || nElements || address || nBytes || bar || nFractionalBits || signedFlag)
        throw ChimeraTK::logic_error(
            std::string("Map file error. Register Type is VOID (width field set to 0). All other fields must be '0'."));
    }
  }

  /********************************************************************************************************************/

  void MapFileParser::parseMetaData(std::string line) {
    std::string metadata_name, metadata_value;

    // Remove the '@' character...
    line.erase(line.begin(), line.begin() + 1);

    // ... and remove all the whitespace after it
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](int c) { return !isspace(c); }));

    std::istringstream is;
    is.str(line);
    is >> metadata_name;
    if(!is) {
      throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " + std::to_string(line_nr));
    }
    line.erase(line.begin(), line.begin() + metadata_name.length()); // remove name from the string
    line.erase(std::remove_if(line.begin(), line.end(), [](unsigned char x) { return std::isspace(x); }),
        line.end()); //remove whitespaces from rest of the string (before and after the value)
    metadata_value = line;
    metadataCatalogue.addMetadata(metadata_name, metadata_value);
    is.clear();
  }

  /********************************************************************************************************************/

  MapFileParser::ParsedLine MapFileParser::parseLine(std::string line) {
    ParsedLine pl;

    std::istringstream is;
    is.str(line);

    // extract register name
    std::string name;
    is >> name;
    pl.pathName = name;
    pl.pathName.setAltSeparator(".");

    // extract mandatory address information
    is >> std::setbase(0) >> pl.nElements >> std::setbase(0) >> pl.address >> std::setbase(0) >> pl.nBytes;
    if(!is) {
      throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " + std::to_string(line_nr));
    }

    // Note: default values for optional information are set in ParsedLine declaration

    // extract bar
    is >> std::setbase(0) >> pl.bar;

    // extract width
    if(!is.fail()) {
      is >> std::setbase(0) >> pl.width;
      if(pl.width > 32) {
        throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " +
            std::to_string(line_nr) + ": register width too big");
      }
    }

    // extract bit interpretation field (nb. of fractional bits, IEEE754, VOID, ...)
    if(!is.fail()) {
      std::string bitInterpretation;
      is >> bitInterpretation;
      if(!is.fail()) {
        // width is needed to determine whether type is VOID
        std::tie(pl.type, pl.nFractionalBits) = getTypeAndNFractionalBits(bitInterpretation, pl.width);
        if(pl.nFractionalBits > 1023 || pl.nFractionalBits < -1024) {
          throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " +
              std::to_string(line_nr) + ": too many fractional bits");
        }
      }
    }

    // extract signed flag
    if(!is.fail()) {
      is >> std::setbase(0) >> pl.signedFlag;
    }

    // extract access mode string (RO, RW, WO, INTERRUPT)
    if(!is.fail()) {
      std::string accessString;
      is >> accessString;
      if(!is.fail()) {
        // first transform to uppercase
        std::transform(accessString.begin(), accessString.end(), accessString.begin(),
            [](unsigned char c) { return std::toupper(c); });

        // first check if access mode is INTERRUPT and additionally check the interrupt controller number and
        // interrupt number
        auto interruptData = getInterruptData(accessString);

        if(interruptData.first) {
          pl.registerAccess = NumericAddressedRegisterInfo::Access::INTERRUPT;
          pl.interruptCtrlNumber = interruptData.second.first;
          pl.interruptNumber = interruptData.second.second;
        }
        else if(accessString == "RO") {
          pl.registerAccess = NumericAddressedRegisterInfo::Access::READ_ONLY;
        }
        else if(accessString == "RW") {
          pl.registerAccess = NumericAddressedRegisterInfo::Access::READ_WRITE;
        }
        else if(accessString == "WO") {
          pl.registerAccess = NumericAddressedRegisterInfo::Access::WRITE_ONLY;
        }
        else
          throw ChimeraTK::logic_error("Parsing error in map file '" + file_name + "' on line " +
              std::to_string(line_nr) + ": invalid data access");
      }
    }

    checkFileConsitencyAndThrowIfError(pl.registerAccess, pl.type, pl.nElements, pl.address, pl.nBytes, pl.bar,
        pl.width, pl.nFractionalBits, pl.signedFlag);

    return pl;
  }

  /********************************************************************************************************************/

  bool MapFileParser::isScalarOr1D(const RegisterPath& pathName) {
    auto [module, name] = splitStringAtLastDot(pathName);
    return !boost::algorithm::starts_with(name, MULTIPLEXED_SEQUENCE_PREFIX) &&
        !boost::algorithm::starts_with(name, SEQUENCE_PREFIX);
  }

  /********************************************************************************************************************/

  bool MapFileParser::is2D(const RegisterPath& pathName) {
    auto [module, name] = splitStringAtLastDot(pathName);
    return boost::algorithm::starts_with(name, MULTIPLEXED_SEQUENCE_PREFIX);
  }

  /********************************************************************************************************************/

  RegisterPath MapFileParser::makeSequenceName(const RegisterPath& pathName, size_t index) {
    auto [module, name] = splitStringAtLastDot(pathName);
    assert(boost::algorithm::starts_with(name, MULTIPLEXED_SEQUENCE_PREFIX));
    name = name.substr(MULTIPLEXED_SEQUENCE_PREFIX.size()); // strip prefix
    auto r = RegisterPath(module) / (SEQUENCE_PREFIX + name + "_" + std::to_string(index));
    r.setAltSeparator(".");
    return r;
  }

  /********************************************************************************************************************/

  RegisterPath MapFileParser::make2DName(const RegisterPath& pathName) {
    auto [module, name] = splitStringAtLastDot(pathName);
    assert(boost::algorithm::starts_with(name, MULTIPLEXED_SEQUENCE_PREFIX));
    name = name.substr(MULTIPLEXED_SEQUENCE_PREFIX.size()); // strip prefix
    auto r = RegisterPath(module) / name;
    r.setAltSeparator(".");
    return r;
  }

  /********************************************************************************************************************/

  void MapFileParser::handle2D(const ParsedLine& pl) {
    // search for sequence entries matching the given register, create ChannelInfos from them
    std::vector<NumericAddressedRegisterInfo::ChannelInfo> channels;
    size_t bytesPerBlock = 0; // size of one block in bytes (one sample for all channels)
    while(true) {
      auto it = parsedLinesMap.find(makeSequenceName(pl.pathName, channels.size()));
      if(it == parsedLinesMap.end()) break;
      auto plch = it->second;

      if(plch.address < pl.address) {
        throw ChimeraTK::logic_error(
            "Start address of channel smaller than 2D register start address ('" + pl.pathName + "').");
      }

      channels.emplace_back(NumericAddressedRegisterInfo::ChannelInfo{
          uint32_t(plch.address - pl.address) * 8, plch.type, plch.width, plch.nFractionalBits, plch.signedFlag});

      bytesPerBlock += plch.nBytes;
      if(plch.nBytes != 1 && plch.nBytes != 2 && plch.nBytes != 4) {
        throw ChimeraTK::logic_error("Sequence word size must correspond to a primitive type");
      }
    }

    if(channels.empty()) {
      throw ChimeraTK::logic_error("No sequences found for register " + pl.pathName);
    }
    assert(bytesPerBlock > 0);

    // make sure channel bit interpretation widthes are not wider than the actual channel width
    for(size_t i = 0; i < channels.size() - 1; ++i) {
      auto actualWidth = channels[i + 1].bitOffset - channels[i].bitOffset;
      if(channels[i].width > actualWidth) {
        channels[i].width = actualWidth;
      }
    }
    {
      // last channel needs special treatment
      auto actualWidth = bytesPerBlock * 8 - channels.back().bitOffset;
      if(channels.back().width > actualWidth) {
        channels.back().width = actualWidth;
      }
    }

    // compute number of blocks (= samples per channel)
    auto nBlocks = std::floor(pl.nBytes / bytesPerBlock);

    // create 2D entry
    auto name2D = make2DName(pl.pathName);
    auto registerInfo = NumericAddressedRegisterInfo(name2D, pl.bar, pl.address, nBlocks, bytesPerBlock * 8, channels,
        pl.registerAccess, pl.interruptCtrlNumber, pl.interruptNumber);
    pmap.addRegister(registerInfo);

    // create 1D entry for reading the multiplexed raw data
    assert(pl.nBytes % 4 == 0);
    auto registerInfoMuxedRaw = NumericAddressedRegisterInfo(name2D + ".MULTIPLEXED_RAW", pl.nBytes / 4, pl.address,
        pl.nBytes, pl.bar, 32, 0, true, pl.registerAccess, NumericAddressedRegisterInfo::Type::FIXED_POINT,
        pl.interruptCtrlNumber, pl.interruptNumber);
    pmap.addRegister(registerInfoMuxedRaw);
  }

  /********************************************************************************************************************/

} // namespace ChimeraTK
