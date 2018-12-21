#ifndef CHIMERATK_IEEE754_SINGLE_CONVERTER_H
#define CHIMERATK_IEEE754_SINGLE_CONVERTER_H

#include <boost/numeric/conversion/cast.hpp>
#include "Exception.h"

#include <float.h> // for float limits


namespace ChimeraTK{

  template<typename SourceType, typename DestType>
  struct RoundingRangeCheckingDataConverter{
    /** define round type for the boost::numeric::converter */
    template<class S>
    struct Round {
      static S nearbyint ( S s ){
        return round(s);
      }
      
      typedef boost::mpl::integral_c<std::float_round_style,std::round_to_nearest> round_style;
    };

    typedef boost::numeric::converter<DestType, SourceType, boost::numeric::conversion_traits<DestType, SourceType>,
      boost::numeric::def_overflow_handler, Round<SourceType> > converter;

  };
  
  /** Needs to have the same interface as FixedPointConverter, except for the constructor.
   *  Converter for IEEE754 single precision (32bit) floating point.
   */
  struct IEEE754_SingleConverter{

    template<typename CookedType>
    CookedType toCooked(uint32_t rawValue) const;

    template<typename CookedType>
    uint32_t toRaw(CookedType cookedValue) const;

  };

  template<typename CookedType>
  CookedType IEEE754_SingleConverter::toCooked(uint32_t rawValue) const{
    //Step 1: convert the raw data to the "generic" representation in the CPU: float
    void * warningAvoider = &rawValue;
    float genericRepresentation = *(reinterpret_cast<float *>(warningAvoider));

    //Step 2: convert the float to the cooked type
    CookedType cooked;

    try{
      cooked = RoundingRangeCheckingDataConverter<float,CookedType>::converter::convert(genericRepresentation);
    }catch( boost::numeric::positive_overflow & ) {
      throw ChimeraTK::logic_error("positive overflow"); //+_variableName);
    }catch( boost::numeric::negative_overflow & ) {
      throw ChimeraTK::logic_error("negative overflow"); //+_variableName);
    }

    return cooked;
  }

  template<>
  std::string IEEE754_SingleConverter::toCooked<std::string>(uint32_t rawValue) const{
    //Step 1 as in the normal template: reinterpret cast
    void * warningAvoider = &rawValue;
    float genericRepresentation = *(reinterpret_cast<float *>(warningAvoider));

    //Step 2: convert to string
    return std::to_string(genericRepresentation);
  }
    
  template<typename CookedType>
  uint32_t IEEE754_SingleConverter::toRaw(CookedType cookedValue) const{
    // step 1: convert from cooked to the generic representation in the CPU (float)
    float genericRepresentation;
    try{
      genericRepresentation = RoundingRangeCheckingDataConverter<CookedType,float>::converter::convert(cookedValue);
    }catch( boost::numeric::positive_overflow & ) {
      genericRepresentation = FLT_MAX;
    }catch( boost::numeric::negative_overflow & ) {
      genericRepresentation = -FLT_MAX;
    }

    // step 2: reinterpret float to int32 to send it to the device
    void * warningAvoider = &genericRepresentation;
    int32_t rawValue = *(reinterpret_cast<int32_t *>(warningAvoider));

    return rawValue;
  }

  template<>
  uint32_t IEEE754_SingleConverter::toRaw(std::string cookedValue) const{
    // step 1: convert string to float
    float genericRepresentation;
    try{
      genericRepresentation = std::stof(cookedValue);
    }catch(std::exception &e){
      //FIXME: VariableName
      // Note: We cannot do out of range limitations because we don't know if too large or too small
      throw ChimeraTK::logic_error( e.what() );
    }

    // step 2 as in the normal template: reinterpret cast
    void * warningAvoider = &genericRepresentation;
    int32_t rawValue = *(reinterpret_cast<int32_t *>(warningAvoider));

    return rawValue;
  }
  
}// namespace ChimeraTK

#endif // CHIMERATK_IEEE754_SINGLE_CONVERTER_H
