#include "usip.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>

#define error(F,...) fprintf(stderr, F "\n", ##__VA_ARGS__)


static char const HELP[] =
    "Usage: usip [ARGS] "                                               "\n"
    ""                                                                  "\n"
    "-b BUFLEN : Set length of buffer."                                 "\n"
    "-l        : Read a message line by line from the standard input."  "\n"
    "-r        : Read a message from the standard input."               "\n"
    "-p        : Read content from the standard input, until EOF."      "\n"
    ""                                                                  "\n"
    "-a PARAM:VALUE             : Append/set parameter."                "\n"
    "-i PARAM:VALUE             : Insert parameter."                    "\n"
    "-1 FIRST -2 SECOND -3 REST : Set first line."                      "\n"
    ""                                                                  "\n"
    "-G PARAM        : Print parsed parameter."                         "\n"
    "-M              : Print parsed SIP/HTTP message."                  "\n"
    "-P              : Print parsed content."                           "\n"
    ;


int
main(int argc, char *argv[])
{
	struct	 usip usip;
	char	 buffer[1024];
	size_t	 pos = 0;
	char	*nl;
	char	const *cs;
	
	size_t arg_buffer_length = 1024;
	char  *arg_parameter[USIP_MAX_PARAMETERS][2];
	char   arg_parameter_type[USIP_MAX_PARAMETERS];
	int    arg_parameter_count = 0;
	char  *arg_first_line[3] = {NULL,NULL,NULL};
	bool   arg_read_fgets    = 0;
	bool   arg_read_fread    = 0;
	bool   arg_read_payload  = 0;
	bool   arg_print_message = 0;
	bool   arg_print_payload = 0;
	int    c,i;
	
	if (argc==1) {
		fprintf(stdout, "%s", HELP);
		return 1;
	}
	
	usip_start(&usip,buffer,sizeof(buffer));
	
	while ((c = getopt (argc, argv, "h" "b:" "a:i:" "G:MP" "1:2:3:" "lrp")) != -1) {
		switch (c) {
		case 'h':
			fprintf(stdout, "%s", HELP);
			return 1;
		case 'b':
			arg_buffer_length = atoi(optarg);
			if(arg_buffer_length < 1024) {
				error("Minimun size for buffer is `%ld`.", arg_buffer_length);
				goto err;
			}
			break;
		case 'G':
		case 'i':
		case 'a':
			if(arg_parameter_count == USIP_MAX_PARAMETERS) {
				error("Too much parameters.");
				goto err;
			}
			arg_parameter[arg_parameter_count][0] = optarg;
			if(c != 'G') {
				char *p = strchr(optarg,':');
				if(p) {
					*p='\0';
					p++;
				} else {
					p="";
				}
				arg_parameter[arg_parameter_count][1] = p;
			}
			arg_parameter_type[arg_parameter_count++] = c;
			break;
		case '1': arg_first_line[0] = optarg; break;
		case '2': arg_first_line[1] = optarg; break;
		case '3': arg_first_line[2] = optarg; break;
		case 'l': arg_read_fgets    = 1; break;
		case 'r': arg_read_fread    = 1; break;
		case 'p': arg_read_payload  = 1; break;
		case 'M': arg_print_message = 1; break;
		case 'P': arg_print_payload = 1; break;
		case '?':
		default:
			return 1;
		}
	}
	
	/* Insert parameters. */
	for (i=0; i<arg_parameter_count; i++) {
		if (arg_parameter_type[i] == 'i') {
			usip_set_parameter(&usip, arg_parameter[i][0], arg_parameter[i][1]);
		}
	}
	
	/* Parse message from standard input. */
	struct usip *parsed = NULL;
	while(arg_read_fgets || arg_read_fread) {
		size_t shift;
		if(arg_read_fgets) {
			char *nl = fgets(buffer+pos,sizeof(buffer)-pos,stdin);
			if(!nl) break;
			shift = strlen(nl);
		} else if (arg_read_fread) {
			if(fread(buffer+pos,1,1,stdin)==0) break;
			shift = 1;
		} else {
			break;
		}
		pos += shift;
		if((parsed = usip_forward(&usip, shift,1))) {
			break;
		}
	}

	/* Modify first line. */
	if(arg_first_line[0]) { usip_set_first(&usip,arg_first_line[0]); }
	if(arg_first_line[1]) { usip_set_second(&usip,arg_first_line[1]); }
	if(arg_first_line[2]) { usip_set_rest(&usip,arg_first_line[2]); }
	
	/* Append parameters. */
	for(int i=0;i<arg_parameter_count;i++) {
		if(arg_parameter_type[i] == 'a') {
			usip_set_parameter(&usip,arg_parameter[i][0],arg_parameter[i][1]);
		}
	}
	
	/* Get payload from standard input. */
	if(arg_read_payload) {
		size_t l = fread(buffer+pos,1,sizeof(buffer)-pos,stdin);
		usip_set_content(&usip,buffer+pos,l);
		usip_forward(&usip,l,0);
		pos += l;
	}
	
	/* Print message. */
	if(arg_print_message) {
		size_t l;
		if(usip_serialize(&usip,&nl,&l)) {
			fwrite(nl,1,l,stdout);
		}
	}
	
	/* Print parameters. */
	for(i=0; i<arg_parameter_count; i++) {
		if(arg_parameter_type[i] != 'G') {
			continue;
		}
		if(!strcasecmp(arg_parameter[i][0],"1")) {
			cs = usip_get_first(&usip);
			fprintf(stdout, "%s\n",(cs)?cs:"");
		} else if(!strcasecmp(arg_parameter[i][0],"2")) {
			cs = usip_get_second(&usip);
			fprintf(stdout, "%s\n",(cs)?cs:"");
		} else if(!strcasecmp(arg_parameter[i][0],"3")) {
			cs = usip_get_rest(&usip);
			fprintf(stdout,"%s\n",(cs)?cs:"");
		} else {
			cs = usip_get_parameter(&usip,arg_parameter[i][0]);
			fprintf(stdout,"%s\n",(cs)?cs:"");
		}
		
	}
     
	/* Print content. */
	if (arg_print_payload) {
		void *content; size_t content_length;
		if(usip_get_content(&usip,&content,&content_length)) {
			fwrite(content,1,content_length,stdout);
			return 0;
		} else {
			goto err;
		}
	}
	
	return 0;
 err:
	return 1;
}
