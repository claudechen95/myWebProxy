/* Name:Claude Chen
 * Andrew:qiangc
 *
 *
 * proxy.c - A simple, concurrent proxy with a small cache 
 * Functionality: forward clients requests to
 * Web server and retrieve data from server and forward to client.
 *     
 */
#include "csapp.h"
#include "cache.h"

//initializing variables
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static pthread_rwlock_t rwlock;
extern int Time;

void doit(int fd);
void read_requesthdrs(rio_t *rio, char *hostname, char *new_headers);
int parse_uri(char *uri, char *hostname, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, 
     char *shortmsg, char *longmsg);
void *thread(void *vargp);


void sigint_handler() 
{
    clean_up();
    pthread_rwlock_destroy(&rwlock);
    exit(0);
}

int main(int argc, char **argv)
{    

  	int listenfd, *connfd;
  	struct sockaddr_in clientaddr;    
    socklen_t clientlen = sizeof(clientaddr);
    pthread_t tid;
    
    Signal(SIGPIPE, SIG_IGN);
    Signal(SIGINT, sigint_handler);

  //check command line args
  if(argc != 2){
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  //initialize lock
  pthread_rwlock_init(&rwlock,NULL);

  // init cache
  init_cache();

  //open listen fd
  listenfd = Open_listenfd(argv[1]);

  while(1){
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    pthread_create(&tid, NULL, thread, (void *)connfd);
  }
  
  return 0;
}



/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd){


  char buf[MAXLINE],method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char hostname[MAXLINE], path[MAXLINE], port_str[MAXLINE];
  char new_headers[MAXLINE],response[MAXLINE],request_buf[MAXLINE];
  char content_buf[MAX_OBJECT_SIZE];
  int clientfd; 
  int n;
  cache_entry* hit;
  size_t obj_size = 0;
  rio_t rio, rio2;
    
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);  //read the first line of the request

  //cpy request
  strcpy(request_buf, buf);

  	
    /* Search this request in the cache */
    if((hit = find(buf))!= NULL){
      /* cache hit*/
   	  //only one writer can access the cache at the same time
      pthread_rwlock_rdlock(&rwlock);
      Rio_writen(fd, hit->content, hit->size+1);
      Time++;
      pthread_rwlock_unlock(&rwlock);

      return;
    }                                                                       


  sscanf(buf, "%s %s %s", method, uri, version);

  //method other than GET is unimplemented.
  if(strcasecmp(method, "GET")){
    clienterror(fd, "Error: unimplemented method","","","");
    return;
  }

  //parse the uri, get hostname, path and port
  if(parse_uri(uri, hostname, path, port_str) == -1){
    clienterror(fd, "Error: parsing error.","","","");
    return;
  }

  //read the headers from client and buildup new headers
    read_requesthdrs(&rio, hostname, new_headers);


    sprintf(buf, "%s %s HTTP/1.0\r\n", method, path);
    strcat(buf, new_headers);

    clientfd = Open_clientfd(hostname, port_str);

    /* forwoard request to server*/
    Rio_writen(clientfd, buf, strlen(buf));

    Rio_readinitb(&rio2, clientfd);



    //forward server's response to client
    while((n = Rio_readnb(&rio2, response, MAXLINE)) > 0){
      obj_size += n;
      Rio_writen(fd, response, n);
      if(obj_size <= MAX_OBJECT_SIZE){
        strcat(content_buf,response);
      }
    }

    if (obj_size <= MAX_OBJECT_SIZE) {
    	pthread_rwlock_wrlock(&rwlock);
    	insert(request_buf,content_buf);
    	Time++;
    	pthread_rwlock_unlock(&rwlock);
    }
   
    Close(clientfd);
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    doit(connfd);
    Free(vargp);
    Close(connfd);
    return NULL;
}



/*
 * read_requesthdrs: read headers from client's request, buildup new 
 * request header
 */
void read_requesthdrs(rio_t *rio, char *hostname, char *new_headers){
  char buf[MAXLINE]; 
  int has_host = 0;

  //append required hdrs 
  strcat(new_headers, user_agent_hdr);
  strcat(new_headers, connection_hdr);
  strcat(new_headers, proxy_connection_hdr);

  while(Rio_readlineb(rio, buf, MAXLINE)!=0){
    if (!strcmp(buf, "\r\n")) break;
    else if(strstr(buf, "User-Agent"))continue;
    else if (strstr(buf, "Connection")) continue;
    else if (strstr(buf, "Proxy-Connection")) continue;
    else if(strstr(buf, "Host")) {
        //if there is host, attach host to new headers
        strcat(new_headers, buf); 
        has_host = 1;
      }
    else strcat(new_headers, buf);
        //attach other headers to new headers 
    }
  
  
  if(!has_host){
    sprintf(new_headers, "%s Host: %s", new_headers, hostname);
  }

  strcat(new_headers, "\r\n");
  return;
}

/*
 * parse_uri: parse the uri, get host name, path and port #
 */
int parse_uri(char *uri, char *hostname, char *path, char *port){

  char *useful;
  char *pathptr;
  char *portptr;
  // char host[MAXLINE];
  if(strncasecmp(uri, "http://", strlen("http://")) != 0){
    printf("Bad URI\n");
    return -1;
  }

  useful = uri + strlen("http://"); 


  if ((portptr = strchr(useful,':'))!=NULL){
   /* port number detected*/
	   portptr++;
	   if ( (pathptr = strchr(useful,'/'))!=NULL){
	   	//there is a path
	     strncpy(port,portptr,pathptr-portptr);
	     strcpy(path,pathptr);
	     port[pathptr-portptr] = '\0';
	   }
	   else{
	     strcpy(path,"/");
	     strcpy(port,portptr);
	      
	   }
	   strncpy(hostname,useful,(portptr--)-useful);
	   hostname[portptr-useful] = '\0';
  }
  else{
   if((pathptr = strchr(useful,'/'))!=NULL ){
       //there is a path 
       strncpy(hostname,useful,pathptr-useful);
       strcpy(path,pathptr);
       hostname[pathptr-useful] = '\0';
        
     }
     else
     {
       strcpy(hostname,useful);
       strcpy(path,"/");
      
     }
     // default port 
     strcpy(port,"80");
  }



  return 0;
}

/*report error*/
void clienterror(int fd, char *cause, char *errnum, 
     char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
