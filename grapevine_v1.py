# COPYRIGHT: Robosub Club of the Palouse under the GPL v3

"""This defines the python communication interface.

This assumes that robosub_settings.py is on PYTHONPATH.

"""

import Queue
import os
import threading
import time
import zmq
from robosub_settings import settings
from numpy import array, frombuffer

def get_socket_name(module_name, socket_type='pub'):
    """Determines the socket for module_name."""
    try:
        ip = settings[module_name]['ip']
    except KeyError:
        # FIXME this isn't the best solution for pub vs sub.
        if socket_type == 'pub':
            ip = '*'
        else:
            ip = '127.0.0.1'
    return 'tcp://{ip}:{port}'.format(ip=ip, port=settings[module_name]['port'])

class Communicator(object):
    def __init__(self, module_name,
                 subscriber_buffer_length=1024,
                 subscriber_high_water_mark=1024):
        """
        'module_name' must follow the folder path name convention that
            specifies a module.
        'subscriber_buffer_length' and 'subscriber_high_water_mark' control zmq
            memory settings.

        """
        self.settings = settings
        self.module_name = module_name
        self.module_name_debug = os.path.join(self.module_name, "debug")

        if not os.path.isdir('/tmp/robosub'):
            os.mkdir('/tmp/robosub')

        # Prepare our publisher
        self.publisher = {}
        self.publisher['next_message_number'] = 1
        self.publisher['context'] = zmq.Context(1)
        self.publisher['socket'] = self.publisher['context'].socket(zmq.PUB)
        self.publisher['socket'].hwm = settings['publisher_high_water_mark']
        self.publisher['socket'].setsockopt(
                zmq.SNDBUF, settings['publisher_buffer_length'])
        self.publisher['socket'].bind(get_socket_name(self.module_name))

        # Prepare the debug channel
        self.publisher_debug = {}
        self.publisher_debug['next_message_number'] = 1
        self.publisher_debug['context'] = zmq.Context(1)
        self.publisher_debug['socket'] = \
                self.publisher_debug['context'].socket(zmq.PUB)
        self.publisher_debug['socket'].hwm = \
                settings['publisher_high_water_mark']
        self.publisher_debug['socket'].setsockopt(
                zmq.SNDBUF, settings['publisher_buffer_length'])
        self.publisher_debug['socket'].bind(
                get_socket_name(self.module_name_debug))

        # Note: Even though the bind call returns, the socket isn't actually
        # ready for some short amount of time after this call. zmq will
        # drop messages that are published in this time.

        # Prepare our subscribers
        self.subscribers = {}
        for mname in self.listening():
            self.subscribers[mname] = {}
            mdata = self.subscribers[mname]
            mdata['context'] = zmq.Context(1)
            mdata['socket'] = mdata['context'].socket(zmq.SUB)
            mdata['socket'].setsockopt(zmq.SUBSCRIBE, '')
            mdata['socket'].setsockopt(zmq.RCVBUF, subscriber_buffer_length)
            mdata['socket'].hwm = subscriber_high_water_mark
            mdata['socket'].connect(get_socket_name(mname, 'sub'))
            mdata['queue'] = Queue.Queue()
            mdata['last_message'] = None

    def get_last_message(self, module_name, block=False):
        """Reads all messages in the queue and returns the last one.

        The messages prior to the last one will be discarded. If no
        new messages were received since the last call to this method,
        this will return the last message again. If block is True,
        this will block the current thread until a new message is
        generated.

        """
        # Updates the last message
        list(self.get_messages(module_name, block))
        last_msg = self.subscribers[module_name]['last_message']
        # Kludge: Just after startup, the Communicator might not have
        # observed any messages. Rather than let the caller choke on None,
        # let's delay the return and try to grab some useable value.
        if last_msg is None:
            list(self.get_messages(module_name, block=block))
            last_msg = self.subscribers[module_name]['last_message']
        return last_msg

    def get_messages(self, module_name, block=False):
        """Returns a generator that yields all available messages."""
        subscriber_data = self.subscribers[module_name]
        if block:
            msg = subscriber_data['socket'].recv_json()
            self.subscribers[module_name]['last_message'] = msg
            yield msg
        while True:
            try:
                msg = subscriber_data['socket'].recv_json(zmq.DONTWAIT)
                if msg:
                    self.subscribers[module_name]['last_message'] = msg
                yield msg
            #except zmq.error.ZMQError: # XXX version 2.2.4
            except zmq.ZMQError:
                raise StopIteration()

    def bind_video_stream(self, port, n_ports=30):
        """Bind one end of a socket pair for video streaming."""
        # FIXME change the dictionary structure for stream socket pair.
        # Maybe make an entirely new dictionary.
        self.publisher['stream'] = {}
        self.publisher['stream']['context'] = zmq.Context(1)
        self.publisher['stream']['socket'] = \
                    self.publisher['stream']['context'].socket(zmq.PAIR)

        for i in xrange(n_ports):
            self.publisher['stream']['socket'].bind(
                    "tcp://*:{port}".format(port=port + i))
        #self.poller = zmq.Poller()
        #self.poller.register(self.publisher['stream']['socket'], zmq.POLLOUT)

    def connect_video_stream(self, port, addr='127.0.0.1'):
        """Connect one end of a socket pair for video streaming."""
        self.subscribers['stream'] = {}
        self.subscribers['stream']['context'] = zmq.Context(1)
        self.subscribers['stream']['socket'] = \
                    self.subscribers['stream']['context'].socket(zmq.PAIR)
        self.subscribers['stream']['socket'].connect(
                    "tcp://{addr}:{port}".format(addr=addr, port=port))

    def send_image(self, image):
        """Send an image over the connected stream server socket"""

        metadata = dict(dtype = str(image.dtype), shape = image.shape)
        try:
            self.publisher['stream']['socket'].send_json(
                    metadata, flags=zmq.SNDMORE | zmq.NOBLOCK)
            self.publisher['stream']['socket'].send(
                    image, copy=True, track=False, flags=zmq.NOBLOCK)
        except zmq.ZMQError:
            pass

    def recv_image(self):
        """Receive an image from the connected stream client socket."""
        metadata = self.subscribers['stream']['socket'].recv_json()
        message = self.subscribers['stream']['socket'].recv(
                    copy=True, track=False)
        buf = buffer(message)
        image = frombuffer(buf, dtype=metadata['dtype'])
        return image.reshape(metadata['shape'])

    def listening(self):
        """Returns a list of modules this Communicator is listening to."""
        listening_to = settings[self.module_name].get('listen')
        if not listening_to:
            listening_to = []
        return listening_to

    def publish_message(self, message):
        """Publishes a message to all listeners.

        Messages should be in the form of a dict. The 'message_number' and
        'time' fields will automatically be updated. If the message is not
        a dict, this will create a dict and store the message under the key
        'message'.

        """
        if not isinstance(message, dict):
            message = {"message": message}
        message['message_number'] = self.publisher['next_message_number']
        self.publisher['next_message_number'] += 1
        message['timestamp'] = time.time()
        message['module_name'] = self.module_name
        self.publisher['socket'].send_json(message)

    def debug(self, *args):
        message = {}
        message['message'] = ' '.join([str(x) for x in args])
        message['message_number'] = self.publisher_debug['next_message_number']
        self.publisher_debug['next_message_number'] += 1
        message['timestamp'] = time.time()
        message['module_name'] = self.module_name_debug
        self.publisher_debug['socket'].send_json(message)

