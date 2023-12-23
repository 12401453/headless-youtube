#include <curl/curl.h>
#include <string>
#include <iostream>

class CurlFetcher {
    public:
        CurlFetcher(const char* dict_url, std::string curl_cookies="") {
            m_dict_url = dict_url;
            m_curl_cookies = curl_cookies;
        }
        void fetch() {
            CURL *curl;
            CURLcode res;

            curl = curl_easy_init();

            if(curl) {
                curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36");
                curl_easy_setopt(curl, CURLOPT_URL, m_dict_url);
                curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

                if(!m_curl_cookies.empty()) {
                    curl_easy_setopt(curl, CURLOPT_COOKIE, m_curl_cookies.c_str());

                }

                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &m_get_html);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

                /* Perform the request, res will get the return code */
                res = curl_easy_perform(curl);
                /* Check for errors */
                if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                }

                curl_easy_cleanup(curl);
                
            }
        }
        std::string m_get_html;
    private:
                //here void *userdata is the pointer passed into curl_easy_setopt(curl, CURLOPT_WRITEDATA, &m_get_html); since this is a std::string pointer I have to cast it to that type before I can use it
        static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {

            ((std::string*)userdata)->append(ptr, size*nmemb); //has to be std::string::append not ::assign, because this callback writes a max of 16Kb and repeats itself until the end of the data, so assign just overwrites itself repeatedly and leaves you with just the final 16Kb of the HTML page
            return size*nmemb;
        }

        const char* m_dict_url;
        std::string m_curl_cookies;
};
