#include "generaldomo/util.hpp"
#include <chrono>
#include <thread>
#include <signal.h>

using namespace generaldomo;


remote_identity_t recv_serverish(zmq::socket_t& sock,
                                 zmq::multipart_t& mmsg)
{
    int stype = sock.getsockopt<int>(ZMQ_TYPE);
    if (ZMQ_SERVER == stype) {
        return recv_server(sock, mmsg);
    }
    if(ZMQ_ROUTER == stype) {
        return recv_router(sock, mmsg);
    }
    throw std::runtime_error("recv requires SERVER or ROUTER socket");
}


remote_identity_t recv_server(zmq::socket_t& server_socket,
                              zmq::multipart_t& mmsg)
{
    zmq::message_t msg;
    auto res = server_socket.recv(msg, zmq::recv_flags::none);
    uint32_t routing_id = msg.routing_id();
    mmsg.decode(msg);
    return remote_identity_t((char*)&routing_id, 4);
}

remote_identity_t recv_router(zmq::socket_t& router_socket,
                              zmq::multipart_t& mmsg)
{
    mmsg.recv(router_socket);
    remote_identity_t rid = mmsg.popstr();
    mmsg.pop();                 // empty
    return rid;
}


void send_serverish(zmq::socket_t& sock,
                    zmq::multipart_t& mmsg, remote_identity_t rid)
{
    int stype = sock.getsockopt<int>(ZMQ_TYPE);
    if (ZMQ_SERVER == stype) {
        return send_server(sock, mmsg, rid);
    }
    if(ZMQ_ROUTER == stype) {
        return send_router(sock, mmsg, rid);
    }
    throw std::runtime_error("send requires SERVER or ROUTER socket");
}

void send_server(zmq::socket_t& server_socket,
                 zmq::multipart_t& mmsg, remote_identity_t rid)
{
    zmq::message_t msg = mmsg.encode();
    const char* d = rid.data();
    uint32_t routing_id = d[0] << 24 | d[1] << 16 | d[2] << 8 | d[3];
    msg.set_routing_id(routing_id);
    server_socket.send(msg, zmq::send_flags::none);
}

void send_router(zmq::socket_t& router_socket,
                 zmq::multipart_t& mmsg, remote_identity_t rid)
{
    mmsg.pushmem(NULL, 0);
    mmsg.pushstr(rid);
    mmsg.send(router_socket);
}


void recv_clientish(zmq::socket_t& socket,
                    zmq::multipart_t& mmsg)
{
    int stype = socket.getsockopt<int>(ZMQ_TYPE);
    if (ZMQ_CLIENT == stype) {
        recv_client(socket, mmsg);
        return;
    }
    if(ZMQ_DEALER == stype) {
        recv_dealer(socket, mmsg);
        return;
    }
    throw std::runtime_error("recv requires CLIENT or DEALER socket");
}

void recv_client(zmq::socket_t& client_socket,
                 zmq::multipart_t& mmsg)
{
    zmq::message_t msg;
    auto res = client_socket.recv(msg, zmq::recv_flags::none);
    mmsg.decode(msg);
    return;
}


void recv_dealer(zmq::socket_t& dealer_socket,
                 zmq::multipart_t& mmsg)
{
    mmsg.recv(dealer_socket);
    mmsg.pop();                 // fake being REQ
    return;
}
    

void send_clientish(zmq::socket_t& socket,
                    zmq::multipart_t& mmsg)
{
    int stype = socket.getsockopt<int>(ZMQ_TYPE);
    if (ZMQ_CLIENT == stype) {
        send_client(socket, mmsg);
        return;
    }
    if(ZMQ_DEALER == stype) {
        send_dealer(socket, mmsg);
        return;
    }
    throw std::runtime_error("send requires CLIENT or DEALER socket");
}

void send_client(zmq::socket_t& client_socket,
                 zmq::multipart_t& mmsg)
{
    zmq::message_t msg = mmsg.encode();
    client_socket.send(msg, zmq::send_flags::none);
}

void send_dealer(zmq::socket_t& dealer_socket,
                 zmq::multipart_t& mmsg)
{
    mmsg.pushmem(NULL,0);       // pretend to be REQ
    mmsg.send(dealer_socket);
}




std::chrono::milliseconds now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

void sleep_ms(std::chrono::milliseconds zzz)
{
    std::this_thread::sleep_for(zzz);
}

static int s_interrupted = 0;
void s_signal_handler (int signal_value)
{
    s_interrupted = 1;
}
bool interrupted() 
{
    return s_interrupted==1; 
}

// Call from main()
void catch_signals ()
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}
