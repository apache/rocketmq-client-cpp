#include "curl/curl.h"
#include <iostream>

size_t write_callback(void* contents, size_t size, size_t nmemb, void* user_param) {
  std::string result;
  result.reserve(size * nmemb);
  result.append(reinterpret_cast<const char*>(contents), size * nmemb);
  std::cout << "Callback invoked. Size: " << size << ", nmemb: " << nmemb << ". Content: " << result << std::endl;
  return size * nmemb;
}

int main(int argc, char* argv[]) {
  CURL* curl;
  CURLcode res;
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  const char* url = "http://www.taobao.com";
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "curl_easy_perform() failed" << curl_easy_strerror(res) << std::endl;
    }
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  return 0;
}