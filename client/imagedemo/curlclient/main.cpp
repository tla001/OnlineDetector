extern "C"{
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

//iso
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//others
#include "curl/curl.h"
}

//c++
#include <iostream>
#include <string>
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#define psln(x) std::cout << #x " = " << (x) << std::endl

using namespace std;

size_t WriteFunction(void *input, size_t uSize, size_t uCount, void *arg) {
	size_t uLen = uSize * uCount;
	string *pStr = (string*) (arg);
	pStr->append((char*) (input), uLen);
	return uLen;
}

int main(int argc,char **argv){
	if(argc<3){
		printf("usage:./a.out uri pic\n");
		exit(-1);
	}
	CURL *pCurl = NULL;
	CURLcode code;
	code = curl_global_init(CURL_GLOBAL_DEFAULT);
	if (code != CURLE_OK) {
		cout << "curl global init err" << endl;
		return -1;
	}
	pCurl = curl_easy_init();
	if (pCurl == NULL) {
		cout << "curl easy init err" << endl;
		return -1;
	}

	curl_slist *pHeaders = NULL;
	string sBuffer;
	string header = "username:tla001";
	pHeaders = curl_slist_append(pHeaders, header.c_str());

	ifstream in;
	in.open(argv[2], ios::in | ios::binary);
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
	curl_easy_setopt(pCurl, CURLOPT_URL, argv[1]);
	curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, pHeaders);
	curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 20);
	//	curl_easy_setopt(pCurl, CURLOPT_HEADER, 1);
	curl_easy_setopt(pCurl, CURLOPT_POST, 1L);
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, sendBuff);
	curl_easy_setopt(pCurl, CURLOPT_POSTFIELDSIZE, sendSize);
	curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, &WriteFunction);
	curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &sBuffer);

	code = curl_easy_perform(pCurl);
	if (code != CURLE_OK) {
		cout << "curl perform err,retcode="<<code << endl;
		return -1;
	}
	long retcode = 0;
	code = curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &retcode);
	if (code != CURLE_OK) {
		cout << "curl perform err" << endl;
		return -1;
	}
	//cout << "[http return code]: " << retcode << endl;
	//cout << "[http context]: " << endl << sBuffer << endl;
	using rapidjson::Document;
	Document doc;
	doc.Parse<0>(sBuffer.c_str());
	if (doc.HasParseError()) {
		rapidjson::ParseErrorCode code = doc.GetParseError();
		psln(code);
		return -1;
	}
	using rapidjson::Value;
	Value &content = doc["content"];
	if (content.IsArray()) {
		for (int i = 0; i < content.Size(); i++) {
			Value &v = content[i];
			assert(v.IsObject());
			cout<<"object "<<"["<<i+1<<"]"<<endl;
			if (v.HasMember("class") && v["class"].IsString()) {
				cout <<"\t[class]:"<<v["class"].GetString()<<endl;
			}
			if (v.HasMember("prob") && v["prob"].IsDouble()) {
				cout <<"\t[prob]:"<<v["prob"].GetDouble()<<endl;
			}
			cout<<"\t***************************"<<endl;
			if (v.HasMember("left") && v["left"].IsInt()) {
				cout <<"\t[left]:"<<v["left"].GetInt()<<endl;
			}
			if (v.HasMember("right") && v["right"].IsInt()) {
				cout <<"\t[right]:"<<v["right"].GetInt()<<endl;
			}
			if (v.HasMember("top") && v["top"].IsInt()) {
				cout <<"\t[top]:"<<v["top"].GetInt()<<endl;
			}
			if (v.HasMember("bot") && v["bot"].IsInt()) {
				cout <<"\t[bot]:"<<v["bot"].GetInt()<<endl;
			}
			cout<<endl;

		}
	}

	delete[] picBin;
	delete[] sendBuff;
	curl_easy_cleanup(pCurl);

	curl_global_cleanup();
	return 0;
}
