/** \file jstream.h */

#ifndef JSTREAM_INC
#define JSTREAM_INC

#include <setjmp.h>
#include <stdio.h>

/** \mainpage

    \section json_data Json values in the data object

    An input text is taken to represent a Json value by the
    jstream function, that converts it into a bynary format
    in an array of unsigneds, whose 0-th item denotes the
    type of value, according to the following encoding:
        - 0 = null
        - 1 = true
        - 2 = false
        - 3 = number
        - 4 = string
        - 5 = array
        - 6 = object

    After that it follows:
        - If code == 0 or == 1 or == 2, nothing.
        - if code == 3, a double.
        - if code == 4, a C-string.
        - If code == 5, an unsigned n (the number of elements)
            followed by n values.
        - If code == 6, an unsigned n (the number of elements)
            followed by n pairs of values.
    
    When parsing a stream, the string representing the value
    contained in the Json grows to host new data. If an
    allocation error occurs, all is freed and an error code
    is returned. */

/** Value codes (null is already represented by NULL). */
enum {
    TRUE = 1,
    FALSE = 2,
    NUMBER = 3,
    STRING = 4,
    ARRAY = 5,
    OBJECT = 6
};

/** Error codes: they are returned in the referenced
    parameter err by the jstream function. */
enum {
    ERR_NONE,
    ERR_MEMORY,
    ERR_VALUE,
    ERR_NULL,
    ERR_FALSE,
    ERR_TRUE,
    ERR_NUMBER,
    ERR_NUMBER_TOO_LONG,
    ERR_EOS_INSIDE_STRING,
    ERR_COMMA,
    ERR_COLON,
    ERR_CLOSED_BRACKET,
};

/** A jstream is an array of unsigned numbers: strings and
    doubles are casted (eg if sizeof(unsigned) == 4 then
    a string of n characters takes (n+1) / 4 items (+1 because
    of the ending '\0'), while a double takes 2 items, etc). */
typedef unsigned *jstream_t;

/** Structure used to represent in bytes parsed values from
    a stream. */
typedef struct jstream_param_s {
// public
    int error;          ///< error code (0 means no error)
    jstream_t obj;      ///< object under construction
    unsigned size;      ///< number of words (=unsigned) allocated
    int (*get)(void);   ///< function that scan the next character
    int clast;          ///< last scanned character
// private
    jmp_buf env;        ///< environment used by exceptions
} *jstream_param_t;

/** Parse a json stream: it the get field of the structure *p
    to be assigne to a function (pointer) returning the next
    character read from the stream, or a negative value in case
    of end or error.
    Return the following values inside the structure *p:
        obj = pointer to the memory area containing data
        size = length of such area in bytes
        error = error code (0 meaning "no error")
        clast = last item got from the stream and that
                follows the parsed json value.
    The address obj is also returned as value from the
    function (in case of error it is NULL and no object is
    allocated).
    Warning: it is the caller responsibility to deallocate
    p->obj once it is no longer needed, via free(p->obj). */
extern jstream_t jstream(jstream_param_t p);

/** Dump an jstream_t object to a text file. Return the
    address of the first item following the object in the
    array obj. */
extern jstream_t jstream_dump(FILE *f, jstream_t obj);

/** Given the address of a Json value s dumped by jstream, return
    the address of the value immediately following it. If the code
    at obj[0] is not valid, return NULL. */
extern jstream_t jstream_skip(jstream_t obj);

#endif
