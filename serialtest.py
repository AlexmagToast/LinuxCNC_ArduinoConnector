import argparse
import logging
import traceback

import crc8
import msgpack
import serial
from cobs import cobs


class MsgPacketizer:
    def __init__(self):
        self._recv_bytes = bytearray()
        self._recv_msgs = {}
        self._callbacks = {}

    def feed(self, buffer: bytearray):
        logging.debug(f"feed: {buffer}")
        self._recv_bytes += buffer
        logging.debug(f"cached bytes: {self._recv_bytes}")
        # find delimiter (0x00)
        while self._recv_bytes.find(b"\x00") != -1:
            # if delimiter found, split and process buffer one by one
            [chunk, self._recv_bytes] = self._recv_bytes.split(b"\x00", maxsplit=1)
            logging.debug(f"cobs encoded: {chunk}, rest: {self._recv_bytes}")
            # decode cobs (NOTE: 0x00 on last byte is not required)
            decoded = cobs.decode(chunk)
            logging.debug(f"cobs decoded: {decoded}")
            # devide into idnex, data, crc
            index = decoded[0]
            data = decoded[1:-1]
            crc = decoded[-1].to_bytes(1, byteorder="big")
            logging.debug(f"index: {index}, data: {data}, crc: {crc}")
            # check crc8
            hash = crc8.crc8()
            hash.update(data)
            digest = hash.digest()
            logging.debug(f"calculated crc: {digest}")
            if digest == crc:
                # unpack msgpack
                payload = msgpack.unpackb(data, use_list=True, raw=False)
                logging.debug(f"msgpack decoded to: {payload}")
                self._recv_msgs[index] = payload
            else:
                logging.warn(f"crc not mathcd (recv: {crc}, calc: {digest})")
                continue

        # iterate over received messages and print them and remove them from buffer
        for index, msg in self._recv_msgs.items():
            # do some stuff in user-defined callback function
            logging.debug(f"received msg = index: {index}, msg: {msg}")
            if (index in self._callbacks) and (self._callbacks[index] is not None):
                self._callbacks[index](msg)

        self._recv_msgs.clear()

    def subscribe(self, index: int, callback: callable):
        self._callbacks[index] = callback


# logging.basicConfig(level=logging.INFO)
logging.basicConfig(level=logging.DEBUG)

# -p, --port /dev/tty.usbserial-0185D96C
# -b, --baudrate 115200
argparser = argparse.ArgumentParser()
argparser.add_argument("port", help="serial port")
argparser.add_argument("-b", "--baudrate", default=115200, help="baudrate")
args = argparser.parse_args()

arduino = serial.Serial(args.port, args.baudrate)

msgpacketizer = MsgPacketizer()
msgpacketizer.subscribe(0x01, lambda msg: print(f"callback 0x01: msg = {msg}"))

while True:
    try:
        num_bytes = arduino.in_waiting
        if num_bytes > 0:
            logging.debug(f"bytes in_waiting: {num_bytes}")
            msgpacketizer.feed(arduino.read(num_bytes))

    except KeyboardInterrupt:
        print("KeyboardInterrupt")
        break

    except Exception:
        just_the_string = traceback.format_exc()
        logging.error(just_the_string)

arduino.close()