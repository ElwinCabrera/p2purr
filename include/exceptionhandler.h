#ifndef EXCEPTION_HANDLER_H
#define EXCEPTION_HANDLER_H

#include "global_config.h"


#include <iostream> 
#include <netdb.h>
#include <stdio.h>
#include <exception>
#include <string.h>
#include <cerrno>


using std::cout;
using std::endl;
using std::string;




class GenericException : public std::exception {
private:
    const char *message;
    int status_code;

public:
    //GenericException(int status_code=-1) {}
    GenericException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    virtual const char* what () {
        std::cout << "Exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class SocketException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    //SocketException() {}
    //SocketException(int status_code=-1) {}
    SocketException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code) {}
    
    const char* what () {
        std::cout << "'socket(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class SocketOptionsException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    SocketOptionsException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'setsockopt(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class BindException : public GenericException {
private:
    const char *message ;
    int status_code;
public:
   
    BindException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code) {}
    
    const char* what () {
        std::cout << "'bind(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class ListenException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    ListenException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'listen(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class AcceptException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    AcceptException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return "";
        }
        std::cout << "'accept(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class ConnectException : public GenericException {
private:
    const char *message ;
    int status_code;
public:
    
    ConnectException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'connect(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};


class SendException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    SendException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'send(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};


class ReceiveException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    ReceiveException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        // errno of EAGAIN or EWOULDBLOCK in recv means that the socket is marked nonblocking and the receive operation
        // would block, or a receive timeout had been set and the
        // timeout expired before data was received
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return "";
        }
        std::cout << "'recv(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};


class GetAddrInfoException : public GenericException {
private:
    const char *message;
    int ai_ret_code;
public:
    
    GetAddrInfoException(const char *msg="", int ai_ret_code=-1) : message(msg), ai_ret_code(ai_ret_code){}
    
    const char* what () {
        std::cout << "'getaddrinfo(..)' exception occured" << std::endl;
        std::cout << "addrinfo return code: " << ai_ret_code << " -> " << gai_strerror(ai_ret_code) << std::endl;
        //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ai_ret_code));
        //std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class SocketCloseException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    SocketCloseException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'close(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class SocketShutdownException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    SocketShutdownException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'shutdown(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class FCTRLException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    FCTRLException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'fctrl(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class PollException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    PollException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "'poll(..)' exception occured" << std::endl;
        std::cout << "Last errno saved: " << errno << " -> " << strerror(errno) << std::endl;
        return message;
    }
};

class PacketException : public GenericException {
private:
    const char *message;
    int status_code;
public:
    
    PacketException(const char *msg="", int status_code=-1) : message(msg), status_code(status_code){}
    
    const char* what () {
        std::cout << "exception occured while handling a packet" << std::endl;
        return message;
    }
};

#endif //EXCEPTION_HANDLER_H