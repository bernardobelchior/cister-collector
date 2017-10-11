import serial
import requests
import json
from configparser import SafeConfigParser

serial_path = '/dev/ttyUSB0'
serial_baudrate = 115200


def read_config():
    config = SafeConfigParser()
    config.read('config.ini')
    return config.get('REST API', 'host')


def read(host):
    serial_port = serial.Serial(serial_path, serial_baudrate)

    while True:
        line = serial_port.readline().decode('ascii')
        split_line = line.split()
        print(line)
        if split_line[0] == "Sensor:":
            mote_id = int(split_line[1])
            timestamp = int(split_line[2])
            temperature = float(split_line[3])
            humidity = float(split_line[4])

            print("ID: " + split_line[1] + "\tTimestamp: " + split_line[2] +
                  "\tTemperature: " + split_line[3] + "\tHumidity: " + split_line[4])

            send_request(host, mote_id, timestamp, temperature, humidity)


def send_request(host, mote_id, timestamp, temperature, humidity):
    response = requests.put(
        host + '/addMeasurement',
        headers={'Content-Type': 'application/json'},
        data=json.dumps({
            'moteId': str(mote_id),
            'timestamp': str(timestamp * 1000),
            'temperature': str(temperature),
            'humidity': str(humidity)
        }))
    print(response)


def start():
    host = read_config()
    read(host)


start()
