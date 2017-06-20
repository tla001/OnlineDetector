#include "imagedeal.h"
#include "mainwindow.h"

extern MainWindow *w;

size_t WriteFunction(void *input, size_t uSize, size_t uCount, void *arg) {
    size_t uLen = uSize * uCount;
    string *pStr = (string*) (arg);
    pStr->append((char*) (input), uLen);
    return uLen;
}
ImageDeal::ImageDeal(){
    code = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (code != CURLE_OK) {
        cout << "curl global init err" << endl;
        return;
    }
    pCurl = curl_easy_init();
    if (pCurl == NULL) {
        cout << "curl easy init err" << endl;
        return;
    }
    url="";
    fileName="";
    timeout=10;
}
ImageDeal::~ImageDeal(){
    curl_easy_cleanup(pCurl);
    curl_global_cleanup();
}

void ImageDeal::init(string _url,string _fileName, int _timeout){
    url=_url;
    fileName=_fileName;
    timeout=_timeout;
}

void ImageDeal::doWork(){
    curl_slist *pHeaders = NULL;
    string sBuffer;
    string header = "username:tla001";
    pHeaders = curl_slist_append(pHeaders, header.c_str());

    ifstream in;
    in.open(fileName, ios::in | ios::binary);
    if (!in.is_open()) {
        printf("open err\n");
        exit(-1);
    }
    in.seekg(0, ios_base::end);
    const size_t maxSize = in.tellg();
    in.seekg(0);
    char * picBin = new char[maxSize];
    in.read(picBin, maxSize);
    in.close();
    cout << maxSize << endl;

    size_t sendSize = maxSize + sizeof(size_t);
    char *sendBuff = new char[sendSize];
    //	sprintf(sendBuff, "%d", maxSize);
    memcpy(sendBuff, &maxSize, sizeof(size_t));
    //	size_t tmp = 0;
    //	memcpy(&tmp, sendBuff, sizeof(size_t));
    //	cout << "tmp=" << tmp << endl;
    memcpy(sendBuff + sizeof(size_t), picBin, maxSize);
    curl_easy_setopt(pCurl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHeaders);
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, timeout);
    //	curl_easy_setopt(pCurl, CURLOPT_HEADER, 1);
    curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, sendBuff);
    curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, sendSize);
    curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &WriteFunction);
    curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &sBuffer);

    code = curl_easy_perform(pCurl);
    if (code != CURLE_OK) {
        memset(buff,0,sizeof(buff));
        sprintf(buff,"3curl perform err,retcode=%d\n",code);
        w->doDispInfoWrite(buff);
//        cout << "curl perform err,retcode="<<code << endl;
        return;
    }
    long retcode = 0;
    code = curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &retcode);
    if (code != CURLE_OK) {
//        cout << "curl perform err" << endl;
        memset(buff,0,sizeof(buff));
        sprintf(buff,"3curl perform err\n");
        w->doDispInfoWrite(buff);
        return;
    }
    //cout << "[http return code]: " << retcode << endl;
    //cout << "[http context]: " << endl << sBuffer << endl;
    using rapidjson::Document;
    Document doc;
    doc.Parse<0>(sBuffer.c_str());
    if (doc.HasParseError()) {
        rapidjson::ParseErrorCode code = doc.GetParseError();
        psln(code);
        return;
    }
    ostringstream oss;
    oss.str("");
    oss<<3;
    using rapidjson::Value;
    Value &content = doc["content"];
    if (content.IsArray()) {
        for (int i = 0; i < content.Size(); i++) {
            Value &v = content[i];
            assert(v.IsObject());
            oss<<"object "<<"["<<i+1<<"]"<<endl;
            if (v.HasMember("class") && v["class"].IsString()) {
                oss <<" [class]:"<<v["class"].GetString()<<endl;
            }
            if (v.HasMember("prob") && v["prob"].IsDouble()) {
                oss <<" [prob]:"<<v["prob"].GetDouble()<<endl;
            }
            oss<<" ***************************"<<endl;
            if (v.HasMember("left") && v["left"].IsInt()) {
                oss <<" [left]:"<<v["left"].GetInt()<<endl;
            }
            if (v.HasMember("right") && v["right"].IsInt()) {
                oss <<" [right]:"<<v["right"].GetInt()<<endl;
            }
            if (v.HasMember("top") && v["top"].IsInt()) {
                oss <<" [top]:"<<v["top"].GetInt()<<endl;
            }
            if (v.HasMember("bot") && v["bot"].IsInt()) {
                oss <<" [bot]:"<<v["bot"].GetInt()<<endl;
            }
            oss<<endl;
        }
    }
    memset(buff,0,sizeof(buff));
    sprintf(buff,"%s",oss.str().c_str());
    w->doDispInfoWrite(buff);
    delete[] picBin;
    delete[] sendBuff;
}
