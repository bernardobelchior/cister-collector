# Cister Collector


# Run

In the terminal window, go to the cister-collector folder.   <br /> 
Run the following line:

```
make TARGET=sky savetarget NODEID=0x0001 MOTES=/dev/ttyUSB0 cister-collector.upload login
```
* `TARGET=sky` The target plattform. We are going to use the Tmote Sky
* `savetarget` Saves the target for any future compilations
* `NODEID=0x0001` Node id is set to 1
* `MOTES=/dev/ttyUSB0`  Used to pass tty-devices on which to apply the action
* `cister-collector.upload` This will upload the code on the Tmote Sky
* `login` This will enable us to view the output. If permission error occurs, use `sudo -s` command at the beginning

