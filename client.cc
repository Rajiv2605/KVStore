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
                cout << "Success: " << call->reply.key() << endl;
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
    cin >> option;
    string temp;
    getline(cin, temp);
    if (option = 1)
    {
        while (true)
        {
            string inp;
            getline(cin, inp);
            string token = inp.substr(0, inp.find(" "));

            cout << token;
        }
    }
    kvStore.PutKey("50", "500");
    kvStore.GetKey("50");
    kvStore.GetKey("50");

    cout << "Press control-c to quit" << endl
         << endl;
    thread_.join(); // blocks forever

    return 0;
}
