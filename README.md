# sockets-lumify

Simple sockets interface to use in C++

Originally written for the [Lumify](https://github.com/mikaelmello/lumify) project and now has its own repository to be improved.

### How to use

* Copy *only* the `sockets-lumify` folder to your project, the one with the `.hpp` and `.cpp` files.
* Include it in your program using `#include "sockets.hpp"`
* When compiling, be sure to add the path to the `sockets-lumify` folder in the `-I` flag.

Example, suppose you have this project structure:

```
├── includes
│   ├── myclass.hpp
│   └── sockets-lumify
│       ├── sockets.cpp
│       └── sockets.hpp
├── source
│   └── myclass.cpp
└── main.cpp
```

When compiling `main.cpp`, you will have the following command:

`g++ -std=c++11 -I"includes/" -I"includes/sockets-lumify" main.cpp -o main`

You can make it easier by writing a Makefile, such as the one in the `examples` folder.

This is obviously not the best way, recommendations to improve this part are welcome.

### Examples

Go to the `examples` folder and execute `make tcp` to create a client and a server executables to see the library in action.