/*
  LinuxCNC_ArduinoConnector
  By Alexander Richter, info@theartoftinkering.com &
  Ken Thompson (not THAT Ken Thompson), https://github.com/KennethThompson
  
  MIT License
  Copyright (c) 2023 Alexander Richter & Ken Thompson

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#ifndef RXBUFFER_H_
#define RXBUFFER_H_

class RXBuffer
{
  public:
  RXBuffer()
  {
    
  }
  ~RXBuffer()
  {
    
  }
  protected:

  virtual void onMessage(uint8_t* d, const size_t& size)=0;

  void reset()
  {
    _bytesWritten = 0;
  }


  int getpos(uint8_t * d, const size_t& size)
  {
    int fi = 0; // index the delim was found at 
    for( int x = 1; x < size+1; x++)
    {
      if(d[x-1] == 0x00)
      {
        fi = x;
        
        break;
      }
    }
    return fi;
  }

  void feed(uint8_t * d, const size_t& size)
  {
    // loop through bytes to be written and determine if there is a terminator/delimitor, i.e., 0x00
    int fi = getpos(d, size);
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
      _bytesWritten = (size - fi);
      
      //memcpy(_rxBuffer, (void*)&d[fi], (size - fi));
      //_wptr += (size - fi);
      //_bytesWritten = (size - fi);
    }
    else if( fi != 0 && (_bytesWritten + fi) <= _size )
    {
      // Here we have a delim, and enough room to fit the bytes up to the delim
      memcpy((void*)&_rxBuffer[_bytesWritten], d, fi);
      _bytesWritten += fi;
      // CALLBACK HERE..
      onMessage(_rxBuffer, _bytesWritten);
      reset();
      // do we have remaining bytes?
      
      if( size - fi > 0)
      {
        if (getpos((uint8_t*)&d[fi], (size-fi)))
        {
          memcpy(_rxBuffer, (void*)&d[fi], (size - fi));
          _bytesWritten = (size - fi);
          onMessage(_rxBuffer, _bytesWritten);
          reset();
        }
        else
        {
          memcpy(_rxBuffer, (void*)&d[fi], (size - fi));
          _bytesWritten = (size - fi);
        }
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


  uint8_t _rxBuffer[RX_BUFFER_SIZE];
  size_t _bytesWritten = 0; // Current number of bytes written to buffer
  size_t _size = sizeof(_rxBuffer); // Total size of buffer
  uint8_t _delim = 0x00;
};

#endif