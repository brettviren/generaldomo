#ifndef GENERALDOMO_WORKER_HPP_SEEN
#define GENERALDOMO_WORKER_HPP_SEEN

#include "generaldomo/util.hpp"
#include "generaldomo/logging.hpp"

namespace generaldomo {

    /*! The generaldomo worker API
     *
     * Applications may use a Worker to simplify participating in the
     * GDP worker subprotocol.  The protocol is exercised through
     * calls to Worker::work().
     *
     * The application must arrange to use the "clientish" socket
     * corresponding to what is in use by the broker but otherwise,
     * differences related to the socket type are subsequently erased
     * by this class.
     */

    class Worker {
    public:
        /// Create a worker providing service.  Caller keeps socket eg
        /// so to poll it along with others.
        Worker(zmq::socket_t& sock, std::string broker_address,
               std::string service, logbase_t& log);
        ~Worker();

        // API methods

        /// Send reply from last request, if any, and get new request.
        /// Both reply and request multiparts begin with "Frames 5+
        /// request body" of 7/MDP.
        zmq::multipart_t work(zmq::multipart_t& reply);

    private:
        zmq::socket_t& m_sock;
        std::string m_address;
        std::string m_service;
        logbase_t& m_log;
        int m_liveness{HEARTBEAT_LIVENESS};
        time_unit_t m_heartbeat{HEARTBEAT_INTERVAL};
        time_unit_t m_reconnect{HEARTBEAT_INTERVAL};
        time_unit_t m_heartbeat_at{0};
        bool m_expect_reply{false};
        std::string m_reply_to{""};

    private:

        std::function<void(zmq::socket_t& server_socket,
                           zmq::multipart_t& mmsg)> recv;
        std::function<void(zmq::socket_t& server_socket,
                           zmq::multipart_t& mmsg)> send;

        void connect_to_broker(bool reconnect = true);

    };


    // An echo worker actor function
    void echo_worker(zmq::socket_t& pipe, std::string address, int socktype);

}

#endif
