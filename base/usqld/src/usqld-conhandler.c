#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <pthread.h>
#include <sqlite.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "usql.h"

#include "usqld-network.h"
#include "usqld-protocol.h"
#include "usqld-server.h"

#include "usqld-conhandler.h"

#ifndef FILENAME_MAX
#define FILENAME_MAX 512
#endif 

/**
  configuration structure for a thread
 */
typedef struct{
  httplike_socket * sock;
  sqlite * db;
  char * database_name;
  usqld_config * config;
  int terminate_now = 0
}usqld_tc;

int usqld_do_rowid(usqld_tc * tc, usqld_packet * packet);

void sigpipe_handler(int sig){
  fprintf(stderr,"Broken pipe on FD \n");
}


int usqld_do_interrupt(usqld_tc * tc,int session_id){
  usqld_packet * packet;
  int rv;
  sqlite_interrupt(tc->db);
  
  packet =  XDR_tree_new_void();
  rv = usqld_session_send_packet(tc->sock,session_id,packet);
  XDR_tree_free(packet);
  
  return rv;
}

/**
   This is the httplike packet respondand. This callback gets invoked
   when httplike_pump recieves a whole packet (qwho's content is stored
   in an input buffer);
 */
void server_packet_handler(httplike_sock * sock
			   httplike_packet *httplike_packet){

  XDR_tree * p=NULL,*err_response=NULL;
  XDR_io * io_chan;
  usqld_tc * tc = NULL;
  int session_id = 0;
  char * session_hval = NULL,method=NULL;
  
  //packet->content_len should be >0 here
  //although zero length packets are allowes
  if(packet->content_len <0){
    fprintf(stderr,"error server_packet_hander: zero_length packet");
    goto sph_internal_error;
  }

  if(session_hval = httplike_packet_get_header("SESSION-ID")){
    if(1!=sscanf(session_hval,"%d",&session_id)){
      fprintf(stderr,"Session ID could not be parsed out of header which was present\n");
      // this is an internal error because the session_id header /should/ be an int.
      goto sph_internal_error;
    }
  }

  if(!(method=packet->operation) || 0!=strcasecmp(packet->operation,"USQLD")){
    fprintf(stderr,"the target of this packet (\"%s\") is not present,or understood\n",packet->operation);
    err_response= usqld_error_packet(USQLD_TARGET_NOT_UNDERSTOOD,"This target is not understood\n");
    goto sph_tolerable_error;
  }
    
  tc = (usqld_tc *)httplike_socket_get_data(sock);
  io_chan = XDR_io_bind_membuf(httplike_packet->content,packet_content_len,0);
  
  if(XDR_OK!=(err_num =XDR_deserialize_elem(usqld_get_schema,io_chan,&p))){
    fprintf(stderr,"the error %d deserializing this packet\n",err_num);
    goto sph_internal_error;
  }

  XDR_io_free(io_chan);
  io_chan = NULL;
  
  if(!tc->connected){
    if(usqld_get_packet_type(p)!= PICKLE_CONNECT)
      resp_rv = usqld_do_connect(tc,p);
    else
      
  }else{
    switch(usqld_get_packet_type(p)){
    case PICKLE_INTERRUPT:
      {
	resp_rv = usqld_do_interrupt(tc,session_id);
      }
    case PICKLE_QUERY: 
      resp_rv = usqld_do_query(tc,p,session_id);
      break; 
    case PICKLE_DISCONNECT:
      tc->terminate_now =1;
      break;
    case PICKLE_REQUEST_ROWID:
      resp_rv = usqld_do_rowid(tc,p,session_id);
      break;
    case PICKLE_REQUEST_CHANGES:
      resp_rv = usqld_do_changes(tc,p,session_id);
    default:
      {
	fprintf(stderr,"unsupported packet type %d\n",
		usqld_get_packet_type(p));
	err_response= usqld_error_packet(USQLD_PACKET_NOT_UNDERSTOOD,"This packet is not understood\n");
	goto sph_tolerable_error;
	break;
      }  
    }
    
    if(resp_rv!=USQLD_OK){
    }
    return;
  }
 sph_tolerable_error:
  {
    assert(err_response!=NULL);
    usqld_session_send_packet(sock,session_id,err_response);
    XDR_tree_free(err_response)
      if(io_chan)
	XDR_io_free(io_chan);
    return;
  }
 sph_internal_error:
  {
    if(io_chan)
      XDR_io_free(io_chan);
    
    return;
  }
}


#define VERSION_INCOMPAT

int usqld_protocol_version_cmp(const char * version){
    
  }
  
/*
  implements the connection request.
 */
int  usqld_do_connect(usqld_tc * tc,int session_id,usqld_packet * p){
  char * database = NULL;
  char * version = NULL;
  XDR_tree_compound * content;
  usqld_packet * reply = NULL;
  int rv = USQLD_OK;
  char * errmsg = NULL;
  char fn_buf[FILENAME_MAX];

  if(tc->db!=NULL){
    reply = usqld_error_packet(SQLITE_CANTOPEN,"database already open");
    goto connect_send_reply;
  }

  content = XDR_TREE_COMPOUND(XDR_t_get_union_t(XDR_TREE_COMPOUND(p)));
  
  version = XDR_t_get_string(XDR_TREE_STR(XDR_t_get_comp_elem(content,0)));
  database = XDR_t_get_string(XDR_TREE_STR(XDR_t_get_comp_elem(content,1)));
  
  fprintf(stderr,"got connect (%s,%s)\n",version,database);
  
  if(strcmp(version,USQLD_PROTOCOL_VERSION)!=0){
    char buf[256];
    snprintf(buf,256,"Your client's protocol version (%s) does not match"\
	     " the server version (%s)",version,USQLD_PROTOCOL_VERSION);
    
    reply = usqld_error_packet(USQLD_VERSION_MISMATCH,buf);
    fprintf(stderr,"Version Mismatch\n");
    goto connect_send_reply;
  }

  strncpy(fn_buf,tc->config->db_base_dir,FILENAME_MAX);
  strncat(fn_buf,database,FILENAME_MAX);

  if(NULL==(tc->db=sqlite_open(fn_buf,0644,&errmsg))){
    reply = usqld_error_packet(SQLITE_CANTOPEN,errmsg);
    goto connect_send_reply;
  }
  sqlite_busy_timeout(tc->db,101);
  tc->database_name = strdup(database);
  /*we are set*/
  reply = usqld_ok_packet();
  goto connect_send_reply;
  
  connect_send_reply:
  assert(reply!=NULL);
  rv =  usqld_send_packet(tc->client_fd,reply);
  XDR_tree_free(reply);
  return rv; 
}

/*
  structure passed to the row-send
 
 */
typedef struct{
  usqld_tc * tc;
  int headsent;
  int rv;
  int terminate_now;
  int session_id;
  int interrupted;
}usqld_row_context;

int usqld_send_row(usqld_row_context * rc,
		   int nfields,		   char ** fields,
		   char ** heads){
  
  XDR_tree *rowpacket = NULL;
  XDR_tree_compound *field_elems;
  int i;
  int rv;
//  fprintf(stderr,"about to try sending a row\n");  
  
#ifdef VERBOSE_DEBUG
  for(i = 0;i<nfields;i++){
    fprintf(stderr,"\t%s,",heads[i]);
  }
  fprintf(stderr,"]\n");
  fprintf(stderr,"(");
  for(i = 0;i<nfields;i++){
    fprintf(stderr,"\t%s,",fields[i]);
     }
  fprintf(stderr,")\n");
#endif
     
  if(!rc->headsent){
    XDR_tree  * srpacket = NULL;
    XDR_tree_compound  *head_elems = NULL;
    
    XDR_tree_new_compound(XDR_VARARRAY,
			  nfields,
			  &head_elems);
    for( i =0;i<nfields;i++){
      XDR_t_set_comp_elem(head_elems,i,XDR_tree_new_string(heads[i]));
    }
    
    
    srpacket = XDR_tree_new_union(PICKLE_STARTROWS,
				  (XDR_tree*)head_elems);
    if(USQLD_OK!=(rv=usqld_send_packet(rc->tc->client_fd,
				       srpacket))){
      XDR_tree_free(srpacket);
      rc->rv = rv;
      rc->terminate_now =1;
      return 1;			 
    }
    XDR_tree_free(srpacket);
    rc->headsent = 1;
  }
  
  XDR_tree_new_compound(XDR_VARARRAY,
			nfields,
			(XDR_tree_compound**)&field_elems);
  
  for(i = 0;i<nfields;i++){
    XDR_t_set_comp_elem(field_elems,i,XDR_tree_new_string(fields[i]));
  }
  rowpacket = XDR_tree_new_union(PICKLE_ROW,
				 (XDR_tree*)field_elems);
  
  if(USQLD_OK!=(rv=usqld_send_packet(rc->tc->client_fd,
				     rowpacket))){
    XDR_tree_free(rowpacket);
    rc->rv = rv;
    return 1;
    
  }
  XDR_tree_free(rowpacket);
  
  usqld_pump_sock(rc->tc->sock);
  return 0;  
}

/*
  implements a the response to a query request
 */
int usqld_do_query(usqld_tc * tc, usqld_packet * packet){
  XDR_tree * reply = NULL;
  int rv =0;
  char * sql ,*errmsg;
  usqld_row_context rc;
  
  rc.tc = tc;
  rc.rv =0;
  rc.headsent = 0;
  rc.terminate_now = 0;
  rc.interrupted = 0;
  if(NULL==tc->db){
    reply = usqld_error_packet(USQLD_NOT_OPEN,
			       "You do not have a database open");
    goto query_send_reply;
  }
  sql = XDR_t_get_string(
          XDR_TREE_STR(
	    XDR_t_get_comp_elem(XDR_TREE_COMPOUND(packet),1)));
 
  if(SQLITE_OK!=(rv=sqlite_exec(tc->db,
				sql,
				(sqlite_callback)usqld_send_row,
				(void*)&rc,
				&errmsg))){
    
    
    if(rc.interrupted){
      rv = 0;
      goto query_send_reply;// no reply no error
    }
    if(rc.terminate_now)
      goto query_send_reply; //fatal error
    if(errmsg==NULL){
      reply = usqld_error_packet(rv,"Unknown sql error");
    }else{
      reply = usqld_error_packet(rv,errmsg);
    }
    
    //fatal termination
    
    goto query_send_reply; 
  }else
    reply = usqld_ok_packet();
  
 query_send_reply:

  if(reply){
    rv =  usqld_send_packet(tc->client_fd,reply);
    XDR_tree_free(reply);
  }
  
  return rv; 
  
}


  
void * usqld_conhandler_main(usqld_conhand_init  * init){
  usqld_tc tc;
  usqld_packet * p = NULL;
  int rv = USQLD_OK,resp_rv = USQLD_OK;
  
  bzero(&tc,sizeof(usqld_tc));
   
  tc.db =NULL;
  tc.client_fd = init->fd;
  tc.config = init->config;   
  free(init);
  
  while(!terminate_now && 
	USQLD_OK==(rv=usqld_recv_packet(tc.client_fd,&p))){

    }
    XDR_tree_free(p);
    if(resp_rv!=USQLD_OK){
      fprintf(stderr,"error %d while sending response\n",resp_rv);
      break;
    }
  }
  
  if(rv!=USQLD_OK){
    fprintf(stderr,"error %d while demarshalling request\n",rv);
  }
  
  close(tc.client_fd);
  if(tc.db){
    sqlite_close(tc.db);
  }
  
  if(tc.database_name){
      free(tc.database_name);
   }
   
   return NULL;
}


/*
  implements a the response to a rowid request
 */
int usqld_do_rowid(usqld_tc * tc, usqld_packet * packet){
  XDR_tree * reply = NULL;
  int rv =0;
  usqld_row_context rc;
  
  rc.tc = tc;
  rc.rv =0;
  rc.headsent = 0;
  rc.terminate_now = 0;
  rc.interrupted = 0;
  if(NULL==tc->db){
    reply = usqld_error_packet(USQLD_NOT_OPEN,"You do not have a database connection.");
    goto query_send_reply;
  }
 
	rv=sqlite_last_insert_rowid(tc->db);

  reply = usqld_rowid_packet(rv);
   
  
	query_send_reply:

  if(reply){
    usqld_send_packet(tc->client_fd,reply);
    XDR_tree_free(reply);
  }
  
  return SQLITE_OK; 
}
  
 
