#include "TcpListener.h"
#include <fstream>
#include "CurlFetcher.cpp"
#include <sys/un.h>

struct app_state {
    bool mpv_running;
    bool paused;
    bool muted;
    short int pageno;
    double seek_percentage;
    std::string search_query;
    std::string js_deets_array;

};

class ytServer : public TcpListener {
    public:
        ytServer(const char *ipAddress, int port, bool show_output) : TcpListener(ipAddress, port), m_show_output{show_output}/*, m_MPV_running{false}*/ {
            if(!m_show_output) std::cout.setstate(std::ios_base::failbit);
            memset(&m_app_state, '\0', sizeof(app_state));
            m_app_state.mpv_running = false;
            m_app_state.paused = false;
            m_app_state.muted = false;
            m_app_state.pageno = 1;
            m_app_state.seek_percentage = 0.0;        
        }

    protected:
        //virtual void onClientConnected(int clientSocket);
        //virtual void onClientDisconnected(int clientSocket);
        virtual void onMessageReceived(int clientSocket, const char *msg, int length);

    private:
        int getRequestType(const char* msg);     
        
        bool c_strStartsWith(const char* str1, const char* str2);
        int c_strFind(const char* haystack, const char* needle);
        int c_strFindNearest(const char* haystack, const char* needle1, const char* needle2);

        void buildGETContent(short int page_type, char* url_c_str, std::string &content, bool cookies_present);
        void insertTextSelect(std::ostringstream &html);
        void insertLangSelect(std::ostringstream &html);
        void sendBinaryFile(char* url_c_str, int clientSocket, const std::string &content_type);
        
        int checkHeaderEnd(const char* msg);
        void buildPOSTedData(const char* msg, bool headers_present, int length);
        void setURL(const char* msg);
        int getPostFields(const char* url);
        void handlePOSTedData(const char* post_data, int clientSocket);
        bool readCookie(std::string cookie[3], const char* msg);

        bool curlLookup(std::string _POST[1], int clientSocket);
        std::string URIDecode(std::string &text);
        std::string htmlspecialchars(const std::string &innerHTML);
        std::string escapeQuotes(const std::string &quoteystring);

        bool showText(std::string _POST[1], int clientSocket);
        bool ytSearch(std::string _POST[1], int clientSocket);
        bool ytSearchServerSideParsing(std::string _POST[1], int clientSocket);

        bool playMPV_execvp(std::string _POST[2], int clientSocket);
        bool playMPV_stdSystem(std::string _POST[2], int clientSocket);

        void startMPV(const std::string& command);
        bool controlMPV(std::string _POST[1], int clientSocket);
        bool unsafeURL(const std::string& arg);

        bool getCurrentTimePos(std::string _POST[1], int clientSocket);
        bool listenForPlaybackStart(std::string _POST[1], int clientSocket);

        bool shutdownDevice(std::string _POST[1], int clientSocket);

        void runMPVCommand(const char* command, char* response_buf);

        const char*         m_post_data;
        std::string         m_post_data_incomplete;
        int                 m_total_post_bytes;
        int                 m_bytes_handled;
        bool                m_POST_continue;
        char                m_url[50]; //only applies to POST urls; you can crash the server by sending it a POST request with a >50 char url but not by having long-named GETted resource amongst the HTML_DOCS
        std::string         m_cookies[3] {"1", "1", "1"};
        bool                m_show_output;

        int m_ipc_socket;
        int m_client_socket;
        sockaddr_un m_unix_sock_address;
        app_state m_app_state;



};
