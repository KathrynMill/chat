#pragma once

#ifdef HAVE_GRPC
#include <grpcpp/grpcpp.h>
#include "social_service.grpc.pb.h"

class SocialServiceImpl final : public chat::social::SocialService::Service {
public:
    ::grpc::Status AddFriend(::grpc::ServerContext* context,
                             const chat::social::AddFriendRequest* request,
                             chat::social::AddFriendResponse* response) override;

    ::grpc::Status ListFriends(::grpc::ServerContext* context,
                               const chat::social::ListFriendsRequest* request,
                               chat::social::ListFriendsResponse* response) override;

    ::grpc::Status CreateGroup(::grpc::ServerContext* context,
                               const chat::social::CreateGroupRequest* request,
                               chat::social::CreateGroupResponse* response) override;

    ::grpc::Status AddGroup(::grpc::ServerContext* context,
                            const chat::social::AddGroupRequest* request,
                            chat::social::AddGroupResponse* response) override;

    ::grpc::Status ListGroups(::grpc::ServerContext* context,
                              const chat::social::ListGroupsRequest* request,
                              chat::social::ListGroupsResponse* response) override;
};
#endif




