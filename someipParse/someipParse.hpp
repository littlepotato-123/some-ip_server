/*
 * @Author       : mark
 * @Date         : 2020-06-25
 * @copyleft Apache 2.0
 */ 
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <arpa/inet.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
//#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class someipParse {
public:

    enum MESSAGE_TYPE { 
        REQUEST = 0,
        REQUEST_NO_RETURN,
        NOTIFICATION,
        RESPONSE,
        ERROR
    };

    enum PARSE_STATUS {
        HEAD,
        PAYLOAD,
        FINISH
    };
    
    // enum HTTP_CODE {
    //     NO_REQUEST = 0,
    //     GET_REQUEST,
    //     BAD_REQUEST,
    //     NO_RESOURSE,
    //     FORBIDDENT_REQUEST,
    //     FILE_REQUEST,
    //     INTERNAL_ERROR,
    //     CLOSED_CONNECTION,
    // };
    
    someipParse() { Init(); }
    ~someipParse() = default;

    void Init();
    bool parse(Buffer& buff){
        if(buff.ReadableBytes() < 16) {
            if(buff.ReadableBytes() > 0) buff.RetrieveAll();
            return false; 
        }
        while(buff.ReadableBytes() && state_ != FINISH) {
            switch(state_)
            {
                case HEAD:
                    ParseHead(buff);
                    break;
                case PAYLOAD:
                    ParsePayload(buff);
                    break;
                default:
                    break;
            }
            if(buff.ReadableBytes() <= 0) break;
        }
        return true;
    }

    //std::string path() const;
    //std::string& path();
    //std::string method() const;
    //std::string version() const;
    //std::string GetPost(const std::string& key) const;
    //std::string GetPost(const char* key) const;

    //bool IsKeepAlive() const;

    /* 
    todo 
    void HttpConn::ParseFormData() {}
    void HttpConn::ParseJson() {}
    */

private:
    //bool ParseRequestLine_(const std::string& line);
    PARSE_STATUS state_ = HEAD;

    void ParseHead(Buffer& buff) {
        uint32_t* tmp_ptr_32;
        uint16_t* tmp_ptr_16;
        //uint8_t* tmp_ptr_8;
        //service_id 
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        service_id = ntohs(*tmp_ptr_16);
        buff.Retrieve(2);
        //method_id
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        method_id = ntohs(*tmp_ptr_16);
        buff.Retrieve(2);
        //length
        tmp_ptr_32 = (std::uint32_t* )buff.Peek();
        length = ntohl(*tmp_ptr_32);
        buff.Retrieve(4);
        //client id
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        client_id = ntohs(*tmp_ptr_16);
        buff.Retrieve(2);
        //session id
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        session_id = ntohs(*tmp_ptr_16);
        buff.Retrieve(2);
        //protocol version 固定为0x01
        protocal_version = (*buff.Peek());
        buff.Retrieve(1);
        //Interface Version
        interface_version = (*buff.Peek());
        buff.Retrieve(1);
        //Message Type
        message_type = (*buff.Peek());
        buff.Retrieve(1);
        //return code
        return_code = (*buff.Peek());
        buff.Retrieve(1);
        if(buff.ReadableBytes() <= 0) state_ = FINISH;
        else state_ = PAYLOAD;
    };

    void ParsePayload(Buffer& buff){
        auto cur_ptr = buff.Peek();
        auto end_prt = buff.BeginWrite();
        while(cur_ptr != end_prt) {
            pay_load_.emplace_back(*cur_ptr++);
        }
        buff.RetrieveAll();
        state_ = FINISH;
    }

    //void ParsePath_();
    //void ParsePost_();
    //void ParseFromUrlencoded_();

    //static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);

   // PARSE_STATE state_;


    std::uint32_t length;
    std::uint16_t service_id, method_id, client_id, session_id;
    std::uint8_t message_type, protocal_version, interface_version, return_code;
    
    std::unordered_map<std::string, std::string> header_;
    //std::unordered_map<std::string, std::string> pay_load;
    std::vector<std::uint8_t> pay_load_;
    //static const std::unordered_set<std::string> DEFAULT_HTML;
    //static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    //static int ConverHex(char ch);   
};


#endif //HTTP_REQUEST_H