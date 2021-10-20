#include <iostream>
#include <memory>
#include <string>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "keyvalue.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using keyvalue::KeyRequest;
using keyvalue::KeyValueReply;
using keyvalue::KeyValueRequest;
using keyvalue::KVStore;

class KVStoreClient
{
public:
    explicit KVStoreClient(std::shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

    std::string GetKey(const std::string &user)
    {
        KeyRequest request;
        request.set_key(user);

        KeyValueReply reply;

        ClientContext context;

        CompletionQueue cq;

        Status status;

        std::unique_ptr<ClientAsyncResponseReader<KeyValueReply>> rpc(
            stub_->PrepareAsyncGetKey(&context, request, &cq));

        rpc->StartCall();

        rpc->Finish(&reply, &status, (void *)1);
        void *got_tag;
        bool ok = false;
        GPR_ASSERT(cq.Next(&got_tag, &ok));

        GPR_ASSERT(got_tag == (void *)1);
        GPR_ASSERT(ok);

        if (status.ok())
        {
            return reply.key();
        }
        else
        {
            return "RPC failed";
        }
    }

    std::string PutKey(const std::string &user)
    {
        KeyValueRequest request;
        request.set_key(user);

        KeyValueReply reply;

        ClientContext context;

        CompletionQueue cq;

        Status status;

        std::unique_ptr<ClientAsyncResponseReader<KeyValueReply>> rpc(
            stub_->PrepareAsyncPutKey(&context, request, &cq));

        rpc->StartCall();

        rpc->Finish(&reply, &status, (void *)1);
        void *got_tag;
        bool ok = false;
        GPR_ASSERT(cq.Next(&got_tag, &ok));

        GPR_ASSERT(got_tag == (void *)1);
        GPR_ASSERT(ok);

        if (status.ok())
        {
            return reply.key();
        }
        else
        {
            return "RPC failed";
        }
    }

    std::string DeleteKey(const std::string &user)
    {
        KeyRequest request;
        request.set_key(user);

        KeyValueReply reply;

        ClientContext context;

        CompletionQueue cq;

        Status status;

        std::unique_ptr<ClientAsyncResponseReader<KeyValueReply>> rpc(
            stub_->PrepareAsyncDeleteKey(&context, request, &cq));

        rpc->StartCall();

        rpc->Finish(&reply, &status, (void *)1);
        void *got_tag;
        bool ok = false;
        GPR_ASSERT(cq.Next(&got_tag, &ok));

        GPR_ASSERT(got_tag == (void *)1);
        GPR_ASSERT(ok);

        if (status.ok())
        {
            return reply.key();
        }
        else
        {
            return "RPC failed";
        }
    }

private:
    std::unique_ptr<KVStore::Stub> stub_;
};

int main(int argc, char **argv)
{
    KVStoreClient kvStore(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));
    std::string user("world");
    std::string reply = kvStore.GetKey(user); // The actual RPC call!
    std::cout << "Get received: " << reply << std::endl;
    reply = kvStore.PutKey(user); // The actual RPC call!
    std::cout << "Put received: " << reply << std::endl;
    reply = kvStore.DeleteKey(user); // The actual RPC call!
    std::cout << "Delete received: " << reply << std::endl;

    return 0;
}
