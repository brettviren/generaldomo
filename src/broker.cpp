// The broker code is implemented closely following the C++ example
// for Majordomo in the Zguide (mdbroker.cpp).  However, we use cppzmq
// and abstract away the differences between ROUTER and SERVER

#include "generaldomo/broker.hpp"
#include "generaldomo/util.hpp"
#include <zmq_addon.hpp>

using namespace generaldomo;

#define HEARTBEAT_LIVENESS  3       //  3-5 is reasonable
#define HEARTBEAT_INTERVAL  2500    //  msecs
#define HEARTBEAT_EXPIRY    HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS

Broker::Service::~Service () {
    for (auto req : requests) {
        delete(req);
    }
}

Broker::Broker(zmq::socket_t&& sock, logbase_t& log)
    : m_sock(std::move(sock))
    , m_log(log)
{
}

Broker::~Broker()
{
    while (! m_services.empty()) {
        delete m_services.begin()->second;
        m_services.erase(m_services.begin());
    }
    while (! m_workers.empty()) {
        delete m_workers.begin()->second;
        m_workers.erase(m_workers.begin());
    }
}


void Broker::bind(std::string address)
{
    m_sock.bind(address);
}

void Broker::proc_one()
{
    zmq::multipart_t msg;
    routing_id_t rid = recv_serverish(m_sock, msg);
    std::string sender = ...;    // frame 0
    std::strin gheader = ...; // frame 1
    if (header.compare(MDPC_CLIENT) == 0) {
        client_process(sender, msg);
    }
    else if (header.compare(MDPW_WORKER) == 0) {
        worker_process (sender, msg)
    }
    else {
        m_log.error("invalid message");
    }
}

void Broker::start()
{
    int64_t now = now_us();
    int64_t heartbeat_at = now + HEARTBEAT_INTERVAL;

    zmq::poller_t<> poller;
    poller.add(m_sock, zmq::event_flags::pollin);
    while (! interrupted()) {
        int64_t timeout = heartbeat_at - now;
        if (timeout < 0) {
            timeout = 0;
        }

        std::vector<zmq::poller_event<>> events(1);
        int rc = poller.wait_all(events, timeout);
        if (rc > 0) {           // got one
            proc_one();
        }

        now = now_us();
        if (now >= heartbeat_at) {
            purge_workers();
            for (auto& wrk : m_waiting) {
                worker_send(&wrk, (char*)MDPW_HEARTBEAT, "", NULL);
            }
            heartbeat_at += HEARTBEAT_INTERVAL;
            now = s_clock();
        }
    }
}




void broker(zmq::socket& pipe, std::string address, int socktype)
{
}

