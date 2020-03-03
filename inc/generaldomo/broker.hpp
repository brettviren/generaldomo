#ifndef GENERALDOMO_BROKER_HPP_SEEN
#define GENERALDOMO_BROKER_HPP_SEEN

#include "generaldomo/logging.hpp"
#include "generaldomo/util.hpp"
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <list>
#include <functional>

namespace generaldomo {

    /*! The generaldomo broker actor.
     *
     * The broker is in the form of a function intended to be called
     * from a zmq::actor_t.
     *
     * The address should be in the usual ZeroMQ format.  The borker
     * will bind() to it.
     * 
     * The socktype should be either SERVER or ROUTER.
     * If ROUTER, the broker will act as a 7/MDP v0.1 broker.
     * Else it will act as a GDP broker.
     */
    void broker_actor(zmq::socket_t& pipe, std::string address, int socktype);


    /*! The generaldomo broker class */

    class Broker {
    public:

        /// Create a broker with a ROUTER or SERVER socket 
        Broker(zmq::socket_t&& sock, logbase_t& log);
        ~Broker();

        /// Bind the socket to the address
        void bind(std::string address);

        /// Begin brokering (main loop)
        void start();

        /// Process one input on socket
        void proc_one();

        typedef std::chrono::milliseconds time_unit_t;

        /// Do heartbeat processing given next heatbeat time. 
        void proc_heartbeat(time_unit_t heartbeat_at);

    private:

        std::function<remote_identity_t(zmq::socket_t& server_socket,
                                        zmq::multipart_t& mmsg)> recv;
        std::function<void(zmq::socket_t& server_socket,
                           zmq::multipart_t& mmsg, remote_identity_t rid)> send;

        struct Service;

        // This is a proxy for the remote worker
        struct Worker {
            // The identity of a worker.
            remote_identity_t identity; 

            // The owner, if known.
            Service* service{nullptr};
            // Expire the worker at this time, heartbeat refreshes.
            time_unit_t expiry{0};
        };

        // This collects workers for a given service
        struct Service {

            // Service name, that is the "thing" that its workers know how to do.
            std::string name;

            // List of client requests for this service.  These need
            // to be multipart_t which are not available as pointer!
            // How to store effciently to avoid copy? FIXME
            std::deque<zmq::multipart_t> requests;

            // List of waiting workers.
            std::list<Worker*> waiting;

            // How many workers the service has
            size_t nworkers{0};

            ~Service ();
        };


    private:
        void purge_workers();
        Service* service_require(std::string name);
        void service_dispatch(Service* srv);
        void service_internal(remote_identity_t rid, std::string service_name,
                              zmq::multipart_t& mmsg);

        Worker* worker_require(remote_identity_t identity);
        void worker_delete(Worker*& wrk, int disconnect);

        void worker_process(remote_identity_t sender, zmq::multipart_t& mmsg);
        void worker_waiting(Worker* wkr);

        void client_process(remote_identity_t client_id, zmq::multipart_t& mmsg);

    private:

        zmq::socket_t m_sock;
        logbase_t& m_log;
        std::unordered_map<remote_identity_t, Service*> m_services;
        std::unordered_map<remote_identity_t, Worker*> m_workers;
        std::unordered_set<Worker*> m_waiting;
    };



}

#endif
