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

using namespace std;

class KVStoreClient
{
public:
    explicit KVStoreClient(shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

    void GetKey(const string &key)
    {
        KeyRequest request;
        request.set_key(key);

        AsyncClientCall *call = new AsyncClientCall;

        call->response_reader =
            stub_->PrepareAsyncGetKey(&call->context, request, &cq_);

        call->response_reader->StartCall();

        call->response_reader->Finish(&call->reply, &call->status, (void *)call);
    }

    void PutKey(const string &key, const string &value)
    {
        KeyValueRequest request;
        request.set_key(key);
        request.set_value(value);

        AsyncClientCall *call = new AsyncClientCall;

        call->response_reader =
            stub_->PrepareAsyncPutKey(&call->context, request, &cq_);

        call->response_reader->StartCall();

        call->response_reader->Finish(&call->reply, &call->status, (void *)call);
    }

    void DeleteKey(const string &key)
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
            {
                cout << "key: " << call->reply.key() << endl;
                cout << "value: " << call->reply.value() << endl;
                cout << "message: " << call->reply.message() << endl;
                cout << "status: " << call->reply.status() << endl;
            }
            else
                cout << "RPC failed" << endl;

            delete call;
        }
    }

private:
    struct AsyncClientCall
    {
        KeyValueReply reply;
        ClientContext context;
        Status status;
        unique_ptr<ClientAsyncResponseReader<KeyValueReply>> response_reader;
    };

    unique_ptr<KVStore::Stub> stub_;
    CompletionQueue cq_;
};

int main(int argc, char **argv)
{
    KVStoreClient kvStore(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));

    thread thread_ = thread(&KVStoreClient::AsyncCompleteRpc, &kvStore);

    int option;
    cout << "Modes:\nInteractive Mode: press 1 \nBatch Mode: press 2" << endl;
    cin >> option;

    if (option == 1)
    {
        while (true)
        {
            cout << "Enter a valid command: ";
            string type, key, value;
            cin >> type;
            if (!type.compare("GET"))
            {
                cin >> key;
                kvStore.GetKey(key);
            }
            else if (!type.compare("PUT"))
            {
                cin >> key;
                cin >> value;
                kvStore.PutKey(key, value);
            }
            else if (!type.compare("DELETE"))
            {
                cin >> key;
                kvStore.DeleteKey(key);
            }
            else
            {
                cout << "Invalid input";
                break;
            }
            sleep(1);
        }
    }

    cout << "Press control-c to quit" << endl
         << endl;
    thread_.join(); // blocks forever

    return 0;
}
