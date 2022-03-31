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
//#include "../cereal/archives/binary.hpp"
//#include "../cereal/types/memory.hpp"
//#include "../cereal/types/string.hpp"
#include <sstream>
#include "../pool/sqlconnpool.h"


class someipParse {
public:

    // enum MESSAGE_TYPE { 
    //     REQUEST = 0,
    //     REQUEST_NO_RETURN,
    //     NOTIFICATION,
    //     RESPONSE,
    //     ERROR
    // };

    enum PARSE_STATUS {
        HEAD,
        PAYLOAD,
        FINISH
    };
    
    someipParse() = default;
    ~someipParse() = default;
    
    void parse(Buffer& buff){
        //bool ret = true;
        while(buff.ReadableBytes() && state_ != FINISH) {
            //std::cout<<buff.ReadableBytes()<<std::endl;
            switch(state_)
            {
                case HEAD:
                    ParseHead(buff);
                    //std::cout<<buff.ReadableBytes()<<std::endl;
                    break;
                case PAYLOAD:
                    ParsePayload(buff);
                    break;
                default:
                    break;
            }
            if(buff.ReadableBytes() <= 0) break;
        }
        state_ = HEAD;
        //return ret;
    }

    std::uint32_t length;
    std::uint16_t service_id, method_id, client_id, session_id, message_type;
    std::uint8_t message_type_tmp, protocal_version, interface_version, return_code;
    std::string pay_load_binary;
    std::string PayLoad_real = "abc";

    // struct someip_string_PayLoad{
    //     std::string buf;
    //     template<typename Archive> void serialize(Archive& ar) {
    //         ar(buf);
    //     }
    // };

private:
    //someip_string_PayLoad PayLoad_struct;
    PARSE_STATUS state_ = HEAD;

    void ParseHead(Buffer& buff) {
        uint32_t* tmp_ptr_32;
        uint16_t* tmp_ptr_16;
        //service_id 
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        service_id = htons(*tmp_ptr_16);
        buff.Retrieve(2);
        //std::cout<<"service_id  "<< service_id <<std::endl;
        //method_id
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        method_id = htons(*tmp_ptr_16);
        buff.Retrieve(2);
        //std::cout<<"method_id  "<< method_id <<std::endl;
        //length
        tmp_ptr_32 = (std::uint32_t* )buff.Peek();
        length = htonl(*tmp_ptr_32);
        buff.Retrieve(4);
        //std::cout<<"length  "<< length <<std::endl;
        //client id
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        client_id = htons(*tmp_ptr_16);
        buff.Retrieve(2);
        //std::cout<<"client_id  "<< client_id <<std::endl;
        //session id
        tmp_ptr_16 = (std::uint16_t* )buff.Peek();
        session_id = htons(*tmp_ptr_16);
        buff.Retrieve(2);
        //std::cout<<"session_id  "<< session_id <<std::endl;
        //std::cout<<buff.ReadableBytes()<<std::endl;
        //protocol version 固定为0x01
        protocal_version = (*buff.Peek());
        buff.Retrieve(1);
        //std::cout<<"protocal_version  "<< (int)protocal_version <<std::endl;
        //Interface Version
        interface_version = (*buff.Peek());
        buff.Retrieve(1);
        //std::cout<<"interface_version  "<< interface_version <<std::endl;
        //Message Type
        message_type_tmp = (*buff.Peek());
        message_type = (std::uint16_t)message_type_tmp;
        buff.Retrieve(1);
        //std::cout<<"message_type  "<< message_type <<std::endl;
        //return code
        return_code = (*buff.Peek());
        buff.Retrieve(1);
        //std::cout<<"return_code  "<< return_code <<std::endl;
        //std::cout<<buff.ReadableBytes()<<std::endl;
        if(length <= 8) state_ = FINISH;
        else state_ = PAYLOAD;
    };

    void ParsePayload(Buffer& buff){
        uint32_t byte_to_read = length - 8;
        if(buff.ReadableBytes() < byte_to_read) {
            buff.RetrieveAll();
            state_ = FINISH;
            //ret = false;
            return;
        }
        pay_load_binary.clear();

        auto cur_ptr = buff.Peek();
        auto end_ptr = cur_ptr + byte_to_read;
        while(cur_ptr != end_ptr) {
            pay_load_binary.push_back(*cur_ptr++);
        }
        buff.Retrieve(byte_to_read);
        // {
        //     std::istringstream iss(pay_load_binary);
        //     cereal::BinaryInputArchive archive(iss);
        //     archive(PayLoad_struct);
        // }
        // PayLoad_real = std::move(PayLoad_struct.buf);
        PayLoad_real = std::move(pay_load_binary);
        state_ = FINISH;
    }
};


#endif //SOMEIPPARSE_H