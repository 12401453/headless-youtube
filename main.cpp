//compile with 
//    g++ CurlFetcher.cpp TcpListener.cpp ytServer.cpp main.cpp -lcurl -o ytserv
//run with
//    ./ytserv <portnumber> [silent]
#include "ytServer.h"

int main(int argc, char *argv[]) {
    
    if(argc < 2) {
        std::cout << "Need to specify portnumber" << std::endl;
        return 0;
    }

    bool show_output = true;
    if(argc > 2) {
        const char* output_flag = argv[2];
        if(!strcmp(output_flag, "silent")) show_output = false;
    }
    
    int portno = atoi(argv[1]);
    ytServer ytserv("0.0.0.0", portno, show_output);
    if(ytserv.init() != 0) {
        std::cout << "Init failed" << std::endl;
        return 0;
    } 
    ytserv.run();
    return 0;
}
