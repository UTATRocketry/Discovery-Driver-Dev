# Discovery-Driver-Dev
Driver development repo for Discovery avionics system.


## Run Cam Driver
RunCam Device Protocol specification:
http://note.youdao.com/groupshare/?token=9AD3F89F0B92488E8241F58CAEDF7939&gid=29699666
https://support.runcam.com/hc/en-us/articles/360014537794-RunCam-Device-Protocol

USART1 Settings:
    Mode: Asynchronous
    Baud Rate: 115200
    Word Length: 8 Bits
    Parity: None
    Stop Bits: 1
    Mode: TX/RX
    Hardware Flow Control: None

### Set Up:
- Create a project in CubeIDE using the .ioc file
- Add the RunCam_Driver directory to the Drivers directory
- Add Example code from the example_main.c to the main.c file CubeIDE generates

