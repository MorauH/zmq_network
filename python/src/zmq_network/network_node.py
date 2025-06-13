

"""
    This module is one node for the program network.
    
    single_threaded_response: set to false to allow multiple simultaneous callbacks
    
    Subscribe and publish to multiple topics. All published topics use one port.

    Pattern(s) supporte supportedd:
    - Publish/Subscribe
"""

import zmq
import threading

from .config_reader import get_node_address, is_valid_topic, get_node_topics


class Node:
    def __init__(self, node_name: str = None, single_threaded_response=True):
        self.node_name = node_name
        self.context = zmq.Context(1)
        self.sub_threads: list[threading.Thread] = []
        self.sockets: list[zmq.Socket] = []
        self.pub_socket: zmq.Socket = None
        self.pub_topic: bytes = None
        self.recv_lock = threading.Lock() if single_threaded_response else None
        
    def close(self):
        # Close sockets and threads

        if self.pub_socket is not None:
            self.pub_socket.close()
        
        self.context.term()

        if len(self.sockets) != 0:
            raise Exception("Sockets not closed when context terminated")
            for sock in self.sockets:
                sock.close()
        if len(self.sub_threads) != 0:
            raise Exception("Threads not closed when context terminated")
            for t in self.sub_threads:
                t.join()

    def __del__(self):
        self.close()
    
    def _recv_message(self, socket: zmq.Socket, callback: callable):
        while True:
            try:
                data = socket.recv()
            except zmq.ZMQError as e:
                # context termination error is expected
                if e.errno != zmq.ETERM:
                    print(e)
                self.sockets.remove(socket)
                self.sub_threads.remove(threading.current_thread())
                break

            # split on unit seperator
            topic, message = data.split(b'\x1f', 1)

            if self.recv_lock:
                with self.recv_lock:
                    callback(topic, message)
            else:
                callback(topic, message)
    
    # main loop for node to be implemented by inheriting class
    def run(self):
        raise NotImplementedError("Node.run() not implemented")
    
    # bind as a host port
    def init_publisher(self):
        if self.node_name is None:
            raise Exception("Node name not set")
        try:
            # fetch node configs
            address = get_node_address(self.node_name)
        except Exception as e:
            print(e)
            print("Binding failed!")
            return
        socket: zmq.Socket = self.context.socket(zmq.PUB)
        socket.bind(address)
        self.pub_socket = socket
        print(f"Bound to {address}")
    
    # publish bytes for all listeners
    def publish_bytes(self, message: bytes, topic: bytes):
        if self.pub_socket is None:
            raise Exception("Node not bound")
        self.pub_socket.send(topic + b'\x1f' + message)
    
    # overlay for string messages
    def publish_str(self, message: str, topic: bytes):
        self.publish_bytes(bytes(message, "utf-8"), topic)
    
    # publish for zmq messages
    def publish_zmq(self, message: zmq.Message, topic: bytes):
        self.publish_bytes(message.bytes, topic)


    # start listening on other node for topic
    def subscribe(self, node: str, topic: str, callback: callable):
        try:
            address = get_node_address(node)
            valid_topic = is_valid_topic(node, topic) if topic != "" else True
            if not valid_topic:
                raise Exception(f"Topic: {topic} not valid for Node: {node}")
        except Exception as e:
            print(e)
            print("Subscription failed!")
            return
        
        socket: zmq.Socket = self.context.socket(zmq.SUB)
        socket.connect(address)
        socket.setsockopt(zmq.SUBSCRIBE, topic)
        self.sockets.append(socket)

        # start a new thread, listening for messages
        t = threading.Thread(target=self._recv_message, args=(socket, callback))
        self.sub_threads.append(t)
        t.start()

