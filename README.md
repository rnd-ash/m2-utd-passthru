# M2 under the dash passthru
Passthru API (j2534) Driver for Macchina M2 Under the dash

# What works (35%)
* Driver registration
* Driver calling from userspace application
* Logging
* ISO15765

# What doesn't work (65%)
* ISO1941
* CAN

# Requirments
* Works on Win7+ (x86 and x64) - Windows XP may work, but untested
* Follow the guide [here](https://docs.macchina.cc/m2-docs/arduino) to get Arduino IDE setup to receive the arduino sketch
* Install Visual studio with C++ support in order to build the DLL


# Usage
1. Run installer/install.bat
2. Compile the driver module, copy the compiled dll to C:\Program Files (x86)\macchina\passthru\
3. Open the macchina directory in arduino IDE and upload to M2 UTD
4. IMPORTANT: Set the Macchina M2 as COM12 in device manager! (Currently its hard coded in the dll but i have plans to change that)
5. Select "Macchina-Passthru" as your J2534 device

# Logging
Log file is located at C:\Program Files (x86)\macchina\passthru\activity.log

It is suggested for now to use WSL to tail the log file to get live data
