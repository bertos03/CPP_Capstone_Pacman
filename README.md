
# Pacman

A kind of a pacman style game - for the final assignment ("capstone") in the Udacity C++ nanodegree :)

## Features

- The map can be configured to the user's preferences (see './data/map.txt). You can add 1 (P)acman and multiple (M)onsters and (G)oodies. The map itself is constructed by paths (.) and walls (x). Additionally, you can for example adjust the speed of pacman and the monsters.
- Yor are free to design a map in the dimensions you like. The element size will adapt accordingly.
- The map has basic checks for integrity and will fix some bugs automatically.
- Sound support is given as long as the system supports that (enable it by uncommenting 'AUDIO' in './src/definitions.h').
- The goal of the game is to collect all goodies and not getting caught by the monsters.



## Installation

The whole project uses cmake/make. All necessary files for finding the SDL libraries are located in the 'cmake' subdirectory.
(Works fine on Mac and Udacity workspace)

The following C++ frameworks were used and have to be installed to make the program compile correctly:
- SDL2
- SDL2_ttf
- SDL2_image
- SDL2_audio (which will not run Udacity Workspace)

To build the project, simply run

```bash
cd build
cmake ..
make 
./pacman
```
It is possible to make adjustments in 'src/definitions.h', such as defining the macro AUDIO for systems where audio is supported.

## Details

### File structure
```bash
data/  -> directory with all audio/image/map data
cmake/ -> directory with all necessary functions to locate the SDL libraries
src/main.cpp        -> main functions
src/definitions.h   -> basic pre-compilation settings
src/audio.cpp       -> audio functions
src/events.cpp      -> for evaluating keyboard events
src/game.cpp        -> the game logic
src/globaltypes.cpp -> some globally used datatypes
src/goodie.cpp      -> the class definition for goodie objects
src/map.cpp         -> the class definition for map object
src/mapelement.cpp  -> the base class for goodie/monster/pacman
src/monster.cpp     -> the class definition for monster objects
src/pacman.cpp      -> the class definition for pacman object
src/renderer.cpp    -> the class definition for the renderer
```

### Udacity criteria
#### Loops, Functions, I/O
|(Y)es/(N)o|Task|
|--|--|
|Y|A variety of control structures are used in the project.|
|Y|The project code is clearly organized into functions.|
|Y|The project reads data from an external file or writes data to a file as part of the necessary operation of the program.|
|Y|The project accepts input from a user as part of the necessary operation of the program.|

#### Object Oriented Programming
|(Y)es/(N)o|Task|
|--|--|
|Y|The project code is organized into classes with class attributes to hold the data, and class methods to perform tasks.|
|Y|All class data members are explicitly specified as public, protected, or private.|
|Y|All class members that are set to argument values are initialized through member initialization lists.|
|TODO|All class member functions document their effects, either through function names, comments, or formal documentation. Member functions do not change program state in undocumented ways.|
|Y|Appropriate data and functions are grouped into classes. Member data that is subject to an invariant is hidden from the user. State is accessed via member functions.|
|Y|Inheritance hierarchies are logical. Composition is used instead of inheritance when appropriate. Abstract classes are composed of pure virtual functions. Override functions are specified.|
|N|One function is overloaded with different signatures for the same function name.|
|N|One member function in an inherited class overrides a virtual base class member function.|
|N|One function is declared with a template that allows it to accept a generic parameter.|

#### Memory Management
|(Y)es/(N)o|Task|
|--|--|
|Y|At least two variables are defined as references, or two functions use pass-by-reference in the project code.|
|Y|At least one class that uses unmanaged dynamically allocated memory, along with any class that otherwise needs to modify state upon the termination of an object, uses a destructor.|
|Y|The project follows the Resource Acquisition Is Initialization pattern where appropriate, by allocating objects at compile-time, initializing objects when they are declared, and utilizing scope to ensure their automatic destruction.|
|n.r.|For all classes, if any one of the copy constructor, copy assignment operator, move constructor, move assignment operator, and destructor are defined, then all of these functions are defined.|
|n.r.|For classes with move constructors, the project returns objects of that class by value, and relies on the move constructor, instead of copying the object.|
|Y|The project uses at least one smart pointer: unique_ptr, shared_ptr, or weak_ptr. The project does not use raw pointers.|

Annotation: No need for creating copy/move constructor or copy/move assignment operator.

#### Concurrency
|(Y)es/(N)o|Task|
|--|--|
|Y|roject uses multiple threads in the execution.|
|N|A promise and future is used to pass data from a worker thread to a parent thread in the project code.|
|Y|A mutex or lock (e.g. std::lock_guard or `std::unique_lock) is used to protect data that is shared across multiple threads in the project code.|
|N|A std::condition_variable is used in the project code to synchronize thread execution.|
