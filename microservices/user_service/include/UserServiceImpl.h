#pragma once

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
#include "user_service.grpc.pb.h"

class UserServiceImpl final : public chat::user::UserService::Service {
public:
    ::grpc::Status Reg(::grpc::ServerContext* context,
                       const chat::user::RegRequest* request,
                       chat::user::RegResponse* response) override;

    ::grpc::Status Login(::grpc::ServerContext* context,
                         const chat::user::LoginRequest* request,
                         chat::user::LoginResponse* response) override;

    ::grpc::Status Logout(::grpc::ServerContext* context,
                          const chat::user::LogoutRequest* request,
                          chat::user::LogoutResponse* response) override;
};
#endif




