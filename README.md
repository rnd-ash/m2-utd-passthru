# m2-utd-passthru
Passthru API (j2534) Driver for Macchina M2 Under the dash

# What works (5%)
* Driver registration
* Driver calling from userspace application
* Logging

# What doesn't work (95%)
* ISO1941
* CAN
* ISO15765

# Usage
1. Run installer/install.bat
2. Compile the driver module, copy the compiled dll to C:\Program Files (x86)\macchina\passthru\
3. Open the macchina directory in arduino IDE and upload to M2 UTD
4. Select "Macchina-Passthru" as your J2534 device
