import serial
import time
import tkinter as tk
import matplotlib.pyplot as plt
import matplotlib as plta
from collections import deque  # Import deque for circular buffer
import numpy as np  # Import NumPy for signal processing

plta.use('TkAgg')
# Replace these values with your actual settings
serial_port = 'COM5'  # Replace with your ESP32's serial port name
baudrate = 9600  # Replace with the communication baud rate

# Simulated data structure (replace with actual parsed data from ESP32)
data = {
    "BYP": True,
    "BEN": True,
    "FAN":True,
    "WiFi":True,
    "PI":True,
    "BVI": 0,
    "BVO": 0,
    "BCI": 0,
    "CO": 0,
    "Wh": 0,
    "temp1": 0,
    "temp2": 0,
    "IE": 0,
    "ACL": 0,
    "ACV":0,
    "ACI":0,
    "ACF":0,
    "SOC": 0,
    "ERR":0,
    "BNC":0,
    "IUV":0,
    "IOC":0,
    "OOV":0,
    "OOC":0,
    "OTE":0,
    "REC":0,
    "MPPTA":"mppt",
    "CM":"psu",
    }  # Placeholder for received data

# Number of data points to display (adjust as needed)
num_data_points =55

# Placeholder data (replace with actual data parsing logic)
pl_data = np.zeros(num_data_points)  # Initialize with zeros

# Initialize timestamps (equally spaced)
timestamps = np.linspace(0, 1/40, num_data_points)

def update_data():
    global data, pl_data, timestamps
    try:
        received_data = ser.readline().decode('ascii').strip()
        # Split data string based on comma separators
        data_pairs = received_data.split(',')
        for pair in data_pairs:
            key, value = pair.split(':')
            # Convert string value to appropriate data type (float, int, bool)
            if key.lower() == "acdata":
                volt, period = pair.split('_')
                pl_data = float(volt)
                timestamps = float(period)
            elif key.lower() == "System":
                received_sentence = value.lower()
                text_buffer.append(received_sentence)  # Add new sentence to the buffer
                if len(text_buffer) > 10:  # Remove oldest sentence if buffer is full
                    text_buffer.popleft()

            elif value.lower() == "true":
                data[key] = True
            elif value.lower() == "false":
                data[key] = False
            else:
                try:
                    data[key] = float(value)
                except ValueError:
                    # Handle potential conversion errors (e.g., invalid data)
                    print(f"Error converting value for {key}")
    except Exception as e:
        print(f"Error reading data: {e}")
     # Update text widget content with entire buffer
    text_widget.delete("1.0", tk.END)  # Clear existing text
    text_widget.insert(tk.END, "\n".join(text_buffer)) 
    plt.plot(timestamps, pl_data)  # Use timestamps for x-axis
    plt.draw()
    window.after(1000, update_data)  # Schedule next update in 1 second


def create_data_label(parent, label, value, units, index, rw):
    # Create a label for data (use grid layout for column display)
    font = ("Arial", 12, "bold")  # Adjust font properties as desired
    label_widget = tk.Label(parent, text=label, font=font)
    label_widget.grid(row=rw, column=index, padx=5, pady=5)
    try:
        # Attempt to convert value to a float
        float_value = float(value)
        text = f"{float_value:.1f} {units}"  # Use formatted value with units
    except ValueError:
        # If conversion fails, assume value is a string and display it directly
        text = value
    value_widget = tk.Label(parent, text=text, font=font)
    value_widget.grid(row=rw+1, column=index, padx=5, pady=5)


# Create the main window
window = tk.Tk()
window.title("ESP32 Telemetry")
window.geometry("1080x720")  # Adjusted window size

# Create a frame for data display with a border (using 'bd' and 'relief')
data_frame = tk.Frame(window, **{"bd": 2, "relief": "groove"})
data_frame.pack(padx=10, pady=10)



# Separator frame for the border (not needed here)
# separator_frame = tk.Frame(data_frame, **{"width": 2, "bg": "gray"})

# Create labels for power input and temperature
create_data_label(data_frame,  "buckByP:", data["BYP"], "", 1, 0)
create_data_label(data_frame,  "InvEn:", data["IE"], "", 10, 5)
create_data_label(data_frame,  "ErrCnt:", data["ERR"], "", 2, 0)
create_data_label(data_frame,  "Algo:", data["MPPTA"], "", 3, 0)
create_data_label(data_frame, "EnBuck:", data["BEN"], "", 0, 0)
create_data_label(data_frame, "Fan:", data["FAN"], "", 4, 0)
create_data_label(data_frame, "Wifi:", data["WiFi"], "", 5, 0)
create_data_label(data_frame, "IUV:", data["IUV"], "", 6, 0)
create_data_label(data_frame, "IOC:", data["IOC"], "", 7, 0)
create_data_label(data_frame, "OOV:", data["OOV"], "", 8, 0)
create_data_label(data_frame, "OOC:", data["OOC"], "", 9, 0)
create_data_label(data_frame, "OTE:", data["OTE"], "", 10, 0)
create_data_label(data_frame, "REC:", data["REC"], "", 11, 0)
create_data_label(data_frame, "CM:", data["CM"], "", 12, 0)
create_data_label(data_frame, "SolarP:", data["PI"], "W", 2, 3)
create_data_label(data_frame, "SolarV:", data["BVI"], "V", 0, 3)
create_data_label(data_frame, "ChgV:", data["BVO"], "V", 4, 3)
create_data_label(data_frame, "SolarI:", data["BCI"], "A", 1, 3)
create_data_label(data_frame, "ChrgI:", data["CO"], "A", 5, 3)
create_data_label(data_frame, "HEnergy:", data["Wh"], "Wh", 3, 3)
create_data_label(data_frame, "Temp1:", data["temp1"], "°C", 10, 3)
create_data_label(data_frame, "Temp2:", data["temp2"], "°C", 11, 3)
create_data_label(data_frame, "SoC:", data["SOC"], "%", 12, 3)
create_data_label(data_frame, "ACVolt:", data["ACV"], "V", 0, 5)
create_data_label(data_frame, "AC I:", data["ACI"], "A", 1, 5)
create_data_label(data_frame, "ACFreq:", data["ACF"], "HZ", 2, 5)
create_data_label(data_frame, "ACload:", data["ACL"], "A", 11, 5)


# Create a frame for the scrolling text display with a border
text_frame = tk.Frame(window, **{"bd": 2, "relief": "groove"})
text_frame.pack(padx=10, pady=10)

# Text widget for displaying messages
text_widget = tk.Text(text_frame, height=15, width=50)
text_widget.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

# Scrollbar for the text widget
scrollbar = tk.Scrollbar(text_frame, command=text_widget.yview)
scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
text_widget.config(yscrollcommand=scrollbar.set)

# Circular buffer to store received sentences (maximum 10)
text_buffer = deque(maxlen=10)  # Replace placeholders with actual received sentences


# Create the plot
plt.ion()  # Enable interactive mode
fig, ax = plt.subplots()
ax.set_ylim(-500, 500)  # Set y-axis limits (adjust based on your data range)
ax.set_xlabel('Time (seconds)')
ax.set_ylabel('Data Value')

# Open serial port connection (handle potential errors)
try:
    ser = serial.Serial(serial_port, baudrate)
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    exit(1)

# Start data update loop
update_data()

# Start the main event loop to keep the window open
window.mainloop()

# Show plot
plt.show()
