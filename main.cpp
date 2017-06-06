#if !FEATURE_LWIP
    #error [NOT_SUPPORTED] LWIP not supported for this target
#endif

#include "mbed.h"
#include "EthernetInterface.h"
#include "TCPServer.h"
#include "TCPSocket.h"
#include "wdg.h"
#include <iostream>
#include <string>
#include <sstream>
#include <map>

Serial pc(SERIAL_TX, SERIAL_RX);

TCPServer srv;
TCPSocket clt_sock;
SocketAddress clt_addr;

int chkSocket=0;
bool bConnect = false;
void main2()
{
    while(true)
    {
        if(bConnect==true) WatchdogRefresh();
        Thread::wait(1000);
    }
}



// motor
bool bRotateflag = true;
int iState=0;  //0: not feed ; 1:feed
int RightMostAngle = 10.0f;
int LeftMostAngle = -10.0f;

class CSg90 : public PwmOut
{
public:
    CSg90(PinName pwmPin):PwmOut(pwmPin)
    {
        period_ms(20);// 50Hz to trigger SG90
        fnSetRotateAngle(-45.0f);
    }
    void fnSetRotateAngle(float fAngle)
    {
        if (fAngle > RightMostAngle ) fAngle = RightMostAngle;
        else if(fAngle < LeftMostAngle) fAngle = LeftMostAngle;

        int iRotate = (- (fAngle - RightMostAngle) * 950.0f / RightMostAngle) + 500.0f;
        pulsewidth_us(iRotate);
    }
};
void main_motor() {
    CSg90 mypwm(PB_10);
    int fRotateInc = 0;
    while(1) { // back and forward once
        wait(0.1);
        switch (iState)
        {
            case 0:
                break;
            case 1:
                if(fRotateInc <= RightMostAngle && bRotateflag)
                {
                    fRotateInc += 2;
                }
                else
                {
                    bRotateflag = false;
                    fRotateInc -= 2;
                    if(fRotateInc <= LeftMostAngle)
                    iState = 0;
                }
                mypwm.fnSetRotateAngle((float)fRotateInc);
                break;
        }
    }
}



// HTTP part
namespace dts {
    char http_content[5000];
    //std::string response200 = "<html><body><h1>Hello, World!</h1></body></html>\n";
    std::string response200 = "<!DOCTYPE html> <html> <body> <p>Click the button to trigger a function that will output 'Hello World' in a p element with id='demo'.</p> <button id='clicker'>Click me</button> <p id='demo'></p> <script src='https://cdnjs.cloudflare.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script> <script> $('#clicker').click(function () { $.ajax({ method: 'POST', url: '/feed', data: 'test=123&hi=no', success: function (data) { console.log(data); document.getElementById('demo').innerHTML = data; }, error: function (err) { console.log(err); document.getElementById('demo').innerHTML = err; } }); }); </script> </body> </html> ";
    std::string response404 = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>\n";
    std::string responsePOST = "success!";
    std::map<int, std::string> reason;
}
void HttpResponse(int status, std::string &content) {
    std::string res = "HTTP/1.1 ";
    std::stringstream ss;
    ss << status; // int to sstream
    res += ss.str() + " " + dts::reason[status] + "\n";
    res += "Content-Type: text/html\nConnection: Closed\n\n";
    res += content;
    clt_sock.send(res.c_str(), res.length());
}
void HttpRequest (char *c) {
    std::string s(c), body;
    //std::cout << "DEBUG:" << s << std::endl << std::endl;
    body = s.substr(s.find("\r\n\r\n")+4); // get post data
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
        } else {
            HttpResponse(404, dts::response404);
        }
    } else if ( method == "POST" ) {
        if (uri == "/feed") {
            HttpResponse(200, dts::responsePOST);
            if(iState == 0){
                iState = 1;
                bRotateflag = true;
            }else{
                iState = 0;
            }
        } else {
            HttpResponse(404, dts::response404);
        }
    }
}

int main()
{
    // add
    dts::reason[200] = "OK";
    dts::reason[404] = "Not Found";
    Thread motor;
    motor.start(callback(main_motor));
    //

    Thread t1; // t1 for watch dog
    printf("Basic TCP server example\r\n");
    int ret=1;
    WatchdogInit();
    EthernetInterface eth;
    ret=eth.connect();
    if (ret==0) {
        bConnect = true;
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
                pc.printf("Connect Sucess!!\r\n");
                pc.printf("Accept from %s:%d\r\n", clt_addr.get_ip_address(), clt_addr.get_port());
                chkSocket=1;
                ret =-1;
                if (ret!=0)
                {
                    ret = clt_sock.recv(dts::http_content, sizeof(dts::http_content)); //recv how many byte of data
                    // pc.printf("%d : \r\n",ret); // print data size
                    // add
                    pc.printf("Our Result: \r\n");
                    HttpRequest(dts::http_content);
                }
                pc.printf("%s is closed!\r\n\n",clt_addr.get_ip_address());
                chkSocket=0;
                clt_sock.close();      
            }
        }
    }
}
