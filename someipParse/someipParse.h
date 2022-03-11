#ifndef SOMEIPPARSE_H
#define SOMEIPPARSE_H
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
    
    someipParse() = default;
    ~someipParse() = default;
    
    bool parse(Buffer& buff){
        if(buff.ReadableBytes() < 16) {
            if(buff.ReadableBytes() > 0) buff.RetrieveAll();
            return false; 
        }
        bool ret = true;
        while(buff.ReadableBytes() && state_ != FINISH) {
            switch(state_)
            {
                case HEAD:
                    ParseHead(buff);
                    break;
                case PAYLOAD:
                    ParsePayload(buff, ret);
                    break;
                default:
                    break;
            }
            if(buff.ReadableBytes() <= 0) break;
        }
        return ret;
    }

    std::uint32_t length;
    std::uint16_t service_id, method_id, client_id, session_id;
    std::uint8_t message_type, protocal_version, interface_version, return_code;
    std::string pay_load_;
    std::string PayLoad;

private:

    PARSE_STATUS state_ = HEAD;

    void ParseHead(Buffer& buff) {
        uint32_t* tmp_ptr_32;
        uint16_t* tmp_ptr_16;
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

        if(length <= 8) state_ = FINISH;
        else state_ = PAYLOAD;
    };

    void ParsePayload(Buffer& buff, bool& ret){
        uint32_t byte_to_read = length - 8;
        if(buff.ReadableBytes() < byte_to_read) {
            buff.RetrieveAll();
            state_ = FINISH;
            ret = false;
            return;
        }
        pay_load_.clear();

        auto cur_ptr = buff.Peek();
        auto end_ptr = cur_ptr + byte_to_read;
        while(cur_ptr != end_ptr) {
            pay_load_.push_back(*cur_ptr++);
        }
        buff.Retrieve(byte_to_read);
        state_ = FINISH;
    }


};


#endif //SOMEIPPARSE_H