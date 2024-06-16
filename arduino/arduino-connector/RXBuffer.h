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
  RXBuffer() : _bytesWritten(0), _size(sizeof(_rxBuffer)), _delim(0x00)
  {
  }

  virtual ~RXBuffer() = default;

protected:
  virtual void onMessage(uint8_t* d, const size_t& size) = 0;

  void reset()
  {
    _bytesWritten = 0;
  }

  int getpos(const uint8_t* d, const size_t& size) const
  {
    for (size_t x = 0; x < size; ++x)
    {
      if (d[x] == _delim)
      {
        return static_cast<int>(x + 1); // return position of the delimiter
      }
    }
    return 0; // No delimiter found
  }

  void feed(const uint8_t* d, const size_t& size)
  {
    int fi = getpos(d, size);

    if (fi != 0)
    {
      handleDelimiter(d, size, fi);
    }
    else if (_bytesWritten + size > _size)
    {
      reset(); // Buffer overrun potential, so reset
    }
    else
    {
      memcpy(&_rxBuffer[_bytesWritten], d, size);
      _bytesWritten += size;
    }
  }

private:
  void handleDelimiter(const uint8_t* d, const size_t& size, int fi)
  {
    if (_bytesWritten + fi > _size)
    {
      reset();
      memcpy(_rxBuffer, &d[fi], size - fi);
      _bytesWritten = size - fi;
    }
    else
    {
      memcpy(&_rxBuffer[_bytesWritten], d, fi);
      _bytesWritten += fi;
      onMessage(_rxBuffer, _bytesWritten);
      reset();

      size_t remaining = size - fi;
      if (remaining > 0)
      {
        int nextDelimPos = getpos(&d[fi], remaining);
        if (nextDelimPos)
        {
          handleDelimiter(&d[fi], remaining, nextDelimPos);
        }
        else
        {
          memcpy(_rxBuffer, &d[fi], remaining);
          _bytesWritten = remaining;
        }
      }
    }
  }

  uint8_t _rxBuffer[RX_BUFFER_SIZE];
  size_t _bytesWritten;
  size_t _size;
  uint8_t _delim;
};

#endif // RXBUFFER_H_