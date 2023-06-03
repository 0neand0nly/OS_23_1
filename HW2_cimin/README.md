# 2023OS_deltaDebugging
This Program is a simple Delta Debugging algorithm that was assigned for OS Homework2. 
The user interface is composed of four options which are -i, -m, -o, and executable binary file(target program). The additional options are supported after the executable binary file with the – command. 


-i : Crashing input file_path

-m: Error message that will be compared with the returned error message from the target program

-o: Output file_path where the reduced input will be saved

executable binary(target program): The target program that will be executed with the crashing input.

There are three examples are provided with the program. It is balance, jsmn, and libxml2 each of them can be built with executing the build.sh in their directory. 

Once you are done with building the program. You can compile cimin into an executable program by running the Makefile. This can be done by typing in make in the terminal. It will automatically run all three cases and save its results except for the last one. You will need to stop the program by pressing CTRL + C.


