# jstream

### (c) 2023 by Paolo Caressa <github.com/pcaressa/jstream>

## What?

jstream is yet another Json parser. Actually, the Json parsed by jstream is not standard, since there's no Unicode support, just ASCII: it was enough for the application I needed, to port it to support Unicode would essentially mean to use `wchar` instead of `char`.

## Why?

I wrote it in need of a parsed that consume characters from a stream (a text file, a http channel etc.).

# How?

To use it in your code just include the `jstream.h` file, that exports two data types and two functions:

    - type `jstream_t`, an alias for `unsigned*`
    - type `struct jstream_param_s`, the data type used to pass and receive parameters to and from the function `jstream`
    - function`jstream` that scans a json value from the stream and returns it into a memory block whose address is also returned as value.
    - function `jstream_dump` that prints the content of a memory block produced by `jstream` to a text file in Json format.

You need to define a function `int get(void)` that each time it is called consumes a character in the stream and returns it (or a negative value to denote an error or the end of the stream).

Look at the file `jsondump.c` that uses `fgetc` as `get` and prints the result on the terminal (thus implements an echo for Json texts that drops space characters) to see how to use it in practice.

Enjoy,
Paolo