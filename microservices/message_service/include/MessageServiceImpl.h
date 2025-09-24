#pragma once

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
#include "message_service.grpc.pb.h"

class MessageServiceImpl final : public chat::message::MessageService::Service {
public:
    ::grpc::Status OneChat(::grpc::ServerContext* context,
                           const chat::message::OneChatRequest* request,
                           chat::message::OneChatResponse* response) override;

    ::grpc::Status GroupChat(::grpc::ServerContext* context,
                             const chat::message::GroupChatRequest* request,
                             chat::message::GroupChatResponse* response) override;

    ::grpc::Status ListMessages(::grpc::ServerContext* context,
                                const chat::message::ListMessagesRequest* request,
                                chat::message::ListMessagesResponse* response) override;
};
#endif




