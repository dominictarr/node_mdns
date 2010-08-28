#include <sstream>

#include <dns_sd.h>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "mdns_utils.hpp"
#include "mdns_service_ref.hpp"

using namespace v8;
using namespace node;

namespace node_mdns {

static
void
OnServiceRegistered(DNSServiceRef sdRef, DNSServiceFlags flags,
        DNSServiceErrorType errorCode, const char * name, const char * regtype,
        const char * domain, void * context)
{
    HandleScope scope;
    Local<Value> args[8];
    size_t argc = 8;
    if (errorCode == kDNSServiceErr_NoError) {
        argc = 1;
        args[0] = Local<Value>::New(Null());
    } else {
        args[0] = mdnsError("mdns operation failed", errorCode);
        argc = 1;
    }

    ServiceRef * serviceRef = reinterpret_cast<ServiceRef*>(context);
    Handle<Function> callback = serviceRef->GetCallback();
    Handle<Object> this_ = serviceRef->GetThis();

    if ( ! callback.IsEmpty() && ! this_.IsEmpty()) {
        callback->Call(this_, argc, args);
    }
}

Handle<Value>
DNSServiceRegister(Arguments const& args) {
    if (args.Length() != 10) {
        std::ostringstream msg;
        msg << "expected 12 but got " << args.Length() << " arguments";
        return throwError(msg.str().c_str());
    }
    
    if ( ! ServiceRef::HasInstance(args[0])) {
        return throwTypeError("argument 1 must be a DNSServiceRef (sdRef)");
    }
    ServiceRef * serviceRef = ObjectWrap::Unwrap<ServiceRef>(args[0]->ToObject());
    if (serviceRef->IsInitialized()) {
        return throwError("DNSServiceRef is already initialized");
    }

    if ( ! args[1]->IsInt32()) {
        return throwError("argument 2 must be an integer (DNSServiceFlags)");
    }
    DNSServiceFlags flags = args[1]->ToInteger()->Int32Value();

    if ( ! args[2]->IsInt32()) {
        return throwTypeError("argument 3 must be an integer (interfaceIndex)");
    }
    uint32_t interfaceIndex = args[2]->ToInteger()->Int32Value();

    const char * name(NULL);
    if ( ! args[3]->IsNull() && ! args[3]->IsUndefined()) {
        if ( ! args[3]->IsString()) {
            return throwTypeError("argument 4 must be a string (name)");
        }
        name = * String::Utf8Value(args[3]->ToString());
    }

    if ( ! args[4]->IsString()) {
        return throwTypeError("argument 5 must be a string (regtype)");
    }
    const char * regtype = * String::Utf8Value(args[4]->ToString());

    const char * domain(NULL);
    if ( ! args[5]->IsNull() && ! args[5]->IsUndefined()) {
        if ( ! args[5]->IsString()) {
            return throwTypeError("argument 6 must be a string (domain)");
        }
        domain = * String::Utf8Value(args[5]->ToString());
    }

    const char * host(NULL);
    if ( ! args[6]->IsNull() && ! args[6]->IsUndefined()) {
        if ( ! args[6]->IsString()) {
            return throwTypeError("argument 7 must be a string (host)");
        }
        domain = * String::Utf8Value(args[6]->ToString());
    }

    if ( ! args[7]->IsInt32()) {
        return throwTypeError("argument 8 must be an integer (port)");
    }
    int raw_port = args[7]->ToInteger()->Int32Value();
    if (raw_port > std::numeric_limits<uint16_t>::max() || raw_port < 0) {
        return throwError("argument 8: port number is out of bounds.");
    }
    uint16_t port = static_cast<uint16_t>(raw_port);

    uint16_t txtLen(0);
    const void * txtRecord(NULL);
    if ( ! args[8]->IsNull() && ! args[8]->IsUndefined()) {
        if (Buffer::HasInstance(args[8])) {
            std::cout << "TODO: implement txt record" << std::endl;
        } else {
            return throwTypeError("argument 9 must be a buffer (txtRecord)");
        }
    }

    if ( ! args[9]->IsNull() && ! args[9]->IsUndefined()) {
        if ( ! args[9]->IsFunction()) {
            return throwTypeError("argument 10 must be a function (callBack)");
        }
        serviceRef->SetCallback(Local<Function>::Cast(args[9]));
    }

    DNSServiceErrorType error = DNSServiceRegister( & serviceRef->GetServiceRef(),
            flags, interfaceIndex, name, regtype, domain, host, htons(port),
            txtLen, txtRecord, OnServiceRegistered, serviceRef);
    if (error != kDNSServiceErr_NoError) {
        return throwMdnsError("DNSServiceRegister()", error);
    }
    if ( ! serviceRef->SetSocketFlags()) {
        return throwError("Failed to set socket flags (O_NONBLOCK, FD_CLOEXEC)");
    }
    return Undefined();
}

} // end of namespace node_mdns