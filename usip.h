#ifndef USIP_H
#define USIP_H

#include <stdlib.h>
#include <stdbool.h>
#ifndef USIP_MAX_PARAMETERS
#  define USIP_MAX_PARAMETERS 30
#endif

struct usip_kv {
	char	*var;
	char	*val;
};

struct usip_state {
	bool wait_word    : 1; bool wait_space   : 1;
	bool wait_nl      : 1; bool prev_was_nl  : 1;
	enum {
		USIP_READING_FIRST,  USIP_READING_SECOND,  USIP_READING_THIRD,
		USIP_READING_PARAMS, USIP_READING_CONTENT, USIP_COMPLETE
	} field;
	struct	usip_kv *param;
};

struct usip {
	struct	 usip_state state;
	char	 const *first,*second,*rest;
	struct	 usip_kv pars[USIP_MAX_PARAMETERS];
	char	*content;
	size_t	 content_length;
	struct {
		char *buffer; size_t pos,length;
	} data;
	struct {
		char *serialized; size_t size;
	} serial;
};
     
extern int	usip_start(struct usip *, void *, size_t);
extern void	usip_touch(struct usip *);
     
extern struct usip *	usip_forward(struct usip *,size_t,int);
extern bool		usip_serialize(struct usip *,char **,size_t *);

extern void	usip_set_first(struct usip *,char const *);
extern void	usip_set_second(struct usip *,char const *);
extern void	usip_set_rest(struct usip *,char const *);
extern void	usip_set_content(struct usip *,void *,size_t);
extern bool	usip_set_parameter(struct usip *,char const *,char *);

extern bool	usip_get_content(struct usip *,void **,size_t *);

extern char const *	usip_get_first(struct usip *);
extern char const *	usip_get_second(struct usip *);
extern char const *	usip_get_rest(struct usip *);
extern char const *	usip_get_parameter(struct usip *,char const *);

#endif
