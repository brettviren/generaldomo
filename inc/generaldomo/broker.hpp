#ifndef GENERALDOMO_BROKER_HPP_SEEN
#define GENERALDOMO_BROKER_HPP_SEEN

#include "generaldomo/logging.hpp"
#include "zmq.hpp"
#include <unordered_map>
#include <unordered_set>

namespace generaldomo {

    /*! The generaldomo broker class */

    class Broker {
    public:

        /// Create a broker with a ROUTER or SERVER socket 
        Broker(zmq::socket_t&& sock, util::logbase_t& log);
        ~Broker();

        /// Bind the socket to the address
        void bind(std::string address);

        /// Begin brokering
        void start();

    private:

        struct Service;

        // This is a proxy for the remote worker
        struct Worker {
            std::string identity; 
            // The owner, if known.
            Service* service{nullptr};
            // Expire the worker at this time, heartbeat refreshes.
            int64_T expiry{0};
        };

        // This collects workers for a given service
        struct Service {

            // The service name.
            std::string name;
            // List of client requests for this service.
            std::deque<zmq::message_t*> requests;
            // List of waiting workers.
            std::list<Worker*> waiting;
            // How many workers the service has
            size_t nworkers{0};

            ~Service ();
        };


    private:
        void purge_workers();
        Service* service_require(std::string name);
        void service_dispatch(Service* srv, zmq::message_t* msg);
        void service_internal(std::string service_name, zmq::message_t* msg);

        Worker* worker_require(std::string identity);
        void worker_delete(Worker* wkr, int disconnect);
        void worker_process(std::string  sender, zmq::message_t* msg);
        void worker_send(Worker* wkr, std::string command,
                         std::string option, zmq::message_t* msg);
        void worker_waiting(Worker* wkr);

        void client_process(std::string sender, zmq::message_t* msg);

    private:

        zmq::socket_t m_sock;

        std::unordered_map<std::string, Service*> m_services;
        std::unordered_map<std::string, Worker*> m_workers;
        std::unordered_set<Worker*> m_workers;
    };



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
    void broker_actor(zmq::socket& pipe, std::string address, int socktype);

}

#endif
