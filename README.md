# R@ional
I made a multiprecision arithmetic class for the challenge of it. It is a tool for approximating π and other (often irrational) numbers using multiprecision integers.
## Build
This may be built using g++ with the following command:  
```g++ -std=c++23 ./rtn.cpp -o ./rtn```  
This has been tested on Debian Linux but since only the C++ standard library is used it should build on most platforms.
## Usage
You may use R@ional by:  
```
./rtn [flags]
```
where flags could be:  
```-A```: Algorithm  
```-A e```: Complex exponential function exp([a+b·i]/c) (followed by integers a, b and c separated by space)  
```-A n```: Nilakantha's series  
```-A w```: Walli's series  
```-a, --all```: output all to screen (and file), otherwise only first and last  
```-b, --base```: set number base of printed output (default value: 2*5)  
```-h, --help```: help  
```-i, --iterations```: iterations of algorithm (in decimal, default value: 1)  
```-f, --filename```: output file name  

## Examples
Approximate π using the Nilakantha series and show all intermediate iterations:  
```
./rtn -A n -i 20 -a
```  
Approximate π using the Wallis product and save to file this_pi:  
```
./rtn -A n -i 20 -f this_pi
```  
Approximate $e^{-22i/7}$ ($\approx e^{-\pi}$) using the complex exponential function:  
```
./rtn -A e 0 -22 7 -i 20
```  
Compute 40 more iteration of existing file and display the output in binary:  
```
./rtn -f file -i 40 -b 2
```  
Display the most recent value for the given file without further computation:  
```
./rtn -f file -i 0
```  
Notice that the algorithm flag is optional for existing files, as the algorithm will be determined from the file.

## File system
When final and/or intermediate results are saved to file it is done so in entries. An entry encodes an unsigned integer by saving the size of the integer in a [utf-8 format](https://en.wikipedia.org/w/index.php?title=UTF-8&oldid=1366559328#Description) (1-4 byte(s)) where the order is from "Byte 1" to "Byte 4" (see link). This is followed by the actual integer starting with the least significant byte first (little endian). Each file starts out with an entry with the letter 'v' for version then a version number entry and finally an entry indicating the algorithm. The structure of subsequent entries depend on the algorithm.