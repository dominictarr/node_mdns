#include "resolver.hpp"

#include <iostream>

namespace node_mdns {

using namespace v8;

Persistent<String> Resolver::resolved_symbol;

void
Resolver::Initialize(Handle<Object> target) {
    HandleScope scope;
    Local<FunctionTemplate> t = mDNSBase::Initialize(target, New);

    resolved_symbol = NODE_PSYMBOL("resolved");

    NODE_SET_PROTOTYPE_METHOD(t, "doStart", DoStart);

    target->Set(String::NewSymbol("Resolver"), t->GetFunction());
}

bool 
Resolver::DoStart(DNSServiceFlags flags, uint32_t interface_index,
        const char * name, const char * regtype, const char * domain)
{
    if (ServiceRef()) return false;

    int status = DNSServiceResolve( & ServiceRef(), flags, interface_index,
            name, regtype, domain, & on_resolve, this);
    if (kDNSServiceErr_NoError != status) {
        // XXX deal with error
        return false;
    }

    prepareSocket();

    Ref();

    return true;
}

v8::Handle<v8::Value>
Resolver::New(v8::Arguments const& args) {
    HandleScope scope;

    Resolver * resolver = new Resolver();
    resolver->Wrap(args.This());

    return args.This();
}

Resolver::Resolver() : mDNSBase() {}

v8::Handle<v8::Value>
Resolver::DoStart(v8::Arguments const& args) {
    HandleScope scope;
    Resolver * resolver = ObjectWrap::Unwrap<Resolver>(args.This());

    if ( 5 > args.Length()) {
        return ThrowException(Exception::Error(
                String::New("expected 5 arguments")));
    } 
    if (! args[0]->IsString()) {
        return ThrowException(Exception::Error(
                String::New("argument 0 must be a string")));
    }
    String::Utf8Value name(args[0]->ToString());

    if (! args[1]->IsString()) {
        return ThrowException(Exception::Error(
                String::New("argument 1 must be a string")));
    }
    String::Utf8Value regtype(args[1]->ToString());

    if (! args[2]->IsString()) {
        return ThrowException(Exception::Error(
                String::New("argument 2 must be a string")));
    }
    String::Utf8Value domain(args[2]->ToString());

    if (! args[3]->IsInt32()) {
        return ThrowException(Exception::Error(
                String::New("argument 3 must be an integer")));
    }
    uint32_t interface_index(args[3]->ToInteger()->Int32Value());

    if (! args[4]->IsInt32()) {
        return ThrowException(Exception::Error(
                String::New("argument 4 must be an integer")));
    }
    DNSServiceFlags flags(args[4]->ToInteger()->Int32Value());
    
    bool r = resolver->DoStart(flags, interface_index, *name, *regtype, *domain);

    if (!r) {
        // XXX
        /*
        return ThrowException(Exception::Error(
            String::New(ad->ErrorMessage())));
        */
    }
    return Undefined();
}

void
Resolver::on_resolve(DNSServiceRef /*sdRef*/, DNSServiceFlags flags,
        uint32_t interface_index, DNSServiceErrorType error_code,
        const char * fullname, const char * hosttarget, uint16_t port,
        uint16_t txt_record_length, const unsigned char * txt_record,
        void * context)
{
    Resolver * resolver = static_cast<Resolver*>(context);
    resolver->on_resolve(flags, interface_index, error_code, fullname, hosttarget,
            port, txt_record_length, txt_record);
}

void
Resolver::on_resolve(DNSServiceFlags flags, uint32_t interface_index,
        DNSServiceErrorType error_code, const char * fullname,
        const char * hosttarget, uint16_t port,
        uint16_t txt_record_length, const unsigned char * txt_record)
{
    Emit(resolved_symbol, 0, NULL);
}

} // end of namespace node_mdns
