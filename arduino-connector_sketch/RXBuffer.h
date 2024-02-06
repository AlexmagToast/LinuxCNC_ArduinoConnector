#ifndef RXBUFFER_H_
#define RXBUFFER_H_

class RXBuffer
{
  public:
  RXBuffer(const size_t& size)
  {
    init(size);
  }
  ~RXBuffer()
  {
    
  }
  protected:

  virtual void onMessage(uint8_t* d, const size_t& size){}

  void reset()
  {
    if(_rxBuffer != NULL)
    {
      delete [] _rxBuffer;
    }
    _rxBuffer = new uint8_t[_size];
    _bytesWritten = 0;

  }



  void feed(uint8_t * d, const size_t& size)
  {
    // loop through bytes to be written and determine if there is a terminator/delimitor, i.e., 0x00
    int fi = 0; // index the delim was found at 
    for( int x = 1; x < size+1; x++)
    {
      if(d[x-1] == 0x00)
      {
        fi = x;
        
        break;
      }
    }
    //Serial.println(fi);

    if( fi != 0 && (_bytesWritten + fi) > _size)
    {
      //Serial.println("fi != 0, Buffer Overrun, resetting!");
      // Here we have a buffer overrun potential, so we just bail and reset.
      reset();
      // write remainder of bytes into the buffer that we just reset
      //Serial.print("Copying remaining bytes, length =");
      //Serial.println((size - fi));
      
      memcpy(_rxBuffer, (void*)&d[fi], (size - fi));
      //_wptr += (size - fi);
      _bytesWritten = (size - fi);
    }
    else if( fi != 0 && (_bytesWritten + fi) <= _size )
    {
      // Here we have a delim, and enough room to fit the bytes up to the delim
      memcpy((void*)&_rxBuffer[_bytesWritten], d, fi);
      _bytesWritten += fi;
      // CALLBACK HERE..

      reset();
      // do we have remaining bytes?
      
      if( size - fi > 0)
      {
        //Serial.print("Copying remaining bytes, lenth =");
        //Serial.println((size - fi));
        // Copy those bytes into the reset buffer
        memcpy(_rxBuffer, (void*)&d[fi], (size - fi));
        //_wptr += (size - fi);
        _bytesWritten = (size - fi);
      }
      
    }
    else if( fi == 0 && (_bytesWritten + size) > _size)
    {
      //Serial.print("No Delim, no space, current bytes written =");
      //Serial.println(_bytesWritten);
      //Serial.print("Number of bytes trying to write =");
      //Serial.println(size);
      //Serial.print("_size =");
      //Serial.println(_size);
      //Serial.println("fi == 0, buffer Overrun, resetting!");
      // here we do not have a delim and the incoming bytes exceed the available buffer, so just reset.
      reset();
    }
    else
    {
      // We have space and no delim
      //Serial.print("No Delim, plenty of space, current bytes written =");
      //Serial.println(_bytesWritten);
      //Serial.print("Number of bytes to write =");
      //Serial.println(size);
      memcpy((void*)&_rxBuffer[_bytesWritten], d, size);
      _bytesWritten += size;
      //_wptr += size;
    }
    
  }
private:

  void init(const size_t& size)
  {
    _size = size;
    reset(); 
  }

  uint8_t * _rxBuffer = NULL;
  uint8_t * _wptr = NULL;
  size_t _bytesWritten = 0; // Current number of bytes written to buffer
  size_t _size = 0; // Total size of buffer
  uint8_t _delim = 0x00;
};

#endif