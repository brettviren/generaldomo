/*! Generaldomo protocol 
 *
 * This header holds 7/MDP constants.
 */

#ifndef GENERALDOMO_PROTOCOL_HPP_SEEN
#define GENERALDOMO_PROTOCOL_HPP_SEEN

namespace generaldomo {

    namespace mdp {
        namespace client {
            // Identify the type and version of the client sub-protocol
            const char* ident = "MDPC01";

        }
        namespace worker {
            // Identify the type and version of the worker sub-protocol
            const char* ident = "MDPW01";
            
            /// Worker commands as strings
            const char* ready = "\001";
            const char* request ="\002";
            const char* reply = "\003";
            const char* heartbeat = "\004";
            const char* disconnect = "\005";
        }
    }
}

#endif
