"""from tkinter import *
from tkinter.ttk import Combobox


window=Tk()
var = StringVar()
var.set("one")
data = ("one", "two", "three", "four")
cb = Combobox(window, values=data)
cb.place(x=60, y=150)

lb=Listbox(window, height=5, selectmode='multiple')
for num in data:
    lb.insert(END,num)
lb.place(x=250, y=150)

v0=IntVar()
v0.set(1)
r1=Radiobutton(window, text="male", variable=v0,value=1)
r2=Radiobutton(window, text="female", variable=v0,value=2)
r1.place(x=100,y=50)
r2.place(x=180, y=50)
                
v1 = IntVar()
v2 = IntVar()
C1 = Checkbutton(window, text = "Cricket", variable = v1)
C2 = Checkbutton(window, text = "Tennis", variable = v2)
C1.place(x=100, y=100)
C2.place(x=180, y=100)

window.title('Arduino-connector Configurator')
window.geometry("800x600+10+10")
window.mainloop()"""


"""

import os
import tkinter as tk
from tkinter import filedialog

def get_serial_devices():
    serial_path = "/dev/serial/by-id/"
    yaml_path = "config.yaml"
    if os.path.exists(serial_path):
        devices = [f for f in os.listdir(serial_path) if os.path.islink(os.path.join(serial_path, f))]
        return devices
    else:
        return []

def on_dropdown_change(event):
    selected_device = dropdown_var.get()
    if selected_device:
        device_label.config(text=f"Selected device: {selected_device}")
    else:
        device_label.config(text="No device selected")

def main():
    root = tk.Tk()
    root.title("Serial Device Selector")

    serial_devices = get_serial_devices()

    if not serial_devices:
        tk.Label(root, text="No serial devices found.", font=("Helvetica", 12)).pack(pady=10)
    else:
        dropdown_var = tk.StringVar(root)
        dropdown_var.set("Select a device")

        dropdown = tk.OptionMenu(root, dropdown_var, *serial_devices)
        dropdown.pack(pady=10)
        dropdown_var.trace_add("write", on_dropdown_change)

        device_label = tk.Label(root, text="No device selected", font=("Helvetica", 12))
        device_label.pack(pady=10)

    exit_button = tk.Button(root, text="Exit", command=root.destroy)
    exit_button.pack(pady=10)

    root.mainloop()

if __name__ == "__main__":
    main()
""""""
import os
import tkinter as tk
from tkinter import filedialog

def get_serial_devices():
    serial_path = "/dev/" #serial/by-id/
    if os.path.exists(serial_path):
        devices = [f for f in os.listdir(serial_path) if os.path.islink(os.path.join(serial_path, f))]
        return devices
    else:
        return []

def get_yaml_files_in_current_folder():
    current_folder = os.path.dirname(os.path.abspath(__file__))
    yaml_files = [f for f in os.listdir(current_folder) if f.endswith(".yaml") or f.endswith(".yml")]
    return yaml_files

def on_serial_dropdown_change(event):
    selected_device = serial_dropdown_var.get()
    if selected_device:
        serial_device_label.config(text=f"Selected serial device: {selected_device}")
    else:
        serial_device_label.config(text="No serial device selected")

def on_yaml_dropdown_change(event):
    selected_file = yaml_dropdown_var.get()
    if selected_file:
        yaml_file_label.config(text=f"Selected YAML file: {selected_file}")
    else:
        yaml_file_label.config(text="No YAML file selected")

def main():
    root = tk.Tk()
    root.title("Device and File Selector")

    # Specify preselected filenames
    preselected_serial_device = "your_preselected_serial_device"
    preselected_yaml_file = "your_preselected_yaml_file.yaml"

    serial_devices = get_serial_devices()
    yaml_files = get_yaml_files_in_current_folder()

    if not serial_devices and not yaml_files:
        tk.Label(root, text="No serial devices or YAML files found.", font=("Helvetica", 12)).pack(pady=10)
    else:
        # Serial Device Dropdown
        if serial_devices:
            serial_dropdown_var = tk.StringVar(root)
            serial_dropdown_var.set(preselected_serial_device if preselected_serial_device in serial_devices else "Select a serial device")

            serial_dropdown = tk.OptionMenu(root, serial_dropdown_var, *serial_devices)
            serial_dropdown.pack(pady=10)
            serial_dropdown_var.trace_add("write", on_serial_dropdown_change)

            serial_device_label = tk.Label(root, text=f"Selected serial device: {preselected_serial_device}", font=("Helvetica", 12))
            serial_device_label.pack(pady=10)

        # YAML File Dropdown
        if yaml_files:
            yaml_dropdown_var = tk.StringVar(root)
            yaml_dropdown_var.set(preselected_yaml_file if preselected_yaml_file in yaml_files else "Select a YAML file")

            yaml_dropdown = tk.OptionMenu(root, yaml_dropdown_var, *yaml_files)
            yaml_dropdown.pack(pady=10)
            yaml_dropdown_var.trace_add("write", on_yaml_dropdown_change)

            yaml_file_label = tk.Label(root, text=f"Selected YAML file: {preselected_yaml_file}", font=("Helvetica", 12))
            yaml_file_label.pack(pady=10)

    exit_button = tk.Button(root, text="Exit", command=root.destroy)
    exit_button.pack(pady=10)

    root.mainloop()

if __name__ == "__main__":
    main()
"""

"""


from tkinter import *
from tkinter.ttk import *

class app:
    def __init__(self, master):
        self.master = master
        self.master.geometry("800x600")
        self.master.minsize(800,600)
        self.startscreen()
        #python_image = PhotoImage(file='./assets/python.png')
        #self.Label(root, image=python_image).pack()
    
    def startscreen(self):
        for i in self.master.winfo_children():
            i.destroy()

        
        bg = PhotoImage(file = "/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/src/ArduinoChip_300px.png") 
        label1 = Label( root, image = bg) 
        label1.place(x= 20, y = 0)
 
        lbl=Label(root, text="windowsize", font=("Arial", 14))
        lbl.place(relx=0.5, y=200)
        lbl1=Label(root, text="Let's start adding your Arduinos!", font=("Arial", 11))
        lbl1.place(relx=0.5, y=220)
        btn=Button(root, text="edit config", command=self.configscreen)
        btn.place(relx=0.25, y=500)
        btn1=Button(root, text="new config", command=self.connectscreen)
        btn1.place(relx=0.5, y=500)
        btn2=Button(root, text="exit", command=self.connectscreen)
        btn2.place(relx=0.75, y=500)


    def connectscreen(self):
        for i in self.master.winfo_children():
            i.destroy()
                
        btn=Button(root, text="This is Button widget", command=self.startscreen)
        btn.place(x=280, y=100)
        lbl=Label(root, text="Arduino-connector config-tool V0.1", font=("Arial", 14))
        lbl.place(x=260, y=50)
        lbl=Label(root, text="Let's start adding your Arduinos!", font=("Arial", 11))
        lbl.place(x=260, y=150)

    def configscreen(self):
        for i in self.master.winfo_children():
            i.destroy()
        
        tabControl = Notebook(root) 
        
        for i in 5:
            tab'i' = 
        tab1 = Frame(tabControl) 
        tab2 = Frame(tabControl)
        
        tabControl.add(tab1, text ='mcu_1') 
        tabControl.add(tab2, text ='add mcu') 
        tabControl.pack(expand = 1, fill ="both") 
        
        
root = Tk()
app(root)
root.mainloop()

"""

"""window=Tk()

# Add image file 
bg = PhotoImage(file = "/home/alex/Documents/GitHub/LinuxCNC_ArduinoConnector/test.png") 
# Show image using label 
label1 = Label( window, image = bg) 
label1.place(x = 200, y = 0) 


  
txtfld=Entry(window, text="arduino")
txtfld.insert(0,"arduino")
txtfld.place(x=80, y=250)

window.title('Arduino-connector Configuration Tool')
window.geometry("800x600+200+200")"""

#window.mainloop()


import os
import glob
from tkinter import *
from tkinter import ttk
import yaml

class App(Tk):
    def __init__(self):
        super().__init__()
        self.geometry("1000x800")
        self.minsize(800, 600)
        self.configscreen()
        style = ttk.Style()
        style.configure("BW.TLabel", foreground="black")

    def startscreen(self):
        for i in self.winfo_children():
            i.destroy()
        style = ttk.Style()
        style.configure("BW.TLabel", foreground="black")
        #bg = PhotoImage(file="test.png")
        #label = ttk.Label(self, image=bg)
        #label.place(relx=0.5, y=0)

        label1 = ttk.Label(self, text="hello World", style="BW.TLabel")
        label1.pack(pady=100)

        button0 = ttk.Button(self, text="New Config", command=self.newconfig)
        button1 = ttk.Button(self, text="Load Config", command=self.loadconfig)
        button2 = ttk.Button(self, text="Exit", command= quit)

        button0.pack(padx=20, pady=5)
        button1.pack(padx=20, pady=5)
        button2.pack(padx=20, pady=5)

    def configscreen(self):
        for i in self.winfo_children():
            i.destroy()

        TopFrame = ttk.Frame(self, style="BW.TLabel")
        TopFrame.pack(padx=5, pady=5, fill="x")

        LoadBtn = ttk.Button(TopFrame, text="Load Config")
        SaveBtn = ttk.Button(TopFrame, text="Save Config")
        ExitBtn = ttk.Button(TopFrame, text="Exit", command=self.quit)
        ExitBtn.pack(side="right")
        SaveBtn.pack(side="right")
        LoadBtn.pack(side="right")

        combo_var = StringVar()
        combo = ttk.Combobox(TopFrame, textvariable=combo_var, state='normal', justify='left')
        combo.pack(padx=5, fill="x", side="left")
        
        current_folder = os.getcwd()
        yaml_files = self.get_yaml_files(current_folder)
        combo['values'] = yaml_files


        MCU = ttk.Notebook(self, padding=10)
        tab1 = ttk.Frame(MCU)
        tree_basic = ttk.Treeview(tab1, columns=("Name", "Value"))
        tree_basic.column("#0", width=120, minwidth=120)
        tree_basic.pack(expand=1, fill="both", padx=5,pady=5)
        
        for i in range(5):
            tree_basic.insert('', "end", text=f"Admin-{i}", iid=i, open=False)
        tree_basic.insert('', "end", text='John Doe', iid=5, open=False)
        tree_basic.insert('', "end", text='Jane Doe', iid=6, open=False)
        tree_basic.move(5, 0, 1)
        tree_basic.move(6, 0, 1)

        MCU.add(tab1, text='Basic Treeview')
        MCU.pack(expand=1, fill="x", ipadx=5, ipady=5)


        SettingsF = ttk.Frame(tab1)
        SettingsF.pack(fill="x",side="right")
        Button1 = ttk.Button(SettingsF)
        Button1.pack(padx=5, pady=5)
        





    def populate_tree(self, tree):
        # Insert some sample data into the tree
        for i in range(1, 6):
            tree.insert("", "end", text=f"Item 1", values=(f"Name {i}", f"Value {i}", ))

    def populate_multicolumn_tree(self, tree):
        # Insert some sample data into the multi-column tree
        for i in range(1, 6):
            tree.insert("", "end", text=f"Item {i}", values=(f"Value {i}", f"Description {i}", f"Type {i}"))


    
    def PopulateMenus(self):
        SettingsF = ttk.LabelFrame(self.tab1, text="Settings")
        SettingsF.pack(side="left")
        print(self.combo_var.get())
        yaml_content = self.read_yaml(self.combo_var.get())  # Pass the combo_var directly        yaml_parser = YamlParser(yaml_content)
        
        treeview = self.populate_treeview(self,yaml_content,SettingsF)
        treeview.pack(side="right", fill="both", expand=True)



    def populate_treeview(self, data, parent):
        for key, value in data.items():
            item_id = f"{parent}.{key}" if parent else key

            if isinstance(value, dict):
                # If the value is a dictionary, create a parent node
                self.treeview.insert(parent, "end", item_id, text=key)

                # Recursively populate the treeview for nested dictionaries
                self.populate_treeview(value, item_id)
            else:
                # If the value is a primitive, create a leaf node
                self.treeview.insert(parent, "end", item_id, text=f"{key}: {value}")

    def loadconfig(self):
        for i in self.winfo_children():
            i.destroy()

        label = ttk.Label(self, text="please select a yaml file", style="BW.TLabel")
        label.pack(pady=100)
        # Create a ComboBox to display YAML files
        combo_var = StringVar()
        self.combo = ttk.Combobox(self, textvariable=combo_var, state='normal' , width=400, justify='left')
        self.combo.pack(padx=100, pady=50)

        # Create a Button to trigger the action

        button = ttk.Button(self, text="Load YAML File", command=self.on_button_click)
        button.pack(padx=10, pady=50)

        # Get YAML files in the current folder and populate the ComboBox
        current_folder = os.getcwd()
        yaml_files = self.get_yaml_files(current_folder)
        self.combo['values'] = yaml_files
        button2 = ttk.Button(self, text="back", command= self.startscreen)
        button2.pack(padx=20, pady=5)


    def newconfig(self):
        for i in self.winfo_children():
            i.destroy()

        Frame1 = Frame(self)

        label = ttk.Label(self, text="please name new yaml file", style="BW.TLabel")
        label.pack(pady=100)
        # Create a ComboBox to display YAML files
        combo_var = StringVar()
        self.combo = ttk.Combobox(self, textvariable=combo_var, state='normal' , width=400, justify='left')
        self.combo.pack(padx=100, pady=50)

        # Create a Button to trigger the action

        button = ttk.Button(self, text="create YAML File", command=self.on_button_click)
        button.pack(padx=10, pady=50)

        # Get YAML files in the current folder and populate the ComboBox
        current_folder = os.getcwd()
        yaml_files = self.get_yaml_files(current_folder)
        self.combo['values'] = yaml_files
        button2 = ttk.Button(self, text="back", command= self.startscreen)
        button2.pack(padx=20, pady=5)


    def get_yaml_files(self, folder_path):
        """
        Get a list of YAML files in the specified folder.
        """
        yaml_files = glob.glob(os.path.join(folder_path, '*.yaml'))
        yaml_files += glob.glob(os.path.join(folder_path, '*.yml'))
        return yaml_files

    def on_button_click(self):
        """
        Event handler for the button click.
        """
        selected_path = self.combo.get()
        print("Selected Path:", selected_path)

        
    def read_yaml(file_path):
        """
        Reads a YAML file and returns the parsed content as a Python object.
        """
        with open(file_path, 'r') as file:
            yaml_content = yaml.safe_load(file)
        return yaml_content

    def write_yaml(data, file_path):
        """
        Writes a Python object to a YAML file.
        """
        with open(file_path, 'w') as file:
            yaml.dump(data, file, default_flow_style=False)

    def count_occurrences(data, key_to_count):
        """
        Counts the occurrences of a specific key in a YAML object.
        """
        count = 0
        if isinstance(data, dict):
            for key, value in data.items():
                if key == key_to_count:
                    count += 1
                if isinstance(value, (dict, list)):
                    count += App.count_occurrences(value, key_to_count)
        elif isinstance(data, list):
            for item in data:
                count += App.count_occurrences(item, key_to_count)
        return count
    
    

app = App()
app.mainloop()



"""
{'mcu': {
    'alias': 'Arduino R4 Wifi', 
    'component_name': 'arduino', 
    'dev': '/dev/cu.usbmodem14201', 
    'io_map': {
        'digitalInputs': [
            {
                'pin_id': 3, 
                'pin_name': 'din.3', 
                'input_pullup': True, 
                'pin_debounce': 0
            }, 
            {
                'pin_id': 4, 
                'pin_name': 'din.4', 
                'input_pullup': True
            }, 
            {
                'pin_id': 5, 
                'pin_name': 'din.5', 
                'input_pullup': True
            }, 
    {'pin_id': 6, 
    'pin_name': 'din.6', 
    'input_pullup': True}, 
    {'pin_id': 7, 
    'pin_name': 'din.7', 
    'input_pullup': True}, 
    {'pin_id': 8, 
    'pin_name': 'din.8', 
    'input_pullup': True}, 
    {'pin_id': 9, 
    'pin_name': 'din.9', 
    'input_pullup': True}, 
    {'pin_id': 10, 
    'pin_name': 'din.10', 
    'input_pullup': True}, 
    {'pin_id': 11, 
    'pin_name': 'din.11', 
    'input_pullup': True}, 
    {'pin_id': 12, 
    'pin_name': 'din.12', 
    'input_pullup': True}, 
    {'pin_id': 13, 
    'pin_name': 'din.13', 
    'input_pullup': True}, 
    {'pin_id': 14, 
    'pin_name': 'din.14', 
    'input_pullup': True}, 
    {'pin_id': 15, 
    'pin_name': 'din.15', 
    'input_pullup': True}, 
    {'pin_id': 16, 
    'pin_name': 'din.16', 
    'input_pullup': True}, 
    {'pin_id': 17, 
    'pin_name': 'din.17', 
    'input_pullup': True}, 
    {'pin_id': 18, 
    'pin_name': 'din.18', 
    'input_pullup': True}, 
    {'pin_id': 19, 
    'pin_name': 'din.19', 
    'input_pullup': True}, 
    {'pin_id': 20, 
    'pin_name': 'din.20', 
    'input_pullup': True}, 
    {'pin_id': 21, 
    'pin_name': 'din.21', 
    'input_pullup': True}, 
    {'pin_id': 22, 
    'pin_name': 'din.22', 
    'input_pullup': True}, 
    {'pin_id': 23, 
    'pin_name': 'din.23', 
    'input_pullup': True}, 
    {'pin_id': 24, 
    'pin_name': 'din.24', 
    'input_pullup': True}, 
    {'pin_id': 25, 
    'pin_name': 'din.25', 
    'input_pullup': True}, 
    {'pin_id': 26, 
    'pin_name': 'din.26', 
    'input_pullup': True}, 
    {'pin_id': 27, 
    'pin_name': 'din.27', 
    'input_pullup': True}, 
    {'pin_id': 28, 
    'pin_name': 'din.28', 
    'input_pullup': True}, 
    {'pin_id': 29, 
    'pin_name': 'din.29', 
    'input_pullup': True}, 
    {'pin_id': 30, 
    'pin_name': 'din.30', 
    'input_pullup': True}, 
    {'pin_id': 31, 
    'pin_name': 'din.31', 
    'input_pullup': True}, 
    {'pin_id': 32, 
    'pin_name': 'din.32', 
    'input_pullup': True}, 
    {'pin_id': 33, 
    'pin_name': 'din.33', 
    'input_pullup': True}, 
    {'pin_id': 34, 
    'pin_name': 'din.34', 
    'input_pullup': True}, 
    {'pin_id': 35, 
    'pin_name': 'din.35', 
    'input_pullup': True}, 
    {'pin_id': 36, 
    'pin_name': 'din.36', 
    'input_pullup': True}, 
    {'pin_id': 37, 
    'pin_name': 'din.37', 
    'input_pullup': True}, 
    {'pin_id': 38, 
    'pin_name': 'din.38', 
    'input_pullup': True}, 
    {'pin_id': 39, 
    'pin_name': 'din.39', 
    'input_pullup': True}, 
    {'pin_id': 40, 
    'pin_name': 'din.40', 
    'input_pullup': True}, 
    {'pin_id': 41, 
    'pin_name': 'din.41', 
    'input_pullup': True}, 
    {'pin_id': 42, 
    'pin_name': 'din.42', 
    'input_pullup': True}, 
    {'pin_id': 43, 
    'pin_name': 'din.43', 
    'input_pullup': True}, 
    {'pin_id': 44, 
    'pin_name': 'din.44', 
    'input_pullup': True}, 
    {'pin_id': 45, 
    'pin_name': 'din.45', 
    'input_pullup': True}, 
    {'pin_id': 46, 
    'pin_name': 'din.46', 
    'input_pullup': True}, 
    {'pin_id': 47, 
    'pin_name': 'din.47', 
    'input_pullup': True}, 
    {'pin_id': 48, 
    'pin_name': 'din.48', 
    'input_pullup': True}, 
    {'pin_id': 49, 
    'pin_name': 'din.49', 
    'input_pullup': True}, 
    {'pin_id': 50, 
    'pin_name': 'din.50', 
    'input_pullup': True}, 
    {'pin_id': 51, 
    'pin_name': 'din.51', 
    'input_pullup': True}, 
    {'pin_id': 52, 
    'pin_name': 'din.52', 
    'input_pullup': True}, 
    {'pin_id': 53, 
    'pin_name': 'din.53', 
    'input_pullup': True}], 
    'digitalOutputs': [
        {
            'pin_id': 51, 
            'pin_name': 'dout.51'
        }, 
        {  
            'pin_id': 13, 
            'pin_name': 'dout.13'
        }
    ]
}}}



"""

class IO_Map():
    def __init__(self):
        super().__init__()
    
    def AnalogIn():
        ist = 0
