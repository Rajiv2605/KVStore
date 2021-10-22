#include <iostream>
#include <memory>
#include <string>
#include <thread>

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

    void GetKey(const std::string &key)
    {
        KeyRequest request;
        request.set_key(key);

        AsyncClientCall *call = new AsyncClientCall;

        call->response_reader =
            stub_->PrepareAsyncGetKey(&call->context, request, &cq_);

        call->response_reader->StartCall();

        call->response_reader->Finish(&call->reply, &call->status, (void *)call);
    }

    void PutKey(const std::string &key, const std::string &value)
    {
        KeyValueRequest request;
        request.set_key(key);

        AsyncClientCall *call = new AsyncClientCall;

        call->response_reader =
            stub_->PrepareAsyncPutKey(&call->context, request, &cq_);

        call->response_reader->StartCall();

        call->response_reader->Finish(&call->reply, &call->status, (void *)call);
    }

    void DeleteKey(const std::string &key)
    {
        KeyRequest request;
        request.set_key(key);

        AsyncClientCall *call = new AsyncClientCall;

        call->response_reader =
            stub_->PrepareAsyncDeleteKey(&call->context, request, &cq_);

        call->response_reader->StartCall();

        call->response_reader->Finish(&call->reply, &call->status, (void *)call);
    }

    void AsyncCompleteRpc()
    {
        void *got_tag;
        bool ok = false;

        while (cq_.Next(&got_tag, &ok))
        {
            AsyncClientCall *call = static_cast<AsyncClientCall *>(got_tag);
            GPR_ASSERT(ok);

            if (call->status.ok())
                std::cout << "Success: " << call->reply.key() << std::endl;
            else
                std::cout << "RPC failed" << std::endl;

            delete call;
        }
    }

private:
    struct AsyncClientCall
    {
        KeyValueReply reply;
        ClientContext context;
        Status status;
        std::unique_ptr<ClientAsyncResponseReader<KeyValueReply>> response_reader;
    };

    std::unique_ptr<KVStore::Stub> stub_;
    CompletionQueue cq_;
};

int main(int argc, char **argv)
{
    KVStoreClient kvStore(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));

    std::thread thread_ = std::thread(&KVStoreClient::AsyncCompleteRpc, &kvStore);

    for (int i = 0; i < 100; i++)
    {
        std::string key(" Client_request: " + std::to_string(i));
        kvStore.GetKey(key); // The actual RPC call!
        kvStore.PutKey(key, key); // The actual RPC call!
        kvStore.DeleteKey(key); // The actual RPC call!
        
    }
    
    std::cout << "Press control-c to quit" << std::endl
              << std::endl;
    thread_.join(); // blocks forever

    return 0;
}
