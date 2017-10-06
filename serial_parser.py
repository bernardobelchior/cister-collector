import serial

serial_path = '/dev/ttyUSB0'
serial_baudrate = 9600

serial_port = serial.Serial(serial_path, serial_baudrate)


while True:
    line = serial_port.readline()
    split_line = line.split(" ")
    if split_line[0] == "Sensor:":
        print("ID: ")
        print(int(split_line[1]))
        print("\tTimestamp: ")
        print(int(split_line[2]))
        print("\tTemperature: ")
        print(int(split_line[3]))
        print("\tHumidity: ")
        print(int(split_line[4]))
        print("\n")
        # Add to MySQL DB
