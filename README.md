μSIP
====

SIP message parsing and encoding library.

Features:

- No malloc/free is used (for embedded systems).
- Small size.
- Command line utility.

## C library (#include <usip.h>)

The library doesn't use malloc or free whilst parsing, so it's safe
to use in embedded systems. You need to provide a buffer to store
the parsed data.

    void usip_start(struct usip *o, void b[], size_t bsz);
    
After receiving data, parse it:

    struct usip *
    usip_forward(struct usip *usip, size_t shift, int parse);
    
The `shift` parameter is the number of bytes received. The `parse`
parameter is a boolean, if true, the data is parsed, otherwise only
is stored.

If the message is complete, the `usip_forward` function returns the
`usip` pointer, otherwise returns NULL.

You can serialize the message using the `usip_serialize` function.

    bool
    usip_serialize(struct usip *_usip, char **o_buff, size_t *o_len);

The first line of the message is stored in the `first`, `second` and
`rest` fields. The `content` field stores the message body. The
`parameter` field stores the parameters.

    void usip_set_first    (struct usip *usip, char const first[]);
    void usip_set_second   (struct usip *usip, char const second[]);
    void usip_set_rest     (struct usip *usip, char const rest[]);
    void usip_set_content  (struct usip *usip, void *buff, size_t len);
    bool usip_set_parameter(struct usip *usip, char const var[], char val[]);
    
    char const *usip_get_first  (struct usip *_usip);
    char const *usip_get_second (struct usip *_usip);
    char const *usip_get_rest   (struct usip *_usip);
    bool        usip_get_content  (struct usip *usip, void **buff, size_t *len);
    char const *usip_get_parameter(struct usip *usip, char const var[]);

If you modify the structure manually, call `void usip_touch(struct usip *` so
that the serialization works properly.
    
## Command line utility

The command line utility can be used convined with uuri(1) and nc(1)
to create small SIP protocol scripts.

    Usage: usip [ARGS]
    
    -b BUFLEN : Set length of buffer.
    -l        : Read a message line by line from the standard input.
    -r        : Read a message from the standard input.
    -p        : Read content from the standard input, until EOF.
    
    -a PARAM:VALUE             : Append/set parameter.
    -i PARAM:VALUE             : Insert parameter.
    -1 FIRST -2 SECOND -3 REST : Set first line.
    
    -G PARAM        : Print parsed parameter.
    -M              : Print parsed SIP/HTTP message.
    -P              : Print parsed content.

An example [here](./test.sh).

## Collaborating

For making bug reports, feature requests and donations visit
one of the following links:

1. [gemini://harkadev.com/oss/](gemini://harkadev.com/oss/)
2. [https://harkadev.com/oss/](https://harkadev.com/oss/)








