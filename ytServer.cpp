//compile with 
//    g++ CurlFetcher.cpp TcpListener_threads.cpp ytServer.cpp main.cpp -lcurl -o ytserv
//run with
//    ./ytserv <portnumber> [silent]
#include <sstream>
#include "ytServer.h"
#include <math.h>
#include <sys/stat.h>
#include <vector>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <regex>
#include <filesystem>
#include <fcntl.h>

//#include "cJSON.h"

#define HOMEPAGE "/yt_search"

std::mutex mutex;
/*
void ytServer::onClientConnected(int clientSocket) {

}

void  ytServer::onClientDisconnected(int clientSocket) { 
    
} */

void ytServer::onMessageReceived(int clientSocket, const char* msg, int length) {
    
    #define GET 3
    #define POST 4
  
    m_POST_continue = true;

    std::cout << "\n--------------------------------------MESSAGE BEGIN--------------------------------------------------\n\n" << msg;
    std::cout << "\n-----------------------------------------------------------------------------------------------------\n";
    std::cout << "Size of the above message is " << length << " bytes\n\n";
  
    int lb_pos = checkHeaderEnd(msg);
    int get_or_post = getRequestType(msg);
  
    std::cout << get_or_post << std::endl;

    if(get_or_post == -1 && lb_pos == -1) {
        buildPOSTedData(msg, false, length);

        if(m_POST_continue == false) {
            handlePOSTedData(m_post_data, clientSocket);
        }
    }

    else if (get_or_post == GET)
    {
        std::cout << "This is a GET request" << std::endl;

        std::cout << "lb_pos: " << lb_pos << std::endl;
        char msg_url[lb_pos - 12];
        std::cout << "size of msg_url array: " << sizeof(msg_url) << std::endl;

        for (int i = 4; i < lb_pos - 9; i++)
        {
            msg_url[i - 4] = msg[i];
            std::cout << msg[i];
        }
        std::cout << std::endl;
        msg_url[lb_pos - 13] = '\0';

        short int page_type = 0;
        if(!strcmp(msg_url, "/text_viewer")) page_type = 1;
        else if(!strcmp(msg_url, "/add_texts")) page_type = 2;
        else if(!strcmp(msg_url, "/words")) page_type = 3;

        const char* final_msg_url;
        if(!strcmp(msg_url, "/")) {
            final_msg_url = HOMEPAGE;
        }
        else {
            final_msg_url = (const char*) msg_url;
        }

        //#include "docRoot.cpp"
   
        int url_size = strlen("HTML_DOCS") + strlen(final_msg_url) + 1; //sizeof() can be used for c-strings declared as an array of char's but strlen() must be used for char *pointers
        char url_c_str[url_size];
    
        strcpy(url_c_str, "HTML_DOCS");
        strcat(url_c_str, final_msg_url);

        bool cookies_present = false;
        if(page_type > 0) cookies_present = readCookie(m_cookies, msg); //want to avoid reading cookies when serving auxilliary files like stylesheets
        std::cout << "Cookies present? " << cookies_present << std::endl;
        if(cookies_present) {
            std::cout << "text_id cookie: " << m_cookies[0] << "; current_pageno cookie: " << m_cookies[1] << "; lang_id cookie: " << m_cookies[2]  << std::endl;
        }

        std::string content = "<h1>404 Not Found</h1>"; 

        std::string content_type = "text/html;charset=UTF-8";
        
        char fil_ext[5];
        for(int i = 0; i < 4; i++) {
            fil_ext[i] = url_c_str[url_size - 5 + i];
        }
        fil_ext[4] = '\0';
        std::cout << "fil_ext = " << fil_ext << std::endl;

        if(!strcmp(fil_ext, ".css")) {
            content_type = "text/css";
        }
        else if(!strcmp(fil_ext + 1, ".js")) {
            content_type = "application/javascript";
        }
        else if(!strcmp(fil_ext, ".ttf")) {
            content_type = "font/ttf";
            sendBinaryFile(url_c_str, clientSocket, content_type);
            return;
        }
        else if(!strcmp(fil_ext, ".mp3")) {
            content_type = "audio/mpeg";
            sendBinaryFile(url_c_str, clientSocket, content_type);
            return;
        }
        else if(!strcmp(fil_ext, ".png")) {
            content_type = "image/png";
            sendBinaryFile(url_c_str, clientSocket, content_type);
            return;
        }
        else if(!strcmp(fil_ext, ".svg")) {
            content_type = "image/svg+xml";
            sendBinaryFile(url_c_str, clientSocket, content_type);
            return;
        }       


        buildGETContent(page_type, url_c_str, content, cookies_present);       

     

        std::ostringstream oss;
        oss << "HTTP/1.1 200 OK\r\n";
    //    oss << "Cache-Control: no-cache, private \r\n";
        oss << "Content-Type: " << content_type << "\r\n";
        oss << "Content-Length: " << content.size() << "\r\n";
        oss << "\r\n";
        oss << content;

        std::string output = oss.str();
        int size = output.size() + 1;

        sendToClient(clientSocket, output.c_str(), size);
    }
    else if(get_or_post == POST) {  

        std::cout << "This is a POST request" << std::endl;
        
       buildPOSTedData(msg, true, length);
       if(m_POST_continue == false) {
            handlePOSTedData(m_post_data, clientSocket);
       } 
    } 
    
}


bool ytServer::c_strStartsWith(const char *str1, const char *str2)
{
    int strcmp_count = 0;
    int str2_len = strlen(str2);
 
    int i = -1;
   
    while ((*str1 == *str2 || *str2 == '\0') && i < str2_len)
    {
        strcmp_count++;
        str1++;
        str2++;
        i++;
    }
 
    if (strcmp_count == str2_len + 1)
    {
        return true;
    }
    else
        return false;
}

int ytServer::c_strFind(const char* haystack, const char* needle) {

    int needle_startpos = 0;
    int needle_length = strlen(needle);
    int haystack_length = strlen(haystack);
    if(haystack_length < needle_length) return -1;
    char needle_buf[needle_length + 1]; //yes I'm stack-allocating variable-length arrays because g++ lets me and I want the efficiency; it will segfault just the same if I pre-allocate an arbitrary length array which the data is too big for anyway, and I absolutely will not use any of the heap-allocated C++ containers for the essential HTTP message parsing, which needs to be imperceptibly fast.

    needle_buf[needle_length] = '\0';
    for(int i = 0; i < haystack_length; i++) {
        for(int j = 0 ; j < needle_length; j++) {
            needle_buf[j] = haystack[j];
        }
       
        if(c_strStartsWith(needle_buf, needle)) {
            break;
        }
        needle_startpos++;
        haystack++;
    }
    
    if(needle_startpos == haystack_length) {
        return -1;
        }
    else {
        return needle_startpos;
    } 
}

int ytServer::c_strFindNearest(const char* haystack, const char* needle1, const char* needle2) {
    int needles_startpos = 0;
    int needle1_length = strlen(needle1);
    int needle2_length = strlen(needle2);
    int longest_needle_length = needle1_length < needle2_length ? needle2_length : needle1_length;
    int haystack_length = strlen(haystack);
    if(haystack_length < longest_needle_length) return -1;

    char needles_buf[longest_needle_length + 1];
    needles_buf[longest_needle_length] = '\0';

    for(int i = 0; i < haystack_length; i++) {
        for(int j = 0; j < longest_needle_length; j++) {
            needles_buf[j] = haystack[j];
        }
        if(c_strStartsWith(needles_buf, needle1) || c_strStartsWith(needles_buf, needle2)) {
            break;
        }
        needles_startpos++;
        haystack++;
    }

    if(needles_startpos == haystack_length) {
        return -1;
    }
    else {
        return needles_startpos;
    }
}


int ytServer::getRequestType(const char* msg ) {

    if(c_strStartsWith(msg, "GET")) {
        return 3;
    }
    else if(c_strStartsWith(msg, "POST")) {
        return 4;
    }
    else return -1;
}

int ytServer::checkHeaderEnd(const char* msg) {
    int lb_pos = c_strFind(msg, "\x0d");
    int HTTP_line_pos = c_strFind(msg, "HTTP/1.1");
    if(HTTP_line_pos != -1 && HTTP_line_pos < lb_pos) return lb_pos;
    else return -1;
}

 void ytServer::buildGETContent(short int page_type, char* url_c_str, std::string &content, bool cookies_present) {
    
    std::ifstream urlFile;
    urlFile.open(url_c_str);
    
    if (urlFile.good())
    {
        std::cout << "This file exists and was opened successfully." << std::endl;

        std::ostringstream ss_text;
        std::string line;
        while (std::getline(urlFile, line))
        {   
            ss_text << line << '\n';
        }

        content = ss_text.str();

        urlFile.close();
    }
    else
    {
        std::cout << "This file was not opened successfully." << std::endl;
    }
        
}

void ytServer::buildPOSTedData(const char* msg, bool headers_present, int length) {
    std::lock_guard<std::mutex> lock(mutex);
    if(headers_present) {

        setURL(msg);
        
        int content_length_start = c_strFind(msg, "Content-Length:") + 16;
        
        int cl_figures = 0;
        char next_nl;
        while(next_nl != '\x0d') {
            next_nl = msg[content_length_start + cl_figures];
            cl_figures++;
        }
        cl_figures--;
        std::cout << "Number of digits in content-length: " << cl_figures << std::endl;

        char content_length_str[cl_figures + 1];
        for(int i = 0; i < cl_figures; i++) {
            content_length_str[i] = msg[content_length_start + i];
        }
        content_length_str[cl_figures] = '\0';
        int content_length = atoi(content_length_str);
        std::cout << "Parsed-out content-length: " << content_length << std::endl;

        int headers_length = c_strFind(msg, "\r\n\r\n");
        m_POST_continue = false;
        m_bytes_handled = 0;
        m_total_post_bytes = headers_length + 4 + content_length;
        if(m_total_post_bytes > length) {
            std::string post_data {msg + headers_length + 4}; //strcat won't work unless I use malloc() to allocate heap-memory of appropriate size for the stuck-together c-strings, so may as well use std::string if I'm forced to heap-allocate anyway
            //at least I'm only heap-allocating in those instances where the POST data makes the message over 4096 bytes
           m_post_data_incomplete = post_data;
           // std::cout << "headers_length + 4 + content_length > length" << std::endl;
           m_POST_continue = true;
           m_bytes_handled += length;
            
        }
        else {
            m_post_data = msg + headers_length + 4;
        }
  
    }
    else {
        
        std::string post_data_cont {msg};
        m_post_data_incomplete.append(post_data_cont);
        m_post_data = m_post_data_incomplete.c_str();
        m_bytes_handled += length;
        if(m_total_post_bytes == m_bytes_handled) {
            m_POST_continue = false;
        }    
    }

}

std::string ytServer::URIDecode(std::string &text) //stolen off a rando on stackexchange who forgot to null-terminate the char-array before he strtol()'d it, which caused big problems
{
    std::string escaped;

    for (auto i = text.begin(), nd = text.end(); i < nd; ++i)
    {
        auto c = (*i);

        switch (c)
        {
        case '%':
            if (i[1] && i[2])
            {
                char hs[]{i[1], i[2], '\0'};
                escaped += static_cast<char>(strtol(hs, nullptr, 16));
                i += 2;
            }
            break;
        case '+':
            escaped += ' ';
            break;
        default:
            escaped += c;
        }
    }

    return escaped;
}

std::string ytServer::htmlspecialchars(const std::string &innerHTML) {
    std::string escaped;

    for(auto i = innerHTML.begin(), nd = innerHTML.end(); i < nd; i++) {
        char c = (*i);

        switch(c) {
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&#039;";
                break;
            case '&':
                escaped += "&amp;";
                break;
            case '\x5c':
                escaped += "\x5c\x5c";
                break;
            case '\n':
                escaped += "\x5c\x6e"; //we need a literal 'n' for the JSON otherwise it is just a slash plus a newline character
                break;
            case '\t':
                escaped += "\x5c\x74";
                break;
            case '\r':
                escaped += "\x5c\x72";
                break;
            case '\f':
                escaped += "\x5c\x66";
                break;
            default:
                escaped += c;
        }
    }
    return escaped;
}

std::string ytServer::escapeQuotes(const std::string &quoteystring) {
    std::string escaped;
    for(auto i = quoteystring.begin(), nd = quoteystring.end(); i < nd; i++) {

        char c = (*i); //the quotation mark is always a single-byte ASCII char so this is fine
        switch(c) {
            case '\x22':
                escaped += "\\\"";
                break;
            case '\x5c':
                escaped += "\x5c\x5c";
                break;
            case '\n':
                escaped += "\x5c\x6e";
                break;
            case '\r':
                escaped += "\x5c\x72";
                break;
            case '\t':
                escaped += "\x5c\x74";
                break;
            case '\f':
                escaped += "\x5c\x66";
                break;
            default:
                escaped += c;
        }
    }
    return escaped;
}

//in order to avoid using std::vector I cannot save the array of the POSTed data into a variable which persists outside this function, because the size of the array is only known if I know which POST data I am processing, which means the code to create an array out of the m_post_data has to be repeated everytime a function to handle it is called
void ytServer::handlePOSTedData(const char* post_data, int clientSocket) {
    std::cout << "------------------------COMPLETE POST DATA------------------------\n" << post_data << "\n-------------------------------------------------------\n";
    
    int post_fields = getPostFields(m_url);

    if(post_fields == -1) {
        std::string bad_post_response = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 30\r\n\r\nUnrecognised POST url - repent";
        sendToClient(clientSocket, bad_post_response.c_str(), bad_post_response.length() + 1);
        return;
    }

    int equals_positions_arr[post_fields];
    int equals_pos = 0;
    for(int i = 0; i < post_fields; i++) {
        equals_pos = c_strFind(post_data + equals_pos + 1, "=") + equals_pos + 1;
        if(equals_pos == -1) break;
        equals_positions_arr[i] = equals_pos;
        
    }

    int amp_positions_arr[post_fields];
    int amp_pos = 0;
   
  
    for(int i = 0; i < post_fields - 1; i++) {
        amp_pos = c_strFind(post_data + amp_pos + 1, "&") + amp_pos + 1;
        if (amp_pos == -1) break;
        amp_positions_arr[i] = amp_pos;
        
    }
    amp_positions_arr[post_fields -1] = strlen(post_data);
    

    for(int i = 0; i < post_fields; i++) {
        std::cout << "equals position no. " << i << ": " << equals_positions_arr[i] << std::endl;
    }
    for(int i = 0; i < post_fields - 1; i++) {
        std::cout << "amp position no. " << i << ": " << amp_positions_arr[i] << std::endl;
    }
    std::string post_values[post_fields];
    for(int i = 0; i < post_fields; i++) {
        //this could be a really long text so if over a certain size needs to be heap-allocated (however it's also used for literally all POST requests, most of which are tiny, so the limit for stack-allocation will be 512Kb)
        int ith_post_value_length = amp_positions_arr[i] - equals_positions_arr[i] + 1;
        
        if(ith_post_value_length > 524288) {
            char* ith_post_value = new char[ith_post_value_length];
            std::cout << "length of post_value array: " << ith_post_value_length << std::endl;
            for(int j = 0; j < amp_positions_arr[i] - equals_positions_arr[i] - 1; j++) {
                ith_post_value[j] = post_data[equals_positions_arr[i] + 1 + j];
            // printf("j = %i, char = %c | ", j, post_data[equals_positions_arr[i] + 1 + j]);
            }
            ith_post_value[amp_positions_arr[i] - equals_positions_arr[i] - 1] = '\0';
            
            std::string ith_post_value_str {ith_post_value};
            delete[] ith_post_value;
      
            post_values[i] = ith_post_value_str;
        }
        else {
            char ith_post_value[ith_post_value_length];  //In Windows I will make this probably a 4096 bytes fixed-sized-array, because no-one needs 512Kb to hold a lang_id etc.
            std::cout << "length of post_value array: " << ith_post_value_length << std::endl;
            for(int j = 0; j < amp_positions_arr[i] - equals_positions_arr[i] - 1; j++) {
                ith_post_value[j] = post_data[equals_positions_arr[i] + 1 + j];
            // printf("j = %i, char = %c | ", j, post_data[equals_positions_arr[i] + 1 + j]);
            }
            ith_post_value[amp_positions_arr[i] - equals_positions_arr[i] - 1] = '\0';
            
            std::string ith_post_value_str {ith_post_value};
      
            post_values[i] = ith_post_value_str;
        }    
    }

    for (int i = 0; i < post_fields; i++)
    {
       std::cout << "POST value no." << i << ": " << post_values[i] << std::endl;
    }

    if(!strcmp(m_url, "/show_text.php")) {
        bool php_func_success = showText(post_values, clientSocket);
    }
    else if(!strcmp(m_url, "/yt_search.php")) {
        bool php_func_success = ytSearch(post_values, clientSocket);
    }
    else if(!strcmp(m_url, "/curl_lookup.php")) {
        bool php_func_success = curlLookup(post_values, clientSocket);
    }
    else if(!strcmp(m_url, "/play_vid.php")) {
        bool php_func_success = playMPV_stdSystem(post_values, clientSocket);
    }
    else if(!strcmp(m_url, "/control_mpv.php")) {
        bool php_func_success = controlMPV(post_values, clientSocket);
    }
    else if(!strcmp(m_url, "/get_current_time.php")) {
        bool php_func_success = getCurrentTimePos(post_values, clientSocket);
    }
    else if(!strcmp(m_url, "/listen_start.php")) {
        bool php_func_success = listenForPlaybackStart(post_values, clientSocket);
    }
    else if(!strcmp(m_url, "/shutdown_device.php")) {
        bool php_func_success = shutdownDevice(post_values, clientSocket);
    }
    

    std::cout << "m_url: " << m_url << std::endl;
    
}

void ytServer::setURL(const char* msg) {
    int url_start = c_strFind(msg, "/");
  //  printf("url_start: %i\n", url_start);
    int url_end = c_strFind(msg + url_start, " ") + url_start;
  //  printf("url_end: %i\n", url_end);
    //char url[url_end - url_start + 1];
    char url[50];
    memset(url, '\0', 50);
    for (int i = 0; i < url_end - url_start && i < 49; i++)
    {
        url[i] = msg[url_start + i];
    }
    //url[url_end - url_start] = '\0';
    
    strncpy(m_url, url, 50); //m_url is max 50 chars but this is allowed because I tightly control what the POST urls are; using std::string would be wasteful
}

int ytServer::getPostFields(const char* url) {
    if(!strcmp(url, "/show_text.php")) return 1;
    else if(!strcmp(url, "/yt_search.php")) return 1;
    else if(!strcmp(url, "/curl_lookup.php")) return 1;
    else if(!strcmp(url, "/play_vid.php")) return 2;
    else if(!strcmp(url, "/control_mpv.php")) return 1;
    else if(!strcmp(url, "/get_current_time.php")) return 1;
    else if(!strcmp(url, "/listen_start.php")) return 1;
    else if(!strcmp(url, "/shutdown_device.php")) return 1;
    else return -1;
}

bool ytServer::readCookie(std::string cookie[3], const char* msg) {
    int cookie_start = c_strFind(msg, "\r\nCookie") + 9;
    if(cookie_start == 8) return false; //c_strFind() returns -1 if unsuccessful, but I've just added 9 to it so the number signalling no cookies is 8

    const char* cookie_keys[3] {" text_id=", " current_pageno=", " lang_id="};

    for(int i = 0; i < 3; i++) {
        int cookieName_start = c_strFind(msg+cookie_start, cookie_keys[i]);
        if(i == 0 && cookieName_start == -1) return false;
        if(cookieName_start == -1) {
            cookie[i] = "1";
            break;
        }
        //printf("cookieName_start: %i\n", cookieName_start);

        int cookieName_length = strlen(cookie_keys[i]);

        int val_length = c_strFindNearest(msg+cookie_start+cookieName_start + cookieName_length, ";", "\r\n");
        //std::cout << "Cookie val_length: " << val_length << "\n";

        char val[val_length + 1];
        for(int j = 0; j < val_length; j++) {
            val[j] = (msg + cookie_start + cookieName_start + cookieName_length)[j];
            //printf("val[j] = %c\n", val[j]);
        }
        val[val_length] = '\0';

        cookie[i] = val;
    }

    return true;
}

void ytServer::sendBinaryFile(char* url_c_str, int clientSocket, const std::string &content_type) {

    std::ifstream urlFile(url_c_str, std::ios::binary);
    if (urlFile.good())
    {
        std::cout << "This file exists and was opened successfully." << std::endl;

        struct stat size_result;    
        int font_filesize = 0;
        if(stat(url_c_str, &size_result) == 0) {
            font_filesize = size_result.st_size;
            std::cout << "Fontfile size: " << font_filesize << " bytes" << std::endl;
        }
        else {
            std::cout << "Error reading fontfile size" << std::endl;
            return;
        }
        std::string headers =  "HTTP/1.1 200 OK\r\nContent-Type: "+content_type+"\r\nContent-Length: "+ std::to_string(font_filesize) + "\r\n\r\n";
        int headers_size = headers.size();
        const char* headers_c_str = headers.c_str();

        if(font_filesize < 1048576) {
            char content_buf[headers_size + font_filesize + 1];

            memcpy(content_buf, headers_c_str, headers_size); //.size() leaves off null-termination in its count

            urlFile.read(content_buf + headers_size, font_filesize);

            content_buf[headers_size + font_filesize] = '\0';

            sendToClient(clientSocket, content_buf, headers_size + font_filesize);

            urlFile.close();
        }
        else {
            char* content_buf = new char[headers_size + font_filesize + 1];

            memcpy(content_buf, headers_c_str, headers_size); //.size() leaves off null-termination in its count

            urlFile.read(content_buf + headers_size, font_filesize);

            content_buf[headers_size + font_filesize] = '\0';

            sendToClient(clientSocket, content_buf, headers_size + font_filesize);

            delete[] content_buf;
            urlFile.close();
        }      
    }
    else
    {
        std::cout << "This file was not opened successfully." << std::endl;
        return;
    }
    
}

bool ytServer::curlLookup(std::string _POST[1], int clientSocket) {
    
    CurlFetcher query(_POST[0].c_str());
    query.fetch();
    
    std::ostringstream post_response;
    int content_length = query.m_get_html.size();
    post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << query.m_get_html;

    int length = post_response.str().size() + 1;

    sendToClient(clientSocket, post_response.str().c_str(), length);
    return true;
}


bool ytServer::showText(std::string _POST[1], int clientSocket) {
    sendToClient(clientSocket, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n", 63);
    printf("%s\n", URIDecode(_POST[0]).c_str());
    //std::system(URIDecode(_POST[0]).c_str()); //DELETE IMMEDIATELY, JUST A TEST, VERY BAD
    return true;
}

bool ytServer::ytSearch(std::string _POST[1], int clientSocket) {
    std::string search_url = "https://www.youtube.com/results?search_query="+_POST[0];
    printf("%s\n", search_url.c_str());
    CurlFetcher yt_query(search_url.c_str()); //the query will have been URIEncoded by JS
    yt_query.fetch();
    int json_start = yt_query.m_get_html.find("ytInitialData") + 16;
    int json_end = yt_query.m_get_html.find("};", json_start) + 1; 

    std::string yt_json = yt_query.m_get_html.substr(json_start, json_end-json_start);

    std::ostringstream post_response;
    int content_length = yt_json.size();
    post_response << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << content_length << "\r\n\r\n" << yt_json;
    int length = post_response.str().size() + 1;

    sendToClient(clientSocket, post_response.str().c_str(), length);

    return true;
}

bool ytServer::ytSearchServerSideParsing(std::string _POST[1], int clientSocket) {
    std::string search_url = "https://www.youtube.com/results?search_query="+_POST[0];
    printf("%s\n", search_url.c_str());
    CurlFetcher yt_query(search_url.c_str()); //the query will have been URIEncoded by JS
    yt_query.fetch();
    int json_start = yt_query.m_get_html.find("ytInitialData") + 16;
    int json_end = yt_query.m_get_html.find("};", json_start) + 1; 

    std::string yt_json = yt_query.m_get_html.substr(json_start, json_end-json_start);

    //cJSON *yt_json_parsed = cJSON_Parse(yt_json.c_str());
    //cJSON_Delete(yt_json_parsed);

    std::ostringstream post_response;
    int content_length = yt_json.size();
    post_response << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << content_length << "\r\n\r\n" << yt_json;
    int length = post_response.str().size() + 1;

    sendToClient(clientSocket, post_response.str().c_str(), length);

    return true;
}

//not used, test of fork() and execvp()
bool ytServer::playMPV_execvp(std::string _POST[2], int clientSocket) {

    pid_t pid = fork();

    if(pid == 0) {
        std::string vid_url = URIDecode(_POST[0]);
        char* format_arg;
        if(_POST[1] == "0") {
            format_arg = (char*)"--ytdl-format=bestvideo[height<=?1080]+bestaudio";
        }
        else {
            format_arg = (char*)"--ytdl-format=ba";
        }
        std::cout << vid_url << std::endl;
        char* args[6] {(char*)"mpv", (char*)"--no-config", format_arg, (char*)"--no-audio-display", (char*)vid_url.c_str(), nullptr};
        
        for(int i = 0; i < 4; i++) {
            printf("%s\n", args[i]);    
        
        }


        if(execvp("mpv", args) == -1) perror("execvp error: ");
        //if(execvp("mpv", nullptr) == -1) perror("execvp error: ");

        return true;
    }

    else {
        std::string playback_response = "random text";

        std::ostringstream post_response;
        int content_length = playback_response.size();
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << content_length << "\r\n\r\n" << playback_response;
        int length = post_response.str().size() + 1;

        sendToClient(clientSocket, post_response.str().c_str(), length);

        return true;
    }

}

bool ytServer::playMPV_stdSystem(std::string _POST[2], int clientSocket) {

    if(m_app_state.mpv_running) {
        std::string playback_response = "a video is already playing";
        std::ostringstream post_response;
        int content_length = playback_response.size();
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << content_length << "\r\n\r\n" << playback_response;
        int length = post_response.str().size() + 1;

        sendToClient(clientSocket, post_response.str().c_str(), length);
        std::cout << "a video is already playing, playback was not initiated\n";
        return false;
    }

    std::string vid_url = URIDecode(_POST[0]);
    
    if(unsafeURL(vid_url)) {
        std::string post_response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        int length = post_response.size() + 1;

        sendToClient(clientSocket, post_response.c_str(), length);
        std::cout << "invalid youtube-url POSTed\n";
        return false;
    }

    std::string format_arg;
    if(_POST[1] == "0") {
        format_arg = "--ytdl-format=\'(bestvideo[vcodec!^=av1])[height<=1440]+bestaudio\' --fullscreen --hwdec=auto --profile=high-quality";
    }
    else {
        format_arg = "--ytdl-format=ba";
    }
    std::string command = "mpv --no-config "+format_arg+" --no-audio-display --input-ipc-server=mpv_socket "+vid_url;
    std::cout << command << std::endl;
    
    m_app_state.mpv_running = true;
    std::thread playbackThread(&ytServer::startMPV, this, command);
    

    std::string playback_response = "mpv command has been executed";

    std::ostringstream post_response;
    int content_length = playback_response.size();
    post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << playback_response;
    int length = post_response.str().size() + 1;

    sendToClient(clientSocket, post_response.str().c_str(), length);

    std::cout << "MPV is currently running in another thread...\n";
    playbackThread.join(); //blocks execution until playbackThread has finished
    std::cout << "MPV has finished execution\n";

    return true;
}

void ytServer::startMPV(const std::string& command) {
     
    memset(&m_unix_sock_address, '\0', sizeof(sockaddr_un));
    m_unix_sock_address.sun_family = AF_UNIX;
    strcpy(m_unix_sock_address.sun_path, "mpv_socket");

    m_ipc_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if(m_ipc_socket == -1) perror("socket() error: ");

    int bind_return = bind(m_ipc_socket, (sockaddr*)&m_unix_sock_address, sizeof(m_unix_sock_address));
    if(bind_return == -1) perror("bind() error: ");
     
    std::system(command.c_str());
    unlink("mpv_socket");
    close(m_client_socket);
    close(m_ipc_socket);
    m_app_state.mpv_running = false;
}

bool ytServer::unsafeURL(const std::string& arg) {

    if(arg.find("https://youtube.com/watch?v=") != 0) return true;
    
    else {
        std::regex ytID_regex("[A-Za-z0-9_-]+");
        return !std::regex_match(arg.substr(28, arg.length() - 28), ytID_regex);
    }
}

bool ytServer::controlMPV(std::string _POST[1], int clientSocket) {
    
    if(m_app_state.mpv_running == false) {
        std::cout << "control input ignored because no video is currently playing\n";
        sendToClient(clientSocket, "HTTP/1.1 204 No Content\r\n\r\n", 28);
        return false;
    }

    int control_code = std::stoi(_POST[0]);
    std::string control_response = "";

    char mpv_command[100];
    char rd_buf[128];
    memset(rd_buf, '\0', 128);
    memset(mpv_command, '\0', 100); 

    m_client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("client socket is: %d\n", m_client_socket);

    if(connect(m_client_socket, (sockaddr*)&m_unix_sock_address, sizeof(m_unix_sock_address)) == -1) perror("connect() error: ");

        switch(control_code) {
            case 1:
                control_response = "pause button pressed";
                //strncpy(mpv_command, "{ \"command\": [\"keypress\", \"SPACE\"] }\n", 100);
                //if(send(m_client_socket, mpv_command, strlen(mpv_command), 0) == -1) perror("send() error: "); 
                //if(recv(m_client_socket, rd_buf, 128, 0) == -1) perror("recv() error: ");
                runMPVCommand("{ \"command\": [\"keypress\", \"SPACE\"] }\n", rd_buf);
                if(m_app_state.paused) m_app_state.paused = false;
                else m_app_state.paused = true;
                memset(rd_buf, '\0', 128);
                runMPVCommand("{ \"command\": [\"get_property\", \"time-pos\"] }\n", rd_buf);
                printf("%s\n", rd_buf);
                break;
            case 2:
                control_response = "rewind button pressed";
                /*strncpy(mpv_command, "{ \"command\": [\"keypress\", \"LEFT\"] }\n", 100);
                if(send(m_client_socket, mpv_command, strlen(mpv_command), 0) == -1) perror("send() error: ");
                if(recv(m_client_socket, rd_buf, 128, 0) == -1) perror("recv() error: "); */
                runMPVCommand("{ \"command\": [\"keypress\", \"LEFT\"] }\n", rd_buf);
                //printf("%s\n", rd_buf);
                break;
            case 3:
                control_response = "fast-forward button pressed";
                /*strncpy(mpv_command, "{ \"command\": [\"keypress\", \"RIGHT\"] }\n", 100);
                if(send(m_client_socket, mpv_command, strlen(mpv_command), 0) == -1) perror("send() error: ");
                if(recv(m_client_socket, rd_buf, 128, 0) == -1) perror("recv() error: "); */
                runMPVCommand("{ \"command\": [\"keypress\", \"RIGHT\"] }\n", rd_buf);
                //printf("%s\n", rd_buf);
                break;
            case 4:
                control_response = "quit button pressed";
                runMPVCommand("{\"command\": [\"keypress\", \"q\"] }\n", rd_buf);
                //m_app_state.mpv_running = false;
                break;
            case 5:
                control_response = "volume button pressed";
                runMPVCommand("{\"command\": [\"keypress\", \"m\"] }\n", rd_buf);
                if(m_app_state.muted) m_app_state.muted = false;
                else m_app_state.muted = true;
                break;
            case 6:
                control_response = "subs button pressed";
                runMPVCommand("{\"command\": [\"keypress\", \"j\"] }\n", rd_buf);
                break;
        }

    std::ostringstream post_response;
    int content_length = control_response.size();
    post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << control_response;
    int length = post_response.str().size() + 1;

    sendToClient(clientSocket, post_response.str().c_str(), length);
    if(close(m_client_socket) == -1) perror("close() error: ");
    return true;
}

bool ytServer::getCurrentTimePos(std::string _POST[1], int clientSocket) {

    if(m_app_state.mpv_running == false) {

        std::string current_time_json = "no vid is running";
        std::cout << current_time_json << "\n";

        std::ostringstream post_response;
        int content_length = current_time_json.size();
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << current_time_json;
        int length = post_response.str().size() + 1;

        sendToClient(clientSocket, post_response.str().c_str(), length);

        return false;
    }

    char mpv_command[100];
    char rd_buf[128];
    memset(rd_buf, '\0', 128);
    memset(mpv_command, '\0', 100); 

    m_client_socket = socket(AF_UNIX, SOCK_STREAM, 0);

    if(connect(m_client_socket, (sockaddr*)&m_unix_sock_address, sizeof(m_unix_sock_address)) == -1) perror("connect() error: ");
    
    strncpy(mpv_command, "{ \"command\": [\"get_property\", \"time-pos\"] }\n", 100);
    if(send(m_client_socket, mpv_command, strlen(mpv_command), 0) == -1) perror("send() error: ");
    if(recv(m_client_socket, rd_buf, 128, 0) == -1) perror("recv() error: ");

    std::string current_time_json(rd_buf);
    std::cout << current_time_json << "\n";

    std::ostringstream post_response;
    int content_length = current_time_json.size();
    post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << current_time_json;
    int length = post_response.str().size() + 1;

    sendToClient(clientSocket, post_response.str().c_str(), length);

    return true;
}

bool ytServer::listenForPlaybackStart(std::string _POST[1], int clientSocket) {

    if(m_app_state.mpv_running == false) {

        std::string response_text = "no vid is running";
        std::cout << response_text << "\n";

        std::ostringstream post_response;
        int content_length = response_text.size();
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << response_text;
        int length = post_response.str().size() + 1;

        sendToClient(clientSocket, post_response.str().c_str(), length);

        return false;
    }

    std::cout << "Entered listenForPlaybackStart() function, sleeping...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Now we can try connecting to the ipc-server socket\n";

    char mpv_command[100];
    char rd_buf[128];
    memset(rd_buf, '\0', 128);
    memset(mpv_command, '\0', 100); 

    m_client_socket = socket(AF_UNIX, SOCK_STREAM, 0);

    if(connect(m_client_socket, (sockaddr*)&m_unix_sock_address, sizeof(m_unix_sock_address)) == -1) {
	perror("listenForPlaybackStart() connect() error: ");
        while(1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if(connect(m_client_socket, (sockaddr*)&m_unix_sock_address, sizeof(m_unix_sock_address)) != -1) break; 
            else perror("still a connect() error: ");
        }
    }
    
    //the ipc-socket seems to get certain events written to it by default, one of which is the "file-loaded" event which fires when playback starts, so what we do here is call recv() on the socket, which blocks until there is something to read off it, continuously until what we read is the file-loaded event message
    //sometimes however there is two events on the socket for some reason so we use string.find() rather than strcmp();
    std::string text_on_ipc_socket = "";
    while(text_on_ipc_socket.find("{\"event\":\"file-loaded\"}\n") == -1) {
        memset(rd_buf, '\0', 128);
        if(recv(m_client_socket, rd_buf, 128, 0) == -1) perror("recv() error: ");
        text_on_ipc_socket = std::string(rd_buf);
        std::cout << "------------------------------------------\n" << text_on_ipc_socket << "------------------------------------------\n";
        //if(!strncmp(rd_buf, "{\"event\":\"file-loaded\"}\n", 25)) break;
    }
    

    if(close(m_client_socket) == -1) perror("close() error: ");
    std::ostringstream post_response;
    std::string response_text = "mpv has started playback";
    int content_length = response_text.size();
    post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << response_text;
    int length = post_response.str().size() + 1;

    sendToClient(clientSocket, post_response.str().c_str(), length);

    return true;
}

void ytServer::runMPVCommand(const char* command, char* response_buf) {
    
    if(send(m_client_socket, command, strlen(command), 0) == -1) perror("send() error: "); //if you include the null-byte in the command then mpv thinks the message is too long since it only reads up to the '\n', and thus it thinks the command has one extra superfluous byte in it which triggers the "ignoring unterminated command" message
    if(recv(m_client_socket, response_buf, 128, 0) == -1) perror("recv() error: "); //if I don't receive what mpv writes back to me then it thinks there is an error when I close the socket; it doesn't actually affect anything and tbh I think calling recv() unnecessarily is bloat

}

bool ytServer::shutdownDevice(std::string _POST[1], int clientSocket) {

    std::string code = URIDecode(_POST[0]);

    if(code == "shut down right this instant") {
        if(!std::system("systemctl poweroff")) {
            std::ostringstream post_response;
            std::string response_text = "youtube server is shutting down";
            int content_length = response_text.size();
            post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << response_text;
            int length = post_response.str().size() + 1;
            sendToClient(clientSocket, post_response.str().c_str(), length);
            return true;
        }
        else {
            std::ostringstream post_response;
            std::string response_text = "poweroff command didn't work";
            int content_length = response_text.size();
            post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << response_text;
            int length = post_response.str().size() + 1;
            sendToClient(clientSocket, post_response.str().c_str(), length);
            return false;
        }
    }
    else {
        std::cout << "shutdown code is wrong\n";
        std::string response_text = "shutdown code is wrong";
        std::ostringstream post_response;
        int content_length = response_text.size();
        post_response << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << content_length << "\r\n\r\n" << response_text;
        int length = post_response.str().size() + 1;
        sendToClient(clientSocket, post_response.str().c_str(), length);
        return false;
    }
}
