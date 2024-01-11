#include "TcpListener.h"
#include <fstream>
#include <sqlite3.h>
#include <unicode/unistr.h>
#include <unicode/regex.h>
#include <unicode/brkiter.h>
#include "CurlFetcher.cpp"

class ytServer : public TcpListener {
    public:
        ytServer(const char *ipAddress, int port, bool show_output) : TcpListener(ipAddress, port), m_DB_path{"Kazakh.db"}, m_show_output{show_output} {
            if(!m_show_output) std::cout.setstate(std::ios_base::failbit);           
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

        bool playMPV(std::string _POST[2], int clientSocket);

        const char*         m_post_data;
        std::string         m_post_data_incomplete;
        int                 m_total_post_bytes;
        int                 m_bytes_handled;
        bool                m_POST_continue;
        char                m_url[50]; //only applies to POST urls; you can crash the server by sending it a POST request with a >50 char url but not by having long-named GETted resource amongst the HTML_DOCS
        const char*         m_DB_path;
        std::string         m_cookies[3] {"1", "1", "1"};
        bool                m_show_output;

};
