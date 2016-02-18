/*
 * RegisterAccessor2D.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Martin Hierholzer
 */

#ifndef MTCA4U_REGISTER_ACCESSOR_2D_H
#define MTCA4U_REGISTER_ACCESSOR_2D_H

#include <boost/smart_ptr.hpp>

#include "TwoDRegisterAccessorImpl.h"

namespace mtca4u {

  // forward declaration to make a friend
  class TransferGroup;

  /** TODO add documentation
   */
  template<class UserType>
  class TwoDRegisterAccessor : protected TransferElement {

    public:

      /** Do not use this constructor directly. Instead call Device::getRegisterAccessor2D().
       */
      TwoDRegisterAccessor( boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > _accessor )
      : _impl(_accessor)
      {}

      /** Placeholder constructer, to allow late initialisation of the accessor, e.g. in the open function.
       *  @attention Accessors created with this constructors will be dysfunctional!
       */
      TwoDRegisterAccessor()
      {}

      /** Operator to access individual sequences.
       */
      std::vector<UserType> & operator[](size_t sequenceIndex) {
        return _impl->operator[](sequenceIndex);
      }

      /** Const operator to access individual sequences.
       */
      const std::vector<UserType> & operator[](size_t sequenceIndex) const {
        return _impl->operator[](sequenceIndex);
      }

      /** Read the data from the device, de-multiplex the hardware IO buffer and
       *  fill the sequence buffers using the fixed point converters. The read
       *  method will handle reads into the DMA regions as well
       */
      void read() {
        _impl->read();
      }

      /** Multiplex the data from the sequence buffer into the hardware IO buffer,
       * using the fixed point converters, and write it to the device. Can be used
       * to write to DMA memory Areas, but this functionality has not been
       * implemented yet
       */
      void write() {
        _impl->write();
      }

      /**
       * Return the number of sequences (=channels)
       */
      size_t getNumberOfDataSequences() {
        return _impl->getNumberOfDataSequences();
      }

      /** Return number of samples per sequence (=channel)
       */
      inline unsigned int getNumberOfSamples() {
        return _impl->getNumberOfSamples();
      }

      virtual bool isReadOnly() const {
        return _impl->isReadOnly();
      }

      /**
       * Default destructor
       */
      ~TwoDRegisterAccessor() {
      }

      virtual std::vector< boost::shared_ptr<TransferElement> > getHardwareAccessingElements() {
        return _impl->getHardwareAccessingElements();
      }

      virtual void replaceTransferElement(boost::shared_ptr<TransferElement> newElement) {
        if(_impl->isSameRegister(newElement)) {
          _impl = boost::dynamic_pointer_cast< TwoDRegisterAccessorImpl<UserType> >(newElement);
        }
        else {
          _impl->replaceTransferElement(newElement);
        }
      }

      /** Return the shared pointer to the implementation object */
      boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > getSharedPtr() {
        return _impl;
      }

    protected:

      /** Pointer to implementation */
      boost::shared_ptr< TwoDRegisterAccessorImpl<UserType> > _impl;

      // the TransferGroup must be a friend to access the actual accesor
      friend class TransferGroup;

      virtual bool isSameRegister(const boost::shared_ptr<TransferElement const> &other) const {
        return _impl->isSameRegister(other);
      }

  };

} // namespace mtca4u

#endif /* MTCA4U_REGISTER_ACCESSOR_2D_H */
