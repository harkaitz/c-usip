#include "usip.h"
#include <string.h>
#include <stdio.h>
#include <strings.h>

#define UESC_SPACES         " \t\r\n"
#define UESC_REPLACE_FS2SPC "\t \r \n "


static inline
char *uesc_replace(char _s[], char const _from_to[])
{
	if(!_s) {
		return NULL;
	}
	for (char *c=_s; *c; c++) {
		for(char const *f=_from_to; *f; f+=2) {
			if(*c==*f) {
				*c = *(f+1);
			}
		}
	}
	return _s;
}

static inline
char *uesc_trim(char _s[],char const _espaces[])
{
	char *c;
	if(!_s) {
		return NULL;
	}
	for(c=_s; *c && strchr(_espaces,*c); c++) {}
	return c;
}

void
usip_start(struct usip *_usip, void *_buffer, size_t _length)
{
	memset(_usip, 0, sizeof(*_usip));
	_usip->state.wait_word = 1;
	_usip->state.field = USIP_READING_FIRST;
	_usip->data.buffer = _buffer;
	_usip->data.pos    = 0;
	_usip->data.length = _length;
}

void
usip_touch(struct usip *_usip)
{
	_usip->serial.serialized = NULL;
	_usip->serial.size = 0;
}

struct usip_kv *
usip_empty_parameter(struct usip *_usip)
{
	for(int i=0;i<USIP_MAX_PARAMETERS;i++) {
		if(!_usip->pars[i].var)
			return &_usip->pars[i];
	}
	return NULL;
}

bool
usip_serialize(struct usip *_usip, char **o_buff, size_t *o_len)
{
	if(_usip->serial.size == 0 || _usip->serial.serialized == NULL) {
		size_t  l = 0;
		char *b = _usip->data.buffer+_usip->data.pos;
		size_t  m = _usip->data.length-_usip->data.pos;
		l += snprintf(
		    b+l,m-l,
		    "%s %s %s\r\n",
		    (_usip->first) ? _usip->first  : "NONE1",
		    (_usip->second)? _usip->second : "NONE2",
		    (_usip->rest)  ? _usip->rest   : "NONE3"
		);
		if(l>m) {
			goto message_too_long;
		}
		for(int i=0; i<USIP_MAX_PARAMETERS; i++) {
			if(_usip->pars[i].var && _usip->pars[i].val) {
				if(strcasecmp(_usip->pars[i].var, "Content-Length")) {
					l += snprintf(
					    b+l,m-l,
					    "%-15s: %s\r\n",
					    _usip->pars[i].var,
					    _usip->pars[i].val
					);
					if(l>m) {
						goto message_too_long;
					}
				}
			}
		}
		l += snprintf(
		    b+l,m-l,
		    "%-15s: %li\r\n\r\n",
		    "Content-Length", _usip->content_length
		);
		if(l>m) {
			goto message_too_long;
		}
		if((m-l)<_usip->content_length) {
			goto message_too_long;
		}
		memcpy(b+l, _usip->content, _usip->content_length);
		l += _usip->content_length;
		_usip->serial.size       = l;
		_usip->serial.serialized = b;
	}
	if(o_buff) {
		*o_buff = _usip->serial.serialized;
	}
	if(o_len) {
		*o_len  = _usip->serial.size;
	}
	return true;
 message_too_long:
	return false;
}

struct usip *
usip_forward(struct usip *usip, size_t shift, int parse)
{
     if(usip->state.field == USIP_COMPLETE) { return usip; }
     if(!parse) {
          usip->data.pos += shift;
          return usip;
     }
     usip_touch(usip);
     size_t p = usip->data.pos;
     for(size_t i=0;(i<shift) && (p<usip->data.length);i++,p++) {
          char *ptr = &usip->data.buffer[p];
          char  chr = *ptr;
          if(usip->state.field == USIP_READING_CONTENT) {
               if((usip->content + usip->content_length)<=ptr) {
                    usip->state.field = USIP_COMPLETE;
                    break;
               }
          } else {
               /* Skip if waiting for a word. */
               if(usip->state.wait_word) {
                    if(strchr(" \t",chr)) goto next;
                    usip->state.wait_word = 0;
               }
               /* Skip if waiting a newline. */
               if(usip->state.wait_nl) {
                    if(chr == '\n') {
                         *ptr = '\0';
                         usip->state.wait_nl    = 0;
                         usip->state.wait_space = 0;
                    }
                    goto next;
               }
               /* Double line case. */
               if(usip->state.prev_was_nl && (chr == '\n')) {
                    if(usip->content_length) {
                         usip->state.field = USIP_READING_CONTENT;
                    } else {
                         usip->state.field = USIP_COMPLETE;
                    }
                    if(!usip->content) {
                         usip->content = ptr+1;
                    }
                    goto next;
               }
               /* Wait spaces. */
               if(usip->state.wait_space) {
                    if(strchr("\t ",chr)) {
                         *ptr = '\0';
                         usip->state.wait_space = 0;
                    } else {
                         goto next;
                    }
               }
               /* Get fields. */
               if(usip->state.field == USIP_READING_FIRST) {
                    if(!usip->first) {
                         usip->first  = ptr;
                         usip->state.wait_space = 1;
                    } else {
                         usip->state.field = USIP_READING_SECOND;
                         usip->state.wait_word = 1;
                    }
               } else if(usip->state.field == USIP_READING_SECOND) {
                    if(!usip->second) {
                         usip->second = ptr;
                         usip->state.wait_space = 1;
                    } else {
                         usip->state.field = USIP_READING_THIRD;
                         usip->state.wait_word = 1;
                    }
               } else if(usip->state.field == USIP_READING_THIRD) {
                    if(!usip->rest) {
                         usip->rest = ptr;
                         usip->state.wait_nl = 1;
                         usip->state.field = USIP_READING_PARAMS;
                    }
               } else if(usip->state.field == USIP_READING_PARAMS) {
                    if(!usip->state.param) {
                         usip->state.param = usip_empty_parameter(usip);
                         if(!usip->state.param) goto next;
                    }
                    if(usip->state.param->var && usip->state.param->val) {
                         if(chr == '\n') {
                              *ptr = '\0';
                              usip->state.param->val = uesc_trim(usip->state.param->val,UESC_SPACES);
                              usip->state.param->val = uesc_replace(usip->state.param->val,UESC_REPLACE_FS2SPC);
                              if(!strcasecmp(usip->state.param->var,"Content-Length")) {
                                   usip->content_length = atoi(usip->state.param->val);
                                   usip->state.param->var = NULL;
                                   usip->state.param->val = NULL;
                              } else {
                                   usip->state.param = NULL;
                              }
                         }
                    } else if(usip->state.param->var && (!usip->state.param->val)) {
                         if(strchr(" \t",chr)) {
                              *ptr = '\0';
                         } else if(chr == ':') {
                              *ptr = '\0';
                              usip->state.param->val = ptr+1;
                         }
                    } else if(!usip->state.param->var) {
                         if(usip->state.prev_was_nl) {
                              if(strchr("\t ",chr)) {
                                   *(ptr-1) = ' ';
                              } else {
                                   *(ptr-1) = '\0';
                                   usip->state.param->var = ptr;
                              }
                         }
                    }
               }
          next:
               if(chr == '\n') {
                    usip->state.prev_was_nl = 1;
               } else if (chr != '\r') {
                    usip->state.prev_was_nl = 0;
               }
          }

     }
     usip->data.pos = p;
     return (usip->state.field == USIP_COMPLETE)?usip:NULL;
}

void
usip_set_first(struct usip *usip, char const first[])
{
	usip_touch(usip);
	usip->first = first;
}

void
usip_set_second(struct usip *usip, char const second[])
{
	usip_touch(usip);
	usip->second = second;
}

void
usip_set_rest(struct usip *usip, char const rest[])
{
	usip_touch(usip);
	usip->rest   = rest;
}

void
usip_set_content(struct usip *usip, void *buff, size_t len)
{
	usip_touch(usip);
	usip->content = buff;
	usip->content_length = len;
}

bool
usip_set_parameter(struct usip *usip, char const var[], char val[])
{
	struct usip_kv *param = NULL;
	
	for(int i=0; i<USIP_MAX_PARAMETERS; i++) {
		if(!usip->pars[i].var) {
			if(!param) param = &usip->pars[i]; 
		} else if(!strcasecmp(var,usip->pars[i].var)) {
			param = &usip->pars[i];
			break;
		}
	}
	if(param) {
		usip_touch(usip);
		param->var = (char *) var;
		param->val = uesc_replace(uesc_trim(val,UESC_SPACES),UESC_REPLACE_FS2SPC);
		return true;
	} else {
		return false;
	}
}

char const *
usip_get_parameter(struct usip *usip, char const var[])
{
	for(int i=0;i<USIP_MAX_PARAMETERS;i++) {
		if(!usip->pars[i].var) continue;
		if(!strcasecmp(var,usip->pars[i].var)) {
			return usip->pars[i].val;
		}
	}
	return NULL;
}
     
bool
usip_get_content(struct usip *usip, void **buff, size_t *len)
{
	if(usip->data.buffer) {
		*buff = usip->content;
		*len  = usip->content_length;
		return true;
	} else {
		return false;
	}
}


char const *
usip_get_first (struct usip *_usip)
{
	return _usip->first;
}

char const *
usip_get_second(struct usip *_usip)
{
	return _usip->second;
}

char const *
usip_get_rest(struct usip *_usip)
{
	return _usip->rest;
}
