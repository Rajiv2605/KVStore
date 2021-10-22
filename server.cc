#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include "keyvalue.grpc.pb.h"
#include "storage.hpp"

#define THREADPOOL_SIZE 4

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using keyvalue::KeyRequest;
using keyvalue::KeyValueReply;
using keyvalue::KeyValueRequest;
using keyvalue::KVStore;

class ServerImpl final
{
public:
    ~ServerImpl()
    {
        delete &storage_;
        server_->Shutdown();
        for (int i = 0; i < THREADPOOL_SIZE; i++)
            cq_[i]->Shutdown();
    }
    void Run()
    {
        string server_address("0.0.0.0:50051");

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);

        for (int i = 0; i < THREADPOOL_SIZE; i++)
            cq_[i] = builder.AddCompletionQueue();

        server_ = builder.BuildAndStart();
        cout << "Server listening on " << server_address << endl;

        vector<thread> worker_threads;
        for (int i = 0; i < THREADPOOL_SIZE; ++i)
            worker_threads.push_back(thread(&ServerImpl::HandleRpcs, this, i));
        for (auto i{0}; i < worker_threads.size(); i++)
            worker_threads[i].join();
    }

private:
    class CallData
    {
    public:
        virtual void Proceed() = 0;
    };

    class GetKeyFunc final : public CallData
    {
    public:
        GetKeyFunc(KVStore::AsyncService *service, Storage *storage, ServerCompletionQueue *cq, int thread_id_)
            : service_(service), storage_(storage), cq_(cq), responder_(&ctx_), status_(CREATE), thread_id(thread_id_)
        {
            Proceed();
        }

        void Proceed()
        {
            if (status_ == CREATE)
            {
                status_ = PROCESS;

                service_->RequestGetKey(&ctx_, &request_, &responder_, cq_, cq_, this);
            }
            else if (status_ == PROCESS)
            {
                //new client
                new GetKeyFunc(service_, storage_, cq_, thread_id);

                // The actual processing.
                string prefix("Method: GET, Server_thread: ");
                reply_.set_key(prefix + to_string(thread_id) + request_.key());

                status_ = FINISH;
                responder_.Finish(reply_, Status::OK, this);
            }
            else
            {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    private:
        KVStore::AsyncService *service_;
        ServerCompletionQueue *cq_;
        ServerContext ctx_;
        Storage *storage_;

        KeyRequest request_;
        KeyValueReply reply_;

        int thread_id;

        ServerAsyncResponseWriter<KeyValueReply> responder_;

        enum CallStatus
        {
            CREATE,
            PROCESS,
            FINISH
        };
        CallStatus status_;
    };

    class PutKeyFunc final : public CallData
    {
    public:
        PutKeyFunc(KVStore::AsyncService *service, Storage *storage, ServerCompletionQueue *cq, int thread_id_)
            : service_(service), storage_(storage), cq_(cq), responder_(&ctx_), status_(CREATE), thread_id(thread_id_)
        {
            Proceed();
        }

        void Proceed()
        {
            if (status_ == CREATE)
            {
                status_ = PROCESS;

                service_->RequestPutKey(&ctx_, &request_, &responder_, cq_, cq_, this);
            }
            else if (status_ == PROCESS)
            {
                //new client
                new PutKeyFunc(service_, storage_, cq_, thread_id);

                // The actual processing.
                string prefix("Method: PUT, Server_thread: ");
                storage_->handle_put("5", "500");
                reply_.set_key(prefix + to_string(thread_id) + request_.key());

                status_ = FINISH;
                responder_.Finish(reply_, Status::OK, this);
            }
            else
            {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    private:
        KVStore::AsyncService *service_;
        ServerCompletionQueue *cq_;
        ServerContext ctx_;
        Storage *storage_;

        KeyValueRequest request_;
        KeyValueReply reply_;

        int thread_id;
        ServerAsyncResponseWriter<KeyValueReply> responder_;

        enum CallStatus
        {
            CREATE,
            PROCESS,
            FINISH
        };
        CallStatus status_;
    };

    class DeleteKeyFunc final : public CallData
    {
    public:
        DeleteKeyFunc(KVStore::AsyncService *service, Storage *storage, ServerCompletionQueue *cq, int thread_id_)
            : service_(service), storage_(storage), cq_(cq), responder_(&ctx_), status_(CREATE), thread_id(thread_id_)
        {
            Proceed();
        }

        void Proceed()
        {
            if (status_ == CREATE)
            {
                status_ = PROCESS;

                service_->RequestDeleteKey(&ctx_, &request_, &responder_, cq_, cq_, this);
            }
            else if (status_ == PROCESS)
            {
                //new client
                new DeleteKeyFunc(service_, storage_, cq_, thread_id);

                // The actual processing.
                string prefix("Method: DELETE, Server_thread: ");
                reply_.set_key(prefix + to_string(thread_id) + request_.key());

                status_ = FINISH;
                responder_.Finish(reply_, Status::OK, this);
            }
            else
            {
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    private:
        KVStore::AsyncService *service_;
        ServerCompletionQueue *cq_;
        ServerContext ctx_;
        Storage *storage_;

        KeyRequest request_;
        KeyValueReply reply_;

        int thread_id;
        ServerAsyncResponseWriter<KeyValueReply> responder_;

        enum CallStatus
        {
            CREATE,
            PROCESS,
            FINISH
        };
        CallStatus status_;
    };

    // This can be run in multiple threads if needed.
    void HandleRpcs(int thread_id)
    {
        new GetKeyFunc(&service_, &storage_, cq_[thread_id].get(), thread_id);
        new PutKeyFunc(&service_, &storage_, cq_[thread_id].get(), thread_id);
        new DeleteKeyFunc(&service_, &storage_, cq_[thread_id].get(), thread_id);

        void *tag;
        bool ok;
        while (true)
        {
            // cout << "In Handle " << thread_id << endl;
            GPR_ASSERT(cq_[thread_id]->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<CallData *>(tag)->Proceed();
        }
    }

    unique_ptr<ServerCompletionQueue> cq_[THREADPOOL_SIZE];
    KVStore::AsyncService service_;
    unique_ptr<Server> server_;
    Storage storage_;
};

int main(int argc, char **argv)
{
    ServerImpl server;
    server.Run();

    return 0;
}
