#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <time.h>
#include <unistd.h>

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

double total_time;

class KVStoreClient
{
public:
    explicit KVStoreClient(shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

    void GetKey(const string &key)
    {
        KeyRequest request;
        request.set_key(key);
        request.set_timestamp(clock());

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
        request.set_timestamp(clock());

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
        request.set_timestamp(clock());

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
                cout << "\nkey: " << call->reply.key() << endl;
                cout << "value: " << call->reply.value() << endl;
                cout << "message: " << call->reply.message() << endl;
                cout << "status: " << call->reply.status() << endl;
     
                double start = call->reply.timestamp();
                double end = clock() - start;
                double time_taken = ((double)end) / CLOCKS_PER_SEC;

                total_time += time_taken;
                cout <<"cumulative response time: " << total_time <<endl<< endl;
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

void testPut(KVStoreClient *kvStore, int i)
{
    for (int j = 0; j < 5; j++)
        kvStore->PutKey(to_string(((i + 1) * 10) + j), to_string(((i + 1) * 10) + j + 1));
    
}

void testGet(KVStoreClient *kvStore, int i)
{
    for (int j = 0; j < 5; j++)
        kvStore->GetKey(to_string(((i + 1) * 10) + j));
}

int main(int argc, char **argv)
{
    KVStoreClient kvStore(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));

    thread thread_ = thread(&KVStoreClient::AsyncCompleteRpc, &kvStore);

    int option;

    cout << "Interactive Mode - press 1 \nBatch Mode - press 2\n\nEnter valid option:";
    cin >> option;

    if (option == 1)
    {
        while (true)
        {
            cout << "\nEnter a valid command: ";
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
    else if (option == 2)
    {
        ifstream commands;
        commands.open("commands.txt");
        while (commands)
        {
            string line;
            getline(commands, line);
            stringstream ss(line);
            string type, key, value;
            ss >> type;
            if (!type.compare("GET"))
            {
                ss >> key;
                kvStore.GetKey(key);
            }
            else if (!type.compare("PUT"))
            {
                ss >> key;
                ss >> value;
                kvStore.PutKey(key, value);
            }
            else if (!type.compare("DELETE"))
            {
                ss >> key;
                kvStore.DeleteKey(key);
            }
            else
            {
                cout << "Press control-c to quit" << endl;
                break;
            }
        }
    }

    else if (option == 3)
    {

        vector<thread> worker_threads;
        for (int i = 0; i < 10; ++i)
            worker_threads.push_back(thread(testPut, &kvStore, i));

        for (int i = 0; i < 10; ++i)
            worker_threads.push_back(thread(testGet, &kvStore, i));

        for (auto i{0}; i < worker_threads.size(); i++)
            worker_threads[i].join();
    }
    else
    {
        cout<<"Invalid input";
        exit(1);
    }

    thread_.join(); // blocks forever

    return 0;
}
