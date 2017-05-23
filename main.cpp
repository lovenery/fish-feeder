#if !FEATURE_LWIP
    #error [NOT_SUPPORTED] LWIP not supported for this target
#endif

#include "mbed.h"
#include "EthernetInterface.h"
#include "TCPServer.h"
#include "TCPSocket.h"
#include "wdg.h"


Serial pc(SERIAL_TX, SERIAL_RX);
DigitalOut led(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalIn btnSocketSend(USER_BUTTON);

TCPServer srv;
TCPSocket clt_sock;
SocketAddress clt_addr;

int chkSocket=0;
char a[50];
bool bConnect = false;


void SendtoClient()
{
	while(true)
   {
        if(btnSocketSend)
        {
            if(chkSocket==1){
                if (led==1 && led2==1) strcpy(a,"LED1 and LED2 is open");
                else if(led==1 && led2==0)strcpy(a,"Only LED1 is open");
                else if(led==0 && led2==1) strcpy(a,"Only LED2 is open");
                else strcpy(a,"LED1 and LED2 is closed");
                clt_sock.send(a,strlen(a));
            }
            led3=!led3;
            Thread::wait(1000);
        }
    }
}

void main2()
{
   while(true)
   {
        if(bConnect==true)WatchdogRefresh();
        Thread::wait(1000);
    }
}

int main()
{
    Thread t1;
    Thread t2(osPriorityLow,DEFAULT_STACK_SIZE,NULL);
	
    printf("Basic TCP server example\r\n");
    char c[50];
    int ret=1;
    WatchdogInit();
    EthernetInterface eth;
    ret=eth.connect();
    
    if (ret==0){
        bConnect=true;
        t1.start(callback(main2));
        
        printf("The target IP address is '%s'\r\n", eth.get_ip_address());
        
        /* Open the server on ethernet stack */
        srv.open(&eth);
        
        /* Bind the HTTP port (TCP 80) to the server */
        srv.bind(80);
        
        /* listening connections */
        srv.listen();
        
        while (true) {
            ret=srv.accept(&clt_sock,&clt_addr);
            if (ret==0)
            {
							  t2.start(callback(SendtoClient));
								
                pc.printf("sucess!!\r\n");
                pc.printf("accept %s:%d\r\n", clt_addr.get_ip_address(), clt_addr.get_port());
                chkSocket=1;
                ret =-1;
								clt_sock.send("Hello_Client",strlen("Hello_Client"));
                while(ret!=0)
                {
                    ret = clt_sock.recv(c,0xff); //recv how many byte of data
                    pc.printf("%d : ",ret);
                    if(ret>0)
                    {
                        if(c[0]=='0')
                        {
                            led=!led;
                        }
                        else if(c[0]=='1')
                        {
                            led2=!led2;    
                        }
                        for(int i = 0;i < ret;i++){
                            pc.printf("%c",c[i]);}
                        pc.printf("\r\n");
                    } 
                }
                pc.printf("%s is closed!\r\n",clt_addr.get_ip_address());
                chkSocket=0;
								t2.terminate();
                clt_sock.close();      
            }
            
          
            
        }
    }
}
