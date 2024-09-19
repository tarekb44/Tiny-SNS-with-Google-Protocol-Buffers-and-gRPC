/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include<glog/logging.h>
#define log(severity, msg) LOG(severity) << msg; google::FlushLogFiles(google::severity); 

#include "sns.grpc.pb.h"


using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce662::Message;
using csce662::ListReply;
using csce662::Request;
using csce662::Reply;
using csce662::SNSService;


struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Vector that stores every client that has been created
std::vector<Client*> client_db;


class SNSServiceImpl final : public SNSService::Service {

  Client* getClient(const std::string& username) {
    for (Client* client : client_db)
      if (client->username == username)
        return client;

    return nullptr;
  }

  bool isFollowing(Client* client, Client* target) {
    return std::find(client->client_following.begin(), client->client_following.end(), target) != client->client_following.end();
  }

  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    std::string username = request->username();
    Client* c = nullptr;

    for (Client* client : client_db) {
      if (client->username == username) {
        c = client;
        break;
      }
    }

    if (c == nullptr) {
      return Status(grpc::StatusCode::NOT_FOUND, "Client not found");
    }

    for (Client* client : client_db) {
      list_reply->add_all_users(client->username);
    }

    for (Client* follower : c->client_followers) {
      list_reply->add_followers(follower->username);
    }

    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username = request->username();
    std::string username2 = request->arguments(0);

    Client*c1 = getClient(username);
    Client*c2 = getClient(username2);

    if (c2 == nullptr) {
      reply->set_msg("Invalid username");
    } else if (username == username2) {
      reply->set_msg("Invalid username");
    } else {
      if (isFollowing(c1, c2)) {
        reply->set_msg("you have already joined");
      } else {
        c1->client_following.push_back(c2);
        c2->client_followers.push_back(c1);
        reply->set_msg("Follow Successful");
      }
    }

    return Status::OK;
  }

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username = request->username();
    std::string username2 = request->arguments(0);

    Client* c1 = getClient(username);
    Client* c2 = getClient(username2);
   
    if (c1 == nullptr or c2 == nullptr)
    {
      reply->set_msg("Invalid username");
      return Status::OK;
    }

    if (isFollowing(c1, c2)) {
      c1->client_following.erase(std::find(c1->client_following.begin(), c1->client_following.end(), c2));

      auto it_followers = std::find(c2->client_followers.begin(), c2->client_followers.end(), c1);
      if (it_followers != c2->client_followers.end()) {
          c2->client_followers.erase(it_followers);
      }
      reply->set_msg("UnFollow Successful");
    } else {
      reply->set_msg("You are not a follower");
    }

    return Status::OK;
  }

  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username = request->username();
    
    // test 0: Check if the client already exists
    Client* c = getClient(username);

    if (c != nullptr && c->connected)
    {
      reply->set_msg("ALREADY JOINED");
      return Status(grpc::StatusCode::ALREADY_EXISTS, "ALREADY JOINED");
    } 
    else 
    {
      if (c == nullptr) 
      {
        c = new Client();
        c->username = username;
        c->connected = true;
        client_db.push_back(c);
      }
      else 
      {
        c->connected = true;
      }

      reply->set_msg("CONNECTION SUCCESSFUL");
    }

    return Status::OK;
  }


  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {

    /*********
    YOUR CODE HERE
    **********/
    
    return Status::OK;
  }

};

void RunServer(std::string port_no) {
  std::string server_address = "0.0.0.0:"+port_no;
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  log(INFO, "Server listening on "+server_address);

  server->Wait();
}

int main(int argc, char** argv) {

  std::string port = "3010";
  
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }
  
  std::string log_file_name = std::string("server-") + port;
  google::InitGoogleLogging(log_file_name.c_str());
  log(INFO, "Logging Initialized. Server starting...");
  RunServer(port);

  return 0;
}
