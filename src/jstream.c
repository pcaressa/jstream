/** \file jstream.c */

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
    is returned.

    \section json_grammar Grammar accepted

    JSON Grammar accepted by the parser (borrowed from
    <https://www.json.org/>.

    \warning This version assumes that streams
        emit ASCII and not Unicode characters.

\verbatim
    json
        element

    value
        object
        array
        string
        number
        "true"
        "false"
        "null"

    object
        '{' ws '}'
        '{' members '}'

    members
        member
        member ',' members

    member
        ws string ws ':' element

    array
        '[' ws ']'
        '[' elements ']'

    elements
        element
        element ',' elements

    element
        ws value ws

    string
        '"' characters '"'

    characters
        ""
        character characters

    character
        '20' . 'FF' - '"' - '\'
        '\' escape

    escape
        '"'
        '\'
        '/'
        'b'
        'f'
        'n'
        'r'
        't'
        'u' hex hex hex hex

    hex
        digit
        'A' . 'F'
        'a' . 'f'

    number
        integer fraction exponent

    integer
        digit
        onenine digits
        '-' digit
        '-' onenine digits

    digits
        digit
        digit digits

    digit
        '0'
        onenine

    onenine
        '1' . '9'

    fraction
        ""
        '.' digits

    exponent
        ""
        'E' sign digits
        'e' sign digits

    sign
        ""
        '+'
        '-'

    ws
        ""
        '20' ws
        '0A' ws
        '0D' ws
        '09' ws
\endverbatim

*/

/* *** MODULE jstream *** */
            
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jstream.h"

/* *** PRIVATE STUFF *** */

/** If size is multiple of sizeof(unsigned) then return size
    divided by sizeof(unsigned), else return size + 1 divided
    by sizeof(unsigned). */
static unsigned jstream_align(unsigned size)
{
    return (size % sizeof(unsigned) == 0) ? size / sizeof(unsigned)
        : 1 + size / sizeof(unsigned);
}

/** Expand the size of p->obj by n unsigned: the new value
    of p->obj is updated and the address of the first
    allocated additional array is returned. */
static jstream_t jstream_expand(jstream_param_t p, unsigned n)
{
    if (p->obj == NULL) {
        p->obj = malloc(n * sizeof(unsigned));
        if (p->obj == NULL) longjmp(p->env, ERR_MEMORY);
        p->size = n;
        return p->obj;
    }
    unsigned newsize = p->size + n;
    jstream_t objnew = realloc(p->obj, newsize * sizeof(unsigned));
    if (objnew == NULL) longjmp(p->env, ERR_MEMORY);
    p->obj = objnew;
    objnew += p->size;   // points to the new items
    p->size = newsize;
    return objnew;
}

/** Return the first non space character in the stream. */
static int jstream_next(jstream_param_t p)
{
    while (strchr(" \r\t\n", p->clast = p->get()) != NULL)
        ;
    return p->clast;
}

/** Each function implementing a grammar class assumes
    that the first character of the sequence to match
    has already been parsed (it is found in p->clast).
    Also, each function consume the first character
    after the parsed value and stores it into p->clast. */

// Forward declarations
static int jstream_object(jstream_param_t p);
static int jstream_value(jstream_param_t p);

static int jstream_array(jstream_param_t p)
{
    jstream_t objnew = jstream_expand(p, 2);
    objnew[0] = ARRAY;
    objnew[1] = 0;
    // ilen is the index of the length of this object
    size_t ilen = objnew + 1 - p->obj;
    if (jstream_next(p) != ']') {
        for (;;) {
            ++ p->obj[ilen];
            if (jstream_value(p) != ',') break;
            jstream_next(p);    // jstream_value expect this
        }
        if (p->clast != ']') longjmp(p->env, ERR_CLOSED_BRACKET);
    }
    return jstream_next(p);
}

static int jstream_false(jstream_param_t p)
{
    int c;
    if (p->get() != 'a' || p->get() != 'l' || p->get() != 's' || p->get() != 'e'
    || strchr(" \r\t\n]},:", p->clast = p->get()) == NULL)
        longjmp(p->env, ERR_FALSE);
    jstream_t objnew = jstream_expand(p, 1);
    objnew[0] = FALSE;
    return strchr(" \r\t\n", p->clast) == NULL ? p->clast
        : jstream_next(p);
}

static int jstream_null(jstream_param_t p)
{
    int c;
    if (p->get() != 'u' || p->get() != 'l' || p->get() != 'l'
    || strchr(" \r\t\n]},:", p->clast = p->get()) == NULL)
        longjmp(p->env, ERR_NULL);
    jstream_t objnew = jstream_expand(p, 1);
    objnew[0] = (unsigned) NULL;
    return strchr(" \r\t\n", p->clast) == NULL ? p->clast
        : jstream_next(p);
}

static int jstream_number(jstream_param_t p)
{
    int i;
    static char buf[128];
    buf[0] = p->clast;
    for (i = 1; i < sizeof(buf); ++ i) {
        buf[i] = p->get();
        if (strchr("0123456789.+-eE", buf[i]) == NULL) {
            p->clast = buf[i];
            buf[i] = '\0';
            break;
        }
    }
    if (i == sizeof(buf)) longjmp(p->env, ERR_NUMBER_TOO_LONG);
    char *s;
    double d = strtod(buf, &s);
    if (s != buf + strlen(buf)) longjmp(p->env, ERR_NUMBER);
    jstream_t objnew = jstream_expand(p, 1 + sizeof(double)/sizeof(unsigned));
    objnew[0] = NUMBER;
    *(double*)(objnew + 1) = d;
    return strchr(" \n\t\r", p->clast) == NULL ? p->clast
        : jstream_next(p);
}

static int jstream_object(jstream_param_t p)
{
    jstream_t objnew = jstream_expand(p, 2);
    objnew[0] = OBJECT;
    objnew[1] = 0;
    // ilen is the index of the length of this object
    size_t ilen = objnew + 1 - p->obj;
    if (jstream_next(p) != '}') {
        for (;;) {
            ++ p->obj[ilen];
            if (jstream_value(p) != ':') longjmp(p->env, ERR_COLON);
            jstream_next(p);    // jstream_value expect this
            if (jstream_value(p) == '}') break;
            if (p->clast != ',') longjmp(p->env, ERR_COMMA);
            jstream_next(p);    // jstream_value expect this
        }
    }
    return jstream_next(p);
}

static int jstream_string(jstream_param_t p)
{
#   define BUFNUM 16
    static char buf[BUFNUM*sizeof(unsigned)];
    jstream_t objnew = jstream_expand(p, 1);
    objnew[0] = STRING;
    // Scans the string until '"' or the text is over
    int j;
    for (;;) {
        for (j = 0; j < sizeof(buf) && (p->clast = p->get()) != '"'; ++ j) {
            if (p->clast < 0) longjmp(p->env, ERR_EOS_INSIDE_STRING);
            buf[j] = p->clast;
        }
        if (j < sizeof(buf)) break;
        /* The scanned string is longer than buf: dump buf and
            reset it to scan the rest of the string. */
        objnew = jstream_expand(p, BUFNUM);
        memcpy(objnew, buf, sizeof(buf));
    }
    buf[j] = '\0';  // overwrites the ending '"'
    objnew = jstream_expand(p, jstream_align(strlen(buf) + 1));
    strcpy((char*)objnew, buf);
    return jstream_next(p);
}

static int jstream_true(jstream_param_t p)
{
    int c;
    if (p->get() != 'r' || p->get() != 'u' || p->get() != 'e'
    || strchr(" \r\t\n]},:", p->clast = p->get()) == NULL)
            longjmp(p->env, ERR_TRUE);
    jstream_t objnew = jstream_expand(p, 1);
    objnew[0] = TRUE;
    return strchr(" \r\t\n", p->clast) == NULL ? p->clast
        : jstream_next(p);
}

static int jstream_value(jstream_param_t p)
{
    switch (p->clast) {
        case '[': return jstream_array(p);
        case '{': return jstream_object(p);
        case '"': return jstream_string(p);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case '-': return jstream_number(p);
        case 'f': return jstream_false(p);
        case 'n': return jstream_null(p);
        case 't': return jstream_true(p);
    }
    longjmp(p->env, ERR_VALUE);
}

/* *** PUBLIC STUFF *** */

jstream_t jstream(jstream_param_t p)
{
    int e;
    p->obj = NULL;
    p->size = 0;
    p->clast = -1;
    if ((p->error = setjmp(p->env)) == ERR_NONE) {
        jstream_next(p);    // jstream_value expect this
        jstream_value(p);
        return p->obj;
    }
    if (p->obj != NULL) {
        free(p->obj);
        p->obj = NULL;
    }
    return NULL;
}

jstream_t jstream_dump(FILE *f, jstream_t obj)
{
    switch (obj[0]) {
    case 0 /* NULL */:
        fputs("null", f);
        return obj + 1;
    case FALSE:
        fputs("false", f);
        return obj + 1;
    case TRUE:
        fputs("true", f);
        return obj + 1;
    case NUMBER:
        fprintf(f, "%g", *(double*)(obj + 1));
        return obj + 1 + sizeof(double)/sizeof(unsigned);
    case STRING: {
        fputc('"', f); fputs((char*)(obj + 1), f); fputc('"', f);
        return obj + 1 + jstream_align(strlen((char*)(obj + 1)) + 1);   // + 1 for the '\0'
    }
    case ARRAY: {
        fputc('[', f);
        unsigned len = obj[1];
        jstream_t next = obj + 2;
        for (unsigned i = 0; i < len; ++ i) {
            next = jstream_dump(f, next);
            if (i + 1 < len) fputc(',', f);
        }
        fputc(']', f);
        return next;
    }
    case OBJECT: {
        fputc('{', f);
        unsigned len = obj[1];
        jstream_t next = obj + 2;
        for (unsigned i = 0; i < len; ++ i) {
            next = jstream_dump(f, next);
            fputc(':', f);
            next = jstream_dump(f, next);
            if (i + 1 < len) fputc(',', f);
        }
        fputc('}', f);
        return next;
    }
    default:
        fprintf(f, "Invalid object code: %u at %p\n", obj[0], obj);
    }
    return NULL;
}

jstream_t jstream_skip(jstream_t obj)
{
    switch (obj[0]) {
        case 0 /* NULL */: case FALSE: case TRUE:
            return obj + 1;
        case NUMBER:
            return obj + 1 + sizeof(double)/sizeof(unsigned);
        case STRING:
            return obj + 1 + jstream_align(strlen((char*)(obj + 1)) + 1);
        case ARRAY: {
            jstream_t next = obj + 2;
            for (unsigned i = 0; i < obj[1]; ++ i) {
                next = jstream_skip(next);
            }
            return next;
        }
        case OBJECT: {
            jstream_t next = obj + 2;
            for (unsigned i = 0; i < obj[1]; ++ i) {
                next = jstream_skip(jstream_skip(next));
            }
            return next;
        }
    }
    return NULL;
}
