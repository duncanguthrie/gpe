
/*

	Configfile I/O routines

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config-parser.h" /* Scheme_t */
#include "cfgfile.h"

static FILE* configfile;

int set_file_open(int openon);

int write_sections( Scheme_t *Schemes, int schemescount);

// some helper for parsing strings
/*unsigned char last_char(unsigned char* s);
int count_char(unsigned char* s, unsigned char c);
int get_first_char(unsigned char* s);
int get_param_val(unsigned char* line, unsigned char* param, unsigned char* value);
*/
#if 0
int get_param_val(unsigned char* line, unsigned char* param, unsigned char* value)
{
	int st,sep,a,b;
	
	st=get_first_char(line);
	for (sep=st;sep<strlen(line);sep++)
		if (line[sep]=='=') break;
	for (a=sep;a<strlen(line);a++)
		if (line[a]=='"') break;
	for (b=a+1;b<strlen(line);b++)
		if (line[b]=='"') break;
	param=strncpy(param,&line[st],sep-st);
	param[sep-st]='\0';	
	if ((b-a) > 1) value=strncpy(value,&line[a+1],b-a-1);	
	value[b-a-1]='\0';
}



int get_first_char(unsigned char* s)
{
	int i=0;
	for (i=0;i<strlen(s);i++)
		if ((s[i] != ' ') && (s[i] != '\t')) return i;
	return -1;
}


int count_char(unsigned char* s, unsigned char c)
{
	int a = 0;
	int i;
	for (i=0;i<strlen(s);i++)
		if (s[i] == c) a++;
	return a;
}


unsigned char last_char(unsigned char* s)
{
	int i;
	if (strlen(s)<=0) return (unsigned char)0;
	for (i=1;i<strlen(s);i++)
		if ((s[strlen(s)-i] != ' ') && (s[strlen(s)-i] != '\t')) return s[strlen(s)-i];
	return (unsigned char)0;
}


int get_file_text()
{
	char* ret;
	int i = 0;
	unsigned char buf[256];
	configtext=NULL;		

	do
	{
		ret = fgets(buf,255,configfile);
		if (ret) {
			i++;
			buf[255]='\0';
			configtext=realloc(configtext,i*sizeof(unsigned char*));
			configtext[i-1]=(unsigned char*)malloc(sizeof(unsigned char)*(strlen(ret)+2));				
			strcpy(configtext[i-1],buf);
		}
	}	
	while(ret != NULL);
	return i;	
}


int get_scheme_list()
{
	char scheme[32];
	char socket[32];
	char instance[32];
	char hwaddr[32];
	int i, j, k;
	int l=0;
	for (i=0;i<configlen;i++)
		if ((count_char(configtext[i],',') >= 3) && (count_char(configtext[i],'#') == 0) && (count_char(configtext[i],')') == 1))
		{
			l++;
			k=0;
			for (j=0;j<strlen(configtext[i]);j++)
			{
				if (configtext[i][j]==',') {
					switch (k) {
						case 0:scheme[j]='\0';
						break;
						case 1:socket[j-strlen(scheme)-1]='\0';
						break;
						case 2:instance[j-strlen(scheme)-strlen(socket)-2]='\0';
						break;
					}
					j++; k++;
				}
				if (configtext[i][j]==')') {
						hwaddr[j-strlen(scheme)-strlen(socket)-strlen(instance)-3]='\0';
						break;
					break;
				}
				switch (k) {
					case 0:scheme[j]=configtext[i][j];
					break;
					case 1:socket[j-strlen(scheme)-1]=configtext[i][j];
					break;
					case 2:instance[j-strlen(scheme)-strlen(socket)-2]=configtext[i][j];
					break;
					case 3:hwaddr[j-strlen(scheme)-strlen(socket)-strlen(instance)-3]=configtext[i][j];
					break;
				}
			}
			printf("name: %s\n",scheme);
			printf("socket: %s\n",socket);
			printf("instance: %s\n",instance);
			printf("hwaddr: %s\n",hwaddr);

			schemelist=(Schemelist_t*)realloc(schemelist,l*sizeof(Schemelist_t));
			strcpy(schemelist[l-1].name,scheme);
			strcpy(schemelist[l-1].socket,socket);
			strcpy(schemelist[l-1].instance,instance);
			strcpy(schemelist[l-1].hwaddr,hwaddr);
			schemelist[l-1].firstline=i;
		}
		else // find end
		{
			if ((count_char(configtext[i],';') == 2) && (count_char(configtext[i],'#') == 0) && (count_char(configtext[i],')') == 0))
			{
				schemelist[l-1].lastline=i;
				printf("end: %i\n",schemelist[l-1].lastline);
			}
		}
	schemeslen = l;
	return l;
}


int get_section_nr(char* section)
{
	int i;
	for (i=0;i<schemeslen;i++)
		if (!strcmp(schemelist[i].name,section)) return i;
	return -1;
}


int get_section_start(char* section)
{
	int i;
	for (i=0;i<schemeslen;i++)
		if (!strcmp(schemelist[i].name,section)) return schemelist[i].firstline;
	return -1;
}


int get_section_end(char* section)
{


	int i;
	for (i=0;i<schemeslen;i++)
		if (!strcmp(schemelist[i].name,section)) return schemelist[i].lastline;
	return -1;
}





#endif
int set_file_open(int openon)
{
	if (openon){
		configfile = fopen(WLAN_CONFIGFILE,"w");
	}
	else
	{
		fclose(configfile);
		configfile = NULL;
	}
	return (int)configfile;
}


int write_sections( Scheme_t *Schemes, int sc)
{
	int s_start, s_end;
	printf("saving %i schemes\n",sc);
	set_file_open(TRUE);	
	fprintf(configfile,"#! /bin/sh\n");
	fprintf(configfile,"#\n");
	fprintf(configfile,"Wireless LAN Configuration by gpe-wlancfg\n");
	fprintf(configfile,"#\n");
	fprintf(configfile,"# generated by gpe-wlancfg, you may change/add/remone comments\n");
	fprintf(configfile,"# but comments an formating will be lost if you use gpe-wlancfg\n");
	fprintf(configfile,"\n\n");
	fprintf(configfile,"case \"$ADDRESS\" in\n\n");
	for (s_start=0;s_start<sc;s_start++)
	{
		fprintf(configfile,"%s,%s,%s,%s)\n",Schemes[s_start].Scheme,Schemes[s_start].Socket,Schemes[s_start].Instance,Schemes[s_start].HWAddress);
		fprintf(configfile,"\tINFO=\"%s\"\n",Schemes[s_start].Info);
		fprintf(configfile,"\tESSID=\"%s\"\n",Schemes[s_start].ESSID);
		fprintf(configfile,"\tNWID=\"%s\"\n",Schemes[s_start].NWID);
		fprintf(configfile,"\tCHANNEL=\"%s\"\n",Schemes[s_start].Channel);
		fprintf(configfile,"\tMODE=\"%s\"\n",Schemes[s_start].Mode);
		fprintf(configfile,"\tFREQ=\"%s\"\n",Schemes[s_start].Frequency);
		fprintf(configfile,"\tRATE=\"%s\"\n",Schemes[s_start].Rate);
		
		if (!strcmp(Schemes[s_start].Encryption,"on")){
			if (Schemes[s_start].KeyFormat){
				fprintf(configfile,"\tKEY=\"");
				if (strlen(Schemes[s_start].key1)) fprintf(configfile,"s:%s [1] key ",Schemes[s_start].key1);
				if (strlen(Schemes[s_start].key2)) fprintf(configfile,"s:%s [2] key ",Schemes[s_start].key2);
				if (strlen(Schemes[s_start].key3)) fprintf(configfile,"s:%s [3] key ",Schemes[s_start].key3);
				if (strlen(Schemes[s_start].key4)) fprintf(configfile,"s:%s [4] key ",Schemes[s_start].key4);
				fprintf(configfile,"[%s] %s\"\n",Schemes[s_start].ActiveKey,Schemes[s_start].EncMode);
			}
			else
			{
				fprintf(configfile,"\tKEY=\"");
				if (strlen(Schemes[s_start].key1)) fprintf(configfile," %s [1] key ",Schemes[s_start].key1);
				if (strlen(Schemes[s_start].key2)) fprintf(configfile," %s [2] key ",Schemes[s_start].key2);
				if (strlen(Schemes[s_start].key3)) fprintf(configfile," %s [3] key ",Schemes[s_start].key3);
				if (strlen(Schemes[s_start].key4)) fprintf(configfile," %s [4] key ",Schemes[s_start].key4);
				fprintf(configfile,"[%s] %s\"\n",Schemes[s_start].ActiveKey,Schemes[s_start].EncMode);
			}
		}
		fprintf(configfile,"\tIWCONFIG=\"%s\"\n",Schemes[s_start].iwconfig);
		fprintf(configfile,"\tIWSPY=\"%s\"\n",Schemes[s_start].iwspy);
		fprintf(configfile,"\tIWPRIV=\"%s\"\n",Schemes[s_start].iwpriv);
			
		fprintf(configfile,"\t;;\n\n");
		
	}
	fprintf(configfile,"esac\n");
	
	set_file_open(FALSE);	
}

