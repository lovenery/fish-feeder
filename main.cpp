#if !FEATURE_LWIP
    #error [NOT_SUPPORTED] LWIP not supported for this target
#endif

#include "mbed.h"
#include "EthernetInterface.h"
#include "TCPServer.h"
#include "TCPSocket.h"
#include "wdg.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <map>

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


char path[255];
char c[5000];
namespace dts {
		const std::string endl = "\r\n";
		std::string response200 = "<!DOCTYPE html> <html> <body> <p>Click the button to trigger a function that will output 'Hello World' in a p element with id='demo'.</p> <button id='clicker'>Click me</button> <p id='demo'></p> <script src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script> <script> $('#clicker').click(function () { $.ajax({ method: 'POST', url: '/feed', data: 'test=123&hi=no', success: function (data) { console.log(data); document.getElementById('demo').innerHTML = data; }, error: function (err) { console.log(err); document.getElementById('demo').innerHTML = err; } }); }); </script> </body> </html> ";
		//std::string response200 = "<html><body><h1>Hello, World!</h1></body></html>\n";
		std::string response404 = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>\n";
		std::string responsePOST = "success!";
}
std::map<int, std::string> reason;

void HttpResponse(int status, std::string &content) {
	std::string res = "HTTP/1.1 ";

	std::stringstream ss;
	ss << status;
	
	res += ss.str() + " " + reason[status] + "\n";
	res += "Content-Type: text/html\nConnection: Closed\n\n";
	res += content;
	clt_sock.send(res.c_str(), res.length());

}
void HttpRequest (char *c) {
	
		std::string s(c), body;
		//std::cout << "heyheyheyyyyyyy" << s << std::endl << std::endl;
		body = s.substr(s.find("\r\n\r\n")+4);
		s = s.substr(0, s.find('\n'));
		std::stringstream ss(s);
		std::string method, uri, version;
		ss >> method >> uri >> version;
		
		std::cout << "Method: " << method << std::endl;
		std::cout << "URI: " << uri << std::endl;
		std::cout << "Version: " << version << std::endl;
		std::cout << "Body: " << body << std::endl;
		
		if ( method == "GET" ) {
			if (uri == "/") {
				HttpResponse(200, dts::response200);
				//clt_sock.send(dts::response200.c_str(), dts::response200.length());
			} else {
				HttpResponse(404, dts::response404);
				//clt_sock.send(dts::response404.c_str(), dts::response404.length());
			}
		} else if ( method == "POST" ) {
			if (uri == "/feed") {
				HttpResponse(200, dts::responsePOST);
			} else {
				HttpResponse(404, dts::response404);
			}
		}
}

int main()
{
		reason[200] = "OK";
		reason[404] = "Not Found";
    Thread t1;
    Thread t2(osPriorityLow,DEFAULT_STACK_SIZE,NULL);
	
    printf("Basic TCP server example\r\n");

    int ret=1;
    WatchdogInit();
    EthernetInterface eth;
    ret=eth.connect();
    
    if (ret==0) {
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
            ret=srv.accept(&clt_sock, &clt_addr);
            if (ret==0)
            {
							  t2.start(callback(SendtoClient));
								
                pc.printf("Connect Sucess!!\r\n");
                pc.printf("Accept from %s:%d\r\n", clt_addr.get_ip_address(), clt_addr.get_port());
                chkSocket=1;
                ret =-1;

								//clt_sock.send("Hello_Client",strlen("Hello_Client"));

                if (ret!=0)
                {
                    ret = clt_sock.recv(c, sizeof(c)); //recv how many byte of data
                    pc.printf("%d : \r\n",ret);

										HttpRequest(c);

                }
                pc.printf("%s is closed!\r\n\n",clt_addr.get_ip_address());
                chkSocket=0;
								t2.terminate();
                clt_sock.close();      
            }
            
          
            
        }
    }
}

	
		/*
		char str[] = "HTTP/1.1 200 OK\nContent-Type: text/html\nConnection: Closed\n\n<html><body><h1>Hello, World!</h1></body></html>\n";
		char str404[] = "HTTP/1.1 404 Not Found\nConnection: Closed\nContent-Type: text/html; charset=iso-8859-1\n\n<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>\n";

		char *space = strstr(strstr(c, "GET"), " ") + 1;
		memset(path, 0, sizeof(path)); // clear path var
		strncpy(path, space, strstr(space, " ") - space);
		printf("Requesting: %s\r\n", path);
		if (strcmp(path, "/") == 0) {
				clt_sock.send(str, strlen(str));
		} else {
				clt_sock.send(str404, strlen(str404));
		}
		*/


/*
int GETindex = 0;
int HTTPindex = 0;
for(int i = 0;i < ret;i++) {
		if (c[i] == 'G' && c[i+1] == 'E' && c[i+2] == 'T')
				GETindex = i+3;
		if (c[i] == 'H' && c[i+1] == 'T' && c[i+2] == 'T')
				HTTPindex = i-1;
}
for(int i = GETindex; i < HTTPindex; i++){
		pc.printf("%c",c[i]);
}



                    char *space = strstr(strstr(c, "GET"), " ") + 1;
                    memset(path, 0, sizeof(path)); // clear path var
                    strncpy(path, space, strstr(space, " ") - space);
                    printf("Requesting: %s\r\n", path);
                    if (strcmp(path, "/") == 0) {
                        clt_sock.send(str, strlen(str));
                    } else {
                        clt_sock.send(str404, strlen(str404));
                    }
*/
