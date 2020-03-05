#include "generaldomo/worker.hpp"
#include "generaldomo/protocol.hpp"


using namespace generaldomo;

Worker::Worker(zmq::socket_t& sock, std::string broker_address,
               std::string service, logbase_t& log)
    : m_sock(sock)
    , m_address(broker_address)
    , m_service(service)
    , m_log(log)
{
    int stype = m_sock.getsockopt<int>(ZMQ_TYPE);
    if (ZMQ_CLIENT == stype) {
        recv = recv_client;
        send = send_client;
    }
    else if (ZMQ_DEALER == stype) {
        recv = recv_dealer;
        send = send_dealer;
    }
    else {
        throw std::runtime_error("worker must be given DEALER or CLIENT socket");
    }

    connect_to_broker(false);
}

Worker::~Worker() { }

void Worker::connect_to_broker(bool reconnect)
{
    if (reconnect) {
        m_sock.disconnect(m_address);
    }

    int linger=0;
    m_sock.setsockopt(ZMQ_LINGER, linger);
    // set socket routing ID?
    m_sock.connect(m_address);
    m_log.debug("worker connect to " + m_address);

    zmq::multipart_t mmsg;
    mmsg.pushstr(m_service);          // 3
    mmsg.pushstr(mdp::worker::ready); // 2
    mmsg.pushstr(mdp::worker::ident); // 1
    send(m_sock, mmsg);

    m_liveness = HEARTBEAT_LIVENESS;
    m_heartbeat_at = now_ms() + m_heartbeat;
}

zmq::multipart_t Worker::work(zmq::multipart_t& reply)
{
    if (reply.size()) {
        reply.pushmem(NULL,0);             // 4
        reply.pushstr(m_reply_to);         // 3
        reply.pushstr(mdp::worker::reply); // 2
        reply.pushstr(mdp::worker::ident); // 1
        send(m_sock, reply);
    }

    zmq::poller_t<> poller;
    poller.add(m_sock, zmq::event_flags::pollin);
    while (! interrupted()) {
        
        std::vector< zmq::poller_event<> > events(1);
        int rc = poller.wait_all(events, m_heartbeat);
        if (rc > 0) {           // got one
            zmq::multipart_t mmsg;
            recv(m_sock, mmsg);
            m_liveness = HEARTBEAT_LIVENESS;
            std::string header = mmsg.popstr();  // 1
            assert(header == mdp::worker::ident);
            std::string command = mmsg.popstr(); // 2
            if (mdp::worker::request == command) {
                m_reply_to = mmsg.popstr(); // 3
                mmsg.pop();                 // 4
                return mmsg;                // 5+
            }
            else if (mdp::worker::heartbeat == command) {
                // nothing
            }
            else if (mdp::worker::disconnect == command) {
                connect_to_broker();
            }
            else {
                m_log.error("invalid worker command: " + command);
            }
        }
        else {                  // timeout
            --m_liveness;
            if (m_liveness == 0) {
                m_log.debug("disconnect from broker - retrying...");
            }
            sleep_ms(m_reconnect);
            connect_to_broker();
        }
        if (now_ms() >= m_heartbeat_at) {
            zmq::multipart_t mmsg;
            mmsg.pushstr(mdp::worker::heartbeat); // 2
            mmsg.pushstr(mdp::worker::ident);     // 1
            send(m_sock, mmsg);
            m_heartbeat_at += m_heartbeat;
        }
    }
    if (interrupted()) {
        m_log.info("interupt received, killing worker");
    }
    
    return zmq::multipart_t{};
}


void echo_worker(zmq::socket_t& pipe, std::string address, int socktype)
{
    pipe.send(zmq::message_t{}, zmq::send_flags::none);

    // fixme: should get this from a more global spot.
    console_log log;

    // fixme: should implement BIND actor protocol 
    zmq::context_t ctx;
    zmq::socket_t sock(ctx, socktype);
    Worker worker(sock, address, "echo", log);

    zmq::multipart_t reply;
    while (true) {
        zmq::multipart_t request = worker.work(reply);
        if (request.size()) {
            break;
        }
        reply = std::move(request);
    }
    // fixme: should poll on pipe to check for early shutdown
    zmq::message_t die;
    auto res = pipe.recv(die, zmq::recv_flags::none);
}
