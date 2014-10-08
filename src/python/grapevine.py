#!/usr/bin/env python

"""This defines the python Grapevine communication interface."""

import zmq
from socket import inet_ntoa
from threading import Thread
from time import time, sleep
from zeroconf import ServiceBrowser, Zeroconf

class _Publisher(object):

    def __init__(self,
                 service_name,
                 subscriber_buffer_length,
                 subscriber_high_water_mark):
        """
        """
        pass

class _Subscriber(object):

    class _ServiceHandler(object):
        """Provides callback for Zeroconf service browser."""

        def __init__(self, registered_svcs):
            self._registered_svcs = registered_svcs

        def removeService(self, zeroconf, type, name):
            try:
                self._registered_svcs.pop(name)
            except KeyError:
                print 'service removal error'
            print 'service', name, 'disappeared'

        def addService(self, zeroconf, type, name):
            info = zeroconf.getServiceInfo(type, name)
            self._registered_svcs[name] = info
            print 'service:', info.getName()
            print 'address:', inet_ntoa(info.getAddress())
            print 'port', info.getPort()
            print 'priority', info.getPriority()
            print 'type', info.getType()

    def __init__(self,
                 zeroconf,
                 context,
                 subscriber_buffer_length,
                 subscriber_high_water_mark,
                 protocol='tcp',
                 domain='local'):
        """
        """
        self._context = context
        self._svc_extension = '_grapevine._{protocol}.{domain}.'.format(
                    protocol=protocol,
                    domain=domain)
        self._registered_svcs = dict()
        self._zeroconf = zeroconf
        self._browser = ServiceBrowser(self._zeroconf,
                                       self._svc_extension,
                                       self._ServiceHandler(self._registered_svcs))

    def _subscribe_thread(self, name):
        while True:
            print 'thread', name, 'started'

    def subscribe(self, svc_name):
        name = "{svc_name}.{extension}".format(
                    svc_name=svc_name,
                    extension=self._svc_extension)
        t = Thread(target=_Subscriber._subscribe_thread,
                   name=name,
                   args=(self, name,))
        t.setDaemon(True)
        t.start()

        pass

class Communicator(object):

    def __init__(self, name):
        self._context = zmq.Context()
        self._zeroconf = Zeroconf()
        self._subscriber = _Subscriber(self._zeroconf, self._context, 1024, 1024)
        self._subscriber.subscribe('test')
        self._publisher = None
        self._name = name

    def add_subscriber(self, service_name):
        if self._subscriber is None:
            self._subscriber = self._context.socket(zmq.SUB, self._name)
            self._subscriber.setsockopt(zmq.SUBSCRIBE, "{0}-subscriber".format(self._name))


    def add_publisher(self, service_name):
        if self._publisher is None:
            self._publisher = self._context.socket(zmq.PUB)

if __name__ == '__main__':
    print 'ha'
