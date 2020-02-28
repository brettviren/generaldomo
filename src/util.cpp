#include <chrono>
#include <signal.h>
using namespace generaldomo


util::routing_id_t util::recv_serverish(zmq::socket_t& sock, zmq::multipart_t& mmsg)
{
    int stype = sock.getsockopt<int>(ZMQ_TYPE);
    if (zmq::sock_type::server == stype) {
        zmq::message_t msg;
        auto res = sock.recv(msg);
        util::routing_id_t ret = msg.routing_id(msg);
        mmsg.clear();
        mmsg.decode(msg);       // doesn't clear
        return ret;
    }
    if (zmq::socket_type::router == stype) {
        mmsg.recv(sock);        // note, clears any previous junk
        size_t ind=0, nparts = mmsg.size();
        zmq::multipart_t* env = new zmq::multipart_t;
        while (mmsg.size()) {
            env->add(mmsg.pop());
            if (env->back().empty()) { // that was the envelop end
                break;
            }
        }
        util::routing_id_t ret = env;
        return ret;
    }
}

util::routing_id_t util::recv_clientish(zmq::socket_t& sock, zmq::multipart_t& mmsg)
{
}


int64_t util::now_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();    
}



static int s_interrupted = 0;
void s_signal_handler (int signal_value)
{
    s_interrupted = 1;
}
bool util::interrupted() 
{
    return s_interrupted==1; 
}

// Call from main()
void util::catch_signals ()
{
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);
    sigaction (SIGINT, &action, NULL);
    sigaction (SIGTERM, &action, NULL);
}
