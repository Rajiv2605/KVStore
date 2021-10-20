#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include "keyvalue.grpc.pb.h"

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
        server_->Shutdown();
        cq_->Shutdown();
    }
    void Run()
    {
        std::string server_address("0.0.0.0:50051");

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        cq_ = builder.AddCompletionQueue();
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        HandleRpcs();
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
        GetKeyFunc(KVStore::AsyncService *service, ServerCompletionQueue *cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
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
                new GetKeyFunc(service_, cq_);

                // The actual processing.
                std::string prefix("From Server GET ");
                reply_.set_key(prefix + request_.key());
                std::cout << reply_.key() << std::endl;

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

        KeyRequest request_;
        KeyValueReply reply_;

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
        PutKeyFunc(KVStore::AsyncService *service, ServerCompletionQueue *cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
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
                new PutKeyFunc(service_, cq_);

                // The actual processing.
                std::string prefix("From Server PUT ");
                reply_.set_key(prefix + request_.key());
                std::cout << reply_.key() << std::endl;

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

        KeyValueRequest request_;
        KeyValueReply reply_;

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
        DeleteKeyFunc(KVStore::AsyncService *service, ServerCompletionQueue *cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE)
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
                new DeleteKeyFunc(service_, cq_);

                // The actual processing.
                std::string prefix("From Server DELETE ");
                reply_.set_key(prefix + request_.key());
                std::cout << reply_.key() << std::endl;

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

        KeyRequest request_;
        KeyValueReply reply_;

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
    void HandleRpcs()
    {
        new GetKeyFunc(&service_, cq_.get());
        new PutKeyFunc(&service_, cq_.get());
        new DeleteKeyFunc(&service_, cq_.get());

        void *tag;
        bool ok;
        while (true)
        {
            std::cout << "In Handle\n";
            GPR_ASSERT(cq_->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<CallData *>(tag)->Proceed();
        }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    KVStore::AsyncService service_;
    std::unique_ptr<Server> server_;
};

int main(int argc, char **argv)
{
    ServerImpl server;
    server.Run();

    return 0;
}
