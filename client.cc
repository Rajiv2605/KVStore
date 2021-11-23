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

#define GET 1
#define PUT 2
#define DELETE 3

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using keyvalue::KVStore;
using keyvalue::DistRequest;
using keyvalue::DistReply;

using namespace std;


class DistStoreSender
{
public:
    DistStoreSender(std::shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

    DistReply DistMethod(const int &key, const string &value, const int &type)
    {
        DistRequest request;
        
        request.set_type(type);
        request.set_key(key);
        request.set_value(value);

        DistReply reply;
        ClientContext context;
        Status status = stub_->DistMethod(&context, request, &reply);
        
        return reply;
    }

private:
    std::unique_ptr<KVStore::Stub> stub_;
};

int main(int argc, char **argv)
{
    string serverToJoin;
    cout << "\nEnter port no. of server to join: ";
    cin >> serverToJoin;
    string addr("localhost:" + serverToJoin);
    DistStoreSender distStoreSender(
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

    int option;

    // cout << "Interactive Mode - press 1 \nBatch Mode - press 2\n\nEnter valid option:";
    // cin >> option;

    option = 1;

    if (option == 1)
    {
        while (true)
        {
            cout << "\nEnter a valid command: ";
            string type, value;
            int key;
            cin >> type;
            if (!type.compare("GET"))
            {
                cin >> key;
                DistReply reply = distStoreSender.DistMethod(key, "", GET);
                cout<<reply.message()<<endl;
                cout<<reply.key()<<endl;
                cout<<reply.value()<<endl;
                cout<<reply.success()<<endl;
            }
            else if (!type.compare("PUT"))
            {
                cin >> key;
                cin >> value;
                distStoreSender.DistMethod(key, value, PUT);
            }
            else if (!type.compare("DELETE"))
            {
                cin >> key;
                distStoreSender.DistMethod(key, "", DELETE);
            }
            else
            {
                cout << "Invalid input";
                break;
            }
            // sleep(1);
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
                // distStoreSender.GetKey(key);
            }
            else if (!type.compare("PUT"))
            {
                ss >> key;
                ss >> value;
                // distStoreSender.PutKey(key, value);
            }
            else if (!type.compare("DELETE"))
            {
                ss >> key;
                // distStoreSender.DeleteKey(key);
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

        // vector<thread> worker_threads;
        // for (int i = 0; i < 10; ++i)
        //     worker_threads.push_back(thread(testPut, &kvStore, i));

        // for (int i = 0; i < 10; ++i)
        //     worker_threads.push_back(thread(testGet, &kvStore, i));

        // for (auto i{0}; i < worker_threads.size(); i++)
        //     worker_threads[i].join();
    }
    else
    {
        cout<<"Invalid input";
        exit(1);
    }

    // thread_.join(); // blocks forever

    return 0;
}
