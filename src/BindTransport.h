/* Copyright (c) 2010 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "Common.h"
#include "Service.h"
#include "TransportManager.h"

#ifndef RAMCLOUD_BINDTRANSPORT_H
#define RAMCLOUD_BINDTRANSPORT_H

namespace RAMCloud {

/**
 * This class defines an implementation of Transport that allows unit
 * tests to run without a network or a remote counterpart (it injects RPCs
 * directly into a Service instance's #dispatch() method).
 */
struct BindTransport : public Transport {
    explicit BindTransport(Service* service = NULL)
        : services()
    {
        if (service)
            addService(*service, "mock:");
    }

    ServiceLocator
    getServiceLocator()
    {
        return ServiceLocator("mock:");
    }

    ServerRpc* serverRecv() {
        return NULL;
    }

    void
    addService(Service& service, const string locator) {
        services[locator] = &service;
    }

    Transport::SessionRef
    getSession(const ServiceLocator& serviceLocator) {
        const string& locator = serviceLocator.getOriginalString();
        ServiceMap::iterator it = services.find(locator);
        if (it == services.end()) {
            throw TransportException(HERE, format("Unknown mock host: %s",
                                                  locator.c_str()));
        }
        return new BindSession(*this, *it->second, locator);
    }

    Transport::SessionRef
    getSession() {
        return transportManager.getSession("mock:");
    }

    struct BindServerRpc : public ServerRpc {
        BindServerRpc() {}
        void sendReply() {}
        DISALLOW_COPY_AND_ASSIGN(BindServerRpc);
    };

    struct BindClientRpc : public ClientRpc {
        explicit BindClientRpc(BindTransport& transport,
                               Buffer& request, Buffer& response,
                               Service& service)
            : transport(transport), request(request), response(response),
              service(service) {}
        bool isReady() { return true; }
        void wait() {
            Service::Rpc rpc(NULL, request, response);
            service.handleRpc(rpc);
        }

        BindTransport& transport;
        Buffer& request;
        Buffer& response;
        Service& service;
        DISALLOW_COPY_AND_ASSIGN(BindClientRpc);
    };

    struct BindSession : public Session {
        explicit BindSession(BindTransport& transport, Service& service,
                             const string& locator)
            : transport(transport), service(service), locator(locator) {}
        ClientRpc* clientSend(Buffer* request, Buffer* response) {
            return new(response, MISC) BindClientRpc(transport, *request,
                                                     *response, service);
        }
        void release() {
            delete this;
        }
        BindTransport& transport;
        Service& service;
        const string locator;
        DISALLOW_COPY_AND_ASSIGN(BindSession);
    };

    typedef std::map<const string, Service*> ServiceMap;
    ServiceMap services;
    DISALLOW_COPY_AND_ASSIGN(BindTransport);
};

}  // namespace RAMCloud

#endif
