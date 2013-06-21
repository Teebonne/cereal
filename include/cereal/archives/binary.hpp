/*
  Copyright (c) 2013, Randolph Voorhies, Shane Grant
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of cereal nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef CEREAL_ARCHIVES_BINARY_HPP_
#define CEREAL_ARCHIVES_BINARY_HPP_

#include <cereal/cereal.hpp>
#include <cereal/types/common.hpp>
#include <stack>
#include <sstream>

namespace cereal
{
  // ######################################################################
  class BinaryOutputArchive : public OutputArchive<BinaryOutputArchive, AllowEmptyClassElision>
  {
    public:
      BinaryOutputArchive(std::ostream & stream) :
        OutputArchive<BinaryOutputArchive, AllowEmptyClassElision>(this),
        itsStream(stream)
    { }

      //! Writes size bytes of data to the output stream
      void saveBinary( const void * data, size_t size )
      {
        size_t const writtenSize = itsStream.rdbuf()->sputn( reinterpret_cast<const char*>( data ), size );

        if(writtenSize != size)
          throw Exception("Failed to write " + std::to_string(size) + " bytes to output stream! Wrote " + std::to_string(writtenSize));
      }

      //! Pushes a placeholder for data onto the archive and saves its position
      void pushPosition( size_t size )
      {
        itsPositionStack.push( itsStream.tellp() );
        for(size_t i = 0; i < size; ++i)
          itsStream.rdbuf()->sputc('\0'); // char doesn't matter, but null-term is zero
      }

      //! Pops the most recently pushed position onto the archive, going to the end
      //! of the archive if the stack is empty
      /*! @return true if the stack is empty and we are at the end of the archive */
      bool popPosition()
      {
        if( itsPositionStack.empty() ) // seek to end of stream
        {
          itsStream.seekp( 0, std::ios_base::end );
          return true;
        }
        else
        {
          itsStream.seekp( itsPositionStack.top() );
          itsPositionStack.pop();
          return false;
        }
      }

      //! Resets the position stack and seeks to the end of the archive
      void resetPosition()
      {
        while( !popPosition() );
      }

    private:
      std::ostream & itsStream;
      std::stack<std::ostream::pos_type> itsPositionStack;
  };

  // ######################################################################
  class BinaryInputArchive : public InputArchive<BinaryInputArchive, AllowEmptyClassElision>
  {
    public:
      BinaryInputArchive(std::istream & stream) :
        InputArchive<BinaryInputArchive, AllowEmptyClassElision>(this),
        itsStream(stream)
    { }

      //! Reads size bytes of data from the input stream
      void loadBinary( void * const data, size_t size )
      {
        size_t const readSize = itsStream.rdbuf()->sgetn( reinterpret_cast<char*>( data ), size );

        if(readSize != size)
          throw Exception("Failed to read " + std::to_string(size) + " bytes from input stream! Read " + std::to_string(readSize));
      }

    private:
      std::istream & itsStream;
  };

  //! Saving for POD types to binary
  template<class T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  save(BinaryOutputArchive & ar, T const & t)
  {
    ar.saveBinary(std::addressof(t), sizeof(t));
  }

  //! Loading for POD types from binary
  template<class T> inline
  typename std::enable_if<std::is_arithmetic<T>::value, void>::type
  load(BinaryInputArchive & ar, T & t)
  {
    ar.loadBinary(std::addressof(t), sizeof(t));
  }

  //! Serializing NVP types to binary
  template <class Archive, class T> inline
  CEREAL_ARCHIVE_RESTRICT_SERIALIZE(BinaryInputArchive, BinaryOutputArchive)
  serialize( Archive & ar, NameValuePair<T> & t )
  {
    ar( t.value );
  }

  template <class T> inline
  typename std::enable_if<std::is_array<T>::value, void>::type
  save(BinaryOutputArchive & ar, T const & array)
  {
    ar.saveBinary(array, traits::sizeof_array<T>() * sizeof(typename std::remove_all_extents<T>::type));
  }

  template <class T> inline
  typename std::enable_if<std::is_array<T>::value, void>::type
  load(BinaryInputArchive & ar, T & array)
  {
    ar.loadBinary(array, traits::sizeof_array<T>() * sizeof(typename std::remove_all_extents<T>::type));
  }

  template <class T> inline
  void save(BinaryOutputArchive & ar, BinaryData<T> const & bd)
  {
    ar.saveBinary(bd.data, bd.size);
  }

  template <class T> inline
  void load(BinaryInputArchive & ar, BinaryData<T> & bd)
  {
    ar.loadBinary(bd.data, bd.size);
  }
} // namespace cereal

#endif // CEREAL_ARCHIVES_BINARY_HPP_