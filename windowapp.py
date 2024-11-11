import serial
import time
import threading
import queue
import tkinter as tk
import matplotlib.pyplot as plt
import matplotlib as plta
from collections import deque  # Import deque for circular buffer
import numpy as np  # Import NumPy for signal processing

plta.use('TkAgg')
# Replace these values with your actual settings
serial_port = 'COM5'  # Replace with your ESP32's serial port name
baudrate = 115200  # Replace with the communication baud rate

data_queue = queue.Queue()

# Simulated data structure (replace with actual parsed data from ESP32)
data = {
    "BYP": True,
    "BEN": True,
    "FAN":True,
    "WIFI":True,
    "PI":True,
    "BVI": 0,
    "BVO": 0,
    "BCI": 0,
    "CO": 0,
    "WH": 0,
    'temp1': 0,
    'temp2': 0,
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
    "FSPS": 0,
    }  # Placeholder for received data

# Number of data points to display (adjust as needed)
num_data_points =55

# Placeholder data (replace with actual data parsing logic)
pl_data = np.zeros(num_data_points)  # Initialize with zeros

# Initialize timestamps (equally spaced)
timestamps = np.linspace(0, 1/40, num_data_points)

def serial_thread():
    global data_queue
    # Open serial port connection (handle potential errors)
    try:
        ser = serial.Serial(serial_port, baudrate)
        ser.setDTR(False)
        ser.setRTS(False)
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        exit(1)
    while True:
        try:
            received_data = ser.readline()
            decoded_string = str(received_data[0:len(received_data)].decode("ascii"))
            # Process the data and put it in the queue
            data_queue.put(decoded_string)
        except Exception as e:
            print(f"Error reading serial data: {e}")

def update_label_value(label, new_value,unit):
  label.config(text=f"{new_value} {unit}")


def update_data():
    global data, pl_data, timestamps
    while not data_queue.empty():
        received_data =  data_queue.get()
        decoded_string = str(received_data[0:len(received_data)])
        # Split data string based on comma separators
        data_pairs = decoded_string.split(',')
        for pair in data_pairs:
            if ':' in pair:
                key, value = pair.split(':')
                # Convert string value to appropriate data type (float, int, bool)
                if key.lower() == "acdata":
                    volt, period = pair.split('_')
                    pl_data = float(volt)
                    timestamps = float(period) 
                elif str(key) == "System":
                    print(key.lower())
                    received_sentence = value.lower()
                    text_buffer.append(received_sentence)  # Add new sentence to the buffer
                    if len(text_buffer) > 10:  # Remove oldest sentence if buffer is full
                        text_buffer.popleft()

                elif value.lower() == '1':
                    #print("true")
                    data[key] = True
                elif value.lower() == '0':
                    #print("false")
                    data[key] = False
                else:
                    try:
                        data[key] = float(value)
                    except ValueError:
                        # Handle potential conversion errors (e.g., invalid data)
                        print(f"Error converting value for {key}")
            else:
                # Handle the case where there's no colon or the pair is empty
                print(f"Invalid pair: {pair}")
    update_label_value(byp_label, data["BYP"], "")  
    update_label_value(inven_label, data["IE"], "")  
    update_label_value(errcnt_label, data["ERR"], "")  
    update_label_value(algo_label, data["MPPTA"], "")  
    update_label_value(enbuck_label, data["BEN"], "")
    update_label_value(fan_label, data["FAN"], "") 
    update_label_value(wifi_label, data["WIFI"], "") 
    update_label_value(iuv_label, data["IUV"], "") 
    update_label_value(ioc_label, data["IOC"], "") 
    update_label_value(oov_label, data["OOV"], "")
    update_label_value(ooc_label, data["OOC"], "")  
    update_label_value(ote_label, data["OTE"], "")  
    update_label_value(rec_label, data["REC"], "")  
    update_label_value(cm_label, data["CM"], "") 
    update_label_value(solarp_label, data["PI"], "W")   
    update_label_value(solarv_label, data["BVI"], "V") 
    update_label_value(chgv_label, data["BVO"], "V")
    update_label_value(solari_label, data["BCI"], "A") 
    update_label_value(chrgi_label, data["CO"], "A") 
    update_label_value(wh_label, data["WH"], "WH")
    update_label_value(temp1_label, data['temp1'], "°C")
    update_label_value(temp2_label, data['temp2'], "°C")
    update_label_value(soc_label, data["SOC"], "%")
    update_label_value(acvolt_label, data["ACV"], "V")
    update_label_value(aci_label, data["ACI"], "I")
    update_label_value(acfreq_label, data["ACF"], "HZ")
    update_label_value(acload_label, data["ACL"], "A") 
    update_label_value(feedback_label, data["FSPS"], "HZ")     
     # Update text widget content with entire buffer
    text_widget.delete("1.0", tk.END)  # Clear existing text
    text_widget.insert(tk.END, "\n".join(text_buffer)) 
    plt.plot(timestamps, pl_data)  # Use timestamps for x-axis
    plt.draw()
    window.after(100, update_data)  # Schedule next update in 1 second


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
    return value_widget


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
byp_label = create_data_label(data_frame,  "buckByP:", "", "", 1, 0)
inven_label = create_data_label(data_frame,  "InvEn:", "", "", 10, 5)
errcnt_label = create_data_label(data_frame,  "ErrCnt:", "", "", 2, 0)
algo_label = create_data_label(data_frame,  "Algo:", "", "", 3, 0)
enbuck_label = create_data_label(data_frame, "EnBuck:", "", "", 0, 0)
fan_label = create_data_label(data_frame, "Fan:", "", "", 4, 0)
wifi_label = create_data_label(data_frame, "Wifi:", "", "", 5, 0)
iuv_label = create_data_label(data_frame, "IUV:", "", "", 6, 0)
ioc_label = create_data_label(data_frame, "IOC:", "", "", 7, 0)
oov_label = create_data_label(data_frame, "OOV:", "", "", 8, 0)
ooc_label = create_data_label(data_frame, "OOC:", "", "", 9, 0)
ote_label = create_data_label(data_frame, "OTE:", "", "", 10, 0)
rec_label = create_data_label(data_frame, "REC:", "", "", 11, 0)
cm_label = create_data_label(data_frame, "CM:", "", "", 12, 0)
solarp_label = create_data_label(data_frame, "SolarP:", "", "", 2, 3)
solarv_label = create_data_label(data_frame, "SolarV:", "", "", 0, 3)
chgv_label = create_data_label(data_frame, "ChgV:", "", "", 4, 3)
solari_label = create_data_label(data_frame, "SolarI:", "", "", 1, 3)
chrgi_label = create_data_label(data_frame, "ChrgI:", "", "", 5, 3)
wh_label = create_data_label(data_frame, "HEnergy:", "", "", 3, 3)
temp1_label = create_data_label(data_frame, "Temp1:", "", "", 10, 3)
temp2_label = create_data_label(data_frame, "Temp2:", "", "", 11, 3)
soc_label = create_data_label(data_frame, "SoC:", "", "", 12, 3)
acvolt_label = create_data_label(data_frame, "ACVolt:", "", "", 0, 5)
aci_label = create_data_label(data_frame, "AC I:", "", "", 1, 5)
acfreq_label = create_data_label(data_frame, "ACFreq:", "", "", 2, 5)
acload_label = create_data_label(data_frame, "ACload:", "", "", 11, 5)
feedback_label = create_data_label(data_frame, "Fedbck:", "", "", 3, 5)

# Create a frame for the scrolling text display with a border
text_frame = tk.Frame(window, **{"bd": 2, "relief": "groove"})
text_frame.pack(padx=10, pady=10)

# Text widget for displaying messages
text_widget = tk.Text(text_frame, height=20, width=50)
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

# Start data update loop
update_data()

# Start the serial thread
serial_thread = threading.Thread(target=serial_thread)
serial_thread.daemon = True  # Daemon thread will exit when main program exits
serial_thread.start()

# Start the main event loop to keep the window open
window.after(100, update_data)  # Schedule the first update
window.mainloop()

# Show plot
plt.show()
