#ifndef GENERALDOMO_UTIL_HPP_SEEN
#define GENERALDOMO_UTIL_HPP_SEEN

#include <zmq_addon.hpp>

#include <variant>

namespace generaldomo {

    namespace util {


        typedef std::variant<uint32_t, zmq::multipart_t*> routing_id_t;

        // See discussion in docs.  Erase socket behavior.
        routing_id_t recv_serverish(zmq::socket_t& sock, zmq::multipart_t& mmsg);
        routing_id_t recv_clientish(zmq::socket_t& sock, zmq::multipart_t& mmsg);
        // routing_id_t send_serverish(zmq::socket_t& sock, zmq::multipart_t& mmsg);
        // routing_id_t send_clientish(zmq::socket_t& sock, zmq::multipart_t& mmsg);

        
        /*! Current system time in microseconds. */
        int64_t now_us();

        /*! Return true when a signal has been sent to the application.
         *
         * Exit main loop if ever true.
         *
         * The catch_signals() function must be called in main() for
         * this to ever return true.
         */
        bool interrupted();

        /*! Catch signals and set interrupted to true. */
        void catch_signals ();
    }

}

#endif
