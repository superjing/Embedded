/*
 * LPC2103开发板测试程序
 * 
 * 用途：5110液晶+ENC28J60网络模块测试程序
 * 
 * 作者					日期				备注
 * Huafeng Lin			20010/10/01			新增
 * Huafeng Lin			20010/10/02			修改
 * 
 */

#include <stdio.h>
#include <LPC2103.h>
#include <string.h>

#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "net.h"
#include "PCD5544.h"

#define LED  ( 1 << 17 )	//P0.17控制LED

#define PSTR(s) s
extern void delay_ms(unsigned char ms);

static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
static uint8_t myip[4] = {192,168,1,88};

#define MYWWWPORT 80
#define MYUDPPORT 1200
#define BUFFER_SIZE 1500
static uint8_t buf[BUFFER_SIZE+1];

static char password[]="lcsoft";
 
uint8_t verify_password(char *str)
{
    if (strncmp(password,str,5)==0){
            return(1);
    }
    return(0);
}

int8_t analyse_get_url(char *str)
{
    uint8_t loop=1;
    uint8_t i=0;
    while(loop){
        if(password[i]){
            if(*str==password[i]){
                str++;
                i++;
            }else{
                return(-1);
            }
        }else{
            loop=0;
        }
    }
    if (*str == '/'){
        str++;
    }else{
        return(-3);
    }

    if (*str < 0x3a && *str > 0x2f){
        return(*str-0x30);
    }
    return(-2);
}

uint16_t moved_perm(uint8_t *buf)
{
    uint16_t plen;
    plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 301 Moved Permanently\r\nLocation: "));
    plen=fill_tcp_data(buf,plen,password);
    plen=fill_tcp_data_p(buf,plen,PSTR("/\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
    plen=fill_tcp_data_p(buf,plen,PSTR("<h1>301 Moved Permanently</h1>\n"));
    plen=fill_tcp_data_p(buf,plen,PSTR("add a trailing slash to the url\n"));
    return(plen);
}

uint16_t print_webpage(uint8_t *buf,uint8_t on_off)
{
    uint16_t plen;
    plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
	plen=fill_tcp_data_p(buf,plen,PSTR("<h1 align=center>LCSOFT测试页面</h1><hr>"));
    plen=fill_tcp_data_p(buf,plen,PSTR("<center><p>当前状态: "));
    if (on_off){
        plen=fill_tcp_data_p(buf,plen,PSTR("<font color=\"#00FF00\">开启</font>"));
    }else{
        plen=fill_tcp_data_p(buf,plen,PSTR("关闭"));
    }
    plen=fill_tcp_data_p(buf,plen,PSTR(" <small><a href=\".\">[刷新]</a></small></p>\n<p><a href=\"."));
    if (on_off){
        plen=fill_tcp_data_p(buf,plen,PSTR("/0\">关闭</a><p>"));
    }else{
        plen=fill_tcp_data_p(buf,plen,PSTR("/1\">开启</a><p>"));
    }
    plen=fill_tcp_data_p(buf,plen,PSTR("</center><hr><br><p align=center>更多信息请访问 <a href=\"http://www.lcsoft.net\" target=blank>http://www.lcsoft.net\n"));
    return(plen);
}

int main()
{
    uint16_t plen;
    uint16_t dat_p;
    uint8_t i=0;
    uint8_t cmd_pos=0;
    int8_t cmd;
    uint8_t payloadlen=0;
    char str[30];
    char cmdval;
    
    enc28j60Init(mymac);
    enc28j60clkout(2);
    delay_ms(10);     

	IODIR |= LED;
	IOSET = LED;

    enc28j60PhyWrite(PHLCON,0xD76);
    delay_ms(20);

	IOCLR = LED;
    i=1;

    init_ip_arp_udp_tcp(mymac,myip,MYWWWPORT);

	LCD_Init();
	LCD_Cls();
	LCD_GotoXY(0,0);
	LCD_PrintStr("5110+ENC28J60");

	LCD_GotoXY(0,1);
	LCD_PrintStr("Lcsoft(C) 2010");
	
	LCD_GotoXY(0,2);
	LCD_PrintStr("--------------");

	LCD_GotoXY(0,5);
	LCD_PrintStr("--------------");

	for(;;)
	{
        plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

        if(plen==0){
            continue;
        }

        if(eth_type_is_arp_and_my_ip(buf,plen)){
			static int icmpcount = 0;

			char sip[15];
			char sicmp[14];

			sprintf(sip,"%u.%u.%u.%u",buf[ETH_ARP_SRC_IP_P],buf[ETH_ARP_SRC_IP_P+1],buf[ETH_ARP_SRC_IP_P+2],buf[ETH_ARP_SRC_IP_P+3]);
			LCD_GotoXY(0,3);
			LCD_PrintStr((unsigned char*)sip);

			sprintf(sicmp,"Packets:%d",++icmpcount);
			LCD_GotoXY(0,4);
			LCD_PrintStr((unsigned char*)sicmp);

            make_arp_answer_from_request(buf);

            continue;
        }

        if(eth_type_is_ip_and_my_ip(buf,plen)==0){
            continue;
        }

        if (i){
			IOSET = LED;
            i=0;
        }else{
			IOCLR = LED;
            i=1;
        }
        
        if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V){
			static int icmpcount = 0;

			char sip[15];
			char sicmp[14];

			sprintf(sip,"%u.%u.%u.%u",buf[IP_SRC_P],buf[IP_SRC_P+1],buf[IP_SRC_P+2],buf[IP_SRC_P+3]);
			LCD_GotoXY(0,3);
			LCD_PrintStr((unsigned char*)sip);

			sprintf(sicmp,"Packets:%d",++icmpcount);
			LCD_GotoXY(0,4);
			LCD_PrintStr((unsigned char*)sicmp);

            make_echo_reply_from_request(buf,plen);
            continue;
        }

        if (buf[IP_PROTO_P]==IP_PROTO_TCP_V&&buf[TCP_DST_PORT_H_P]==0&&buf[TCP_DST_PORT_L_P]==MYWWWPORT){
            if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V){
                make_tcp_synack_from_syn(buf);
                continue;
            }
            if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V){
                init_len_info(buf);
                dat_p=get_tcp_data_pointer();
                if (dat_p==0){
                    if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V){
                            make_tcp_ack_from_any(buf);
                    }
                    continue;
                }
                if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
                    plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
                    goto SENDTCP;
                }
                if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
                    plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
                    plen=fill_tcp_data_p(buf,plen,PSTR("<p>Usage: http://host_or_ip/password</p>\n"));
                    goto SENDTCP;
                }
                cmd=analyse_get_url((char *)&(buf[dat_p+5]));

                if (cmd==-1){
                    plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
                    goto SENDTCP;
                }
                if (cmd==1){
					IOCLR = LED;
					i = 1;
					LCD_GotoXY(0,4);
					LCD_PrintStr("STATUS:ON ");
                }
                if (cmd==0){
					IOSET = LED;
					i = 0;
					LCD_GotoXY(0,4);
					LCD_PrintStr("STATUS:OFF");
                }
                if (cmd==-3){
                    plen=moved_perm(buf);
                    goto SENDTCP;
                }

				plen=print_webpage(buf,(i));
SENDTCP:
                make_tcp_ack_from_any(buf);
                make_tcp_ack_with_data(buf,plen);
                continue;
            }
        }

        if (buf[IP_PROTO_P]==IP_PROTO_UDP_V&&buf[UDP_DST_PORT_H_P]==4&&buf[UDP_DST_PORT_L_P]==0xb0){
            payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
            if (verify_password((char *)&(buf[UDP_DATA_P]))){
                cmd_pos=0;
                while(cmd_pos<payloadlen){
                    cmd_pos++;
                    if (buf[UDP_DATA_P+cmd_pos]==','){
                        cmd_pos++;
                        break;
                    }
                }

                if (cmd_pos<2 || cmd_pos>payloadlen-3 || buf[UDP_DATA_P+cmd_pos+1]!='='){
                    strcpy(str,"e=no_cmd");
                    goto ANSWER;
                }

                if (buf[UDP_DATA_P+cmd_pos]=='t'){
                    cmdval=buf[UDP_DATA_P+cmd_pos+2];
                    if(cmdval=='1'){
						IOCLR = LED;
                        strcpy(str,"t=1");
                        goto ANSWER;
                    }else if(cmdval=='0'){
						IOSET = LED;
                        strcpy(str,"t=0");
                        goto ANSWER;
                    }else if(cmdval=='?'){
						if (IOPIN & LED){
                            strcpy(str,"t=1");
                            goto ANSWER;
                        }
                        strcpy(str,"t=0");
                        goto ANSWER;
                    }
                }
                strcpy(str,"e=no_such_cmd");
                goto ANSWER;
            }
            strcpy(str,"e=invalid_pw");
ANSWER:
            make_udp_reply_from_request(buf,str,strlen(str),MYUDPPORT);
        }
	}
}
