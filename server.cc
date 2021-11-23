#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <stdio.h>
#include <fcntl.h>
#include <sstream>
#include "unistd.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include "keyvalue.grpc.pb.h"
#include "keyvalue.pb.h"
#include "storage.hpp"

#include "sha1.cpp"

int THREADPOOL_SIZE = 4;
string fhsh = "  ";
int server_hash;
map<int, string> id_port;
string portno;

using grpc::Channel;
using grpc::ClientContext;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using keyvalue::KeyRequest;
using keyvalue::KeyValueReply;
using keyvalue::KeyValueRequest;
using keyvalue::KVStore;

using keyvalue::JoinReply;
using keyvalue::JoinRequest;
using keyvalue::ServerComm;
using keyvalue::IdPortMessage;
using keyvalue::IdResponse;


class ServerSender
{
public:
    ServerSender(std::shared_ptr<Channel> channel)
        : stub_(ServerComm::NewStub(channel)) {}

    string Join(const string &ip, const string &port, const int &id)
    {
        JoinRequest request;
        request.set_id(id);
        request.set_port(port);

        JoinReply reply;

        ClientContext context;

        Status status = stub_->Join(&context, request, &reply);

        if (status.ok())
        {
            int sz = reply.id_port_size();
            cout<<"Size: "<<sz<<endl;
            for(int i=0; i<sz; i++)
            {
                const IdPortMessage *msg = &reply.id_port(i);
                cout<<msg->id()<<" "<<msg->port()<<endl;
                id_port[msg->id()] = msg->port();
            }
            return "Success!!\n";
        }
        else
        {
            cout << status.error_code() << ": " << status.error_message() << endl;
            return "RPC failed";
        }
    }

    void Share_key(const string &ip, const string &port)
    {
        IdPortMessage request;
        request.set_id(server_hash);
        request.set_port(portno);
        ClientContext context;
        IdResponse reply;
        Status status = stub_->Share_key(&context, request, &reply);
    }

private:
    std::unique_ptr<ServerComm::Stub> stub_;
};

class ServerReceiver final : public ServerComm::Service
{
    Status Join(ServerContext *context, const JoinRequest *request, JoinReply *reply) override
    {
        for(map<int, string>::iterator i=id_port.begin(); i!=id_port.end(); i++)
        {
            IdPortMessage *idp = reply->add_id_port();
            idp->set_id(i->first);
            idp->set_port(i->second);
        }
        // int sz = request->id_port_size();
        // cout<<"Size: "<<sz<<endl;
        // for(int i=0; i<sz; i++)
        // {
        //     const keyvalue::IdPortMessage *msg = &request->id_port(i);
        //     cout<<msg->id()<<" "<<msg->port()<<endl;
        // }


        // cout<<"New connection from server, id: " << request->id() << endl;
        cout.flush();
        
        // reply->set_message("Successfully joined");
        return Status::OK;
    }

    Status Share_key(ServerContext *context, const IdPortMessage *request, IdResponse *reply) override
    {
        cout<<"Received broadcast from: "<<request->id()<<" "<<request->port()<<endl;
        id_port[request->id()] = request->port();
        string msg = "Received at: " + to_string(server_hash) + ", port: " + portno;
        reply->set_message(msg);
        cout<<"\nIP table:\n";
        for(map<int, string>::iterator i=id_port.begin(); i!=id_port.end(); i++)
        {
            // string pno = i->second;
            // if(pno == portno)
            //     continue;
            cout<<"ID:  "<<i->first<<"  port:   "<<i->second<<endl;
        }
        cout.flush();

        return Status::OK;
    }
};


class ServerImpl final : public KVStore::Service
{
    public:
        ServerImpl(int server_id)
        {
            storage_.set_server_id(server_id);
            storage_.create_db();
        }
        Status GetKey(ServerContext *context, const KeyRequest *request, KeyValueReply *reply) override
        {
            storage_.reader_lock();
            string result = storage_.handle_get(request->key());
            storage_.reader_unlock();
            if (!result.compare("ERROR"))
            {
                reply->set_message("KEY DOES NOT EXISTS");
                reply->set_status(400);
            }
            else
            {
                reply->set_status(200);
                reply->set_value(result);
                reply->set_key(request->key());
            }
            reply->set_timestamp(request->timestamp());

            return Status::OK;
        }

        Status PutKey(ServerContext *context, const KeyValueRequest *request, KeyValueReply *reply) override
        {
            storage_.writer_lock();
            storage_.handle_put(request->key(), request->value());
            storage_.writer_unlock();
            reply->set_status(200);
            reply->set_key(request->key());
            reply->set_value(request->value());

            reply->set_timestamp(request->timestamp());

            return Status::OK;
        }

        Status DeleteKey(ServerContext *context, const KeyRequest *request, KeyValueReply *reply) override
        {
            storage_.writer_lock();
            string result = storage_.handle_delete(request->key());
            storage_.writer_unlock();
            if (!result.compare("ERROR"))
            {
                reply->set_message("KEY DOES NOT EXISTS");
                reply->set_status(400);
            }
            else
            {
                reply->set_status(200);
                reply->set_key(request->key());
            }

            reply->set_timestamp(request->timestamp());

            return Status::OK;
        }
    private:
        Storage storage_;
};

void joinServer(int id)
{
    char ch;
    cout << "\nWant to join the network(y/n): ";
    cin >> ch;

    if (ch == 'y')
    {
        string serverToJoin;
        cout << "\nEnter port no. of server to join: ";
        cin >> serverToJoin;
        string addr("localhost:" + serverToJoin);
        ServerSender comm(
            grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));

        string rep = comm.Join("localhost", serverToJoin, id);
        cout << "received: " << rep << endl;

        for(map<int, string>::iterator i=id_port.begin(); i!=id_port.end(); i++)
        {
            string pno = i->second;
            if(pno == portno)
                continue;

            string addr("localhost:" + pno);
            ServerSender comm(
                grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));
            comm.Share_key("localhost", pno);
        }
    }
}

// Reference: https://stackoverflow.com/questions/8029121/how-to-hash-stdstring
int generate_hash(string server_address)
{
    string hsh = sha1(server_address);
	// hash<string> hasher;
	// size_t hash = hasher(server_address);
	// hash = hash%100;
    // string hsh = to_string(hash);
    fhsh[0] = hsh[0];
    fhsh[1] = hsh[1];
    
    unsigned int x;   
    stringstream ss;
    ss << hex << fhsh;
    ss >> x;
    
    // fhsh = to_string(x);
    cout<<"hash ID: "<< x <<endl;

	return x;
}

void RunServer(const string &port)
{
    ifstream f_config;
    f_config.open("config.txt");
    string portno;
    getline(f_config, portno);
    string server_address("0.0.0.0:" + port);

    string line;
    getline(f_config, line);
    getline(f_config, line);
    getline(f_config, line);
    stringstream ss(line);
    ss >> THREADPOOL_SIZE;
    cout << "Threadpool size: " << THREADPOOL_SIZE << endl;
    
    server_hash = generate_hash(server_address);
    ServerImpl keyService(server_hash);
    ServerReceiver serverCommService;

    // adding port number and id
    id_port[server_hash] = port;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    builder.RegisterService(&keyService);
    builder.RegisterService(&serverCommService);

    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Server listening on " << server_address << endl;

    joinServer(server_hash);

    server->Wait();
}

int main(int argc, char **argv)
{
    int file = open("log.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    dup2(file, fileno(stderr));

    cout << "Server port no.: ";
    cin >> portno;

    RunServer(portno);

    return 0;
}
