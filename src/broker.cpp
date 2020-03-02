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

Broker::remote_identity_t Broker::recv(zmq::multipart_t& mmsg)
{
........
}

Broker::send(zmq::multipart_t& mmsg, Broker::remote_identity_t)
{
.............
}

void Broker::proc_one()
{
    zmq::multipart_t mmsg;
    remote_identity_t sender = recv(mmsg);
    std::string header = mmsg.popstr(); // 7/MDP frame 1
    if (header == MDPC_CLIENT) {
        client_process(sender, mmsg);
    }
    else if (header == MDPW_WORKER) {
        worker_process(sender, mmsg)
    }
    else {
        m_log.error("invalid message from " + sender);
    }
}

void Broker::proc_heartbeat(int64_t heartbeat_at)
{
    auto now = now_us();
    if (now < heartbeat_at) {
        return;
    }
    purge_workers();
    for (auto& wrk : m_waiting) {
        worker_send(&wrk, (char*)MDPW_HEARTBEAT, "", NULL);
    }
    heartbeat_at += HEARTBEAT_INTERVAL;
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
        proc_heartbeat(heartbeat_at);
        now = now_us();
    }
}

void Broker::purge_workers()
{
    auto now = now_us();
    // can't remove from the set while iterating, so make a temp
    std::vector<Worker*> dead;
    for (auto wrk : m_waiting) {
        if (wrk->expiry <= now) {
            dead.push_back(wrk); 
        }
    }
    for (auto wrk : dead) {
        m_log.debug("deleting expired worker: " + wrk->identity);
        worker_delete(wrk,0);
    }
}

Broker::Service* Broker::service_require(std::string name)
{
    Service* srv = m_services[name];
    if (!srv) {
        srv = new Service{name};
        m_services[name] = srv;
        m_log.debug("registering new service: " + name);
    }
    return srv;
}

void Broker::service_dispatch(Service* srv, zmq::multipart_t& mmsg)
{
    if (msg.size()) {
        srv->requests.push_back(mmsg);//fixme: copy!
    }
    purge_workers();
    while (srv->wating.size() and srv->requests.size()) {

        Worker* wrk = srv->waiting.front();
        for (auto maybe : srv->waiting) {
            if (wrk->expiry < maybe->expiry) {
                wrk = maybe;
            }
        }
        // FIXME, do we really want this copy?
        zmq::multipart_t mmsg = srv->requests.front();
        srv->requests.pop_front();
        worker_send(wrk, MDPW_REQUEST, "", mmsg);
        m_waiting.erase(*wrk);
        srv->wating.erase(wrk);
        // mmsg destructs
    }
}

void Broker::service_internal(std::string service_name, zmq::multipart_t& mmsg)
{
    ............
        
}

Broker::Worker* Broker::worker_require(remote_identity_t identity)
{
    Worker* wrk = m_workers[identity];
    if (!wrk) {
        wrk = new Worker{identity};
        m_workers[identity] = wrk;
        m_log.debug("registering new worker");
    }
    return wrk;
}

void Broker::worker_delete(Broker::Worker*& wrk, int disconnect)
{
    if (disconnect) {
        worker_send(wrk, MDPW_DISCONNECT, "", NULL);
    }
    if (wrk->service) {
        size_t nremoved = wrk->service->waiting.erase(wrk);
        wrk->service->nworkers -= nremoved;
    }
    m_waiting.erase(wrk);
    m_workers.erase(wrk->identity);
    delete wrk;
    wrk=0;
}
// mmsg holds starting with 7/MDP Frame 2.
void Broker::worker_process(remote_identity_t sender, zmq::multipart_t& mmsg)
{
    assert(mmsg.size() >= 1);
    const std::string command = mmsg.popstr(); // 0x01, 0x02, ....
    bool worker_ready = (m_workers.find(sender) != m_workers.end());
    Worker* wrk = worker_require(sender);

    if (MDPW_READY == command) {
        if (worker_ready) {     // protocol error
            m_log.error("protocol error (double ready) from: " + sender);
            worker_delete(wrk, 1);
            return;
        }
        if (sender.size() >= 4 && sender.find_first_of("mmi.") == 0) {
            m_log.error("protocol error (worker mmi) from: " + sender);
            worker_delete(wrk, 1);
            return;
        }
        // Attach worker to service and mark as idle
        std::string service_name = mmsg.popstr();
        wrk->service = service_require(service_name);
        wrk->service->nworkers++;
        worker_waiting(wrk);
        return;
    }
    if (MDWP_REPLY == command) {
        if (!worker_ready) {
            worker_delete(wrk, 1);
            return;
        }
        remote_identity_t client_id = mmsg.popstr();
        mmsg.pushstr(wrk->service->name);
        mmsg.pushstr(MDPC_CLIENT);
        send(mmsg, client_id);
        worker_waiting(wrk);
        return;
    }
    if (MDPW_HEARTBEAT == command) {
        if (!worker_ready) {
            worker_delete(wrk, 1);
            return;
        }
        wrk->expiry = now_us() + HEARTBEAT_EXPIRY;
        return;
    }
    if (MDPW_DISCONNECT == command) {
        worker_delete(wrk, 0);
        return;
    }
    m_log.error("invalid input message " + command);
}



void Broker::worker_send(Broker::Worker* wkr, std::string command,
                         std::string option, zmq::message_t* msg)
{
........
}

void Broker::worker_waiting(Broker::Worker* wrk)
{
    m_waiting.insert(wrk);
    wrk->service->waiting.push_back(wrk);
    wrk->expiry = now_us() + HEARTBEAT_EXPIRY;
    service_dispatch(wrk->service);
}

void Broker::client_process(remote_identity_t client_id, zmq::multipart_t& mmsg)
{
    std::string service_name = mmsg.popstr();
    Service* srv = service_require(service_name);
    mmsg.pushmem(NULL,0);
    mmsg.pushstr(client_id);
    if (service_name.size() >= 4 and service_name.find_first_of("mmi.") == 0) {
        service_internal(service_name, mmsg);
    }
    else {
        service_dispatch(srv, mmsg);
    }
}




void broker(zmq::socket& pipe, std::string address, int socktype)
{
}

