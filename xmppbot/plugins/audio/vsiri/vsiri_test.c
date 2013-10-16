#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

int main(int argc, char *argv[])
{
  CURL *curl;
  CURLcode res;
  struct curl_slist slist = {"Content-Type: audio/x-speex-with-header-byte; rate=16000", NULL};
//  struct curl_httppost *formpost=NULL;

  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://www.google.com/speech-api/v1/recognize?"
                     "xjerr=1&client=chromium&lang=ru-RU");
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, &slist);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
                     "qqqiqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq");

    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    curl_easy_cleanup(curl);
//    curl_formfree(formpost);
//    curl_slist_free_all (headerlist);
  }
  return 0;
}
