#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#ifdef linux
//linux only headers
#endif

#ifdef _WIN32
 typedef SOCKET sock_t;
#else
 typedef int sock_t;
#endif


#undef _WIN32

#endif