# jstream

### (c) 2023 by Paolo Caressa <github.com/pcaressa/jstream>

## What?

jstream is yet another Json parser. Actually, the Json parsed by jstream is not standard, since there's no Unicode support, just ASCII: it was enough for the application I needed, to port it to support Unicode would essentially mean to use `wchar` instead of `char`.

## Why?

I wrote it in need of a parser that consumes characters from a stream (a text file, a http channel etc.).

## How?

To use it in your code, just include the `jstream.h` file, that exports two data types and three functions:

- type `jstream_t`, an alias for `unsigned*`
- type `struct jstream_param_s`, the data type used to pass and receive parameters to and from the function `jstream`
- function `jstream` that scans a json value from the stream and returns it into a memory block whose address is also returned as value.
- function `jstream_dump` that prints the content of a memory block produced by `jstream` to a text file in Json format.
- function `jstream_skip` used to scan the memory area where the parsed Hson has been stored.

You need to define a function `int get(void)` that, each time it is called, consumes a character in the stream and returns it (or a negative value to denote an error or the end of the stream), and assign its address to the `p->get` tag of a `struct jstream_param_s` variable `p`.

After `jstream` has been called, you'll find its return values inside the `p` structure: the general scheme is

    struct jstream_param_s p;
    p.get = your_get_function;
    jstream(&p);
    if (p.error) {
        printf("Error #%i\n", p.error);
    } else {
        printf("Address of memory block: %p\n", p.obj);
        printf("Number of items in the memory block: %u\n", p.size);
        printf("Last character scanned from the stream: %i\n", p.clast);
    }


The sequence of characters returned by the `get` function is taken by `jstream` to represent a Json value; `jstream` converts it into a bynary format in an array of unsigneds, whose 0-th item denotes the type of value, which is enumerated in the jstream.h file:

- NULL (0) for `null`
- TRUE (1) for `true`
- FALSE (2) for `false`
- NUMBER (3) for number
- STRING (4) for string
- ARRAY (5) for array
- OBJECT (6) for object

After that it follows:

- If NULL, TRUE or FALSE, nothing.
- If NUMBER, a double.
- If STRING, a C-string (`'\0'`-terminated).
- If ARRAY, an unsigned n (the number of elements) followed by n values.
- If OBJECT, an unsigned n (the number of elements) followed by n pairs of values.

The `jstream_skip` function skips the current value (if it is an array or an object skip all of it).

For an example, look at the file `jsondump.c` that uses `fgetc` as `get` and prints the result on the terminal (thus implements an echo for Json texts that drops space characters) to see how to use it in practice.

Enjoy,
Paolo
