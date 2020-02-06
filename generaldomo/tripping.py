"""Round-trip demonstrator

While this example runs in a single process, that is just to make
it easier to start and stop the example. Client thread signals to
main when it's ready.
"""

import sys
import threading
import time

import zmq

from .zhelpers import zpipe

def serverish_recv(sock, *args, **kwds):
    '''Return a message from a serverish socket.

    The socket may be of type ROUTER or SERVER.  Return list of
    [id,msg]

    '''
    if sock.type == zmq.SERVER:
        msg = sock.recv(copy=False)
        return [msg.routing_id,  msg]
                
    if sock.type == zmq.ROUTER:
        msg = sock.recv_multipart(copy=False, *args, **kwds)
        msg[0] = msg[0].bytes
        #print ("broker recv",msg)
        return msg
    raise ValueError(f'unknown socket type {sock.type}')

def serverish_send(sock, cid, msg, *args, **kwds):
    '''Send a message via a serverish socket.
    '''

    if sock.type == zmq.SERVER:
        msg.routing_id = cid
        return sock.send(msg)

    if sock.type == zmq.ROUTER:
        if not isinstance(msg, list):
            msg = [msg]
        msg = [cid] + msg
        #print ("broker send",msg)
        return sock.send_multipart(msg)
    
    raise ValueError(f'unknown socket type {sock.type}')
    

def client_task (ctx, pipe, stype, requests=10000, verbose=False):
    client = ctx.socket(stype)
    client.identity = b'C'
    client.connect("tcp://localhost:5555")

    print (f"Client with socket {stype}...")
    time.sleep(0.5)

    print ("Synchronous round-trip test...")
    start = time.time()
    for r in range(requests):
        client.send(b"hello")
        client.recv()
    print (" %d calls/second" % (requests / (time.time()-start)))

    print ("Asynchronous round-trip test...")
    start = time.time()
    for r in range(requests):
        client.send(b"hello")
    for r in range(requests):
        client.recv()
    print (" %d calls/second" % (requests / (time.time()-start)))

    # signal done:
    pipe.send(b"done")

def worker_task(stype, verbose=False):
    ctx = zmq.Context()
    worker = ctx.socket(stype)
    worker.identity = b'W'
    worker.connect("tcp://localhost:5556")
    time.sleep(0.5)
    print (f"Worker with socket {stype}...")
    worker.send(b"greetings")
    while True:
        msg = worker.recv_multipart(copy=False)
        worker.send_multipart(msg)
    ctx.destroy(0)

def broker_task(fes_type, bes_type, verbose=False):
    # Prepare our context and sockets
    ctx = zmq.Context()
    frontend = ctx.socket(fes_type)
    backend = ctx.socket(bes_type)
    frontend.bind("tcp://*:5555")
    backend.bind("tcp://*:5556")
    print (f"Broker with sockets {fes_type} <--> {bes_type}")

    # Initialize poll set
    poller = zmq.Poller()
    poller.register(backend, zmq.POLLIN)
    poller.register(frontend, zmq.POLLIN)

    feid = beid = None

    fe_waiting = list()
    be_waiting = list()

    while True:
        try:
            items = dict(poller.poll())
        except:
            break # Interrupted

        if frontend in items:
            feid,msg = serverish_recv(frontend)
            be_waiting.append(msg)

        if backend in items:
            beid, msg = serverish_recv(backend)
            if msg.bytes != b"greetings":
                fe_waiting.append(msg)

        if feid:
            for msg in fe_waiting:
                serverish_send(frontend, feid, msg)
            fe_waiting = list()
                    
        if beid:
            for msg in be_waiting:
                serverish_send(backend, beid, msg)
            be_wating = list()

def main(fes_type=zmq.ROUTER, bes_type=zmq.ROUTER,
         requests = 10000, verbose=False):
    otype = {zmq.ROUTER:zmq.DEALER, zmq.SERVER:zmq.CLIENT}
    fec_type = otype[fes_type]
    bec_type = otype[bes_type]

    # Create threads
    ctx = zmq.Context()
    client,pipe = zpipe(ctx)

    client_thread = threading.Thread(target=client_task,
                                     args=(ctx, pipe, fec_type, requests, verbose))
    worker_thread = threading.Thread(target=worker_task,
                                     args=(bec_type,verbose))
    worker_thread.daemon=True
    broker_thread = threading.Thread(target=broker_task,
                                     args=(fes_type, bes_type, verbose))
    broker_thread.daemon=True

    worker_thread.start()
    broker_thread.start()
    client_thread.start()

    # Wait for signal on client pipe
    client.recv()

if __name__ == '__main__':
    main()
