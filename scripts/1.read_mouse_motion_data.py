# This program attempts to connect to the mouse via COM3 to read the motion data and saving them to file
import serial
import os
from datetime import datetime
import time

# Get the directory of the executable
exe_dir = os.path.dirname(os.path.abspath(__file__))
# Create output filename with timestamp
output_file = os.path.join(exe_dir, f"serial_output_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt")

try:
    # Initialize serial connection
    ser = serial.Serial('COM3', 115200, timeout=1)
    print("Connected to COM3 at 115200 baud")
    
    # Send '2' to serial port immediately
    ser.write(b'2')
    print("Sent: 2")
    
    # Open text file for writing
    with open(output_file, 'w') as f:
        print(f"Logging to: {output_file}")
        
        # Track last data received time
        last_data_time = time.time()
        timeout_duration = 1.0  # 1 second timeout
        buffer = ""  # Buffer to accumulate partial data
        
        while True:
            # Read data from serial port
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                buffer += data  # Accumulate data in buffer
                last_data_time = time.time()  # Update last data received time
                
                # Process complete lines
                lines = buffer.split('\n')
                # Keep the last incomplete line in the buffer
                if not data.endswith('\n'):
                    buffer = lines[-1]
                    lines = lines[:-1]
                else:
                    buffer = ""
                
                # Write non-empty lines to file and console
                for line in lines:
                    if line.strip():  # Skip empty lines
                        # Ensure each block starts with a newline if it begins with '[ n ]'
                        if line.strip().startswith('[') and f.tell() > 0:
                            f.write('\n')
                        f.write(line.rstrip() + '\n')
                        print(line.rstrip())
                f.flush()  # Ensure data is written immediately
            
            # Check for timeout
            if time.time() - last_data_time > timeout_duration:
                print("\nNo data received for 1 second, closing program")
                break

except serial.SerialException as e:
    print(f"Serial error: {e}")
except KeyboardInterrupt:
    print("\nProgram terminated by user")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")