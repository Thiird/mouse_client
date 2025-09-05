# This program attempts to connect to the mouse via COM3, if available downloads motion data, displays them and writes the image to file
# If not available allows to load a file containing the files to display the data and write the image to file
import serial
import os
from datetime import datetime
import time
import re
import matplotlib.pyplot as plt
from tkinter import Tk, filedialog, messagebox

# Get the directory of the executable
exe_dir = os.path.dirname(os.path.abspath(__file__))
# Create output filename with timestamp
output_file = os.path.join(exe_dir, f"serial_output_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt")

def parse_output_file(file_path):
    """Parse the output file to extract X and Y hex values and convert to decimal (two's complement)."""
    block_numbers = []
    before_x_values = []
    before_y_values = []
    after_x_values = []
    after_y_values = []
    
    current_block = None
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            print(f"Processing line: {repr(line)}")  # Debug: Show raw line
            # Match block number: [ n ]
            block_match = re.match(r'\[ (\d+) \] [-]+', line)
            if block_match:
                current_block = int(block_match.group(1))
                block_numbers.append(current_block)
                print(f"Block: {current_block}")
                continue
            # Match before line: (\x00)before: |X:anything 0xHH-0xLL - Y:anything 0xHH-0xLL
            before_match = re.match(r'(?:[\x00])?before: \|X:.* 0x([0-9A-Fa-f]{2})-0x([0-9A-Fa-f]{2}) - Y:.* 0x([0-9A-Fa-f]{2})-0x([0-9A-Fa-f]{2})', line)
            if before_match and current_block is not None:
                print(f"Before match: {line}")
                x_high, x_low = int(before_match.group(1), 16), int(before_match.group(2), 16)
                y_high, y_low = int(before_match.group(3), 16), int(before_match.group(4), 16)
                value_x = (x_high << 8) | x_low
                value_y = (y_high << 8) | y_low
                if value_x >= 0x8000:  # Two's complement
                    value_x -= 0x10000
                if value_y >= 0x8000:
                    value_y -= 0x10000
                before_x_values.append(value_x)
                before_y_values.append(value_y)
                continue
            # Match after line: (\x00)after:  |X:anything 0xHH-0xLL - Y:anything 0xHH-0xLL
            after_match = re.match(r'(?:[\x00])?after:  \|X:.* 0x([0-9A-Fa-f]{2})-0x([0-9A-Fa-f]{2}) - Y:.* 0x([0-9A-Fa-f]{2})-0x([0-9A-Fa-f]{2})', line)
            if after_match and current_block is not None:
                print(f"After match: {line}")
                x_high, x_low = int(after_match.group(1), 16), int(after_match.group(2), 16)
                y_high, y_low = int(after_match.group(3), 16), int(after_match.group(4), 16)
                print("BBBBBBB")
                print(str(x_high) + " " + str(x_low) + " " + str(y_high) + " " + str(y_low))
                value_x = (x_high << 8) | x_low
                value_y = (y_high << 8) | y_low
                if value_x >= 0x8000:  # Two's complement
                    value_x -= 0x10000
                if value_y >= 0x8000:
                    value_y -= 0x10000
                print("After X: " + str(value_x))
                print("After Y: " + str(value_y))
                after_x_values.append(value_x)
                after_y_values.append(value_y)
    
    print(f"Block numbers: {block_numbers}")
    print(f"Before X: {before_x_values}")
    print(f"Before Y: {before_y_values}")
    print(f"After X: {after_x_values}")
    print(f"After Y: {after_y_values}")
    return block_numbers, before_x_values, before_y_values, after_x_values, after_y_values

def plot_values(block_numbers, before_x, before_y, after_x, after_y, output_file):
    """Create a line plot of X and Y values (before and after) over time."""
    plt.figure(figsize=(10, 6))
    plt.plot(block_numbers, before_x, label='Before X', linestyle='-', marker='o')
    plt.plot(block_numbers, before_y, label='Before Y', linestyle='-', marker='s')
    plt.plot(block_numbers, after_x, label='After X', linestyle='--', marker='^')
    plt.plot(block_numbers, after_y, label='After Y', linestyle='--', marker='d')
    plt.xlabel('Block Number (Time)')
    plt.ylabel('Decimal Value (Signed)')
    plt.title('X and Y Values (Before and After) Over Time')
    plt.legend()
    plt.grid(True)
    plot_file = os.path.splitext(output_file)[0] + '_plot.png'
    plt.savefig(plot_file)
    print(f"Plot saved to: {plot_file}")
    plt.show()

# Initialize output_file as None in case COM3 fails
output_file_processed = output_file

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
    # Show dialog to select a text file
    root = Tk()
    root.withdraw()  # Hide the main window
    messagebox.showerror("Serial Error", f"Failed to open COM3: {e}\nPlease select a text file to process.")
    output_file_processed = filedialog.askopenfilename(
        title="Select Serial Output File",
        filetypes=[("Text files", "*.txt"), ("All files", "*.*")]
    )
    if not output_file_processed:
        print("No file selected, exiting")
        exit(1)
except KeyboardInterrupt:
    print("\nProgram terminated by user")
finally:
    if 'ser' in locals() and ser.is_open:
        ser.close()
        print("Serial port closed")
    
    # Parse the file (either from serial or user selection) and plot
    try:
        block_numbers, before_x, before_y, after_x, after_y = parse_output_file(output_file_processed)
        if block_numbers:  # Only plot if data was found
            plot_values(block_numbers, before_x, before_y, after_x, after_y, output_file_processed)
        else:
            print("No valid data found in the output file for plotting")
    except Exception as e:
        print(f"Error parsing or plotting data: {e}")