# Dvorak overlayout switcher
Can switch layouts by Alt(Left) + Shift(Left) + 2(or 3).  
Can switch to dvorak(without shortcuts) by Alt(Left) + Shift(Left) + 1.  
If you change layout by other(system) shortcut - grabbing to dvorak will be disabled.  
Only for Linux and, maybe, other Unix-based OS!  

For compile, you may need ```sudo apt install libx11-dev```.  
Change in dvorak_overlayout_switcher.c your layouts.  
Save dvorak_overlayout_switcher.c to ~/bin or /usr/bin 
Compile by ```gcc dvorak_overlayout_switcher.c -o dvorak_overlayout_switcher -O2 -lX11```.  
Add execute permissions to result file.  
Add result file to startup applications.  
For enable detecting of layout changes - add argument to run command: ```dvorak_overlayout_switcher enable```.